# FGC-0029 — GeoTag 并发 EXIF 解析堆内存破坏（R26 状态）

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
    EXPERIMENT_LEDGER.md                   ← E-01…E-40 全部登记（含 SUPERSEDED 标记）
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

FGC-0037（https://github.com/hujunjie-hub/Flight_GroundControl/issues/37）已在 R26 独立修复并通过精确 HEAD 的 MSVC ASan 与 Windows Release 验收，Issue #37 已关闭。该安全修复与本 Issue 的历史堆损坏根因调查相互独立。




---

## R24 current-head validation follow-up — 2026-11-13

The current FGC-0037 validation branch is **fix/fgc-0037-clean**, HEAD **9b22bab756139bd46174515dfb724c0e0120bfdd**, which contains the current phase-2 base. The exact-head Windows Release run [43275165598](https://github.com/hujunjie-hub/Flight_GroundControl/actions/runs/43275165598) passed Unit 198/198, Integration 70/70, executable verification, installer creation, clean installation, and installed-executable verification.

The MSVC ASan run [43275152519](https://github.com/hujunjie-hub/Flight_GroundControl/actions/runs/43275152519) is a **workflow validation failure, not a confirmed product memory failure**:
- ExifParserTest passed 20/20 sequential CTest repetitions.
- GeoTagControllerTest CTest passed its first 1/1 execution.
- The workflow then failed because it required the absent QtTest slot **_concurrentControllersExifStressTest**.
- No AddressSanitizer memory diagnostic was emitted before the stale assertion stopped the job. The intended 20 controller repetitions were not completed.

The R24 implementation did not yet make that correction effective: the sequential workflow step still asserted a test slot that was absent from the checked-in controller test, and the stress harness parsed a QtTest XML shape different from the actual JUnit-style `testsuite/testcase` output. The harness file and multi-process workflow step existed, but the stale sequential gate prevented the matrix from running. R25 below records the code-level correction and exact current-head validation state.

**Status remains BLOCKED / S1.** The bounded EXIF parser path is a mitigation, not proof of the historical first invalid write/free. A successful 12 × 3 matrix would strengthen regression evidence but does not by itself close the root-cause requirement. Do not close Issue #29 until the first invalid operation is identified or an independently accepted causal closure is documented.


---

## R25 — FGC-0029 ASan concurrency gate correction — 2026-11-13

Current branch: `fix/fgc-0037-clean`.

| Commit | Change |
|---|---|
| `c57c9d4a` | Restore `GeoTagControllerTest::_concurrentControllersExifStressTest`: 8 independent controllers, each parsing 24 JPEGs with overlapping QtConcurrent EXIF work |
| `56007c8c` | Register the stress case in the QtTest header |
| `b880c879` | Correct the PowerShell harness to parse the actual JUnit-style QtTest XML (`testsuite/testcase/failure/error`) and require the concurrency case |
| `8068391c` | Correct the sequential ASan XML gate and retain the 12-process × 3-round matrix |

The ASan workflow now has three distinct gates:

1. `ExifParserTest`: 20 sequential runs, including bounded timestamp and concurrent parser regressions.
2. `GeoTagControllerTest`: 20 sequential runs; each XML report must contain `_concurrentControllersExifStressTest`.
3. FGC-0029 multi-process stress: 12 independent `QGroundControl.exe` processes released together after a ready barrier, repeated for 3 rounds. Every process receives isolated `APPDATA`, `LOCALAPPDATA`, `TEMP`, `TMP`, stdout, stderr and XML output. The harness records process IDs, exit codes, timeout state, XML test/failure counts and ASan diagnostics.

The previous exact-head ASan run `43275152519` completed the instrumented build and ExifParserTest 20/20. GeoTagControllerTest itself passed 1/1, but the workflow stopped because the expected stress slot was missing. That run is a workflow-validation failure, not an ASan memory finding and not a completed concurrency matrix.

At the time of the R25 update, current-head CI was pending; the R26 exact-head results below supersede that pending state. **FGC-0029 remains BLOCKED / S1.** A successful 12 × 3 matrix will be regression/mitigation evidence only; it does not identify the historical first invalid operation or prove the Qt-side ownership/refcount root cause. Issue #29 remains OPEN until the causal closure requirement is met.


---

## R26 — exact-head ASan and Windows Release validation — 2026-11-13

**Validated code HEAD:** `d38eaeb6c449ca887fa34bb2eb7ffbddab3b91bb` (based on phase-2 HEAD `c26f20c8209838aeb404382ede506754ecc9f71c`).

### Dedicated MSVC ASan — [43348178961](https://github.com/hujunjie-hub/Flight_GroundControl/actions/runs/43348178961) — SUCCESS

- Complete C/C++ compile and link `/fsanitize=address` instrumentation verified.
- `ExifParserTest`: 20/20 sequential repetitions passed.
- `GeoTagControllerTest`: 20/20 sequential repetitions passed; every report executed `_concurrentControllersExifStressTest`.
- Multi-process matrix: 12 isolated processes × 3 rounds = **36/36** successful process runs; all rounds passed the synchronized ready/start barrier.
- No AddressSanitizer diagnostic; evidence artifact upload succeeded.

### Windows x64 MSVC Release — [43348185004](https://github.com/hujunjie-hub/Flight_GroundControl/actions/runs/43348185004) — SUCCESS

Configure, Build, Unit, serial Integration, installer creation, clean installation, installed executable verification, attestation and artifact upload all passed on the same validated code HEAD.

### Conclusion and boundary

The bounded, locale-independent EXIF timestamp parser and the controller-level concurrency regression have strong exact-head mitigation evidence. The R19 differential matrix remains the leading causal evidence: the legacy `QDateTime::fromString` path produced ASan findings in the B/C arms while the fixed parser D arm did not; runtime QueryType capture showed the legacy path repeatedly querying `ZeroDigit`, `Grouping`, and `GroupSeparator`, while the fixed parser made zero system-locale queries. The Qt 6.11.1 source audit identifies unsynchronized system-locale cache fields as the leading candidate.

**The historical first invalid write/free and exact Qt-side reference-count corruption are still not directly observed.** A clean post-fix stress matrix is mitigation/regression evidence, not proof of the first invalid operation. FGC-0029 therefore remains **OPEN / S1 / BLOCKED**; Issue #29 must not be closed by this R26 pass alone.


## Root-cause source audit — 2026-11-13

The Qt 6.11.1 explicit-format `QDateTime::fromString()` overload used by the original EXIF call sets the parser locale to `QLocale::c()` before parsing. The earlier claim that this exact call necessarily queries `QLocale::system()` is therefore unproven; see [`ROOT_CAUSE_AUDIT_20261113.md`](ROOT_CAUSE_AUDIT_20261113.md). FGC-0029 remains OPEN / S1 / BLOCKED until the historical first invalid operation is captured.
