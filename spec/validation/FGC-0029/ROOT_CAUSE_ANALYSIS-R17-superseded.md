# FGC-0029 根因分析

状态：**已定位，已修复**（证据见 `work/validation/fgc0029-r17/`，打包于 `20261111-FGC-0029-R17.zip`）

## 1. 故障现象（修复前）

```
0xC0000374 STATUS_HEAP_CORRUPTION
FAILURE_BUCKET_ID: HEAP_CORRUPTION_ACTIONABLE_BlockNotBusy_DOUBLE_FREE...ucrtbase.dll!free_base
```

`free_base` 只是“发现”位置。真正的问题是：**同一个 QString 数据块被两个不同的 worker 线程先后释放。**

## 2. 首次非法操作（ASan 直接证据）

ASan 构建（`build-win-asan-geotag`，MSVC `/fsanitize=address`，覆盖
`ExifParser.cc`、`ExifUtility.cc`、`GeoTagController.cc`、`GeoTagControllerTest.cc` 以及
libexif 全部 C 源码）在 16 并发 × 15 轮 = 240 次进程运行中捕获 2 次：

```
work/validation/fgc0029-r17/asan-run2/r11-i15.err
work/validation/fgc0029-r17/asan-run2/r13-i10.err
```

报告要点（`r11-i15.err`）：

```
ERROR: AddressSanitizer: attempting double-free on 0x12ad9c41ceb0 in thread T4
0x12ad9c41ceb0 is located 0 bytes inside of 20-byte region

  分配（thread T4）
    malloc
    Qt6Core!QArrayData::allocate
    Qt6Core!QString::QString
    Qt6Core                       <- QSystemLocale / systemData
    Qt6Core                       <- QLocale::system
    ...
    ExifParser::readTime(QByteArray const&)          ExifParser.cc:29
    GeoTagController::_parseExifForImage             GeoTagController.cc:747
    GeoTagController::_startParseExif lambda         GeoTagController.cc:500
    QtConcurrent::MappedEachKernel::runIteration
    QtConcurrent::ThreadEngineBase::run

  第一次释放（thread T14 —— 另一个 worker）
    free
    Qt6Core                       <- 同一 systemData 释放路径
    Qt6Core                       <- QLocale::system
    ...
    ExifParser::readTime                              ExifParser.cc:29

  第二次释放（thread T4 —— 与分配同线程，触发崩溃）
    同上调用栈
```

第二份报告（`r13-i10.err`）签名一致：分配在 T12、第一次释放在 **T4**、第二次释放回到 T12。

### 2.1 符号定位

用 `ln` 将 ASan 报告中的 Qt6Core 偏移映射到导出符号：

| 偏移 | 最近导出符号 | 含义 |
|---|---|---|
| `+0x1c236d` | `QArrayData::allocate` | 堆分配 |
| `+0x17f3cd` | `QString::QString` | 分配一个 QString |
| `+0x26c89e` / `+0x26c8f8` | `QSystemLocale::query` 邻域 | 系统 locale 查询/释放 |
| `+0x2686a2` | `QSystemLocale::query+0x492` | 系统 locale 查询 |
| `+0x16d7b1` | **`QLocale::system`** | **系统 locale 缓存更新** |
| `+0x1707a8` | `QDateTimeParser::locale` 邻域 | 解析器默认 locale |

分配与第一次释放**都发生在 `QLocale::system()` 内部**，且第一次释放发生在**另一个线程**。

### 2.2 不是“堆头被写坏”

对历史 Dump 用带符号的 `!heap -i`：

```
Detailed information for block entry 000001b4c0145770
Block flags        : 0x0 LFH (free )
Total block size   : 0x4 units (0x40 bytes)
Subsegment         : 0x000001b4bffc3c30
```

失败块是一个**结构完好的 LFH 空闲块**——它此前确实被释放过，现在被再次释放。
这排除了“相邻缓冲区越界写坏块头”的解释，确认是**真正的双重释放**。

## 3. Qt 侧代码事实

### 3.1 `QDateTime::fromString()` 必然构造 `QDateTimeParser`

`qtbase/src/corelib/time/qdatetimeparser_p.h`（v6.11.1）：

```cpp
QDateTimeParser(QMetaType::Type t, Context ctx, QCalendar cal = QCalendar())
    : defaultLocale(QLocale::system()), parserType(t), context(ctx), calendar(cal)
{
}
```

构造函数**无条件**用 `QLocale::system()` 初始化 `defaultLocale`；
`QDateTime::fromString()` 随后虽然会 `setDefaultLocale(QLocale::c())`，
但 `QLocale::system()` 该发生的调用已经发生了。

### 3.2 `QLocale::system()` 每次调用都写进程级全局

`qtbase/src/corelib/text/qlocale.cpp`（v6.11.1）：

```cpp
QLocale QLocale::system()
{
    constexpr auto sysData = []() { ... return &systemLocaleData; };
    Q_CONSTINIT static QLocalePrivate locale(sysData(), -1, DefaultNumberOptions, 1);
    // Calling systemData() ensures system data is up to date; we also need it
    // to ensure that locale's index stays up to date:
    systemData(&locale.m_index);                 // ← 写共享 static 对象
    Q_ASSERT(locale.m_index >= 0 && locale.m_index < locale_data_size);
    locale.m_numberOptions = defaultNumberOptions(locale.m_data->m_language_id);  // ← 再写一次

    return QLocale(locale);                      // ← 同时其它线程在拷贝同一对象
}
```

配合文件顶部：

```cpp
Q_CONSTINIT static QSystemLocale *_systemLocale = nullptr;   // 无锁惰性单例
Q_CONSTINIT static QLocaleData systemLocaleData = {};        // 全进程共享
```

`systemData()` 会查询操作系统 locale（`QSystemLocale::query` → Windows 区域设置），
把结果（含 QString）写进共享的 `systemLocaleData` / `_systemLocale`；同时把
`locale.m_index`、`locale.m_numberOptions` 写回共享 static `locale`。

因此 `QLocale::system()` 是**“读 + 写共享全局”**，不是只读访问；它无法被多个线程并发调用。
这正是 Qt 自身记录过的历史问题（QTBUG-49473 “QDate/QTime/QDateTime use QLocale::system(),
which makes them non-reentrant”）的残余部分。

## 4. 完整因果链

1. `GeoTagController::_startParseExif()` 用 `QtConcurrent::mapped(_state.imageList, ...)` 并行解析 10 张图片。
2. 每个 worker 调用 `_parseExifForImage()` → `ExifParser::readTime()`。
3. `readTime()` 调用 `QDateTime::fromString(dateTimeDigitized, "yyyy:MM:dd HH:mm:ss")`。
4. `QDateTime::fromString()` 在栈上构造 `QDateTimeParser`，其成员初始化调用 `QLocale::system()`。
5. 多个 worker **同时**进入 `QLocale::system()`：线程 A 把新查询到的 locale 字符串写进共享
   `systemLocaleData` 并释放旧 QString；线程 B 同时在拷贝/更新同一份数据 → 同一缓冲区被
   两个线程先后 `free`。
6. 线程 A 再次析构自己那份 QString 时，块已在空闲链表上 → `RtlFreeHeap` 报
   `HEAP_FAILURE_BLOCK_NOT_BUSY` → `RtlReportFatalFailure` → `0xC0000374`。
7. 崩溃线程永远是 QtConcurrent pool 线程，崩溃栈顶永远是 Qt 的日期解析代码
   （`ucrtbase!free_base` ← `QDateTimeParser` ← `QDateTime::fromString`）。

## 5. 为什么必须“组合路径”才复现

| 实验 | 结果 | 解释 |
|---|---|---|
| libexif 单读（1/4/12 线程，144 万次） | 通过 | libexif 本身无共享可变状态 |
| `QDateTime::fromString` 单独（1/4/12 线程） | 多为通过 | 需要**足够长的并发窗口**；单进程线程数少、迭代少时经常躲过竞态 |
| 完整 `readTime` 12 线程 | 曾复现 | 只有走到 Qt 解析器这一步才触发 `QLocale::system()` |
| 12 并发**进程** × GeoTagControllerTest | 稳定复现（2/36、2/240） | 进程间 CPU 争抢使 worker 线程的并发窗口显著变宽 |
| ASan 构建同条件 | 修复前 2/240 报告 | ASan 拦截了进程内所有 `free`（含未插桩的 Qt6Core.dll），已用独立 DLL 实验证明 |

`QLocale::system()` 的竞态是“写-写”型：即使预先调用一次预热也无法消除，
因为**每次调用都会重新写** `locale.m_index` / `m_numberOptions` 并可能刷新系统数据。

## 6. 已排除的其它假设

| 假设 | 排除依据 |
|---|---|
| libexif 内部堆破坏 | libexif 无共享状态；ASan 报告中分配/释放栈完全落在 Qt6Core 的 locale 路径，libexif 帧不在栈上 |
| `ExifData` 生命周期（`readTime` 中 `unref` 后使用） | `unref` 之后不再解引用；ASan 未报 use-after-free 于 libexif 对象 |
| 图像缓存 `QByteArray` 跨线程共享 | `QByteArray` 引用计数为原子；`_readImageCached` 持 `_bufferMutex` 返回拷贝；ASan 未报该路径 |
| Controller 析构与 worker 竞争 | `~GeoTagController` 先置 `_cancel` 再 `waitForFinished()`；崩溃进程仍在测试执行中，非析构期 |
| `QFutureWatcher::waitForFinished()` 泵事件循环导致重入（R16 未验证假设） | **不成立**：`QFutureWatcherBase::waitForFinished()` 仅调用 `futureInterface().waitForFinished()` 阻塞等待，不处理事件（见 `OWNERSHIP_AUDIT.md`） |
| 跨进程共享临时目录/缓存 | 本轮每个实例独立 `TMP/TEMP` 仍复现（见实验台账 E-07），排除临时文件竞争 |
| 测试自身（QSignalSpy/QTRY） | 崩溃发生在 QtConcurrent worker 线程，主线程栈正常 |

## 7. 修复

`src/AnalyzeView/GeoTag/ExifParser.cc`：

- 新增匿名命名空间函数 `parseExifTimestamp(QStringView)`，按 EXIF 规范（CIPA DC-008）
  读取定长 `yyyy:MM:dd HH:mm:ss`，支持相机常见的前导零省略，非法输入返回无效 `QDateTime`。
- `readTime()` 改为 `return parseExifTimestamp(dateTimeDigitized);`，
  **文件内不再出现 `QDateTime::fromString`**（`grep` 可验证）。
- 该实现不访问任何 locale、不分配共享状态，因而可安全地从任意多个线程并发调用。

语义等价性说明：

| 输入 | 修复前（Qt 解析器） | 修复后 |
|---|---|---|
| `2024:05:06 07:08:09` | 有效 | 有效（相同值） |
| `2024:5:6 7:8:9` | 有效（允许省略前导零） | 有效（相同值） |
| `2023:02:29 …` | 无效 | 无效 |
| 月/日/时/分/秒越界 | 无效 | 无效 |
| 分隔符错误 / 尾部多余字符 / 空串 | 无效 | 无效 |
| 时区 | 本地时间（无时区信息） | 本地时间（不变） |

## 8. 上游建议

该缺陷属于 Qt 6.11.1 `QLocale::system()` 的线程安全问题（`QDateTimeParser` 无条件调用它）。
建议向 Qt 提交 issue：`QLocale::system()` 的静态 `QLocalePrivate` 更新需加锁或改为不修改共享状态。
在 Qt 修复前，任何“多线程调用 `QDateTime/QDate/QTime::fromString`（带 format 参数）或
`QLocale::system()`”的代码都有同类风险；本仓库其余调用点见 `OWNERSHIP_AUDIT.md` 第 5 节。
