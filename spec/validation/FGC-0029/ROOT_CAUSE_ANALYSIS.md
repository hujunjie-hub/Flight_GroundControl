# FGC-0029 根因分析（R18 修订版）

> 本文取代 R17 版本。R17 原文完整保留在 `ROOT_CAUSE_ANALYSIS-R17-superseded.md`
> （SHA256 `DC51467D34844590F0D6E7DE0BFFBF0F2BE8CFF13FD599218545450439320B30`），
> 其中"`QLocale::system()` / `systemData()` 无锁修改共享 QString"的结论**已被撤回**，
> 撤回理由见 §5。

## 1. 结论分级

| 级别 | 结论 |
|---|---|
| **已验证** | 修复前 `0xC0000374` 的**首次非法操作**是：Qt 进程级系统 locale 缓存里那个 **1 字符 QString**（`QSystemLocalePrivate::zero`，20 字节数据块）被**两个不同的 worker 线程**先后释放。分配与第一次释放都发生在 `QSystemLocalePrivate::zeroDigit()` 的 `zero = ...` 赋值处；第二次释放发生在线程内的 `QVariant::~QVariant`。两条线程均在 `ExifParser::readTime → QDateTime::fromString` 链路内。 |
| **已验证** | 该缺陷**不依赖 libexif、不依赖 GeoTagController**：一个只调用 `QDateTime::fromString()` 的独立程序在同一 ASan 环境下复现了逐帧相同的报告（`diff-run1/a-r5-i1.err`）。 |
| **已验证** | R17 的修复（移除 `QDateTime::fromString`）**消除触发路径**：修复后同一 16 进程 × 15 轮矩阵 240 次运行 0 份 ASan 报告；修复后 Release 320 次并发运行 0 崩溃。 |
| **未证实** | Qt 内部"谁在什么锁之外读写了 `zero`"这一具体责任字段/调用方。`systemDataMutex` 确实存在，因此 R17 的"无锁写"表述不成立；但 `QSystemLocale::query()` 另有不经该锁的调用方（见 §5.3），是否即本次竞态的另一端，尚无实验证据。 |
| **已撤回** | "已证明 `QLocale::system()` / `systemData()` 在无锁情况下修改共享 QString 导致双重释放"。 |

因此：**根因的"对象 + 首次非法操作 + 触发路径"已闭环；"Qt 内部同步缺陷的确切位置"仍未闭环。**

## 2. 故障现象（修复前）

```
EXCEPTION_CODE_STR:  c0000374
SYMBOL_NAME:         ucrtbase!free_base+0
FAILURE_BUCKET_ID:   HEAP_CORRUPTION_ACTIONABLE_BlockNotBusy_DOUBLE_FREE...ucrtbase.dll!free_base
```

崩溃线程始终是 QtConcurrent pool 线程，栈顶始终是 Qt 的日期/本地化解析代码。

对历史 Dump 用带符号的 `!heap -i`：失败块是一个**结构完好的 LFH 空闲块**
（`Block flags: 0x0 LFH (free)`，`Total block size 0x4 units (0x40)`），即真实的重复释放，
不是块头被写坏。

## 3. 首次非法操作（ASan 三栈）

三份报告逐帧 RVA 相同，详见 `ASAN_STACK_ANALYSIS.md`。摘要：

| | 分配 | 第一次释放 | 第二次释放（报错） |
|---|---|---|---|
| 对象 | 20 字节块 | 同一块 | 同一块 |
| 代码 | `QArrayData::allocate2` ← `QString(QChar*,1)` ← `zeroDigit()` 赋值 | `zeroDigit()` 赋值清理旧值（`QArrayData::deallocate`） | `QVariant::~QVariant`（`+0x42`） |
| 线程（r11-i15） | T4 | **T14** | T4 |
| 线程（r13-i10） | T12 | **T4** | T12 |
| 线程（a-r5-i1） | T12 | **T2** | T12 |

**对象身份的证明**（不是猜测）：

1. 分配尺寸：反汇编 `QArrayData::allocate2` 内部为 `allocSize = capacity*objectSize + 0x12`；
   报告中的 region 是 20 字节 → `2*1 + 18 = 20`，即 **capacity = 1 的 UTF-16 QString**。
2. 分配调用链：`QString::QString(QChar const*, qsizetype)`（导出表区间 +0x9D）。
3. 分配点所在函数：反汇编显示
   `GetLocaleInfoW(lcid, LOCALE_SNATIVEDIGITS, digits, 11)` → `QString(digits, 1)` →
   `mov [rdi+8], newD` → `lock xadd [oldD], -1` → `call QArrayData::deallocate(oldD,2,8)`，
   与 `qlocale_win.cpp` 的 `QSystemLocalePrivate::zeroDigit()` 逐条对应。
4. 第二次释放：`QVariant::~QVariant` 的 shared-payload 释放分支（`+0x42` 处 `call [IAT]`）。
5. 因此该块就是 `QSystemLocalePrivate::zero` —— Qt 进程级系统 locale 缓存中的"本地零数字"。

## 4. 完整因果链（保守表述）

1. `GeoTagController::_startParseExif()` 用 `QtConcurrent::mapped` 并行解析图片。
2. 每个 worker 调用 `ExifParser::readTime()`。
3. 修复前 `readTime()` 调用 `QDateTime::fromString(...)`。
4. `QDateTime::fromString()` 在栈上构造 `QDateTimeParser`，构造函数的成员初始化调用
   `QLocale::system()`（`qdatetimeparser_p.h`）。
5. `QLocale::system()` 访问进程级系统 locale 缓存，最终进入
   `QSystemLocale::query(...)` → `QSystemLocalePrivate::zeroDigit()`，
   在**没有可见同步**的情况下对共享成员 `zero` 赋值。
6. 两个 worker 线程并发进入该赋值：一个线程写入新数据并释放旧数据，另一个线程仍持有/再释放
   同一数据块 → `RtlFreeHeap` 报 `HEAP_FAILURE_BLOCK_NOT_BUSY` → `0xC0000374`。
7. 历史 Dump 与 ASan 报告都印证了这一形态（同一 20 字节块、跨线程、`free_base`）。

## 5. Qt 侧事实核对（R17 说法为何撤回）

### 5.1 `systemData()` 是有锁的

`qtbase/src/corelib/text/qlocale.cpp`（v6.11.1）：

```cpp
static const QLocaleData *systemData(qsizetype *sysIndex = nullptr)
{
    /* This modifies (cross-thread) global state, so is mutex-protected. */
    {
        Q_CONSTINIT static QLocaleId sysId;
        bool updated = false;
        Q_CONSTINIT static QBasicMutex systemDataMutex;
        systemDataMutex.lock();
        if (systemLocaleData.m_language_id == 0) { updateSystemPrivate(); updated = true; }
        if (sysIndex && (updated || *sysIndex < 0)) { *sysIndex = QLocaleData::findLocaleIndex(...); ... }
        systemDataMutex.unlock();
    }
    return &systemLocaleData;
}
```

R17 报告忽略了这把锁，因此"`systemData()` 无锁修改共享 QString"**不成立**。

### 5.2 `zeroDigit()` 的赋值本身没有锁

`qlocale_win.cpp` 的 `QSystemLocalePrivate::zeroDigit()` 直接对成员 `zero` 赋值，
反汇编中未见任何 `lock`/互斥量调用（只有 `lock xadd` 引用计数操作）。该成员属于
`Q_GLOBAL_STATIC(QSystemLocalePrivate, systemLocalePrivate)` 进程级单例。

### 5.3 锁没覆盖到的调用方（候选，未证实）

`QSystemLocale::query()` 并非只在 `systemData()` 内被调用。例如 `qlocale.cpp`：

```cpp
char32_t QLocaleData::zeroUcs() const
{
    if (this == &systemLocaleData) {
        const auto text = systemLocale()->query(QSystemLocale::ZeroDigit).toString();  // 无 systemDataMutex
        ...
    }
    return zero().ucsFirst(single_character_data);
}
```

以及 `QLocaleData::zeroDigit()` → `localeString(...)`（`qlocale.cpp:1059` 的
`systemLocale()->query(type)`）同样不获取 `systemDataMutex`。
**这是候选解释，但本轮没有实验能证明本次崩溃正是经由该调用方发生。**

### 5.4 差分实验的旁证

见 §6：只调用 `QLocale::system()` 的模式 0/240 未复现；只调用 `QDateTime::fromString()` 的
模式 1/240 复现。这说明触发点不在"取一个系统 locale 对象"本身，而在**日期解析过程中对
locale 数据的进一步查询**（与 §5.3 的候选路径方向一致），但样本量不足以定论。

## 6. 差分实验（R18，同一 ASan 构建、同一 16 进程 × 15 轮矩阵）

| 模式 | 内容 | 运行 | ASan 报告 |
|---|---|---:|---:|
| A `qlocale` | 仅 `QLocale::system()` | 240 | **0** |
| B `a` | 仅 `QDateTime::fromString()` | 240 | **1**（`a-r5-i1.err`） |
| C `legacy` | 修复前 `readTime()` 原样（libexif + `QDateTime::fromString`） | 240 | 0 |
| D `c` | 修复后 `ExifParser::readTime()`（`parseExifTimestamp`） | 240 | **0**（15 次为外部负载导致的 300 s 超时，`exit=-999`，非 ASan 报告） |

阴性结果只表示"该矩阵下未复现"，不代表绝对安全。

## 7. 为什么以前只看到"组合路径"才复现

| 历史实验 | 结果 | 现在的解释 |
|---|---|---|
| libexif 单读 1/4/12 线程、144 万次 | 通过 | libexif 无共享可变状态，与缺陷无关 |
| `QDateTime::fromString` 单独 | 多为通过 | 竞态窗口窄，需要足够并发压力；R18 用 16 进程 × 15 轮才以 1/240 复现 |
| 完整 `readTime` 12 线程 | 曾复现 | 只有走到 Qt 日期解析才会触碰该系统 locale 缓存 |
| 12 并发进程 × GeoTagControllerTest | 稳定复现 | 多进程争抢 CPU 使 worker 线程并发窗口显著变宽 |

## 8. 已排除的其它假设

| 假设 | 排除依据 |
|---|---|
| libexif 内部堆破坏 | ASan 三栈完全落在 Qt6Core；仅 `QDateTime::fromString` 的最小程序即可复现 |
| `ExifData` 生命周期 | `unref` 后不再解引用；ASan 未在该对象上报错 |
| `_readImageCached` 的 `QByteArray` 跨线程共享 | 引用计数原子 + 全程持 `_bufferMutex`；ASan 未在该路径报错 |
| Controller 析构与 worker 竞争 | 崩溃发生在测试执行中，非析构期；析构体先置 `_cancel` 再 `waitForFinished()` |
| `QFutureWatcher::waitForFinished()` 泵事件循环（R16 未验证假设） | 不成立：它只调用 `futureInterface().waitForFinished()`，不处理事件 |
| 跨进程共享临时目录 | 每实例独立 `TMP/TEMP` 下仍复现（R17 E-07） |
| 测试自身 | 崩溃线程是 QtConcurrent pool 线程；最小复现程序与测试框架无关 |

## 9. 修复

`src/AnalyzeView/GeoTag/ExifParser.cc`：新增 `parseExifTimestamp()`，按 EXIF 规范
（CIPA DC-008）用定长、与 locale 无关的方式读取 `yyyy:MM:dd HH:mm:ss`；`readTime()` 改用它，
文件内不再出现 `QDateTime::fromString`。

- 不是加锁、降并发、吞异常或跳过解析：是把**对非重入 Qt 调用**的依赖整体移除。
- 语义等价：本地时间解释不变；容忍省略前导零；越界/畸形返回无效 `QDateTime`（见
  `FIX_VALIDATION.md` 的表）。
- 源码注释已按本文结论收敛为"Qt 日期解析在本场景下不可并发调用"，不再断言具体 Qt 缺陷
  （见 `ExifParser.cc` 中 `parseExifTimestamp()` 的文档注释）。

## 10. 上游建议（待确认，不作断言）

若要在 Qt 侧闭环，需要回答：`QSystemLocale::query(ZeroDigit)` 的哪些调用方不持有
`systemDataMutex`，以及并发下 `QSystemLocalePrivate::zero` 的赋值是否因此可与读方重叠。
建议带着 `ASAN_STACK_ANALYSIS.md` 的三栈与反汇编向上游提交问题；在这些问题有答案之前，
本项目按"不在多线程中调用 `QDateTime/QDate/QTime::fromString`（带 format）"约束自己。

## 11. R18 修订记录

| 项 | R17 | R18 |
|---|---|---|
| 20 字节块的身份 | 未确定 | **已确定**：`QSystemLocalePrivate::zero`（1 字符 QString） |
| 分配/首次释放位置 | "`QLocale::system` 区间" | **已定位**：`zeroDigit()` 的赋值（反汇编逐条对应） |
| `systemData()` 是否有锁 | 声称无锁（错误） | **有锁**，撤回原说法 |
| Qt 内部责任字段 | 声称已证明 | **未证实**，改为候选解释 |
| 最小复现 | 需要完整 GeoTag 链路 | **仅 `QDateTime::fromString`** 即可复现 |
| 修复 | 已实施 | 保留，注释同步收敛 |

## 12. 仍待解决

1. Qt 内部同步缺陷的最终归属（需上游确认，或需要能在崩溃瞬间观察锁状态的工具）。
2. Windows CI 的新 run 结果（本会话无法读取 GitHub；见 `FIX_VALIDATION.md`）。
3. FGC-0037 单独跟踪（不与本问题混合）。
