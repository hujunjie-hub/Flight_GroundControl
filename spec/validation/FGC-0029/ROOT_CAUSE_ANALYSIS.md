# FGC-0029 根因分析（R19 修订版）

> 本文取代 R18 版本。历史文本完整保留：
> - R17：`ROOT_CAUSE_ANALYSIS-R17-superseded.md`（SHA256 `DC51467D…20B30`）
> - R18：`ROOT_CAUSE_ANALYSIS-R18-superseded.md`（SHA256 `2628DCAC…38F9F`）
>
> R17 的"`QLocale::system()` / `systemData()` 无锁写共享 QString"已于 R18 撤回；
> R18 把 `0x16D7B1` 记作 `QLocale::system()+0x3B1` 的做法已于 R19 撤回（见 §4）。

本文严格按照证据等级书写：**事实 / 源码确认 / 静态推导 / 未证实** 四类分开，不混写。

---

## 1. 事实（ASan 直接观测）

三份独立报告，逐帧 RVA 完全一致（见 `ASAN_STACK_ANALYSIS.md`）：

| 报告 | 触发程序 | 出错线程 | 分配线程 | 第一次释放线程 |
|---|---|---|---|---|
| `r11-i15.err`（R17） | `GeoTagControllerTest` → `ExifParser::readTime` | T4 | T4 | **T14** |
| `r13-i10.err`（R17） | 同上 | T12 | T12 | **T4** |
| `a-r5-i1.err`（R18） | 仅 `QDateTime::fromString()` | T12 | T12 | **T2** |
| `a-r12-i10.err`（R19） | 仅 `QDateTime::fromString()` | — | — | 另一线程 |
| `legacy-r10-i08.err`（R19） | `ExifParserLegacy::readTime`（8117cfe 原文） | T9 | T9 | **T8** |

**事实 1**：同一个 **20 字节**内存块（`[0x…, 0x…+0x14)`）被**两个不同线程**先后 `free`，
第二次 `free` 被 ASan 判定为 double-free，与产品态 `0xC0000374 / BlockNotBusy_DOUBLE_FREE` 同源。

**事实 2**：分配栈 = `malloc` ← `QArrayData::allocate2` ← `QString::QString(QChar const*, qsizetype)`
← 内部函数 `[0x26C840,0x26C94B)` ← `QSystemLocale::query` ← 内部函数 `[0x16D700,0x16D8FB)`
← … ← `QDateTime::fromString`。

**事实 3**：第一次释放栈 = `free` ← `QArrayData::deallocate`（同一内部函数 `[0x26C840)` 内）
← `QSystemLocale::query` ← `[0x16D700)` ← … ← `QDateTime::fromString`。

**事实 4**：第二次释放栈 = `free` ← `QVariant::~QVariant+0x42` ← 内部函数 `[0x16D700)` ← …
← `QDateTime::fromString`。

**事实 5**：三条线程的分配/释放都发生在同一个 20 字节块上，且该块由 `[0x26C840)` 分配与首次释放。

## 2. 源码 + 反汇编确认（可复核的调用链）

### 2.1 内部函数身份（`.pdata` 边界 + 调用指令，不用"最近导出符号"）

| 区间 | 判定 | 依据 |
|---|---|---|
| `[0x26C840,0x26C94B)` | `QSystemLocalePrivate::zeroDigit()`（`qlocale_win.cpp`） | `GetLocaleInfoW(…, 11)` → `QString(buf,1)` → `mov [rdi+8], newD` → `lock xadd [oldD]` → `QArrayData::deallocate(oldD,2,8)` → 返回 `QVariant`；与源码逐条对应。**置信度：高** |
| `[0x16D700,0x16D8FB)` | `systemLocaleString()`（`qlocale.cpp:1048`） | 唯一具备 `query()` → `QVariant::metaType()` → `QVariant::toString()` 序列的函数。**置信度：高** |
| `[0x167180,0x167216)` | `QLocaleData::zeroDigit()`（`qlocale.cpp:1095`） | 调用 `systemLocaleString()`，并保留 `QString::fromRawData` 回退。**置信度：中高** |
| `[0x167300,0x16745F)` | `QLocaleData::longLongToString(...)` | 由 `QLocale::toString(qlonglong)` 以 `(number, -1, 10, flags)` 调用（反汇编可见实参）。**置信度：中高** |
| `[0x170770,0x1707B2)` | `QLocale::toString(qlonglong)` | 导出符号正好落在函数起点。**置信度：确定** |
| `[0x2ACC20,0x2AD5EF)` 等 | **未解析内部函数** | 无导出、无唯一可匹配序列；仅保留 RVA 与区间 |

### 2.2 已确认的调用链（由调用指令证明）

```
QLocale::toString(qlonglong)                        [0x170770)
  → QLocaleData::longLongToString                     [0x167300)
    → QLocaleData::zeroDigit()                        [0x167180)
      → systemLocaleString()                          [0x16D700)
        → QSystemLocale::query(ZeroDigit)             [0x268210)
          → QSystemLocalePrivate::zeroDigit()         [0x26C840)
```

反汇编证据（`D:\...\dumps\QGroundControl.exe.32020.dmp`，Qt6Core 基址 `0x7FFE5D250000`）：

```
5D3C0788  mov  r9d,-1            ; arg
5D3C07A4  call 5D3B7300          ; toString(qlonglong) → [0x167300)
5D3B735F  call 5D3B7180          ; [0x167300) → [0x167180)
5D3B71A8  call 5D3BD700          ; [0x167180) → [0x16D700)
5D4B869E  call 5D4BC840          ; query+0x48E → [0x26C840)
5D4BC89A  call QString::QString  ; 分配点（报告 0x26C89E）
5D4BC8F4  call QArrayData::deallocate ; 第一次释放（报告 0x26C8F8）
```

### 2.3 锁保护范围（源码确认）

| 路径 | 源码 | 是否持 `systemDataMutex` |
|---|---|---|
| `systemData()` → `updateSystemPrivate()`（首次/失效刷新） | `qlocale.cpp:907-945` | **是** |
| `updateSystemPrivate()` → `query(LocaleChanged)` → `update()` | `qlocale_win.cpp` `update()` | **是**（仅此调用方） |
| `QLocaleData::zeroDigit()` → `localeString`/`systemLocaleString` → `query(ZeroDigit)` | `qlocale.cpp:1048/1067/1095` | **否** |
| `NumericData` 构造 → `getString(ZeroDigit)` | `qlocale.cpp:4411-4429` | **否** |
| `QLocaleData::zeroUcs()` → `query(ZeroDigit)` | `qlocale.cpp:1100-1111` | **否** |

`QSystemLocalePrivate::zero`（1 字符 QString）、`substitutionType`、`sizes` 三个字段都在
`query()` 路径上被无锁读写（`QT_LOCALE_SOURCE_AUDIT.md` §1/§3）。

**结论（源码级）**：`zero` 的写方 `zeroDigit()` 与 `update()` 之间、以及多个 `zeroDigit()`
调用之间，**没有共同的同步机制**。

## 3. 静态推导（不是直接观测）

1. 报告里的"分配 + 第一次释放"都发生在同一次 `zeroDigit()` 赋值里：
   `zero = QString::fromWCharArray(digits, 1)` 会释放**旧值**。因此第一次释放是**合法赋值**的一部分。
2. 要发生 double-free，必须存在另一条对同一数据块的引用/释放：要么另一线程同时进入同一赋值
   （各自读到同一个旧 `d` 指针），要么 `update()` 的 `zero.resize(0)` 与读取方重叠。
3. 哪条先发生、`zero.isEmpty()` 的判定与赋值之间的具体交错、以及引用计数被破坏的**首个操作**，
   现有证据无法确定。

## 4. 已撤回的两次表述

| 轮次 | 原表述 | 撤回理由 |
|---|---|---|
| R17 | "`QLocale::system()` / `systemData()` 无锁修改共享 QString 导致双重释放" | `systemData()` 由 `QBasicMutex systemDataMutex` 保护（R18 复核源码） |
| R18 | "`0x16D7B1` = `QLocale::system()+0x3B1`" | R19 用 `.pdata` 边界证明该 RVA 落在**未导出内部函数** `[0x16D700,0x16D8FB)`；`QLocale::system` 导出只有 16 字节（`[0x16D400,0x16D410)`），且在崩溃栈中根本不出现 |

## 5. 未证实（明确不主张）

1. Qt 内部"哪一次无锁访问最先破坏引用计数"。
2. 两个 worker 线程的精确交错顺序。
3. 深层栈帧（`0x2Axxxx`、`0x1Bxxxx` 中的若干段）是真实调用还是栈展开在 cold fragment 上的
   误归属 —— 这些帧只保留 RVA 与 `.pdata` 区间，不给函数名。

因此，**不得**再写"已证明 Qt 内部双重释放根因"；正确表述是：

> ASan 已确认同一个 20 字节 QString 数据块由两个不同线程先后释放，第二次释放触发 double-free；
> 分配与第一次释放都位于 `QSystemLocalePrivate::zeroDigit()` 的赋值路径；该路径经
> `QSystemLocale::query(ZeroDigit)` 由 `QLocale::toString(qlonglong)` 的数字本地化/补零调用触发，
> 且**不持** `systemDataMutex`。共享字段 `zero` 的无同步读-改-写是**高度相关的候选根因**，
> 但精确的首次破坏操作与线程交错仍待证据确认。

## 6. 修复（保持不变）

`src/AnalyzeView/GeoTag/ExifParser.cc`：`readTime()` 使用 locale 无关的 `parseExifTimestamp()`，
不再调用 `QDateTime::fromString()`。EXIF 时间戳按 CIPA DC-008 即为 locale 无关的定长 ASCII。

- 不是加锁、降并发、吞异常：是把对 Qt 日期解析的依赖整体移除。
- 语义等价与回归测试见 `FIX_VALIDATION.md`。
- 源码注释已按本文口径标注"Qt 内部归属未证实"。

## 7. 差分实验（R19 统一条件，见 `EXPERIMENT_LEDGER.md` E-28）

| 臂 | 内容 | 计划 | 完成 | 超时 | ASan 报告 |
|---|---|---:|---:|---:|---:|
| A | `QLocale::system()` | 240 | 240 | 0 | **0** |
| B | `QDateTime::fromString()` | 240 | 240 | 0 | **1** |
| C | `ExifParserLegacy::readTime`（8117cfe 原文，测试专用副本） | 240 | 240 | 0 | **2** |
| D | `ExifParser::readTime`（修复后） | 240 | 240 | 0 | **0** |

统一条件：12 线程/进程、16 进程/轮、15 轮、**每线程 1000 次迭代**、同一 ASan 构建、同一
Qt 6.11.1 DLL、同一 EXIF 输入。阴性结果只表示"该矩阵未复现"，不推断绝对安全。

## 8. 上游建议

向 Qt 报告时附：`ASAN_STACK_ANALYSIS.md`（三栈）、`QT_LOCALE_SOURCE_AUDIT.md`（字段/锁审计）、
`.pdata` 区间与 RVA。核心问题一句话：**`QSystemLocalePrivate::zero` 在 `query(ZeroDigit)`
路径上无同步读-改-写，而 `systemDataMutex` 不覆盖该路径。**

## 9. 仍开放

1. Qt 内部首个破坏引用计数的操作（需能在崩溃瞬间观察锁/引用计数的工具，或上游确认）。
2. Windows CI（新 HEAD）与 Check Links 的最终结果（见 `LOCAL_VALIDATION_REPORT.md`）。
3. FGC-0037 独立跟踪，`GPSVersionID` 长度缺口单独立项。
