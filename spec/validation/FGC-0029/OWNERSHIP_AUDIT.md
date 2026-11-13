# FGC-0029 对象所有权与异步生命周期审计

审计基线：`8117cfe`（R16）。范围：`ExifParser`、`ExifUtility`、`GeoTagController`、
`QtConcurrent`/`QFuture`/`QFutureWatcher`、`QByteArray`/`QString`、libexif、跨进程资源。

结论概要：

- 本模块**没有**自有的重复释放、悬垂指针或所有权错误能解释该崩溃。
- 崩溃来自 Qt 的**进程级共享状态**（`QLocale::system()`），由 `QDateTime::fromString()` 引入。
- R16 提出的“`waitForFinished()` 泵事件循环导致重入”假设**不成立**（源码证明，见 3.6）。
- 另发现一处与本缺陷无关的防御性缺口（4.2），单独立项，不混入本次根因修复。

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

### 1.2 `readTime()`（修复前）

```cpp
ExifData *data = ExifUtility::loadFromBuffer(buffer);
const QString dateTimeDigitized = ExifUtility::readString(data, EXIF_TAG_DATE_TIME_DIGITIZED, EXIF_IFD_EXIF);
exif_data_unref(data);
...
return QDateTime::fromString(dateTimeDigitized, QStringLiteral("yyyy:MM:dd HH:mm:ss"));
```

| 检查项 | 结论 |
|---|---|
| `ExifData` 生命周期 | `loadFromBuffer` 返回的 `ExifData` 由本函数持有；`readString` 之后 `unref`，之后不再解引用该对象 |
| 输入 `QByteArray` 别名 | libexif `exif_data_load_data()` 不保留输入指针（缩略图另行 `exif_data_alloc` 复制），`unref` 不会碰到调用方缓冲区 |
| `QString` 所有权 | `dateTimeDigitized` 为值语义，函数返回后自然释放 |
| **`QDateTime::fromString` 的可重入性** | **不安全**：构造 `QDateTimeParser` → `QLocale::system()`，见 `ROOT_CAUSE_ANALYSIS.md` |

> 这是本缺陷的**唯一**非法操作来源：本次调用位于 QtConcurrent worker 线程上。

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

结论：**不是**跨进程文件/资源竞争；仅靠**线程级**并发即可触发（ASan 报告中同一进程内两个
worker 线程先后来访同一全局 locale）。

## 5. 其余 `QLocale::system()` 暴露点（同类风险，非本次崩溃）

`grep -rn "fromString" src/` 结果：

| 文件 | 调用 | 线程上下文 | 结论 |
|---|---|---|---|
| `src/AnalyzeView/GeoTag/ExifParser.cc:29` | `QDateTime::fromString` | **QtConcurrent worker** | 本次缺陷，已修复 |
| `src/AnalyzeView/OnboardLogs/OnboardLogController.cc:1045` | `QDate::fromString` | 主线程（目录扫描回调） | 若将来并行化则同类风险 |
| `src/AnalyzeView/OnboardLogs/OnboardLogController.cc:1088` | `QTime::fromString` | 主线程 | 同上 |

建议：后续任何“把日志/目录解析放到 worker 线程”的改动，都不要使用带 format 参数的
`QDateTime/QDate/QTime::fromString`，或等 Qt 修复 `QLocale::system()` 后再放开。

## 6. 次要发现（与本缺陷无关，单独立项）

`ExifParser::write()` 对 GPS Version ID 的四字节写入没有长度检查：

```cpp
ExifEntry *versionEntry = getOrCreateEntry(EXIF_TAG_GPS_VERSION_ID, EXIF_FORMAT_BYTE, 4);
if (versionEntry && versionEntry->data) {
    versionEntry->data[0] = 2; ... versionEntry->data[3] = 0;
}
```

其余写函数都有 `entry->size` 检查，只有此处缺少。若输入图片的 GPS IFD 中
`GPSVersionID` 的 components < 4（畸形/恶意文件），会越界写 1–3 字节。
本轮 ASan 复现中**未**出现该路径（模板图片该标签为 4 字节），因此**不并入本次根因修复**，
以免混淆修复归因；建议新建 issue 按“防御性长度检查”处理。
