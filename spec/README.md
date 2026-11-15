# Flight_GroundControl 项目规范入口

- 版本 5.00
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
18. [Flight 存储与云台配置页规范](Flight-存储与云台功能设计规范.md)：存储 Profile/原因码/FCU 运行时间与遥测新鲜度、云台完整控制权配对校验、轴 capability 门控、SET_ATTITUDE 角速度控制、yaw-lock/retract 回读、MockLink 三设备协议寻址、独立控制与位置命令隔离及设备离线重发现、广播 interval 与参数边界验收（v2.1）。

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
- FGC-0044（飞控诊断四页高级大屏重构）保持 IN_PROGRESS；总览总控、飞行健康、导航时序、遥控与链路四页结构、页面专属 KPI/趋势、原始字段与只读边界均已实现并同步 SPEC/G15。总览包含六域真实 freshness 拓扑（明确显示已接收/超时/等待三态图例），并以同一 family freshness 源显示已接收/超时/等待六域实时数量统计，不合成整机健康分、顶部三项导航标准差 KPI 有效数值中性显示且 freshness/capability 状态独立着色、告警严重度排序、同级按地面站本地接收序号排序、空态等待/首条事件到达/实时优先级重排均有 MockLink 覆盖、优先摘要 4 条与事件历史最近 8 条展示范围动态说明及 5 事件不丢失回归、当前缓存严重/错误/警告/信息/未知五类严重度数量实时统计并受 event family freshness 门控，告警 chip 按实际 HeroPanel 宽度在 3/5 列间响应式重排、FCU_EVENT 启动相对时间、彩色严重度和可换行事件原因；最近事件历史与告警列表的序号、事件 ID、来源 ID、FCU 时间均无省略换行，卡片高度随文本增长。顶部六项 KPI 辅助说明在窄屏最多两行换行，Tile 高度随内容增长；遥控链路 RSSI/质量/丢包格按 3/2/1 列响应式重排，中等宽度丢包格跨整行。健康页完成 13 域矩阵、资源/传感器/预检实例 freshness 门控，以及存储/预检元数据 freshness 和状态颜色语义；导航页完成 capability/UTC/timeQuality freshness 与颜色门控，并补齐 LocalTimebaseDisciplineState 独立分色及 FCU 时间戳有效性语义；四张主视觉按 4/8 + 8/4 桌面比例重排，导航估计不确定度数值与能力状态分色，能力/时间元数据下沉详情卡并去除重复可见字段，时间同步主状态按 2/1 列响应式重排，UTC 不确定度在双列布局下横跨整行并验证五项指标不越界；FCU 单调时间线按 time_boot_us 动态排序、显示微秒精度与顺序编号，时间值横向对齐并完成中窄屏防裁切回归；四页 HeroPanel/StatusCard 统一标题字体、颜色、状态徽标、圆角、内边距和区块间距，并增加跨组件视觉断言；四页顶层网格与详情网格统一使用 ScreenTools 缩放间距，覆盖 1600/1000/900/700/400 px 回归；1672×941 启用垂直紧凑密度（根间距 8 px、页签 42 px、卡片内边距收敛），1100 px 恢复标准密度；保留全部数据与 64 px 趋势绘图区；遥控与链路页采用桌面两行 6/6 + 8/4、中屏 3/3 + 4/2、700/400 px 单列布局；双摇杆、逻辑轴、逐实例链路指标和失控保护各司其职；Mode 1/2/3 双摇杆与逻辑轴共享 freshness、validity mask 和归一化范围门控，超范围值不绘制 marker，恢复有效输入后同步恢复。链路主视觉只呈现 RSSI/质量/丢包与 FCU 数据年龄，通信详情呈现状态/原因/指标有效性及地面站接收年龄；family、实例、FCU 三种年龄分别标注；多实例链路不使用单域聚合徽标；链路质量主视觉汇总新鲜实例的健康/降级/丢失/恢复中状态，覆盖部分断链、全部断链及恢复过程，并与超时/等待 freshness 计数分离；链路状态严重度红/琥珀/绿与消息/实例 freshness 的颜色独立，全部实例丢失但消息仍新鲜时不误报 freshness 超时；宽屏/中屏断言链路质量面板宽度约为失控保护面板 2 倍；RC family 状态徽标只在遥控输入监视主面板显示；逻辑轴面板与下方遥控输入详情卡隐藏重复徽标，但保留 family freshness 色条；失控保护面板不重复展示 linkAvailable 消息状态，通信详情卡统一显示消息接收状态；接收机失效保护触发/未触发与 RC family freshness 分别采用红/绿/琥珀/中性灰颜色；RC 页摘要明确区分「链路消息」family freshness 与「链路实例」逐实例覆盖率。CI、Windows Release 与本地 1672×941 / 100/125/150% DPI 视觉验收集中后置，未执行前不得标记 VERIFIED。 操作、导航、时间和 RC 动态详情字段统一按各自 family freshness 门控，导航指标先 freshness 后 capability。 导航页能力/时间摘要面板及健康维护启动 ID 同步服从 family freshness，消除同页重复数据 stale 不一致。 总览中央拓扑六域端点增加中文标签和指向 FCU 的遥测箭头；节点身份、连接 family 与下方 freshness 卡一一对应，Canvas 标签在 1600/1400/1000/900/700/400 px 断点验证不越界、不重叠；方向仅表示遥测上报，不表示控制命令或空间姿态。 1672×941 紧凑垂直模式增加三张主视觉卡片同排、等高、3/4/5 宽度层级及 heroGrid 边界断言，确保垂直压缩不改变横向信息架构。 1672×941 紧凑垂直模式还遍历总览/健康/导航时序/遥控链路四页，逐页断言主视觉网格与详情网格保持 12 列、趋势网格保持 4 列、共享间距一致且趋势绘图区不少于 64 px。
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

- 2026-11-15 本轮增量：遥控与链路页桌面采用两行 6/6 + 8/4、中屏 3/3 + 4/2、窄屏单列；双摇杆和逻辑轴按有效 RC 输入实时驱动；链路 RSSI/质量/丢包逐实例显示，趋势优先 GCS/伴随计算机/其他数据链路且非 RC 实例均超时不回退 RC；主视觉与详情分离指标数值、原因和诊断上下文；FCU 数据年龄、family 消息接收年龄、实例接收年龄分别标注；多实例链路面板不使用 family 聚合徽标；RC 轴读数等待/有效/无效状态分别使用灰/白/琥珀色，摇杆组合读数按双轴状态使用灰/绿/琥珀色；RC family 状态徽标只在遥控输入监视主面板显示，逻辑轴面板与下方遥控详情卡隐藏重复徽标并保留 freshness 色条；失控保护面板不重复显示 linkAvailable family 消息状态，通信详情卡独占该消息状态；MockLink Integration 在 1600/1000/900/700/400 px 增加 badge 单点显示与状态色条保留断言，并覆盖字段有效性、无效色、age freshness、链路/失控保护面板 2:1 宽度比例和 400 px 双摇杆边界断言。本轮按用户要求跳过 CI，后续集中验收。

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

- 2026-11-15 本轮增量：导航页重复能力/时间摘要面板和健康维护启动 ID 补齐 family freshness；新增 MockLink 等待/接收/超时/恢复断言。本轮未运行 CI。


- 2026-11-15 导航时序页增量：第四主面板改为 FCU 单调时间线，按导航/时间/最近事件的真实 time_boot_us 排序；导航与时间标记服从 family freshness，UTC 无效不影响 FCU 单调时间。删除重复的能力/UTC 校验行，原字段仍保留在对应专属面板。新增 MockLink 排序、UTC 独立性、fresh/stale/recovered 和 1600/1000/900/700/400 px 边界断言；本轮未运行 CI，新增回归为 NOT_RUN。

- 2026-11-15 导航时序页第二轮语义收口：LocalTimebaseDisciplineState 六态独立分色，未知/缺失枚举保持中性灰，family waiting/stale 优先覆盖；FCU 时间线无效/越过 JS 安全整数范围的时间戳改为红色并置于有效点之后，缺失/未上报中性灰。同步修正导航页面规范中的过时 3/3/3/3 布局描述。新增 MockLink 六态与 2^53 边界断言；本轮按要求不运行 CI/Unit/Integration，新增回归 NOT_RUN。

- 2026-11-15 导航时序页第三轮收口：相同 FCU time_boot_us 固定按 navigation → time → event 排序，不依赖 JS sort 稳定性；新增 MockLink 同时间戳回归。按要求不运行 CI/Unit/Integration，新增回归 NOT_RUN。

- 2026-11-15 固件构建快照采用「最近已知」显示；Profile 匹配要求 operation fresh，并使用当前启动周期已接收的 BUILD_INFO 快照；构建快照 age stale 不会单独使静态 Profile 失效，新 boot epoch 清空快照。增加 fresh/stale/recovery/new-boot MockLink 断言。本轮未运行 CI，待集中验收。

- 2026-11-15 总览总控页通信状态收口：顶部「FCU 通信状态」KPI 与只读状态徽标读取 VehicleLinkManager 全链路 communicationLost；通信丢失红色提示，恢复后恢复已收到状态；各遥测域保持独立 freshness。新增 MockLink Integration 和 G15 manifest 验收项。本轮不运行 CI，新增回归为 NOT_RUN，待集中验收。

- 2026-11-15 诊断大屏设计 SPEC 章节编号复核：修复导航时间线、构建快照、遥控链路、总览通信状态和导航时间线精修章节的重复/跳号，确保章节 1–78 连续唯一；不涉及产品代码或测试行为，本轮不运行 CI。

- 2026-11-15 总览拓扑视觉层级收口：六条连接线与六个节点共用各自 family freshness 状态，fresh/stale/waiting 分别使用绿/琥珀/灰；节点卡增加状态色底纹与侧边标识；拓扑作为六域复合面板移除 navigation 单域状态徽标。新增 MockLink fresh→stale→recovered 连线/节点同步断言与 1600–400 px 边界回归。本轮未运行 CI，新增断言 NOT_RUN。

- 2026-11-15 趋势卡状态表达增量：每张趋势卡新增独立「断点 N」计数，历史断点不因最新样本恢复而清除；WAIT/NOW/GAP 单独表达最新样本状态。新增 MockLink 过期→恢复与 1600/400 px footer 边界断言。本轮未运行 CI，新增断言 NOT_RUN。

- 2026-11-15 总览整机状态优先级调整：飞行模式作为主视觉，解锁状态独立显示，失效保护升级为全宽分级安全状态条，控制权下沉为次级行；freshness 与业务状态分开表达。新增 MockLink WAIT/fresh/stale 与 1600/1000/400 px 内容边界断言。本轮未运行 CI，新增回归 NOT_RUN。

- 2026-11-15 总览告警区层级优化：最高严重度事件提升为主告警卡，原因成为主标题，严重度/本地序号/FCU ID/来源/FCU 启动时间分组显示；补充事件总数、明确空态与未知严重度标识。新增 MockLink 首要事件和 700/400 px 边界断言。本轮未运行 CI，新增回归 NOT_RUN。

- 2026-11-15 总览三栏协调性收口：桌面 3/5/4、中屏 2/2/2 列宽保持不变，新增桌面/中屏三张 HeroPanel 等高断言；低于 900 px 仍自然单列堆叠。本轮未运行 CI，新增回归 NOT_RUN。

- 2026-11-15 总览顶部 KPI 与三栏信息去重：顶部仅显示水平位置 σ/速度 σ/航向 σ，导航数值从中栏移至 KPI；中栏只保留六域拓扑 freshness，左栏聚焦飞行模式/解锁/失效保护/控制权，右栏聚焦告警。飞行健康/导航时序/遥控链路页原有六项摘要卡不变。新增 1600/1400/1000/900/700/400 px KPI 边界和页面切换断言。本轮未运行 CI，新增回归 NOT_RUN。

- 2026-11-15 飞行健康页全链路通信丢失提示：健康概览在 communicationLost 时立即红色提示，并明确 13 域矩阵仍按各自接收年龄独立判断；MockLink 恢复路径断言已登记，未运行 CI，新增回归 NOT_RUN。

- 2026-11-15 飞行健康页预检覆盖与资源快照分层：维护横条仅显示预检覆盖数，安全监测独立显示通过/警告/失败/未知结论；新增 CPU/WCET/系统栈/存储快照及字段有效性/响应式边界回归，未运行 CI，新增回归 NOT_RUN。

- 2026-11-15 导航时序页响应式断点补强：新增 1280/1279/760/759 px 精确边界断言，验证 4/8 + 8/4 桌面布局、6 列中屏和单列窄屏切换；本轮不运行 CI/Unit/Integration，新增回归 NOT_RUN。

- 2026-11-15 飞行健康页视觉层级收口：传感器实例卡片化、预检元信息双列分组与逐项检查卡片化、任务资源 CPU/WCET/栈/截止期/AXI 分格；新增 validity 与响应式边界回归，未运行 CI，新增回归 NOT_RUN。

- 2026-11-15 导航/趋势会话边界补强：新 FCU boot epoch 首包允许来自导航等非操作域；operationAvailable/opBootIdHex 被 FactGroup 清空后，趋势采样器在下一次 1 Hz 采样时清除旧会话样本并保持 GAP，直至新操作状态建立会话 ID。新增 MockLink NAV-first 回归和导航/时间/事件旧时间戳门控断言；本轮不运行 CI/Unit/Integration，新增回归 NOT_RUN。

- 2026-11-15 Phase 2.53 飞行健康页高 DPI 与窄屏细节优化：传感器实例头部按 760 px 响应式重排，资源指标卡片抽取为 ScreenTools 缩放组件，预检卡片间距/字号随字体度量；增加1600/1000/900/760/759/700/400 px 内容边界回归，未运行 CI，新增回归 NOT_RUN。

- 2026-11-15 Phase 2.56 FCU 连续重启隔离补强：退役 Boot ID 缓存由 8 提升为有界 64；新增 12 个连续 NAV-first Boot Epoch 回归，验证所有 11 个退役 ID 即使携带更大 FCU 时间戳也被拒绝，并确认当前会话继续更新。事件域与 BUILD_INFO 域首包切换沿用既有专项测试；本轮不运行 CI/Unit/Integration，集中验收。

- 2026-11-15 飞行健康页详情卡片内容高度收口：传感器实例、逐项预检、任务资源卡片均按 ScreenTools 缩放内边距 + 内容 implicitHeight 计算外层高度；MockLink UI Integration 增加内容容器实际高度/上下内边距断言，覆盖 1600/1000/900/760/759/700/400 px。未运行 CI，新增回归 NOT_RUN。


- 2026-11-15 遥控与链路页继续适配：双摇杆卡新增实时横向/纵向 CH 映射标签，随 FlightRadio Mode 1/2/3 同步；摇杆卡高度按新增行自适应。链路质量指标新增严格 freshness/validity/0–1000 permille 门控的比例条，70% 实值显示 70% 填充，超范围/null/空值显示无效/琥珀且隐藏填充，趋势卡值与采样同步使用同一范围门控，恢复有效值后同步恢复。MockLink UI Integration 增加三种手型标签、质量条和趋势值比例/无效/恢复断言。本轮未运行 CI，新增回归 NOT_RUN。


- 2026-11-15 Phase 2.59 导航时序页高 DPI 收口：FCU 单调时间线行高/行距/编号/轨道/时间值列宽改由 ScreenTools 字体度量驱动；补充 1672×941 紧凑模式及 1600/1000/900/700/400 px 的行几何、时间戳完整性和面板边界回归。未运行 CI/Unit/Integration/Windows Release，新增回归 NOT_RUN，待集中验收。


- 2026-11-15 总览拓扑文字高 DPI 统一：Canvas 中心 FCU 与六域端点标签改为 `ScreenTools.normalFontFamily` + `microFontPointSize`（point），移除硬编码 sans-serif/px；MockLink UI Integration 使用相同字体族和点字号计算 QFontMetricsF，覆盖 1600/1400/1000/900/700/400 px 越界与重叠检查。新增静态字体契约；按要求未运行 CI/Unit/Integration，新增回归 NOT_RUN。


- 2026-11-15 飞行健康页 Phase 2.61：健康矩阵单元与维护横条改用 ScreenTools 缩放内边距和内容驱动高度；补充长中文异常摘要换行、维护三/二/一列边界，以及 1672×941/1100 垂直密度切换的 MockLink UI Integration 验收。本轮未运行 CI/Unit/Integration/Windows Release，新增回归 NOT_RUN。


- 2026-11-15 导航时序页 UTC 时间戳全宽适配：FCU UTC 时间独立成堆叠全宽行，固定宽度字体、深色底框、自然换行，保留六位微秒和 UTC 标识；双列布局下横跨两列，UTC 不确定度与 UTC 有效性并排。新增 1672×941 紧凑大屏四卡比例/等高/边界与 1600/1000/900/700/400 px 内容边界回归，未运行 CI/Unit/Integration，新增回归 NOT_RUN。


- 2026-11-15 导航时序页辅助网格高 DPI 收口：能力、时间同步、导航/时间域状态子网格间距统一由 ScreenTools 字体像素宽高驱动，MetricReadout 内部间距随 UI Scale 缩放，标签/状态文本支持窄宽度换行；MockLink UI Integration 新增 1280/1279/760/759 px spacing 与换行断言。本轮未运行 CI/Unit/Integration/Windows Release，新增回归 NOT_RUN，待集中验收。


- 2026-11-15 总览拓扑矢量几何与节点卡片高 DPI 适配：Canvas 中心、四旋翼线框、六域节点、箭头和标签偏移统一使用 ScreenTools 密度比例；画布高度随字体像素高度增长。六域节点卡片的网格间距、内边距、指示点、内容行距和内容驱动高度统一使用 ScreenTools 字体像素度量；MockLink UI Integration 扩展六断点父子边界和几何缩放断言。按要求未运行 CI，新增回归 NOT_RUN。


- 2026-11-15 导航指标层级收口：三项导航估计标准差继续以中性数值为主，状态改用 FamilyStatusBadge；四项导航能力状态由大号文字改为紧凑状态徽标，颜色/文案复用原 capability 与 freshness 门控。新增 1280/1279/760/759 px 及 1600/1672×941 徽标完整边界回归，未运行 CI/Unit/Integration，新增回归 NOT_RUN。


- 2026-11-15 Phase 2.65 左侧整机状态概览高 DPI 适配：飞行模式/解锁布局间距、解锁胶囊、失效保护状态条和控制权只读行改用 ScreenTools 字体像素度量；两条状态行按内容隐式高度加双侧内边距自适应，说明文字自然换行。MockLink UI Integration 扩展六档宽度父子边界与内容高度断言。按要求未运行 CI，新增回归 NOT_RUN。


- 2026-11-15 飞行健康页 Phase 2.66：通过 AppSettings.uiScalePercent 实际切换 150%/200% UI Scale，覆盖 1672×941 与 1280×720 物理视口折算后的双列/单列布局、13 域健康矩阵内容边界、维护横条动态高度和长中文摘要换行；测试恢复原始缩放设置。本轮未运行 CI/Build/Unit/Integration/Windows Release，新增回归 NOT_RUN。


- 2026-11-15 遥控与链路页高 DPI 收口：手型切换按钮、双摇杆舞台/标记/通道映射、逻辑轴刻度条和多链路 RSSI/质量/丢包卡片统一使用 ScreenTools 缩放 token；摇杆和链路指标卡片改为内容驱动高度，避免高 DPI/窄屏裁切。MockLink UI Integration 新增 1000/900/760/759/700/400 px spacing 与父子内容边界断言。按要求未运行 CI，新增回归 NOT_RUN。
