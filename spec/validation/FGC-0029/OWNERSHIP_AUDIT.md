# FGC-0029 对象所有权与异步生命周期审计

审计基线：`8117cfe`（R16）。范围：`ExifParser`、`ExifUtility`、`GeoTagController`、
`QtConcurrent`/`QFuture`/`QFutureWatcher`、`QByteArray`/`QString`、libexif、跨进程资源。

结论概要（R32 当前结论；历史对象所有权审计仍以 R16 源码快照为基线）：

- 当前产品代码的 `ExifParser::readTime()` 已对 EXIF ASCII 格式、component 数、缓冲区长度和 NUL 终止符做边界验证，并直接解析固定格式时间；不再经由 `QDateTime::fromString()`。
- 历史 ASan 报告确认：20 字节 QString 数据块发生跨 worker 线程 double-free。首次破坏引用计数的具体操作、线程交错和 Qt 内部确切调用者仍未证实。
- `QSystemLocalePrivate::zeroDigit()` 的 lazy-cache 路径是高度相关候选机制，不是已证明的 Qt 缺陷或唯一根因。不得将 `QDateTime::fromString()` 栈帧直接等同于首次非法操作。
- FGC-0037 的 GPS Version ID 长度越界缺口已单独修复并关闭；该修复不构成 FGC-0029 的根因证据。
- R31 历史 Release PageHeap run `43444888358` 在 16–32 B full PageHeap + UST 下完成 36/36，无复现；后续 exact-head run `43451171747` 已取消。两者均不构成根因闭环。

## 1. ExifParser

### 1.1 `readString()`

```cpp
char value[256];
exif_entry_get_value(entry, value, sizeof(value));
return QString::fromUtf8(value).trimmed();
```

| 检查项 | 结论 |
|---|---|
| `ExifEntry` 是否需要释放 | 否，entry 由 `ExifData` 的 IFD 拥有；`readString` 不 `unref`，不产生悬垂 |
| `exif_entry_get_value` 返回数据有效期 | 写入调用方提供的栈缓冲区，函数返回后立即拷贝进 `QString`，不保留指针 |
| 缓冲区长度 | libexif 0.6.26 先 `memset(val, 0, maxlen)`，再以 `maxlen-1` 为上限写入，必然 NUL 结尾；`char[256]` 足够 |
| `QString` 所有权 | `fromUtf8` 深拷贝；`trimmed()` 可能共享同一 `QStringData`（原子引用计数），安全 |
| 越界写 | 无（写入范围由 `sizeof(value)` 约束） |

### 1.2 `readTime()`（历史修复前路径）

```cpp
ExifData *data = ExifUtility::loadFromBuffer(buffer);
const QString dateTimeDigitized = ExifUtility::readString(data, EXIF_TAG_DATE_TIME_DIGITIZED, EXIF_IFD_EXIF);
exif_data_unref(data);
...
return QDateTime::fromString(dateTimeDigitized, QStringLiteral("yyyy:MM:dd HH:mm:ss"));
```


**当前 main 实现（R31）**：`ExifParser::readTime()` 先检查 `EXIF_FORMAT_ASCII`、`data`、`components`（1..20）和 `size >= components`，再在声明长度内查找 NUL 终止符；之后通过 `parseExifTimestamp(std::string_view)` 解析 `yyyy:MM:dd HH:mm:ss`，用 `QDate`/`QTime` 校验日期时间。此路径不创建中间 `QString`，也不调用 `QDateTime::fromString()`。

| 检查项 | 结论 |
|---|---|
| `ExifData` 生命周期 | `loadFromBuffer` 返回的 `ExifData` 由本函数持有；`readString` 之后 `unref`，之后不再解引用该对象 |
| 输入 `QByteArray` 别名 | libexif `exif_data_load_data()` 不保留输入指针（缩略图另行 `exif_data_alloc` 复制），`unref` 不会碰到调用方缓冲区 |
| `QString` 所有权 | `dateTimeDigitized` 为值语义，函数返回后自然释放 |
| **历史 `QDateTime::fromString` 路径** | ASan 栈中与 double-free 相关；Qt 内部首次非法引用计数操作仍未证实。当前 `readTime()` 已绕开该路径 |

> 这是历史复现路径上的已知调用点，不足以证明它就是首次非法操作来源。当前 main 已绕开该调用路径。

### 1.3 `write()`

`saveToBuffer()` 内部 `exif_data_save_data()` 返回新分配的缓冲区，本模块用 `free()` 释放，
与 libexif 内部 `exif_mem_free`（默认 `free`）匹配；`ExifData` 不持有该缓冲区（已核对
`exif_data_save_data()` 源码：使用 `exif_data_alloc()` 新建，不写回 `data->data`）。
**无重复释放。**

## 2. ExifUtility

| 函数 | 检查项 | 结论 |
|---|---|---|
| `loadFromBuffer` | 输入别名 | libexif 复制所需数据；不持有调用方指针 |
| `createNew` | `exif_data_fix` 产生的 entry | 由 `ExifData` 拥有，`unref` 时统一释放 |
| `initTag` | 引用计数 | `new` 后 `add_entry`（content 持一份）再 `unref`，平衡 |
| `createTag` | `ExifMem` / `ExifEntry` 引用计数 | entry 持有 mem 的引用；`mem_unref`+`entry_unref` 平衡；`exif_entry_free` 用 entry 自己的 mem 释放 data，无跨分配器 free |
| `readShort/readRational` | 长度检查 | 有 `entry->size` 检查 |
| `writeDateTimeOriginal` | 写入长度 | `entry->size >= 20` 且 `qMin(19, size)`，无越界 |
| `saveToBuffer` | `free(exifBuffer)` | 与 libexif 默认分配器一致；仅释放一次 |

## 3. GeoTagController / QtConcurrent / QFutureWatcher

### 3.1 `_readImageCached()`

- 命中缓存：持 `_bufferMutex` 读取并**返回拷贝**（引用计数 +1）。
- 未命中：锁外读文件，锁内插入。
- 所有对 `_imageBuffers` 的访问（插入、`_evictUnmatchedImages`、`_clearImageCache`）都在同一把
  `QMutex` 下。
- `QByteArray` 的引用计数是原子的，主线程删除缓存条目时 worker 仍持有的拷贝不会被提前释放。

结论：**无共享可变容器竞态**，与 ASan 报告一致（ASan 未在该路径报错）。

### 3.2 `_parseExifForImage()` / `_tagImage()`

- worker 只读取 `_cancel`（`std::atomic<bool>`）与缓存；
- `_tagImage()` 在写 EXIF 前 `imageBuffer.detach()`（写时复制），不会改写缓存中的共享缓冲区；
- 输出路径由 `TagTask` 携带，worker 之间不共享可变对象。

### 3.3 Future / Watcher 所有权

| 对象 | 所有者 | 说明 |
|---|---|---|
| `_exifWatcher` / `_tagWatcher` | `GeoTagController` 成员 | 生命周期覆盖所有回调 |
| Future | `QFutureWatcher::setFuture()` 内部持有 | `QtConcurrent::mapped` 的序列按值拷贝进 kernel |
| Worker lambda | 捕获 `this` | 仅在 `GeoTagController` 存活期间执行（见 3.4） |

### 3.4 析构与取消

```cpp
~GeoTagController() { _cancel = true; _exifWatcher.waitForFinished(); _tagWatcher.waitForFinished(); }
```

- `cancelTagging()` 只置标志 + `cancel()` + 报错，不等待；真正的等待发生在析构。
- 析构返回前成员仍存活，worker 不会访问已析构对象。
- `QObject` 析构会移除投递给本对象的待处理事件（`removePostedEvents`），排队中的
  `finished()` 回调不会打到已析构对象。

### 3.5 `setFuture()` 重入

`_onExifFinished` 内会启动 tag 阶段的 `_tagWatcher.setFuture()`，但**不是**对正在发信号的
watcher 自身 `setFuture()`；`_onTagFinished` 不再设置新 future。无自反重入。

### 3.6 R16 未验证假设：`waitForFinished()` 是否泵事件循环

**不成立。** Qt 6.11.1 `qfuturewatcher.cpp`：

```cpp
void QFutureWatcherBase::waitForFinished()
{
    futureInterface().waitForFinished();
}
```

`QFutureInterfaceBase::waitForFinished()` 仅阻塞等待完成条件，不进入事件循环、不派发事件，
因此析构期间不会因它重入 `GeoTagController` 的槽函数。

### 3.7 跨线程 QObject 访问

worker 内未使用 `QObject::tr()` 之外的对象成员；`tr()` 为静态翻译查询，Qt 内部加锁。
（`_parseExifForImage` 的错误分支会调用 `tr()`，本次崩溃与该分支无关。）

## 4. 跨进程资源

| 资源 | 是否共享 | 证据 |
|---|---|---|
| 临时目录 `TMP/TEMP` | 实验 E-07 中**每个实例独立**仍然复现 | 见 `EXPERIMENT_LEDGER.md` |
| 测试输出 `--unittest-output` | 每实例独立文件 | 运行脚本固定传独立路径 |
| 图像输出目录 | 测试内部 `QTemporaryDir`，天然唯一 | 每进程独立 |
| 全局配置 | 测试框架已有 per-process 设置空间（`f170b55`） | — |
| 文件映射/共享内存 | 本模块无 | 代码审计 |
| 共享缓存目录 | 未使用 | 代码审计 |

结论：历史复现发生在同一进程的 worker 线程之间，因此不是由跨进程临时目录共享导致；但这**不能**证明竞争对象就是全局 locale，也不能定位首次非法操作。`QSystemLocalePrivate::zeroDigit()` 路径目前仍是候选机制，证据边界见 `ROOT_CAUSE_AUDIT_20261113.md`。

## 5. 其他日期解析调用点（源码快照审计；非根因结论）

`grep -rn "fromString" src/` 结果：

| 文件 | 调用 | 线程上下文 | 结论 |
|---|---|---|---|
| `src/AnalyzeView/GeoTag/ExifParser.cc`（历史版本） | `QDateTime::fromString` | **QtConcurrent worker** | 历史触发路径；R31 当前实现已改为 `parseExifTimestamp()` |
| `src/AnalyzeView/OnboardLogs/OnboardLogController.cc` | `QDate::fromString` | 主线程（目录扫描回调） | 当前审计未发现 worker 调用；若并行化需重新评估 |
| `src/AnalyzeView/OnboardLogs/OnboardLogController.cc` | `QTime::fromString` | 主线程 | 当前审计未发现 worker 调用；若并行化需重新评估 |

建议：若后续将日期/时间解析移动到 worker 线程，应使用明确的 locale-independent 解析，或先针对目标 Qt 版本做并发安全审查和压力测试；当前证据不足以把所有 `fromString()` 重载一概判定为不安全。

## 6. 次要发现（FGC-0037，已修复并关闭）

历史版本的 `ExifParser::write()` 对 GPS Version ID 四字节写入未验证输入 entry 的实际格式和长度，畸形 JPEG 可触发越界写。该问题已由 FGC-0037 单独修复：当前实现要求 `EXIF_FORMAT_BYTE`、`components == 4`、`data != nullptr`、`size == 4`，否则在写入前返回失败。回归测试覆盖有效 entry、短 entry、错误格式和错误 component 数；PR #39 已合并，Issue #37 已关闭。

该修复与 FGC-0029 的并发 double-free 根因相互独立，不应混淆归因。
