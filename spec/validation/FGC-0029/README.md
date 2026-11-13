# FGC-0029 — GeoTag 并发 EXIF 解析堆内存破坏（R22 状态）

| 项目 | 内容 |
|---|---|
| Issue | https://github.com/hujunjie-hub/Flight_GroundControl/issues/29（**OPEN**） |
| 等级 | S1 |
| 分支 | `codex/phase2-fcu-telemetry` |
| 根因修复提交 | `8295dc05b52bcf2e839bc2bcf2be38bb9109aadb`（保留，未回退） |
| validated_code_head | `89e2914ff1445e1b86aa2851d6fd7fb8c5a30206`（产品 `src/` 的最后一个相关 HEAD） |
| final_head（R21） | `f00966791ae06327aee4beb3e620b43d78f93015` |
| R21 Windows CI | **43077208284 SUCCESS**（Unit 198/198、Integration 70/70、Installer 与安装后验证通过） |
| R21 Check Links | **43077207915 SUCCESS**（`Errors = 0`） |
| R21 pre-commit | **43077210673 SUCCESS** |
| final_head（R22） | `e6185bd5917db4b5d35f803dd31628b6ace56f4b` |
| R22 Windows CI | **43129824822 SUCCESS**（Unit 198/198、Integration 70/70、`FlightTelemetryDispatchTest` 20.34 s、Installer 与安装后验证通过） |
| R22 Check Links | **43129824818 SUCCESS**（`Errors = 0`） |
| R22 pre-commit | **43129824742 SUCCESS** |
| R23 | 仅文档收尾（归档根目录 README 修正、本 README 的 R22 登记、台账 E-38）；HEAD 与 CI 见 R23 交付的 `RUN-IDENTITY.txt` / `CI-RESULT-R23.md` |
| PR #3 | Open / **Draft**（未合并，main 未修改） |

## 1. 结论状态（不得过度表述）

**ASan 已证明的事实**：同一 **20 字节** QString 数据块，在**每一次独立复现**中，
被**两个不同的 worker 线程**先后 `free`；分配与第一次释放位于 `QSystemLocalePrivate::zeroDigit()`
的赋值路径（内部函数 `[0x26C840,0x26C94B)`），第二次释放为 `QVariant::~QVariant`。
六份报告是**六次独立复现**，各自有独立的堆地址与线程编号（见 §2）。

**仍未证实**：**首次破坏引用计数的具体操作**、两线程的精确交错、以及
`QDateTime::fromString()` 内部触发那 ~20 次 `ZeroDigit` 查询的确切调用者。

**Qt 内部根因的表述级别**：`zero` 字段（系统 locale 缓存里的 1 字符 QString）的**无同步读-改-写**
是**高度相关的候选根因**，不是已证实的 Qt 缺陷。`systemData()` 本身由 `QBasicMutex` 保护，
R17 的"无锁修改"说法已撤回。

## 1.1 HEAD 关系（不得混用）

- `validated_code_head = 89e2914`：产品源码 `src/` 自 `54713e5` 起未再修改，二进制与测试行为归属该 HEAD。
  CI：Windows `43061078850` success、Check Links `43061078592` success、pre-commit `43061078770` success。
- `final_head = f009667`（R21）：只包含文档修改。CI：Windows `43077208284` success、
  Check Links `43077207915` success、pre-commit `43077210673` success。
- `final_head = e6185bd`（R22）：只包含文档修改。CI：Windows `43129824822` success、
  Check Links `43129824818` success、pre-commit `43129824742` success。
- `final_head = d49678e`（R23）：只包含文档修改。CI：Windows `43141092273` success、
  Check Links `43141092396` success、pre-commit `43141092261` success。
- **三个（及以上）文档 HEAD 与 validated_code_head 的 CI 结果互不可替代**；引用时必须写明 HEAD。

## 2. 六份 ASan 报告

| # | 文件 | 来源 | 出错线程 | 分配线程 | 第一次释放线程 | 地址 | 块 |
|---|---|---|---|---:|---:|---|---:|
| 1 | `r11-i15.err` | R17 `GeoTagControllerTest` | T4 | T4 | T14 | `0x12ad9c41ceb0` | 20 B |
| 2 | `r13-i10.err` | R17 同上 | T12 | T12 | T4 | `0x11d4518e56b0` | 20 B |
| 3 | `a-r5-i1.err` | R18 仅 `QDateTime::fromString()` | T12 | T12 | T2 | `0x12106faa6220` | 20 B |
| 4 | `a-r12-i10.err` | R19 仅 `QDateTime::fromString()` | T11 | T11 | T10 | `0x11d2eeea9220` | 20 B |
| 5 | `legacy-r10-i08.err` | R19 legacy `readTime` | T9 | T9 | T8 | `0x12d0ac6a7fc0` | 20 B |
| 6 | `legacy-r11-i10.err` | R19 legacy `readTime` | T1 | T1 | T2 | `0x1196b9fa5590` | 20 B |

三栈逐帧展开见 `ASAN_STACK_ANALYSIS.md`（PE `.pdata` 定界，内部函数不命名）。

## 3. 两条路径

| 路径 | 证据等级 | 说明 |
|---|---|---|
| 格式化（`QLocale::toString(qlonglong)` → `longLongToString` → `zeroDigit` → `systemLocaleString` → `query(ZeroDigit)`） | **ASan 捕获** | 六份报告的调用栈 |
| 解析（`QDateTimeParser::parseSection` → `toUInt` → `NumericData` → `getString(ZeroDigit)`） | **源码静态推导** | 一般非 C locale 情形；`QDateTime::fromString()` 设置 C locale 后在 `NumericData` 短路，**不查询 `ZeroDigit`** |

运行时 QueryType 捕获（2 线程 × 1000 次）：`ZeroDigit` 查询 A(`QLocale::system()`)=**0**、
B(`fromString`)=**40040**、C(legacy)=**40040**、D(修复后)=**0**。

## 4. 目录

```
spec/validation/FGC-0029/
    README.md                              ← 本文件（R22 状态）
    ROOT_CAUSE_ANALYSIS.md                 ← 四级证据划分（FACT / SOURCE-CONFIRMED / STATIC-INFERENCE / UNCONFIRMED）
    ROOT_CAUSE_ANALYSIS-R17-superseded.md  ← 被取代的 R17 原文（保留错误假设）
    ROOT_CAUSE_ANALYSIS-R18-superseded.md  ← 被取代的 R18 原文（保留）
    ASAN_STACK_ANALYSIS.md                 ← 六份报告逐帧解析（.pdata）+ 两路径限定 + 运行时 QueryType
    QT_LOCALE_SOURCE_AUDIT.md              ← 字段/锁/生命周期 + 三条 ZeroDigit 路径 + 解析链 §9
    QT_BINARY_IDENTITY.md                  ← Qt 6.11.1 二进制身份
    OWNERSHIP_AUDIT.md                     ← 所有权与异步生命周期审计
    REPRODUCTION_GUIDE.md                  ← 复现步骤
    FIX_VALIDATION.md                      ← 修复前后对照 + R21 更正
    EXPERIMENT_LEDGER.md                   ← E-01…E-38 全部登记（含 SUPERSEDED 标记；R23a 追加 E-39）
    evidence/                              ← 证据索引（真实文件在 ZIP 内）
```

## 5. 状态清单

- [x] 六份 ASan 报告全部登记，三栈可追溯（`.pdata` 定界）
- [x] ASan 确认 double-free；**首次破坏引用计数的操作仍未证实**（不作"已定位"表述）
- [x] Qt locale 源码审计（`zero`/`substitutionType`/`sizes`/`update()`，三条 `ZeroDigit` 路径，锁范围 A/B/C）
- [x] 最小复现（仅 `QDateTime::fromString()`）
- [x] 修复保留（`parseExifTimestamp()`），回归测试 2 例
- [x] 修复后 Release 320 次 / ASan 320 次 0 失败（R17/R18 数据，未重跑）
- [x] 统一 A/B/C/D 差分矩阵 960/960、0 超时（R19 数据，未重跑）
- [x] 产品代码 HEAD `89e2914`：Windows run `43061078850` success（Unit 198/198、Integration 70/70）
- [x] R21 文档 HEAD `f009667`：Windows `43077208284` success、Check Links `43077207915` success、pre-commit `43077210673` success
- [x] R22 文档 HEAD `e6185bd`：Windows `43129824822` success（Unit 198/198、Integration 70/70、FlightTelemetryDispatchTest 20.34 s）、Check Links `43129824818` success、pre-commit `43129824742` success
- [ ] 首次破坏引用计数的具体操作与线程交错（未证实）
- [ ] 外部独立验收
- [ ] Issue #29 关闭（**保持 OPEN**，由独立验收决定）

## 6. 相关但独立

FGC-0037（https://github.com/hujunjie-hub/Flight_GroundControl/issues/37）独立登记，本轮未处理；
`ExifParser::write()` 的 `GPSVersionID` 长度检查缺口见 `OWNERSHIP_AUDIT.md` §6，同样建议单独立项。


