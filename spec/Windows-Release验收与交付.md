# Windows Release 验收与交付指南

版本：1.0  
适用仓库：Flight_GroundControl  
目标：Windows x64 / MSVC 2022 / Release only

## 1. 验收总原则

本指南定义 GroundControl 当前阶段的本地和 CI 验收闭环。验收必须以当前代码 commit 和本轮运行证据为准。

- 编译通过不代表 Unit/Integration 通过。
- Unit/Integration 通过不代表 Installer 安装通过。
- MockLink 通过不代表真实 Flight UDP SIL 互操作通过。
- Host/SIL 通过不代表 H723 实机测试或飞行安全放行。
- 未运行、跳过、取消、报告缺失、测试数量为零、旧报告或旧二进制混用，一律不能记为 PASS。

## 2. 验收阶段和门禁

| Gate | 内容 | 必需证据 | 通过条件 |
|---|---|---|---|
| G0 | 仓库/分支/源码一致性 | Git 状态、branch、HEAD、PR | 在指定 feature branch；工作区变化可解释；测试与 HEAD 一致 |
| G1 | Windows 环境 | 环境报告 | Windows x64、MSVC 2022 x64、Qt MSVC x64、GStreamer MSVC x64 |
| G2 | CMake Configure | configure.log、CMakeCache 摘要 | Release、Ninja、测试开启、依赖路径有效 |
| G3 | Release Build | build.log | 编译和链接退出码 0，无未解决错误 |
| G4 | Unit | CTest log + JUnit | 发现并执行非零测试；failures/errors=0；无报告缺失 |
| G5 | Integration | CTest log + JUnit | 串行执行；发现并执行非零测试；failures/errors=0 |
| G6 | Executable | 启动验证日志 | 当前 build 的 QGroundControl.exe 成功启动并在超时内正常响应 |
| G7 | Installer | Installer 文件 + SHA-256 | 本轮生成新安装包；架构/版本正确 |
| G8 | Clean Install | 安装日志、文件清单 | 新临时目录安装成功；QGroundControl.exe 和必需插件存在 |
| G9 | Installed Runtime | 启动验证日志 | 移除构建 SDK 的 PATH 后，安装版可执行文件仍能启动 |
| G10 | 静态质量 | lint/pre-commit/CI checks | 适用检查通过；不触发其他平台产品构建 |
| G11 | 证据交付 | RUN_ID、ZIP、SHA-256、报告 | 文件齐全、哈希可复核、commit 对齐 |

只有所有适用 Gate 都 PASS，才允许把本轮标记为 PASS。当前阶段完整 Windows Release 交付要求 G0–G11 全部满足。

## 3. 单元和集成测试要求

默认测试过滤与 Windows CI 一致：

- Unit：CTest label `Unit`，排除 `Flaky|Network`。
- Integration：CTest label `Integration`，排除 `Flaky|Network`，串行执行。
- 记录 CTest 实际发现数、执行数、通过数、失败数、错误数、跳过数。
- JUnit XML 和原始 CTest log 必须同时存在。
- JUnit 的 tests 数量大于 0；failures 和 errors 必须为 0。
- CTest exit code 非零、测试步骤 skipped/cancelled、JUnit 缺失或解析失败，均为 FAIL/UNVERIFIED，不可 PASS。
- 若标签筛选导致 0 个测试，必须检查测试注册、label 和过滤条件，不能通过降低门槛放行。

CI 排除 Flaky/Network 是固定规则，不代表这些场景自动获得覆盖。对 MAVLink 网络行为的专项测试必须按第 5 节另行记录。

## 4. FCU 遥测协议和软件路径验收

本 PR 的核心链路：

`MAVLink wire frame → MockLink/Link → MAVLink parser → Vehicle dispatch → FlightTelemetryFactGroup → QML diagnostic UI`

### 4.1 单元/解码层

- 13 个 FCU 消息 ID、payload length、CRC_EXTRA 与 Flight ICD 一致。
- 所有 enum、boolean、validity mask、capability mask 和 channel mask 均按协议验证。
- 未知/reserved 值不得映射成健康或有效状态。
- 时间戳、boot ID、profile ID 交叉检查；旧数据不得覆盖新数据。
- 事件去重和有界历史正确。
- BUILD_INFO digest chunks 不得混合不同固件快照。

### 4.2 MockLink 真实分发层

`FlightTelemetryDispatchTest` 必须通过序列化字节和真实 MAVLink parser，不允许直接调用 FactGroup 的内部处理函数来代替链路测试。

覆盖：

- 13 类遥测消息通过真实 Vehicle 分发路径进入对应 Vehicle FactGroup。
- 损坏 CRC 在进入 Vehicle dispatch 前被拒绝。
- 错误 system ID / component ID 被拒绝。
- 保留 enum、非法 mask、无效 boolean 被拒绝。
- 重复/乱序/旧时间戳不覆盖最后有效数据。
- 多 Vehicle 实例遥测状态相互隔离。
- 断开/重连后旧状态不会污染新 Vehicle。
- FCU reboot / boot ID 变化后缓存和时间水位正确复位。
- profile 不一致时不得混合数据。

### 4.3 QML 诊断页面

- 无数据、过期、无效 mask、未知枚举显示为不可用/未知，而不是数值 0 或健康。
- 每个遥测 family 和 per-instance age 独立显示。
- 传感器、链路、资源列表不能由一个实例的更新掩盖其他实例过期。
- 事件历史有界，显示内容与消息 ID/时间一致。
- Build info 仅显示遥测摘要，不把 hash 当作签名或可信固件来源证明。
- 页面只读；不得产生 ARM、模式切换、Permit 或执行器授权。

## 5. MAVLink 网络和真实固件互操作专项验收

MockLink 是必要的 host 集成测试，但不等同于真实 UDP SIL。专项测试按阶段执行并单独记录：

| 场景 | 测试方式 | 预期 |
|---|---|---|
| Flight UDP SIL 正常遥测 | 运行匹配的 Flight Host/SIL 发布端，通过 UDP 连接 GroundControl | 13 类消息按 ICD 解码，Fact/QML 状态正确 |
| 丢包 | 可控丢弃一定比例遥测帧 | 显示 family/instance receive age 增长，不伪造健康 |
| 乱序 | 注入较旧时间戳帧 | 旧帧不覆盖最新值 |
| 重复帧 | 重复发送相同事件/周期帧 | 去重/时间水位符合协议 |
| CRC 错误 | 修改 checksum | parser 拒绝，Fact 不更新 |
| 错误来源 | 改 system/component ID | 非目标来源不进入当前 Vehicle |
| 断链/恢复 | 关闭 UDP/MockLink 后恢复 | 状态过期/断开提示正确，恢复后只接受当前有效会话 |
| FCU 重启 | 变化 boot ID 并注入旧 boot 延迟包 | 新会话建立，旧 boot 数据不回滚缓存 |
| profile/schema 变化 | 注入不同 profile/schema | 显示未知/不兼容，不混合快照 |
| 高负载/突发 | 突发发送遥测帧 | UI 不崩溃，缓存有界，数据更新可追踪 |

每项记录：Flight commit、GroundControl commit、传输方式、丢包/乱序参数、RUN_ID、日志、抓包（如有）和结果。若没有匹配的 Flight SIL 发布端，必须标记 NOT_RUN，不能以 MockLink 测试代替。

当前传感器实例 ID 发布端不一致问题继续按 `doc/gcs/FCU_TELEMETRY.md` 处理：GCS 显示原始 ID，不推断传感器名称，直到 Flight 发布端修正并通过 ICD 一致性验证。

## 6. 运行证据要求

每次完整验证使用唯一 RUN_ID：

`YYYYMMDDTHHMMSSfffZ_<groundcontrol-commit-short>`

本地临时报告：

`work/validation/current/`

不可覆盖的归档：

`work/validation/archives/<RUN_ID>/`  
`work/validation/archives/<RUN_ID>.zip`  
`work/validation/archives/<RUN_ID>.zip.sha256`

至少归档：

- `LOCAL_VALIDATION_REPORT.md`
- `configure.log`、`build.log`
- `unit-ctest.log`、`integration-ctest.log`
- `junit-results-windows-unit.xml`、`junit-results-windows-integration.xml`
- 环境摘要、Git branch/HEAD/status
- CMake 配置摘要
- 可执行文件验证日志
- Installer 路径、文件大小、SHA-256
- 安装和安装后运行验证日志
- 证据文件 SHA-256 清单

执行归档：

```powershell
$RunId = (Get-Date).ToUniversalTime().ToString("yyyyMMddTHHmmssfffZ") + "_" + (git rev-parse --short HEAD)
pwsh -File .\\tools\\validation\\Archive-WindowsReleaseRun.ps1 -RunId $RunId -Result PASS
```

只有全部 Gate 通过时才填 PASS。归档脚本会独立校验报告、日志、环境与安装包证据的完整性；它不能替代 G0–G10 的实际验收，也不会把缺失或跳过的测试转为 PASS。

## 7. 问题登记和关闭

- 所有问题统一登记 GitHub Issues，项目 ID 为 `FGC-{issue_number:04d}`。
- 不维护第二份 CSV/Excel/Markdown 动态问题表。
- 每个问题必须关联发现它的 RUN_ID 和具体测试/日志。
- 修复后生成新的 RUN_ID，引用修复 commit 和回归结果。
- 问题关闭必须满足 [问题关闭清单](quality/ISSUE_CLOSURE_CHECKLIST.md)。
- PR Draft、测试 PASS、Issue CLOSED 是不同状态；任何一个都不能自动代表其他两个。

## 8. 本轮交付物清单

每次本地 Agent 完成一轮后，必须交付：

1. 修改文件列表及修改理由。
2. 根因说明和关联 FGC Issue。
3. 当前 branch、HEAD SHA、工作区状态。
4. configure/build 实际命令和退出码。
5. Unit / Integration 实际执行数量、失败/错误/跳过数量。
6. QGroundControl.exe 启动验证结果。
7. Installer 文件名、大小、SHA-256。
8. Clean install 和安装后运行验证结果。
9. RUN_ID、归档目录、ZIP 路径和 ZIP SHA-256。
10. 新提交 SHA、是否推送、PR #3 当前状态。
11. 未解决问题、NOT_RUN / UNVERIFIED 项和下一步建议。

交付报告必须使用 [模板](templates/LOCAL_VALIDATION_REPORT_TEMPLATE.md)。不要只发“已修复”“编译通过”或截图；需要可复核日志和测试报告。

## 9. 阶段完成定义

只有以下条件全部满足，才建议 PR #3 从 Draft 转为可评审状态：

- Windows x64 MSVC Release 配置和编译成功。
- Unit 和 Integration 均实际执行且通过。
- 可执行文件启动验证通过。
- Installer 生成、干净安装、插件依赖和安装后启动验证通过。
- 本 PR 遥测解码和 MockLink 分发测试全部通过。
- 适用的 MAVLink UDP SIL/故障注入专项测试结果明确；未执行项必须明确列为阶段遗留，不能宣称已覆盖。
- 所有 S0/S1 阻断问题关闭或得到明确且经用户接受的处理决策。
- 验收证据与当前 commit 一致且可复核。
- 用户明确授权后才可改变 PR Draft 状态或合并；本指南不授权合并 main。


## 10. GitHub Windows CI 自动证据归档门禁

Windows Release workflow 在 Unit、Integration、Installer、干净安装和安装版启动检查结束后，自动执行 `tools/validation/Archive-WindowsReleaseRun.ps1`，并上传归档 ZIP 与 SHA-256。

自动归档规则：

- 归档脚本同时支持仓库相对路径和绝对路径，CI 直接使用本轮 `$RUNNER_TEMP/build`。
- Configure、Build、Unit、Integration、构建产物启动、Installer、干净安装、安装版启动以及 CI 区域设置步骤全部成功后，CI 才请求 `Result=PASS`。
- CI 传入 `-RequireCiLocale`，归档必须记录 Unit 宿主为 `zh-CN`、Integration 前恢复为 `en-US`。本地归档记录实际区域设置并保留其余证据检查，开发机可沿用原有设置。
- PASS 必须同时包含非空的 `configure.log`、`build.log`、`test-output-windows-unit.txt`、`test-output-windows-integration.txt`，以及 `junit-results-windows-unit.xml`、`junit-results-windows-integration.xml`。
- `cmake-cache-summary.txt` 必须先从本轮 `CMakeCache.txt` 生成，再执行 PASS 文件存在性和非空校验；不能在生成前检查它，否则会把有效构建错误判为归档失败。
- 两份 JUnit 必须可解析、测试数量非零，failures、errors、skipped 均为 0。
- Qt/GStreamer 根目录优先采用存在的 `QT_ROOT_DIR` / `GSTREAMER_1_0_ROOT_MSVC_X86_64` 环境目录；缺失或无效时，分别回退到 CMakeCache 的 `Qt6_DIR`（解析到 Qt 安装根目录）和 `GStreamer_ROOT_DIR`。目录不存在时记录 `UNVERIFIED`。
- PASS 还要求 CMake、Ninja、Python、MSVC、Qt、GStreamer 环境事实可从构建缓存/实际可执行文件解析；MSVC PATH 编译器若可用，必须与 CMake 缓存编译器路径一致。
- 若调用方请求 PASS 但证据不完整，脚本仍先生成诊断 ZIP 和 SHA-256，再将归档验证结果降为 FAIL 并使 CI 步骤失败；不得生成缺报告的 PASS 归档。
- 归档 manifest 的 UTC 时间戳使用 PowerShell `[DateTime]::UtcNow` 属性（不带方法调用括号）；Windows CI 必须实际执行归档步骤并验证 ZIP 与 SHA-256 文件存在，不能仅以脚本静态检查通过作为验收。
- FAIL / PARTIAL 仍保留本轮可获得的日志、JUnit、环境摘要和清单，便于定位失败。
- 归档 manifest 记录 GitHub run ID、attempt、PR head ref、实际 checkout commit、JUnit 统计、环境来源、逐文件 SHA-256 和 ZIP SHA-256。

CI 归档 Artifact 名称为 `windows-release-validation-<github.sha>`，包含：

- `<RUN_ID>.zip`
- `<RUN_ID>.zip.sha256`

此门禁用于验证 FGC-0004（JUnit 报告完整性）和 FGC-0019（构建环境证据真实性）。上传成功不代表测试 PASS；必须检查归档 manifest 中的 `result_validated` 和各项 JUnit 统计。
