# Windows Release 验收与交付指南

版本：2.9  
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
| G0 | 仓库/分支/源码一致性 | Git 状态、branch、HEAD | 在 `main` 上验收，工作区变化可解释，测试与 HEAD 一致 |
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
| G12 | 非英语区域设置回归 | environment.json、Unit/Integration 日志 | Unit 宿主和 Qt system locale 为 zh-CN、应用语言为 English；Integration/归档前恢复 en-US |
| G13 | 飞控诊断大屏 UI（Phase 1） | FlightTelemetryPageUITest、JUnit、页面布局断言 | 标题/摘要/卡片结构存在；12/6/1 列断点正确；UNKNOWN/INVALID/STALE/RECOVERY 语义正确；只读边界保持 |
| G14 | 遥控器双摇杆 UI | FlightRadioPageUITest、JUnit、MockLink RC_CHANNELS wire-path | Mode 1/2/3 通道映射、PWM/指示点和第二帧实时跟随正确；CH1–CH3 时左摇杆隐藏且校准禁用；恢复至少 4 路连续通道后校准重新启用；不触发校准或飞控命令 |
| G15 | 飞控诊断指挥中心（Phase 2.4 四参考页） | FlightTelemetryPageUITest、JUnit、G15 manifest gate | Flight 配置顶栏 56 px、左导航 160 px、连续六项 KPI 带容器与网格边界通过；四个页签 SVG 资源路径和 20×20 图标尺寸断言通过；四个页签实际切换独立内容；每个页签 MouseArea hoverEnabled，未选中 hover/idle 颜色和 active 颜色断言通过；标题区英文副标题在 1600/1000/900/700 px 下按桌面/中屏/小屏策略显示且可见元素不越界；四页明细卡可见集合严格为总览 4、健康 6、导航 2、遥控 2，14 张无遗漏/重复；导航/时间、通信/RC 两组卡在桌面同排且各占网格至少 45% 宽度；四张趋势卡均存在 64 px 绘图区、较早/最新时间轴和 gap-count 状态，趋势卡/详细卡圆角 8 px、健康矩阵单元圆角 6 px；总览/健康/导航时序/遥控链路四页存在；健康页 13 类矩阵 4/2/1/1 列、13 个单元边界无溢出，维护横条 3/2/1/1 列，且无综合健康分，底部 CPU/WCET/栈/存储趋势使用真实资源实例并校验实例年龄与 validity mask；导航页 capability bit2/3/5+6 与 UTC 有效性门控；RC 双摇杆沿用 FlightRadio Mode1/2/3 映射，Mode 2 marker 位置与四轴读数一致，mask 清除时对应 marker 隐藏；链路 RSSI/质量/丢包同时满足 family freshness、实例 receiveAgeMs 和 validity mask；趋势 1 Hz/60 点/断点/Vehicle 与 boot reset；不虚构卫星/HDOP/CH5–CH8/SNR/RTT；原 14 张卡片保留；只读 |
| G16 | 设置侧栏字体与状态颜色一致性 | TopLevelViewsTest、JUnit、QML font/palette properties | 一级/二级菜单字体族、point size、字重一致；普通和选中颜色遵循相同 QGCPalette 语义；二级按钮按自身 enabled 状态选择 palette；页面导航和分区交互不回归 |
| G17 | 全项目字体系统与渲染一致性 | qml-typography-check --all、TopLevelViewsTest、JUnit、Windows DPI screenshots | 递归检查全部 src/**/*.qml；除 ScreenTools 兼容别名外无旧字号令牌；无固定 font.pixelSize、硬编码字体族和未登记的局部字号乘数；QGCLabel/共享控件统一字体族、Hinting 与抗锯齿；RadioButton 继承正确字体族；中文界面按平台选择已安装 CJK 字体；100%/125%/150% DPI 视觉证据齐全 |

只有所有适用 Gate 都 PASS，才允许把本轮标记为 PASS。当前完整 Windows Release 交付要求 G0–G17 全部满足（包括 G14 Flight Radio UI、G15 Flight Diagnostics Phase 2）；仅对本次变更范围适用的专项 Gate 必须有精确 HEAD 证据，不适用项须明确说明。

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

地面站遥测的核心链路：

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

### 4.4 高级大屏 UI（FGC-0044）

正式视觉、信息架构和响应式规则见 [飞控诊断高级大屏设计规范](Flight-Diagnostics-大屏设计规范.md)。

- 顶部展示 FCU 遥测接收、飞行状态、失效保护等级和导航估计四张摘要卡；摘要字段必须受对应 family availability 保护。
- 主诊断区使用 12/6/1 列响应式网格，断点为 1450 px 和 760 px；卡片跨度按设计规范执行。
- 每张诊断卡显示本 family 的接收状态徽标；不能把单一 family 的接收状态扩展成全局健康结论。
- 颜色必须与状态文字同时呈现；UNKNOWN/未上报为灰色、STALE 为黄色、明确的故障/无效为红色。
- 保留所有现有诊断卡片、字段、validity gating、per-instance 年龄和稳定 objectName。
- `FlightTelemetryPageUITest` 必须验证标题、摘要、关键卡片、响应式列数和真实 MockLink wire path 上的 UNKNOWN/INVALID/STALE/RECOVERY。
- 第二阶段增加真实 Fact 的 GCS 本地 1 Hz 趋势采样（最近 60 点、有界、freshness gating、缺失断点、Vehicle/boot epoch 清空），必须标注为 GCS sample，不能冒充 FCU 原生历史。
- 增加 13 个遥测 family 的独立健康/接收年龄矩阵，不生成综合健康分。
- 增加真实 RC 输入轴条，严格遵守 `rcValidAxesMask`；不把 RC 输入称为飞机姿态。
- 当前协议没有飞机姿态角，禁止绘制虚构人工地平仪。
- 页面仍只读，不增加控制操作。
- Phase 2 的飞行态势、导航估计和四轴 RC 面板只能显示真实 Fact；RC 轴同时受 `rcValidAxesMask`、RC family freshness、有限数值和协议轴范围约束。
- 导航精度必须按 FCU capability mask 门控：水平位置 bit3、速度 bit5+bit6 同时置位、航向 bit2；能力缺失时显示“未具备”，趋势写断点，不能把仍在发送的数值当作有效估计。
- 13 类遥测域矩阵分别显示 family 状态和接收年龄；event 按异步域处理，不生成全局健康分。
- 四个趋势仅允许使用位置/速度/航向标准差和控制求解耗时；GCS 复用现有 500 ms Timer 每第二次 tick 采样，最多 60 点，必须标注 GCS 1 Hz sample。
- 过期/无效样本必须形成断点，不允许插值或沿用最后有效值；Vehicle 或 boot epoch 变化清空缓存。
- 没有姿态角遥测时禁止人工地平仪；不得将 RC 输入误称为飞行器姿态。
- `FlightTelemetryPageUITest` 必须经 MockLink 原始 MAVLink wire path 验证真实 RC 值、mask 清除、导航 capability mask 缺失、对应趋势 gap、stale gap、boot epoch 与 active Vehicle reset 和无姿态指示器。
- 飞行健康页 13 类矩阵在 1600/1000/700/400 px 下分别为 4/2/1/1 列；Integration 必须确认 13 个单元全部存在且边界位于矩阵网格内部。总览页不显示重复矩阵。

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

只有全部 Gate 通过时才填 PASS。归档脚本会独立校验报告、日志、环境与安装包证据的完整性；它不能替代 G0–G12 的实际验收，也不会把缺失或跳过的测试转为 PASS。

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
10. 本地验收 SHA、推送 SHA 和 `origin/main` 状态。
11. 未解决问题、NOT_RUN / UNVERIFIED 项和下一步建议。

交付报告必须使用 [模板](templates/LOCAL_VALIDATION_REPORT_TEMPLATE.md)。不要只发“已修复”“编译通过”或截图；需要可复核日志和测试报告。

## 9. 阶段完成定义

以下条件用于判定本阶段验收是否完成：

- Windows x64 MSVC Release 配置和编译成功。
- Unit 和 Integration 均实际执行且通过。
- 可执行文件启动验证通过。
- Installer 生成、干净安装、插件依赖和安装后启动验证通过。
- 遥测解码和 MockLink 分发测试全部通过。
- 适用的 MAVLink UDP SIL/故障注入专项测试结果明确；未执行项必须明确列为阶段遗留，不能宣称已覆盖。
- 所有 S0/S1 阻断问题关闭或得到明确且经用户接受的处理决策。
- 验收证据与当前 commit 一致且可复核。
- Windows Release 本地验收通过后推送 `main`，用同一提交的 CI 结果做复核。


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


## 11. 当前主线验收快照（2026-11-14）

- SPEC 同步前审查基线：`77563b118f8757c12e091ce9427ea369e0b9638d`。
- 最近一次完整成功的 Windows Release：run [43567619588](https://github.com/hujunjie-hub/Flight_GroundControl/actions/runs/43567619588)，HEAD `37035a38d8fcdecca6412b3696704d6416aa3600`，Unit 198/198、Integration 71/71，Installer/clean install/installed runtime PASS。
- 工具栏变更进入基线 `77563b1` 后触发的 Windows run [43571983530](https://github.com/hujunjie-hub/Flight_GroundControl/actions/runs/43571983530) 已因后续文档 push 被取消（CANCELLED，非测试失败）。本轮 SPEC 同步只改文档、不改产品源码；新的精确 HEAD 验收由本次 push 触发，必须以 [GitHub Actions](https://github.com/hujunjie-hub/Flight_GroundControl/actions) 的实时状态为准，不能预先标记 PASS。
- 因此，不能把 37035 的完整验收结果直接标记为 77563 的结果；需等待精确 HEAD 的 Windows run 完成。


## 12. Latest exact-main CI diagnosis — 2026-11-14

Diagnostic HEAD: `866a8fbc9518249daf8824a850c017ebd487b188`.

Windows Release run [43598886092](https://github.com/hujunjie-hub/Flight_GroundControl/actions/runs/43598886092):

| Gate / stage | Result | Evidence |
|---|---|---|
| Configure | PASS | Windows x64 MSVC Release |
| Build | PASS | Release compile/link completed |
| Unit | PASS | 198/198 |
| Integration | FAIL | 69/72; three UI tests failed |
| Installer / clean install / installed runtime | NOT_RUN | Skipped because Integration failed |
| Validation archive | PARTIAL | Diagnostic archive uploaded; does not represent PASS |

Integration failures and corrective action:

1. `FlightTelemetryPageUITest`: RC throttle `0.40F` was compared to a double literal with exact equality. Change the four normalized-axis assertions to a 1e-4 tolerance; production RC values and rendering are unchanged.
2. `FlightRadioPageUITest`: the test expected calibration to remain enabled with only three received channels. Product guard correctly requires at least four channels. Verify enabled at eight channels and disabled at three; do not relax product guard.
3. `ToolbarIndicatorUITest`: APM RemoteID drawer expand capability was not found within the old 500 ms wait. Wait for the loaded page's `_showExpand` capability and then for the button; log page `showExpand` and `expandedComponent` state if absent. New exact-HEAD run must establish whether this is timing or a real page issue.

Pre-commit run [43598886118](https://github.com/hujunjie-hub/Flight_GroundControl/actions/runs/43598886118): SUCCESS.

This run is diagnostic evidence for the failed HEAD only. The fixes require a fresh exact-main Windows Release run. No Installer, clean-install, installed-runtime or overall PASS is claimed until all gates execute successfully.


## 13. 设置侧栏字体与状态颜色一致性（FGC-0046）

- 发现基线：`main@ad5709bfe75ea46526eaad2542872c246eaa297d`，用户提供的设置侧栏截图。
- 根因：`AppSettings.qml` 的二级分区标签使用 `ScreenTools.defaultFontPointSize * 0.9`，而一级页面项使用 `QGCLabel` 默认完整字号；二级项还复用外层 palette，而一级 `SettingsButton` 按控件自身 enabled 状态选择颜色组。
- 修复：两级菜单统一使用 `ScreenTools.normalFontFamily`、`ScreenTools.defaultFontPointSize` 和 `Font.Normal`；二级项改为按自身 `enabled` 选择 QGCPalette 颜色组。层级继续由缩进和布局表达，不再通过缩小字号表达。
- 回归：新增 `TopLevelViewsTest::_testSettingsSidebarTypographyConsistency`，验证两级菜单字体族、字号、字重、普通文字颜色和选中文字颜色一致。
- Windows 100%/125%/150% DPI 的真实显示器观感检查不由 CI 字体属性测试代替；本地实测前保持 NOT_RUN。


## 14. 全项目字体系统与渲染一致性（FGC-0047）

- 规范入口：[全项目字体系统与渲染规范](UI-字体系统与渲染规范.md)。
- 修复范围：QGCApplication 全局默认字体、ScreenTools 语义字号/字重角色、QGCLabel、QGCTextField、QGCRadioButton、QGCCheckBox、QGCComboBox、QGCButton、QGCTabButton、QGCMenuItem、QGCSlider、QGCMarqueeLabel、FlightTelemetry、FlightRadio，以及 AppSettings/FlyView/PlanView/Toolbar/Map/AutoPilotPlugins 等页面的旧字号令牌迁移。
- 已确认的源码缺陷：`QGCRadioButton.qml` 的 contentItem 曾将 `font.family` 错误绑定到 `control.font.pointSize`；必须绑定 `control.font.family`。
- 中文界面字体策略：按操作系统选择已安装的 Microsoft YaHei UI / PingFang SC / Noto Sans CJK SC 等字体；未安装时保留 Qt 平台缺字回退。
- 项目内 QML 旧 `default/small/medium/largeFontPointSize` 引用迁移到语义令牌；兼容别名仅保留在 ScreenTools。三维视图固定 `font.pixelSize` 改为 DPI-aware `font.pointSize`；JoystickIndicator 的 monospace 改用 `ScreenTools.fixedFontFamily`。
- `qml-typography-check --all` 递归覆盖全部 `src/**/*.qml`（当前 459 个），检查旧字号令牌、固定 pixelSize、硬编码字体族、错误 font.family 绑定和未登记的页面局部字号乘数。地图罗盘动态缩放与 DebugWindow 字体预览是显式例外。
- 现有 TopLevelViewsTest 继续验证设置侧栏两级菜单；本轮扩展字体族、抗锯齿、Hinting 断言。
- 当前精确 HEAD 的 Windows Release、Unit、Integration、Installer、clean-install、installed-runtime 结果必须重新执行；未执行项目标记 NOT_RUN。
- Windows 100%/125%/150% DPI 实机视觉检查必须提供截图和环境记录，CI 不得代替，未执行前标记 NOT_RUN。


### G15 四参考页专项补充（2026-11-15）

- 四张 1672×941 原图是四个实际页面，不是单页内的静态主题标签。
- Integration 必须逐一点击四个页签，断言标题、六项 KPI、主布局和页内趋势同步切换。
- 1600/1000/700 px 分别验证主布局 12/6/1 列；健康页主矩阵 4/2/1 列、维护信息横条 3/2/1/1 列。
- Mode 1/2/3 映射读取与 FlightRadio 同一持久化设置；Mode 2 双摇杆位置必须与当前逻辑轴数值一致，RC 输入无效或对应 mask 位清除时 marker 隐藏。
- Windows 本地必须提供 1672×941 参考尺寸以及 100%/125%/150% DPI 截图；UI Integration 通过不能代替像素级视觉签收。


- 链路趋势采样必须同时校验 link family freshness、实例 receiveAgeMs 和字段 validity mask；过期实例不得继续显示上次 RSSI/质量/丢包值。

- 健康页趋势必须使用 resource taskId=0 的 CPU/WCET/stack 与 storageFreeKib；resource 实例年龄超时或 validity bit 缺失时形成断点，不得沿用旧值。


## 15. 飞控诊断大屏视觉验收补充（2026-11-15）

- 使用 1672×941 原始四张参考图作为唯一视觉基准；不要用构建成功或 MockLink 数据正确替代截图验收。
- 顶栏 56 px、Flight 左导航 160 px；检查品牌锁定区、真实状态指示器和现有导航入口未发生功能回归。
- 六项 KPI 必须位于单一连续边框带中，桌面 6 等分；在 1600/1000/700/400 px 检查 6/3/2/1 列、内容边界和窄屏说明。
- 运行截图需分别保存四页，按相同分辨率进行 50% 叠加/差异检查；100/125/150% DPI 另行检查字体、卡片高度和裁切。
- 只有 exact-HEAD Windows Release、Unit、Integration、Installer、clean-install、installed-runtime 通过且本地视觉签收完成，FGC-0044 才可关闭。
