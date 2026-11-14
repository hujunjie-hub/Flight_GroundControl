# FGC-0029 — GeoTag 并发 EXIF 解析堆内存破坏（R32 状态）

| 项目 | 内容 |
|---|---|
| Issue | https://github.com/hujunjie-hub/Flight_GroundControl/issues/29（**OPEN**） |
| 等级 | S1 |
| 产品分支 | `main`（历史诊断分支：`diag/fgc0029-pre-fix-pageheap`） |
| 缓解修复提交 | `8295dc05b52bcf2e839bc2bcf2be38bb9109aadb`（保留；不等同于根因已证实） |
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
| PR #3 | **Merged**（`c26f20c8209838aeb404382ede506754ecc9f71c`） |

## 当前版本复测（2026-11-14）

- `main@cccf82a94682197349ed75afe08d8e8352d662fc` 的 Windows Release，EXE SHA-256 `3b8740af3f0423446553d93ef99b3d38ed8baa8d89d1ed329d3362d2432f9077`。
- 使用同一 Release EXE 运行 `Invoke-Fgc0029AsanStress.ps1` 的 12 进程 × 3 轮同步矩阵。36/36 通过，每份 QtTest 报告 19 项，全部执行 `_concurrentControllersExifStressTest`，失败、跳过、超时和 `0xC0000374` 退出均为 0。本轮执行普通 Release 并发复测。
- 本地运行目录 `work/validation/fgc0029-current-release-20261114T062304684Z`。`process-results.csv` SHA-256 `a5ce3a40b521db7be32b4bd86e598d3d2c5adc5bf34d652a82bb5b0e988b101b`。同提交完整验收 Unit 198/198、Integration 71/71、干净安装与安装后启动通过。
- 当前主线在本轮条件下未复现。历史触发路径的缓解已验证，首次破坏引用计数的操作仍待证实，Issue #29 保持 OPEN / S1 / BLOCKED。

## R31 验证（2026-11-13，历史快照）

- **当前 main 产品/测试基线**：测试 HEAD `0b8cb60953277be90d14dc630cd84678f3d333e0`；Windows Release run `43444900333` SUCCESS，Unit `198/198`、Integration `71/71`，非英文系统区域回归、可执行文件、Installer、干净安装及安装后启动验证全部通过。
- **同 HEAD 的工程检查**：pre-commit `43444900133` SUCCESS；CI Scripts `43444900339` SUCCESS（Python 3.10/3.12）。
- **R31 PageHeap workflow HEAD**：`1d117516f0744ec26126b2e23138c238caf1d101`，只更新 FGC-0029 诊断 workflow；产品源码与测试没有在 `0b8cb60` 后变化。
- **当前 main HEAD**：`e6317f76bd7f573619d9a07d4322fad961bceb53`，本轮只同步 R31 验证文档，不修改产品源码、测试或 workflow。
- **历史版本 Release PageHeap**：run `43444888358` SUCCESS，诊断分支 HEAD `91d368b1edfd485517a729cefcdfc34fe6c7b14d`；历史生产源码 `c746af2608de6c51e8632dd75775fef4dac6d97d`，仅注入当前并发回归用例（来源 `119edd4af97af4e159050650317fd945e4e03ecc`）。
- PageHeap 对 `16..32` 字节分配启用 full-size traces（覆盖历史 `20 B` 块），UST 已启用，WER full dump 已配置。`12 × 3 = 36` 个进程全部通过；每份 XML 为 19 tests，包含并执行 `_concurrentControllersExifStressTest`；0 failure/error/skip/timeout，0 个 `0xC0000374`，0 个 WER dump。证据归档 `14049871972` 已逐项检查。
- **判读**：这是历史 Release 时序下的有效非复现结果，不是根因闭环；不能据此关闭 Issue #29。
- **下一轮诊断**：PageHeap workflow 最新修订在 main `1d117516`、诊断分支 `cf676641b062a062e1d68e48646d79b7accb49ea`；exact-head run `43451171747` 已取消（CANCELLED）。它增加缺失 Debuggers feature 的安装兜底，并校验 GFlags 的 `full size traces` 实际输出。

## R32 mainline 快照（2026-11-14）

- SPEC 同步前审查基线：`main@77563b118f8757c12e091ce9427ea369e0b9638d`。
- 最近一次完整成功的 Windows Release：run `43567619588`，HEAD `37035a38d8fcdecca6412b3696704d6416aa3600`，Unit 198/198、Integration 71/71，Installer/clean install/installed runtime PASS。
- HEAD `77563b1` 的 Windows Release run `43571983530` 已因后续文档 push 被取消（CANCELLED，非测试失败）。本轮 SPEC 同步仅改文档、不改产品源码；最新精确 HEAD 的 CI 状态以 GitHub Actions 实时结果为准，不能将前一 HEAD 的完整验收结论替代当前 run。
- PageHeap follow-up run `43451171747` 已结束，结论为 CANCELLED，不是 pending，也不是 PASS。历史 PageHeap run `43444888358` 的 36/36 阴性结果仍只代表该历史时序未复现。
- 本地 mainline Release 并发复测 `20261114T062304684Z` 的 36/36 结果是未复现证据，不证明根因闭环。
- Issue #29 继续 OPEN / S1 / BLOCKED；ASan 确认历史 double-free，但首次破坏引用计数的操作和线程交错仍未证实。

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
- `validated_product_code_head = 73404fc90bbde92ae111d1f5706d7a7f7ef33985`：当前 main 的 `ExifParser.cc`、`ExifParserTest.cc`、`GeoTagControllerTest.cc` blob 与该已通过 MSVC ASan 的产品代码一致；ASan run `43380452713` 为 20/20 ExifParserTest、20/20 GeoTagControllerTest、36/36 同步进程压力通过，0 ASan findings。
- `main_validation_head = 0b8cb60953277be90d14dc630cd84678f3d333e0`：Windows run `43444900333` Unit 198/198、Integration 71/71、安装包与安装后验证通过。
- `main_workflow_head = 1d117516f0744ec26126b2e23138c238caf1d101`：仅 PageHeap 诊断 workflow 更新；不能把 `0b8cb60` 的 CI 结果误写成对不同产品源码的验证。
- `main_docs_head = e6317f76bd7f573619d9a07d4322fad961bceb53`：仅同步本目录 R31 验证文档；产品源码/测试/workflow 与前述 HEAD 相同。

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
    README.md                              ← 本文件（R31 状态）
    ROOT_CAUSE_ANALYSIS.md                 ← 四级证据划分（FACT / SOURCE-CONFIRMED / STATIC-INFERENCE / UNCONFIRMED）
    ROOT_CAUSE_ANALYSIS-R17-superseded.md  ← 被取代的 R17 原文（保留错误假设）
    ROOT_CAUSE_ANALYSIS-R18-superseded.md  ← 被取代的 R18 原文（保留）
    ASAN_STACK_ANALYSIS.md                 ← 六份报告逐帧解析（.pdata）+ 两路径限定 + 运行时 QueryType
    QT_LOCALE_SOURCE_AUDIT.md              ← 字段/锁/生命周期 + 三条 ZeroDigit 路径 + 解析链 §9
    QT_BINARY_IDENTITY.md                  ← Qt 6.11.1 二进制身份
    OWNERSHIP_AUDIT.md                     ← 所有权与异步生命周期审计
    REPRODUCTION_GUIDE.md                  ← 复现步骤
    FIX_VALIDATION.md                      ← 修复前后对照 + R21 更正
    EXPERIMENT_LEDGER.md                   ← E-01…E-42 全部登记（含 SUPERSEDED 标记；保留失败实验）
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
- [x] R31 main HEAD `0b8cb60`：Windows `43444900333` success（Unit 198/198、Integration 71/71、Installer/clean install/runtime verification）；pre-commit `43444900133`、CI Scripts `43444900339` success
- [x] R31 historical Release PageHeap `43444888358`：36/36 processes, 36/36 stress cases, 0 failures, 0 WER dumps; non-reproduction only, not root-cause closure
- [ ] 首次破坏引用计数的具体操作与线程交错（未证实）
- [ ] 外部独立验收
- [ ] Issue #29 关闭（**保持 OPEN**，由独立验收决定）

## 6. 相关但独立

FGC-0037（Issue #37）已通过 PR #39 合并修复并关闭：`GPSVersionID` 现验证 BYTE 格式、4 个 components、非空数据及精确 4 字节长度；畸形短长度、错误格式和错误 component 数有 ASan 回归覆盖。该问题与 FGC-0029 的历史并发堆损坏根因相互独立。

FGC-0029 仍为 **OPEN / S1 / BLOCKED**：ASan 已确认历史 double-free，当前缓解代码和回归矩阵通过，但首次破坏引用计数的操作、确切线程交错及 Qt 内部调用者尚未证实。
