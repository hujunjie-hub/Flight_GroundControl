# Flight_GroundControl 项目规范入口

- 版本 4.34
- 日期 2026-11-15
- 适用仓库 `hujunjie-hub/Flight_GroundControl`
- 当前工作分支 `main`

## 1. 给本地 Agent 的第一条指令

把下面这段直接交给本地 Codex / Claude Code / 其他代码 Agent：

> 请先完整读取仓库根目录 `AGENTS.md` 和本文件引用的规范。用 PowerShell 7 同步 `main`，检查 Git 状态并保留用户修改。在本地完成 Windows x64 MSVC Release 配置、构建、Unit、Integration、可执行文件和 Installer 验证，按 RUN_ID 归档。记录实际结果并提交，验收通过后直接推送 `main`。GitHub Actions 用于复核推送的提交。

## 2. 项目交付原则

本仓库采用本地 Windows 构建与验收，验证通过后直接交付到 `main`。

- 本地 Agent：扫描代码、定位根因、修改实现和测试、本地编译、运行测试、生成证据、提交修复。
- 用户：在 Windows 开发机运行 Agent，审阅 Agent 的修改和提交；必要时上传证据 ZIP。
- ChatGPT：负责架构与协议审查、验收标准、复杂问题分析、回归证据复核和阶段放行建议。
- GitHub：保存代码、提交和动态问题状态，并对已推送的提交执行 CI。
- `spec/`：保存正式规范、验收标准、问题闭环流程、Agent 操作指南。
- `work/validation/`：保存本地运行日志和临时报告，不提交 Git。

**不要再把每一个编译错误都等待远程 CI 才开始修复。** 本地 Release 编译和聚焦测试是主要快速反馈回路；GitHub Actions 用于提交后的独立复核。

## 3. 规范文件地图

按以下顺序阅读：

1. [Agent 操作总指南](AGENT-操作指南.md)：Agent 每次接手任务的强制步骤。
2. [Windows 本地开发、构建与调试指南](本地Windows开发与调试指南.md)：拉代码、安装依赖、配置 MSVC/Qt/GStreamer、编译和调试。
3. [Windows Release 验收与交付指南](Windows-Release验收与交付.md)：完整验收矩阵、通过条件和交付物。
4. [问题登记与闭环规范](quality/ISSUE_WORKFLOW.md)：问题 ID、类型、严重级别、状态和证据要求。
5. [问题关闭清单](quality/ISSUE_CLOSURE_CHECKLIST.md)：问题关闭前的强制核对。
6. [机器可读验收清单](validation/windows_release_acceptance_manifest.json)：供 Agent 和脚本读取的验收门槛。
7. [本地验收报告模板](templates/LOCAL_VALIDATION_REPORT_TEMPLATE.md)：每轮开发和验收结果必须按此输出。
8. [FCU 遥测接口说明](../doc/gcs/FCU_TELEMETRY.md)：13 类遥测消息、FactGroup、有效性、边界和当前固件发布端限制。
9. 根目录 [AGENTS.md](../AGENTS.md)、[test/README.md](../test/README.md)、[tools/README.md](../tools/README.md) 和 [.github/ci-overview.md](../.github/ci-overview.md)：代码风格、测试框架和 CI 实现细节。
10. [主线评估与优化路线图](主线评估与优化路线图.md)：合并后的基线、优先级、实施顺序和验收标准。
11. [Flight ↔ GroundControl UDP SIL 验证记录](validation/Flight-UDP-SIL-20261114.md)：跨仓库互操作的实际结果与未执行矩阵。
12. `validation/FGC-*`：按 Issue 保存的专项诊断/回归证据；R17/R18 superseded 与 ASan/Qt 源码审计等历史材料保持原样，当前状态以各目录 README 的最新快照为准。FGC-0043 的 EXIF typed-reader 专项验收见 [FGC-0043](validation/FGC-0043/README.md)。
13. [飞控诊断高级大屏设计规范](Flight-Diagnostics-大屏设计规范.md)：飞控诊断页信息架构、视觉令牌、响应式布局、状态语义和 UI 验收标准。
14. [遥控器双摇杆实时监视设计规范](Flight-Radio-可视化设计规范.md)：原有双圆形摇杆示意图、实时 RC 数据跟随、手型映射和调试读数。
15. [全项目字体系统与渲染规范](UI-字体系统与渲染规范.md)：字体族/中文回退、语义字号与字重、抗锯齿/Hinting、颜色和 DPI 验收。
16. [设置侧栏字体与状态颜色规范](UI-设置侧栏字体与颜色规范.md)：设置一级/二级菜单的专项规则，服从全项目字体系统规范。
17. [参数中心 3.0 设计与适配规范](Parameter-Center-3.0-设计与适配规范.md)：单机型唯一绑定、参数域工作台、在线调参、响应验证和分阶段像素级移植。

## 4. 当前固定工程约束

| 项目 | 固定要求 |
|---|---|
| 产品构建平台 | Windows x64 |
| 本地命令环境 | PowerShell 7（`pwsh`） |
| 编译器 | Visual Studio 2022 / MSVC x64 |
| 构建配置 | Release only |
| Qt | 以 `.github/build-config.json` 为准；当前 6.11.1，MSVC 2022 x64 |
| GStreamer | 以 `.github/build-config.json` 为准；当前 Windows 1.28.4 |
| 构建系统 | CMake + Ninja |
| 测试 | Unit + Integration，记录实际执行数量 |
| 安装包 | Windows Installer，必须安装并验证 |
| 开发和交付分支 | `main`，本地验收通过后正常推送 |

Linux、macOS、Android、iOS、ARM64、Windows Debug、AppImage、Docker、Flatpak 等不属于本阶段产品构建和测试目标。Ubuntu runner 仍可执行纯文档、脚本、CodeQL、pre-commit 等与 Linux 产品构建无关的质量检查。

## 5. 遥测和飞控安全边界

GroundControl 中的 Flight FCU 遥测全部是诊断和展示数据：

- 不得通过遥测值自行授予 ARM、Permit、控制权或执行器提交权限。
- 不得把“地面站显示健康”作为飞控安全条件满足的证据。
- 不得绕过 Flight FMS 授权、Permit、安全门或飞控侧状态检查。
- 未知枚举、无效 mask、过期或缺失数据必须显示为未知/不可用，不得推断为健康。
- Host/MockLink 测试通过不代表 UDP SIL 互操作通过，更不代表 H723 实机或真实飞行放行。

当前已知跨仓库问题：Flight ICD 的传感器实例 ID 定义为 IMU=1、GNSS=2、BARO=3、MAG=4，而当前 Flight 发布端枚举顺序与该定义不一致。GroundControl 暂时显示原始 ID，不得擅自猜测或重新定义协议。

## 6. 验收结果状态

测试结果统一使用：

- `PASS`：所有适用断言实际执行且通过，证据完整。
- `FAIL`：至少一个适用断言失败，或构建/测试命令失败。
- `PARTIAL`：部分测试通过，但仍有适用项未完成。
- `NOT_RUN`：尚未执行。
- `UNVERIFIED`：有结果或陈述，但证据不足以确认。

**缺少报告、测试数量为 0、步骤跳过、被取消、运行中断、旧报告混入、源码 commit 不匹配，均不得标记 PASS。**

问题状态与测试状态分别记录。

## 7. 本地交付和证据

每轮本地验证必须生成唯一 RUN_ID，并按 [验收与交付指南](Windows-Release验收与交付.md) 归档：

```text
work/validation/
├── current/
│   ├── configure.log
│   ├── build.log
│   ├── unit-ctest.log
│   ├── integration-ctest.log
│   └── LOCAL_VALIDATION_REPORT.md
└── archives/
    ├── <RUN_ID>/
    ├── <RUN_ID>.zip
    └── <RUN_ID>.zip.sha256
```

Unit/Integration JUnit 报告由 CTest 写入 `build-win-release/`，归档脚本会将其复制到本轮归档目录。运行证据不提交 Git。每个问题引用发现它的 RUN_ID；修复后再生成新 RUN_ID 作为回归证据。把 ZIP 上传到对应 GitHub Issue；需要 ChatGPT 复核时，也可直接把 ZIP 上传到当前对话。

## 8. 当前工作状态（更新至 2026-11-15；历史 CI 诊断保留原日期）

本轮飞控诊断大屏改造开始时的 main 基线为 `64061a9e49f6d2ddeafcf588e60a4069470f75c6`。该基线的 pre-commit run [43574188042](https://github.com/hujunjie-hub/Flight_GroundControl/actions/runs/43574188042) 已成功；Windows Release run [43574188036](https://github.com/hujunjie-hub/Flight_GroundControl/actions/runs/43574188036) 在本轮开始检查时仍为 IN_PROGRESS；Build Results run [43574834468](https://github.com/hujunjie-hub/Flight_GroundControl/actions/runs/43574834468) SUCCESS。后续 SPEC/产品提交触发的 CI 必须按新 HEAD 重新核对。

- 当前长期分支策略为 **只保留 `main`**。
- FGC-0029 仍为 OPEN / S1 / BLOCKED；首次破坏引用计数的具体操作尚未证实。FGC-0043（EXIF typed-reader 畸形条目边界检查）实现和回归测试已在 main；历史 Windows Release/Unit 精确 HEAD 证据通过且相关源文件未变，但当前 MSVC ASan workflow 之前没有运行 ExifUtilityTest，已补入 20 次 ASan 专项矩阵，等待该 workflow 精确 HEAD 执行后再考虑关闭 Issue #43。
- FGC-0044（飞控诊断四页高级大屏重构）保持 IN_PROGRESS；总览总控、飞行健康、导航时序、遥控与链路四页结构、页面专属 KPI/趋势、原始字段与只读边界均已实现并同步 SPEC/G15。总览包含六域真实 freshness 拓扑、告警严重度排序、同级按地面站本地接收序号排序、FCU_EVENT 启动相对时间、彩色严重度和可换行事件原因；最近事件历史与告警列表的序号、事件 ID、来源 ID、FCU 时间均无省略换行，卡片高度随文本增长。顶部六项 KPI 辅助说明在窄屏最多两行换行，Tile 高度随内容增长；遥控链路 RSSI/质量/丢包格按 3/2/1 列响应式重排，中等宽度丢包格跨整行。健康页完成 13 域矩阵、资源/传感器/预检实例 freshness 门控，以及存储/预检元数据 freshness 和状态颜色语义；导航页完成 capability/UTC/timeQuality freshness 与颜色门控；遥控与链路页完成四面板 3/3/3/3 均衡、逐实例链路有效性和非 RC 数据链路优先级。CI、Windows Release 与本地 1672×941 / 100/125/150% DPI 视觉验收集中后置，未执行前不得标记 VERIFIED。 操作、导航、时间和 RC 动态详情字段统一按各自 family freshness 门控，导航指标先 freshness 后 capability。
- FGC-0045（遥控器双摇杆实时监视优化）已登记，状态 RETESTING；三种手型、两帧 RC_CHANNELS、回中和缺失通道场景均已实现。main@866a8fb 的 FlightRadioPageUITest 暴露出测试预期错误：只有 3 路通道时仍要求开始校准 enabled；已按产品至少 4 路通道门控修正测试，生产逻辑不放宽。
- FGC-0046（设置侧栏一级/二级菜单字体亮度不一致）已登记；修复提交 `15b77df` 已统一两级菜单字体和 palette，并新增 TopLevelViewsTest 回归；Windows 100%/125%/150% 实际屏幕观感检查仍需本地完成。
- FGC-0047（全项目字体体系、中文字体回退与渲染一致性）继续扩展到全仓 QML：旧 default/small/medium/large 字号令牌引用迁移到语义令牌；三维视图固定 pixelSize 改为 pointSize；摇杆遥测等宽字族改用 ScreenTools.fixedFontFamily；新增 body-emphasis、caption-emphasis、section-heading、world-overlay 语义令牌。`qml-typography-check --all` 递归扫描全部 `src/**/*.qml`（当前 459 个文件），覆盖旧字号别名、固定 pixelSize、硬编码字体族和页面局部字号乘数；动态地图罗盘缩放和 DebugWindow 字体预览列为明确例外。新提交的精确 HEAD CI 尚待运行；Windows 100%/125%/150% 实机视觉检查尚未完成，保持 NOT_RUN。
- FGC-0049（旧版 SYS_STATUS 传感器状态断链后仍显示正常）保持 FIXED_WAITING_RETEST / S0-CRITICAL；`9806980` 已实现 5500 ms freshness watchdog、断链立即失效和新 SYS_STATUS 恢复门控；`7bf9424` 补充 stale 行的 last-known 历史上下文、琥珀色状态和抽屉 UI 回归。当前所有新增测试均 NOT_RUN，必须在 exact-HEAD 集中验收通过后才能 VERIFIED/CLOSED。
- Flight UDP SIL 原生输出缺口、传感器实例 ID 契约偏差，以及未执行的网络故障注入/H723 实机项目继续单独登记，不以 MockLink 测试替代。

动态 Issue 状态仍以 [GitHub Issues](https://github.com/hujunjie-hub/Flight_GroundControl/issues) 为唯一事实来源；本节只记录有明确 HEAD 和 run ID 的验收快照。每轮从 `origin/main` 同步最新提交。先处理影响本地构建和验收的故障，再按问题严重级别处理其余缺陷。


### 最新 Windows CI 诊断快照（main@866a8fb，2026-11-14）

- Windows Release run [43598886092](https://github.com/hujunjie-hub/Flight_GroundControl/actions/runs/43598886092)：FAIL。Configure / Build PASS；Unit 198/198 PASS；Integration 69/72，失败为 ToolbarIndicatorUITest、FlightTelemetryPageUITest、FlightRadioPageUITest；Installer、clean-install、installed-runtime 因 Integration 失败而跳过。
- pre-commit run [43598886118](https://github.com/hujunjie-hub/Flight_GroundControl/actions/runs/43598886118)：SUCCESS。
- FlightTelemetryPageUITest：MAVLink float 被转为 double 后使用精确相等比较，已改为容差断言。
- FlightRadioPageUITest：测试把 3 路通道场景误当作允许开始校准，已改为验证 8 路可用、3 路禁用。
- ToolbarIndicatorUITest：APMCopter RemoteID 展开能力未在原 500 ms 内出现；测试改为等待页面 `_showExpand` 能力并输出组件诊断，待新 HEAD 验证是加载时序还是页面能力问题。
- 本次旧 HEAD 的失败证据保留，不沿用为新 HEAD 结果；所有修复必须在新 HEAD 重新运行完整 Windows Release。


## 9. 飞控诊断大屏视觉构建状态

- 设计基准：四张 1672×941 参考图；当前实现位于 `FlightTelemetry.qml`、`FlyViewToolBar.qml`、`VehicleConfigView.qml` 和 `SettingsButton.qml`。
- 2026-11-15 本轮增量：Flight 配置顶栏 56 px、左导航 160 px、连续六项 KPI 带、深青蓝卡片令牌；导航仍使用真实组件，不添加死链。
- 2026-11-15 本轮增量：四个诊断页签改用资源内 SVG 图标和统一 20×20 布局盒，避免 Unicode 字符受字体回退/DPI 影响；G15 增加图标资源与尺寸回归断言。
- 2026-11-15 本轮增量：趋势图采用 gap-aware 面积折线、最新有效点高亮和 64 px 绘图区；主面板圆角统一 8 px、健康矩阵单元 6 px。按用户要求，本轮 UI 迭代不逐次触发 CI，后续集中验收。
- 2026-11-15 本轮增量：四个大屏页签增加 140 ms hover/active 过渡与鼠标状态回归断言，选中态优先于悬停态。
- 2026-11-15 本轮增量：标题锁定区按 1280/760 px 分层，桌面同行显示英文副标题、中等宽度改为第二行、小屏隐藏；UI Integration 覆盖 1600/1000/900/700 px 显示策略与边界。
- 2026-11-15 本轮增量：14 张诊断详情卡按总览/健康/导航/遥控四页拆分为 4/6/2/2 张，动态详情标题同步页签；所有字段与状态门控保留，Integration 逐页验证可见集合。
- 2026-11-15 本轮增量：导航/时间与通信/RC 两组明细卡桌面各占 6/12 列同排，窄屏按 3/6、1/1 响应；Integration 检查等宽与同排布局。
- 2026-11-15 本轮增量：健康页明确区分 13 域消息新鲜度矩阵与设备/字段健康结论；移除多域面板的单域状态徽标，详情卡调整为 4/4/4 与 6/3/3 桌面网格并覆盖中窄屏重排。
- 2026-11-15 本轮增量：总览三张主视觉卡与健康页三张主面板启用显式同排等高，其他 HeroPanel 不受影响；Integration 增加高度对齐断言。
- 2026-11-15 飞行健康页专项：中屏主面板调整为概览/矩阵半行同排、安全监测整行；健康详情网格在 760–1279px 独立采用 6 列，与其他页面的断点解耦；预检恢复原因完整换行。资源实例 CPU/WCET/栈/AXI 数值按实例 receiveAgeMs 门控，安全摘要执行器掩码按 actuator 域 freshness 门控。
- 2026-11-15 总览总控页专项：六域状态拓扑与接收年龄实时联动；右侧事件按严重度与序号排序；3:5:4 主面板和 3/2/3/2 节点网格响应式适配；保留原始遥测和只读边界。 本轮补充六档宽度节点边界断言，以及导航节点 fresh/stale/recovered 颜色状态转换断言。 整机多域复合面板不再显示 operation 单域徽标。 告警优先级列表与事件历史同步显示 FCU 启动相对时间，明确区别于 UTC。 最近事件历史采用严重度颜色与原因换行，窄屏不再单行截断故障原因。 告警与历史序号明确标注为地面站本地接收序号，避免误读为 FCU 原生序号。
- 视觉验收仍须使用精确 main HEAD 的 Windows Release 二进制，完成 1672×941 截图叠加和 100/125/150% DPI 检查。

- 2026-11-15 本轮增量：遥控与链路页四张主视觉面板调整为桌面 3/3/3/3 等宽等高；RC 轴读数保留 freshness/validity 状态；链路 RSSI、质量、丢包改为逐实例独立指标格，趋势优先 GCS/伴随计算机/其他数据链路并仅在无非 RC 实例时回退 RC；非 RC 链路超时后不以 RC 值伪装数据链路；MockLink Integration 增加 RC/GCS/其他实例与无效字段覆盖。本轮按用户要求跳过 CI，后续集中验收。

- 2026-11-15 飞行健康页逐实例新鲜度：传感器实例过期时明确显示最近状态/原因/动作；FCU_PREARM_STATUS 每个 checkId 增加本地 receiveAgeMs，QML 500 ms tick 刷新，过期结果不再显示为当前通过/失败；覆盖率只统计新鲜实例/检查项。

- 2026-11-15 飞行健康页继续收口：控制器与执行器详情缓存 Fact 全部受各自 family freshness 门控；安全监测控制求解器摘要与详情一致。

- 2026-11-15 本轮增量：诊断大屏 KPI 辅助说明改为遵守 family freshness、实例 receiveAgeMs 和字段 validity。传感器覆盖率、资源 task 实例覆盖率、执行器掩码、UTC 有效性、RC 接收机 failsafe 和链路实例覆盖率均补充状态门控及 MockLink Integration 断言。本轮未运行 CI，待集中验收。

- 2026-11-15 本轮增量：时间质量 KPI 新增 timeQuality 枚举分色，区分仅本地时钟、同步中、锁定、保持、降级和无效；UTC 不确定度仍独立按 utcValid 分色，time family 超时优先于缓存枚举。本轮新增 MockLink 组合状态断言，未运行 CI。

- 2026-11-15 本轮增量：总览数据链路 KPI 从缓存链路总数调整为新鲜实例数/总数，辅助说明显示新鲜/超时/等待，颜色聚合实例状态；总览与遥控链路专页共享同一聚合颜色规则，并新增 3/3 → 1/3 的 MockLink 回归断言。本轮未运行 CI。


- 2026-11-15 诊断大屏链路 KPI 双页收尾：遥控与链路页的「链路实例」主值同步改为新鲜实例数/总数，与总览页使用同一 helper、辅助说明和聚合颜色；MockLink 验证两页在 3/3 → 1/3 时保持一致。本轮未运行 CI，待集中验证。

- 2026-11-15 本轮增量：顶部 KPI 辅助说明在窄屏使用两行换行并按实际内容自适应 Tile 高度，补充 700 px 双列与 400 px 单列 Integration 断言；本轮未运行 CI。


- 2026-11-15 FGC-0049 收尾增量：stale 传感器行保留最近一次 Normal/Error/Disabled 作为历史上下文，仍以 Stale 前缀和琥珀色明确标识过期；新增多传感器排序和 ToolbarIndicator 抽屉颜色回归。本轮未运行 CI，待集中验证。

- 2026-11-15 本轮增量：遥控链路每实例 RSSI/质量/丢包指标格新增 3/2/1 列自适应，中等宽度丢包率格跨两列；补充 700/400 px MockLink UI Integration 宽度与边界断言。本轮未运行 CI。


- 2026-11-15 RC 窄屏验收补强：FlightTelemetryPageUITest 在 1000/900/700/400 px 下检查遥控输入、逻辑通道、链路质量、失效保护四面板的垂直顺序、等宽与边界；左右摇杆需完整位于输入面板内并保持并排。新增断言 NOT_RUN，待集中验收。

- 2026-11-15 本轮增量：事件历史序号/启动时间/严重程度元信息按 520 px 断点由三列改为两列，窄屏严重程度换至下一行；增加 700/400 px Integration 断言，取消序号和时间的固定宽度省略。本轮未运行 CI。

- 2026-11-15 飞行健康页预检摘要收口：部分上报明确显示 15 项覆盖率与待更新数量；family stale 时只显示最近缓存项数量；仅 15/15 新鲜通过为绿色，补充部分覆盖、过期、全通过和警告状态的 MockLink Integration 断言。本轮未运行 CI，待集中验收。


- 2026-11-15 大屏 SPEC 章节顺序复核：事件历史响应式布局与预检完整覆盖率分别编号为第 50/51 章，所有章节编号唯一且按文档顺序递增。

- 2026-11-15 本轮增量：总览告警列表元信息/FCU 时间取消省略并改为 WordWrap，增加 700/400 px 卡片高度与边界断言。本轮未运行 CI。

- 2026-11-15 飞行健康页资源实例计数 freshness 收口：资源卡任务实例数量现在跟随 resourceAvailable family 状态，超时不再显示缓存计数；增加新鲜/过期 MockLink Integration 断言。本轮未运行 CI，待集中验收。


- 2026-11-15 诊断大屏 KPI 回归加固：六项辅助说明逐项检查 WordWrap、两行上限、实际行数和 Tile 内边界；额外注入长链路说明强制验证双行与 implicitHeight 自适应。本轮未运行测试，待集中验收。

- 2026-11-15 飞行健康页资源覆盖表达升级：资源卡改为显示新鲜/超时/等待实例数量，与传感器卡统一；resourceAvailable family stale 时显示 family 状态，不展示缓存总数。本轮未运行 CI，待集中验收。

- 2026-11-15 本轮增量：操作/导航/时间/RC 详情字段统一按 family freshness 门控；导航指标先 freshness 后 capability；补充 MockLink fresh/stale/recovered 与 RC 5500 ms stale 断言。本轮未运行 CI。

- 2026-11-15 通信链路详情卡统一三态实例覆盖：显示新鲜/超时/等待数量；链路指标按实例 freshness 与 validityMask 门控，超时显示状态并使用琥珀色，补充 MockLink 新鲜/过期回归。本轮未运行 CI，待集中验收。
