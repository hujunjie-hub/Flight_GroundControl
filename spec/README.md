# Flight_GroundControl 项目规范入口

- 版本 4.0
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
- FGC-0044（飞控诊断四页高级大屏重构）保持 RETESTING；四页结构、页面专属 KPI/趋势、健康矩阵 4/2/1/1 与维护横条 3/2/1/1 已同步 SPEC/G15。Integration 覆盖 13 个矩阵单元边界、四个断点、Mode 2 双摇杆值与逻辑轴一致，以及 roll validity bit 清除后对应 marker 隐藏。待 exact-HEAD Windows Release、Unit、Integration、Installer、clean-install、installed-runtime 和本地 1672×941 / 100/125/150% DPI 视觉验收；当前不得标记 VERIFIED。
- FGC-0045（遥控器双摇杆实时监视优化）已登记，状态 RETESTING；三种手型、两帧 RC_CHANNELS、回中和缺失通道场景均已实现。main@866a8fb 的 FlightRadioPageUITest 暴露出测试预期错误：只有 3 路通道时仍要求开始校准 enabled；已按产品至少 4 路通道门控修正测试，生产逻辑不放宽。
- FGC-0046（设置侧栏一级/二级菜单字体亮度不一致）已登记；修复提交 `15b77df` 已统一两级菜单字体和 palette，并新增 TopLevelViewsTest 回归；Windows 100%/125%/150% 实际屏幕观感检查仍需本地完成。
- FGC-0047（全项目字体体系、中文字体回退与渲染一致性）继续扩展到全仓 QML：旧 default/small/medium/large 字号令牌引用迁移到语义令牌；三维视图固定 pixelSize 改为 pointSize；摇杆遥测等宽字族改用 ScreenTools.fixedFontFamily；新增 body-emphasis、caption-emphasis、section-heading、world-overlay 语义令牌。`qml-typography-check --all` 递归扫描全部 `src/**/*.qml`（当前 459 个文件），覆盖旧字号别名、固定 pixelSize、硬编码字体族和页面局部字号乘数；动态地图罗盘缩放和 DebugWindow 字体预览列为明确例外。新提交的精确 HEAD CI 尚待运行；Windows 100%/125%/150% 实机视觉检查尚未完成，保持 NOT_RUN。
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
- 视觉验收仍须使用精确 main HEAD 的 Windows Release 二进制，完成 1672×941 截图叠加和 100/125/150% DPI 检查。
