# Windows Release 验收与交付指南

版本：4.88
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
| G15 | 飞控诊断指挥中心（Phase 2.4 四参考页） | FlightTelemetryPageUITest、JUnit、G15 manifest gate | Flight 配置顶栏 56 px、左导航 160 px、连续六项 KPI 带容器与网格边界通过；四个页签 SVG 资源路径和 20×20 图标尺寸断言通过；四个页签实际切换独立内容；每个页签 MouseArea hoverEnabled，未选中 hover/idle 颜色和 active 颜色断言通过；标题区英文副标题在 1600/1000/900/700 px 下按桌面/中屏/小屏策略显示且可见元素不越界；四页明细卡可见集合严格为总览 4、健康 6、导航 2、遥控 2，14 张无遗漏/重复；导航/时间、通信/RC 两组卡在桌面同排且各占网格至少 45% 宽度；健康矩阵只显示 13 域消息新鲜度，三种状态图例与明细解释准确；HealthCell 最小高度随 ScreenTools 字体度量缩放、实际高度按内容 implicitHeight + 两倍面板内边距计算，13 个矩阵单元和重点域单元内部内容不得裁切；维护横条高度按内容 implicitHeight + 两倍 ScreenTools 面板内边距计算，启动 ID 可任意位置换行，长中文异常摘要触发自然增高；健康矩阵/维护横条内容边界在 1672×941 紧凑与 1100 px 标准垂直模式及 1600/1000/900/760/759/700/400 px 宽度通过；AppSettings.uiScalePercent=150 时以 1672×941 物理视口折算为约 1115×627 逻辑尺寸，健康矩阵和维护横条双列且 13 个单元内容完整；uiScalePercent=200 时以 1280×720 物理视口折算为 640×360 逻辑尺寸，矩阵/维护横条单列、长中文异常摘要自然增高且无裁切；测试结束恢复原始 UI Scale；传感器实例/预检/任务资源详情卡与 CPU/WCET/栈/存储资源指标在 150%/200% 下均满足内容高度和父子边界；健康详情卡桌面 4/4/4、6/3/3，1000px 和窄屏重排；飞行健康页 760–1279px 顶部 3/3/6、详情网格 6 列；<760px 单列；预检恢复原因完整换行；资源实例 CPU/WCET/栈/AXI 仅在实例新鲜时显示数值，执行器安全摘要掩码跟随 actuator 域 freshness；总览与健康页主视觉三卡在桌面同排等高，其他卡片不被强制拉伸；四张趋势卡均存在 64 px 绘图区、较早/最新时间轴和 gap-count 状态，趋势卡/详细卡圆角 8 px、健康矩阵单元圆角 6 px；HeroPanel/StatusCard 共用标题/眉题字体颜色、24 px 基准高度且随 ScreenTools 字体度量缩放的状态徽标、8 px 圆角，以及使用 ScreenTools.defaultFontPixelHeight/Width 缩放的内边距/区块间距；跨组件 UI 断言验证字体 point size/weight/color、徽标尺寸/颜色、卡片背景与间距；总览/健康/导航时序/遥控链路四页存在；健康页 13 类矩阵 4/2/1/1 列、13 个单元边界无溢出，维护横条 3/2/1/1 列，且无综合健康分，底部 CPU/WCET/栈/存储趋势使用真实资源实例并校验实例年龄与 validity mask；导航页 capability bit2/3/5+6 与 UTC 有效性门控；RC 双摇杆沿用 FlightRadio Mode1/2/3 映射，Mode 2 marker 位置与四轴读数一致，mask 清除时对应 marker 隐藏；链路 RSSI/质量/丢包同时满足 family freshness、实例 receiveAgeMs 和 validity mask；趋势 1 Hz/60 点/断点/Vehicle 与 boot reset；不虚构卫星/HDOP/CH5–CH8/SNR/RTT；原 14 张卡片保留；总览拓扑六域节点绑定真实 freshness 和接收年龄；告警按严重度降序、同级按地面站本地接收序号降序；总览三栏宽度顺序、边界与拓扑节点 3/2/3/2 响应式断言通过；六个节点在 1600/1400/1000/900/700/400 px 均位于网格边界内，导航节点颜色通过 fresh→stale→recovered 转换断言；拓扑 Canvas 的 FCU 与六域标签必须继承 ScreenTools.normalFontFamily/microFontPointSize（point），UI Integration 使用相同字体属性进行 QFontMetricsF 越界/重叠检查；拓扑矢量几何和画布高度必须使用 ScreenTools.defaultFontPixelHeight 密度比例；六域节点卡片网格间距、内边距、指示点与内容高度必须使用 ScreenTools 字体像素度量，UI Integration 验证六断点父子边界和内容隐式高度；左侧整机失效保护状态条与控制权行必须按 ScreenTools 内容高度自适应，解锁胶囊按文本 implicit size 加双侧缩放内边距计算；UI Integration 在 1600/1400/1000/900/700/400 px 验证模式行、徽标、状态条、控制权内容和只读说明均不越界；整机多域复合面板不显示 operation 单域状态徽标；告警优先级列表与事件历史显示 FCU_EVENT.time_boot_us 启动相对时间且不标作 UTC；有效/未上报/越界时间输入均有 UI Integration 断言；事件历史严重度颜色与优先级列表一致，事件 ID/来源/原因换行完整显示；只读 ；传感器实例与预检 checkId 独立 receiveAgeMs；过期结果显示最近状态/最近结果，不得冒充当前健康/通过/失败；有效、超时、等待/未收到覆盖率分别统计；MockLink 注入 FCU_SENSOR_HEALTH 与 FCU_PREARM_STATUS 验证新鲜/过期文案和年龄刷新；| 操作/导航/时间/RC 详情按 family freshness 隐藏缓存值，导航数值先 freshness 后 capability；验证 operation/navigation/time fresh→stale→recovered、RC 5500 ms stale 与状态色； 导航页重复能力/时间摘要面板与健康维护启动 ID 必须按 family freshness 隐藏缓存值；验证 waiting/fresh/stale/recovered。；总览页 FCU 通信状态以 VehicleLinkManager.communicationLost 覆盖 latched telemetryAvailable；MockLink setCommLost true/false 验证 KPI/页头丢失与恢复文案及红/绿颜色。；拓扑六条连线与六节点按各自 family freshness 独立着色，节点卡状态文字/年龄同步；六域连接 fresh→stale→recovered 断言通过；多域拓扑面板 statusFamily 为空，不显示 navigation 单域徽标。；四张趋势卡分别显示断点计数与最新样本 WAIT/NOW/GAP；断点数随无效样本增长，最新样本恢复后历史断点保留；1600/400 px footer 标签边界通过。；左侧整机概览飞行模式为主视觉、解锁状态独立标识、失效保护等级为全宽分级安全状态条、控制权为次级行；operation stale 时不得显示缓存业务状态；1600/1000/400 px 安全状态内容边界通过。；右侧告警按严重度突出首项，首项原因主标题/严重度徽标/最高优先级标识清晰；事件数和无事件空态正确；来源/本地序号/FCU 启动相对时间分组，未知严重度不伪装为最高级；700/400 px 内容边界通过。；右侧告警 chip、空态和首要/后续事件卡必须使用 ScreenTools 缩放内边距、徽标/色条尺寸与内容驱动高度；事件历史行距和元信息网格间距使用 ScreenTools 字体像素度量；UI Integration 在 1600/1400/1000/900/700/400 px 验证 chip/卡片/历史行完整边界和文本无省略；总览三栏桌面 3/5/4、中屏 2/2/2 列宽顺序稳定，1600/1400/1000 px 三张主视觉面板等高且边界通过；900 px 以下自然单列，不强制等高。；总览、健康、导航时序、遥控链路四个顶层页面网格与详情网格均使用同一 ScreenTools 缩放行/列间距，并在 1600/1000/900/700/400 px 验证间距一致。；总览顶部仅显示水平位置σ/速度σ/航向σ三项导航 KPI，1600/1400/1000/900/700/400 px 边界通过；中栏拓扑不重复显示导航标准差，左栏不再重复六域 HealthCell，页面切换时仅详情页显示原六项 SummaryTile；1672×941（availableHeight < 960）垂直紧凑模式根间距 8 px、页签 42 px、共享卡片内边距/区块间距收敛；1100 px 高度恢复标准 12 px/46 px；趋势绘图区始终不少于 64 px，中文详情标题、全部遥测字段、14 张详情卡与四张趋势卡完整保留；四页主视觉与详情网格均在 1672×941 紧凑态完成断点/边界回归。
- 飞行健康页专项断点验收：1600 px 顶部 3/6/3、详情 12 列；1000/900 px 顶部 3/3/6、详情 6 列；700/400 px 主面板和详情单列；13 域矩阵边界无溢出；预检恢复原因不得省略。
| G16 | 设置侧栏字体与状态颜色一致性 | TopLevelViewsTest、JUnit、QML font/palette properties | 一级/二级菜单字体族、point size、字重一致；普通和选中颜色遵循相同 QGCPalette 语义；二级按钮按自身 enabled 状态选择 palette；页面导航和分区交互不回归 |
| G17 | 全项目字体系统与渲染一致性 | qml-typography-check --all、TopLevelViewsTest、JUnit、Windows DPI screenshots | 递归检查全部 src/**/*.qml；除 ScreenTools 兼容别名外无旧字号令牌；无固定 font.pixelSize、硬编码字体族和未登记的局部字号乘数；QGCLabel/共享控件统一字体族、Hinting 与抗锯齿；RadioButton 继承正确字体族；中文界面按平台选择已安装 CJK 字体；100%/125%/150% DPI 视觉证据齐全 |
| G18 | 旧版 SYS_STATUS 传感器状态 freshness | FlightTelemetryPageUITest、MockLink SYS_STATUS、JUnit | 通信丢失立即使缓存传感器状态 stale；恢复心跳但未收到新 SYS_STATUS 时仍保持 stale；新帧到达后恢复；历史状态只作诊断上下文 |
| G19 | Flight 存储与云台配置页 | FlightStorageGimbalPageUITest、MockLink FCU_STORAGE_STATUS / Gimbal Manager、JUnit | 存储等待/有效/无效/通信中断/恢复后新帧门控/5500 ms stale 正确；Profile ID、原因码、FCU 运行时间与 GCS 接收年龄正确；无虚构容量或日志操作；云台设备发现/姿态回读/显式获取控制权/完整 owner pair 校验/半 owner pair fail-closed/外部 owner 禁止接管/俯仰与偏航轴 capability 门控/5 Hz 目标回读/GIMBAL_MANAGER_SET_ATTITUDE 角速度控制与停止/yaw-lock 与 retract 状态回读/链路恢复/6500 ms manager 与 3500 ms attitude freshness/Configure 0/-1/-2/-3 权限语义/真实 MockLink 三设备发现与 Gimbal v2 两种寻址/独立姿态、角速度、owner 隔离/广播 interval fan-out 与非法参数拒绝/单设备离线与重发现/未知设备拒绝；Armed/Flying 可访问 |

只有所有适用 Gate 都 PASS，才允许把本轮标记为 PASS。当前完整 Windows Release 交付要求 G0–G19 全部满足（包括 G14 Flight Radio UI、G15 Flight Diagnostics Phase 2）；仅对本次变更范围适用的专项 Gate 必须有精确 HEAD 证据，不适用项须明确说明。

- 飞行健康页逐项 freshness 验收：FCU_SENSOR_HEALTH 的 sensorInstanceId 和 FCU_PREARM_STATUS 的 checkId 各自具有 receiveAgeMs；健康页 500 ms age tick 使年龄持续更新；过期行显示最近一次结果及原因，不得保留无提示的当前态「健康/通过/失败」；覆盖率只统计新鲜项，并区分超时与未收到。
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

- 顶部展示 FCU 通信状态、飞行状态、失效保护等级和导航估计四张摘要卡；摘要字段必须受对应 family availability 保护。
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
- G15 垂直密度 UI 断言覆盖 1672×941 紧凑态与 1672×1100 标准态切换；该自动断言只证明响应式 token 与内容保留，不替代 Windows 100%/125%/150% DPI 实机截图验收。


- 链路趋势采样必须同时校验 link family freshness、实例 receiveAgeMs 和字段 validity mask；过期实例不得继续显示上次 RSSI/质量/丢包值。

- 健康页趋势必须使用 resource taskId=0 的 CPU/WCET/stack 与 storageFreeKib；resource 实例年龄超时或 validity bit 缺失时形成断点，不得沿用旧值。


## 15. 飞控诊断大屏视觉验收补充（2026-11-15）

- 使用 1672×941 原始四张参考图作为唯一视觉基准；不要用构建成功或 MockLink 数据正确替代截图验收。
- 顶栏 56 px、Flight 左导航 160 px；检查品牌锁定区、真实状态指示器和现有导航入口未发生功能回归。
- 六项 KPI 必须位于单一连续边框带中，桌面 6 等分；在 1600/1000/700/400 px 检查 6/3/2/1 列、内容边界和窄屏说明。
- 运行截图需分别保存四页，按相同分辨率进行 50% 叠加/差异检查；100/125/150% DPI 另行检查字体、卡片高度和裁切。
- 只有 exact-HEAD Windows Release、Unit、Integration、Installer、clean-install、installed-runtime 通过且本地视觉签收完成，FGC-0044 才可关闭。


### G15 遥控与链路页专项补充（2026-11-15）

- 桌面宽屏采用两行主视觉：第一行遥控输入监视/逻辑轴输入各 6/12，第二行链路质量/失控保护分别 8/12、4/12；1000 px 中屏使用 3/3 与 4/2 列跨度，700/400 px 四面板单列堆叠。
- 1600 px 与 1000 px 均须断言链路质量面板宽度约为失控保护面板的 2 倍；不得因布局调整削弱链路实例指标的主视觉优先级。
- 失控保护和多实例链路质量面板不得用 operation/link 单域徽标冒充整张面板的综合状态；各数据行/链路实例分别显示真实状态。
- 左右摇杆的轴读数保留 RC family freshness 和逐轴 validity 语义；清除某个逻辑轴 validity bit 后对应 marker 隐藏，读数显示“无效”，不能显示成零。
- 每个链路实例的 RSSI、质量、丢包率分别显示，字段值分别服从 validity mask 与实例 receiveAgeMs；无效字段不得显示原始数值。
- 当 RC 与 GCS 链路同时有效时，链路趋势必须选用 GCS 实例；优先顺序为新鲜 GCS、伴随计算机、其他，存在非 RC 实例但均超时时保留超时/断点；只有无非 RC 实例时才回退 RC。
- 本轮新增/更新的 MockLink Integration 断言覆盖逐实例三个指标、GCS 优先于 RC、非 RC 链路超时后不回退 RC、宽屏 6/6 + 8/4 两行布局、中屏 3/3 + 4/2、700/400 px 单列、双摇杆不越界、无聚合徽标及字段 freshness；本轮未运行 CI，未形成 PASS 证据。后续集中执行 exact-HEAD Windows Release、Unit、Integration、Installer、clean-install、installed-runtime 和本地参考图/DPI 验收。

- 健康页控制器/执行器 freshness 验收：分别注入 controlAvailable、actuatorAvailable fresh → stale → recovered；求解器、序号、求解耗时、DOF、执行器掩码、许可代次、输出策略、原因和 FCU 时间戳必须同步显示状态/恢复值，不得保留旧值冒充当前值。


### G15 KPI 辅助值 freshness 一致性补充（2026-11-15）

- 六项 KPI 的辅助说明不得绕过主值的 family freshness、实例 receiveAgeMs 或 validity/capability 门控。
- 健康页传感器覆盖率按新鲜/超时/等待实例计数；资源摘要按 task 实例 freshness 计数；未收到 actuator family 时不得显示缓存执行器掩码。
- 导航页 UTC 辅助说明和强调色必须同步反映 UTC valid/invalid/time stale/recovered；time 超时后显示“数据超时”并使用琥珀色，不保留旧的“UTC 有效”。
- 遥控页 RC failsafe 辅助说明随 rcInput family freshness 变化；RC 超时后不得继续显示“接收机失效保护未触发”。
- 链路实例摘要分别统计新鲜、超时和等待实例；链路 family 新鲜不代表每个链路实例均新鲜。
- 新增 MockLink UI Integration 断言覆盖上述状态；本轮未运行 CI，不形成 PASS 证据。后续 exact-HEAD 集中执行 Windows Release、Unit、Integration、Installer、clean-install、installed-runtime 与本地参考图/DPI 验收。


### G15 时间质量 KPI 分色补充（2026-11-15）

- 总览与导航页「时间质量」按 timeQuality 枚举分色：仅本地时钟灰、同步中琥珀、锁定绿、保持/降级琥珀、无效红。
- time family 等待/超时优先于缓存 timeQuality 枚举，必须使用 family 对应的灰/琥珀状态。
- UTC 不确定度 KPI 独立使用 utcValid 颜色，不得与 timeQuality 颜色混用；UTC 无效红、time 超时琥珀。
- 新增 MockLink locked/invalid 与 UTC valid 组合断言，以及 family stale 两项 KPI 同时转琥珀的断言；本轮 CI 未运行，留待集中验收。


### G15 总览链路 KPI 覆盖率补充（2026-11-15）

- 总览链路 KPI 主值必须显示新鲜实例数/已知实例总数；辅助说明分别报告新鲜、超时和等待实例及 family 接收年龄。
- 全部实例健康且新鲜时绿色；存在 stale/degraded/recovering 时琥珀色；任一新鲜实例明确 lost 时红色；未知/等待为中性灰。
- 总览与遥控链路专页对同一组实例必须使用一致的主值与聚合颜色；两页主值均显示新鲜实例数/总数，验证三实例全新鲜与 RC 新鲜但 GCS/其他过期两种场景。
- 本轮新增 MockLink UI Integration 断言，CI 未运行，集中验收时执行。


### G18 FGC-0049 旧版 SYS_STATUS 传感器状态 freshness（2026-11-15）

- 运行 `SysStatusSensorInfoTest::_statusFreshnessExpires_test` 与 `_communicationLostInvalidatesStatusUntilNewSysStatus_test`。
- 运行 `VehicleLinkManagerTest::_simpleCommLossTest`，验证真实 MockLink 链路断开时立即失效，重连后由新 SYS_STATUS 恢复。
- 运行 `ToolbarIndicatorUITest::_testLegacySensorStatusStaleOnCommLoss`：Generic MockLink fresh 时提示隐藏；通信丢失后提示可见且明确说明通信丢失；收到新 SYS_STATUS 并恢复 fresh 后提示再次隐藏。
- 通信恢复信号本身不得恢复缓存传感器状态；必须由新 SYS_STATUS 驱动恢复。
- 本轮按用户要求不运行 CI/Unit/Integration；以上状态均为 NOT_RUN，FGC-0049 保持 FIXED_WAITING_RETEST。


### G15 顶部 KPI 窄屏辅助说明换行补充（2026-11-15）

- 内容宽度低于 760 px 时，六项 KPI 辅助说明使用 WordWrap，最多显示两行；仅超过两行才尾部省略。
- Tile 高度按实际内容 implicitHeight 增长，不能固定在 82 px 后裁切辅助说明。
- 集中验收需覆盖页面宽度 700 px 双列和 400 px 单列，确认辅助说明可见、最多两行、Tile 与内容区域不越界。
- 新增 MockLink UI Integration 断言，本轮未运行 CI；此项为待执行验收，不构成 PASS。


### G18 SYS_STATUS stale history context 补充（2026-11-15）

- stale sensor row 显示「Stale · Last: Normal/Error/Disabled」，保留历史上下文但不得作为当前健康；名称与状态顺序保持一致。
- stale row 与顶部 freshness notice 使用同一琥珀色；ToolbarIndicatorUITest 验证文本和颜色一致。
- 新增多传感器 stale 排序单元测试；本轮不运行 CI，集中 Windows Release / Unit / Integration 验收时执行。


### G15 遥控链路指标格响应式重排（2026-11-15）

- 每个链路实例的 RSSI、链路质量、丢包率指标格按 760/420 px 内容宽度断点使用 3/2/1 列。
- 两列时丢包率指标跨两列占满第二行；窄屏单列时三个指标分别整行显示。
- 集中验收需验证 700 px 双列和 400 px 单列，检查三个指标格完整、数值不被挤压，恢复桌面后回到三列。
- 本轮新增 MockLink UI Integration 断言；未运行 CI，不构成 PASS。


### G15 飞行健康页 freshness 与状态表达补充（2026-11-15）

- storageAvailable stale/等待时，SD、NOR、剩余空间、丢弃计数、设置代次、原因和 FCU 时间统一显示 freshness 状态，并继续校验 validity mask。
- prearmAvailable stale/等待时，检查代次、模式掩码、原因和 FCU 时间统一显示 freshness 状态；预检行在 >=760 px 使用 3/3/6 网格、小于 760 px 使用纵向单列。
- 维护横条事件颜色与最新严重度一致；预检颜色仅在 15/15 新鲜且全通过时为绿，失败红、警告/超时琥珀、等待/不完整中性。
- CPU/WCET/栈/AXI 颜色只表示测量字段有效性：有效青色、无效中性灰、过期琥珀，不代表负载健康阈值。
- FlightTelemetryPageUITest 对应断言本轮 NOT_RUN，按用户要求留待集中 CI/本地验证。


### G15 RC 页面 400 px 双摇杆边界补充（2026-11-15）

- 在 1000/900/700/400 px 断点逐一验证遥控输入、逻辑通道、链路质量、失效保护四个面板垂直排列、统一左边界和等宽。
- 左右摇杆必须完整位于遥控输入监视面板内部并保持并排；不允许越界、裁切或因窄屏丢失摇杆。
- 对应断言位于 FlightTelemetryPageUITest::_testFourReferencePagesAndResponsiveLayouts；本轮不运行 CI，集中验收执行。


### G15 事件历史元信息窄屏可读性（2026-11-15）

- 事件序号、FCU 启动时间和严重程度在 >=520 px 三列同排；<520 px 两列，严重程度独立换至下一行右对齐。
- 序号和启动时间不得使用固定宽度或 ElideRight；集中验收需确认 700/400 px 下文本完整、无横向溢出，严重度颜色保持原语义。
- 本轮新增 MockLink UI Integration 断言；未运行 CI，不构成 PASS。


### G15 解锁前检查摘要覆盖率补充（2026-11-15）

- 仅收到部分 checkId 时，摘要明确显示有效项/15、待更新数量和通过/警告/失败/未知计数，不得只显示已收到项而隐去缺失项。
- prearmAvailable family stale 时，摘要和覆盖率显示 family stale 与最近缓存项数量，不把缓存结果算作当前检查。
- 仅 15/15 新鲜且全部通过时使用健康绿色；失败红、警告/逐项超时琥珀、等待/未知/覆盖不完整中性灰。
- 新增 FlightTelemetryPageUITest 断言；本轮按用户要求不运行 CI，集中验收时执行，不构成 PASS。


### G15 告警事件元信息换行与卡片自适应高度（2026-11-15）

- 告警元信息（本地序号、事件 ID、来源 ID）及 FCU 启动相对时间均使用 WordWrap，不允许省略。
- 告警事件内容列的最小宽度设为 0；卡片高度应跟随换行后的 RowLayout 内容高度增长。
- 集中验收在 700/400 px 检查文本 wrapMode、卡片内容高度和元信息/时间右下角均在告警面板边界内。
- 本轮新增 MockLink UI Integration 断言；未运行 CI，不构成 PASS。


### G15 资源任务实例计数 freshness 补充（2026-11-15）

- resourceAvailable family fresh 时显示当前已接收的任务实例数量；family stale/等待时显示对应状态，不把缓存数组长度当作当前数量。
- 实例明细保留最近缓存上下文，但每项 CPU/WCET/栈/截止期/AXI 仍独立受实例 freshness 和字段 validity 控制。
- FlightTelemetryPageUITest 新增真实 MockLink 新鲜计数与 family stale 转换断言；本轮不运行 CI，集中验收时执行，不构成 PASS。


### G15 六项 KPI 辅助说明逐项布局与强制双行回归（2026-11-15）

- 在 700 px 双列与 400 px 单列断点，逐项检查六个 KPI 辅助说明的 WordWrap、两行上限、实际 lineCount、可见状态与 Tile 内边界。
- 通过长链路说明注入强制双行，验证 Tile/内容容器 implicitHeight 自适应，辅助说明底边不得越过 Tile。
- 运行 `FlightTelemetryPageUITest::_testUnknownInvalidAndStaleStatesAreRendered`；本轮按用户要求不运行 CI/Unit/Integration/Windows Release，新增断言保持 NOT_RUN。


### G15 资源实例新鲜度覆盖表达补充（2026-11-15）

- 资源卡与传感器卡统一显示新鲜/超时/等待实例覆盖数量。
- resourceAvailable family stale/等待时显示 family 状态，不展示缓存实例总数；family fresh 时逐实例 receiveAgeMs 决定三类数量。
- FlightTelemetryPageUITest 验证新鲜覆盖文本与 family stale 转换；本轮不运行 CI，集中验收时执行，不构成 PASS。


### G15 通信链路详情卡 freshness 补充（2026-11-15）

- 通信链路详情卡显示新鲜/超时/等待三态实例数量。
- 每个实例 RSSI/质量/丢包指标按 receiveAgeMs 与 validityMask 门控；超时不显示缓存数值，并采用琥珀色状态。
- UI Integration 验证新鲜、超时两种状态及链路实例“最近状态”标签；本轮不运行 CI，集中验收时执行，不构成 PASS。


### G15 固件构建快照与 Profile 匹配 freshness 补充（2026-11-15）

- 构建信息为启动周期快照；family stale 时保留值并标注「最近已知」，不将其显示为当前新鲜数据。
- Profile 匹配要求 operation family fresh，并使用当前启动周期已接收的 BUILD_INFO 静态快照；buildInfo age stale 时标注最近已知构建快照，operation stale/等待时显示无法确认；新 boot epoch 清空旧快照。
- MockLink Integration 使用四个 BUILD_INFO digest chunk 验证 fresh、buildInfo stale + operation fresh、operation stale、operation 恢复以及新 boot epoch 清空快照；本轮不运行 CI，集中验收时执行，不构成 PASS。

### G15 总览总控页全链路通信丢失状态（2026-11-15）

- 总览「FCU 通信状态」KPI 与只读监控徽标必须以 VehicleLinkManager.communicationLost 作为当前链路状态依据；不得仅凭 telemetryAvailable（曾收到过遥测）显示当前通信正常。
- 全链路通信丢失时，两处均显示红色「通信丢失」；恢复心跳后恢复为「已收到 FCU 遥测」及绿色。
- 通信丢失说明必须指出 family freshness 仍独立判定；不得将六域拓扑节点或 13 域健康矩阵强制改成统一状态。
- FlightTelemetryPageUITest 使用 MockLink::setCommLost(true/false) 验证 VehicleLinkManager 心跳丢失/恢复、KPI 文案、页头状态和颜色。本轮按用户要求不运行 CI/Unit/Integration/Windows Release，新增断言为 NOT_RUN，集中验收时执行。


### G15 遥控链路主数值与详情字段去重（2026-11-15）

- 遥控与链路页上方链路质量主视觉面板负责展示每个实例的 RSSI、链路质量、丢包率和 FCU 数据年龄；下方通信详情卡不再重复这些数值。
- 下方详情卡保留 family 状态、实例新鲜/超时/等待覆盖、当前/最近状态、当前/最近原因、指标字段有效性摘要和地面站接收年龄。
- FCU ageUs 标记为「FCU 数据年龄」并显示在上方实例面板；receiveAgeMs 标记为「地面站接收年龄」并保留在详情上下文，两种年龄必须独立门控、不得混淆。
- 字段有效性摘要需分别验证 3/3 有效、0/3 无效、部分无效/未上报以及实例 stale；stale 时不能把最近缓存指标显示为当前有效。
- FlightTelemetryPageUITest 通过 MockLink 原始 MAVLink wire path 验证主视觉数值、FCU 年龄、详情有效性摘要和 stale 颜色；全字段无效的 0/3 摘要必须为警告琥珀色；验证原因仅在下方详情卡显示，主视觉不重复原因；地面站接收年龄与 FCU 数据年龄使用独立标签；本轮按用户要求不运行 CI/Unit/Integration/Windows Release，新增断言保持 NOT_RUN，集中验收时执行。


### G15 多实例链路状态禁止聚合徽标（2026-11-15）

- 遥控与链路页上方「链路质量」HeroPanel 与下方「通信链路」StatusCard 的 `statusFamily` 均必须为空；不得使用 `linkAvailable` 为多实例链路面板生成整卡状态徽标或健康色条。
- 详情卡仍显式展示 linkAvailable 消息接收状态和新鲜/超时/等待实例覆盖；每个实例的状态、原因、字段有效性与地面站接收年龄独立显示，不将 family fresh 等同所有实例健康。
- FlightTelemetryPageUITest 使用 MockLink 验证链路主视觉、链路详情卡与失控保护面板均不带 family 聚合徽标，并验证 per-instance 状态仍正常显示。
- 本轮按用户要求不运行 CI/Unit/Integration/Windows Release；新增断言保持 NOT_RUN，集中验收时执行。


### G15 链路消息与实例接收年龄语义区分（2026-11-15）

- `linkAvailable` family 级年龄标注为「链路状态消息接收年龄」；每个链路实例的 `receiveAgeMs` 标注为「地面站接收年龄」；FCU 上报的 `ageUs` 标注为「FCU 数据年龄」。三者必须在 UI 中独立标注和门控。
- FlightTelemetryPageUITest 通过 MockLink 验证三个稳定 objectName/显示值位置与标签，避免把 family 消息 freshness、实例 freshness 和 FCU 数据年龄混为一谈。
- 本轮按用户要求不运行 CI/Unit/Integration/Windows Release；新增断言保持 NOT_RUN，集中验收时执行。

### G15 总览拓扑六域连线 freshness 与复合面板状态徽标（2026-11-15）

- 六条拓扑连接线和六个节点必须读取同一份 `familyConnectionStates` 状态映射，分别反映 sensorHealth/navigation/link/control/actuator/time 的当前 freshness。
- 新鲜、超时、等待分别显示绿色、琥珀色、中性灰；某一 family 状态变化只改变对应连接线和节点，不影响其他五域。
- 节点卡状态文字、颜色强调、接收年龄与 Canvas 连接状态保持一致；中心 FCU 标识仅作为系统连接示意，不得被解释为飞行姿态或位置。
- 总览「系统拓扑与导航估计」为多域复合面板，`statusFamily` 必须为空，不显示 navigation 单域状态徽标。
- FlightTelemetryPageUITest 验证六域映射、navigation fresh→stale→recovered 连线/节点同步、节点状态文案及复合面板徽标为空；保留 1600/1400/1000/900/700/400 px 节点边界回归。本轮不运行 CI/Unit/Integration/Windows Release，新增断言为 NOT_RUN。

### G15 趋势卡断点计数与最新样本状态（2026-11-15）

- 四张趋势卡均显示独立「断点 N」计数；断点数统计最近 60 点窗口内的缺失/无效样本，断点大于零时为琥珀色，零断点为中性辅助色。
- 最新采样状态单独显示 WAIT/NOW/GAP；最新样本恢复为 NOW 不得清除窗口内历史断点。
- 原有有效样本数/刻度范围、GCS 1 Hz、最近 60 点、时间轴和断点不连线规则保持不变。
- FlightTelemetryPageUITest 验证初始断点 0/WAIT、过期后断点增长/GAP、恢复后 NOW 且历史断点保留，并检查 1600/400 px footer 标签边界。本轮不运行 CI/Unit/Integration/Windows Release，新增断言为 NOT_RUN。


### G15 RC family 状态徽标单点显示（2026-11-15）

- RC family `rcInputAvailable` 的状态徽标全页只在「遥控输入监视」主视觉面板显示；「逻辑轴输入（实时）」和下方「遥控输入」详情卡均不得重复显示同一 family 状态徽标。
- 逻辑轴面板与遥控输入详情卡必须继续绑定 `statusFamily: "rcInputAvailable"`，保留与 RC family freshness 一致的状态色条；只隐藏 badge，不得移除 freshness 语义。
- FlightTelemetryPageUITest 通过 MockLink 在 1600/1000/900/700/400 px 下断言 `flightTelemetry_rcStickMonitor_statusBadge` 可见、`flightTelemetry_rcChannelMonitor_statusBadge` 与 `flightTelemetry_card_rcInput_statusBadge` 不可见，并确认两个次级面板的 `statusFamily` 仍为 `rcInputAvailable`。
- 本轮按用户要求不运行 CI/Unit/Integration/Windows Release；新增断言保持 NOT_RUN，集中验收时执行。

- 失控保护 HeroPanel 不显示 `linkAvailable` family 消息状态；通信链路 StatusCard 独占显示该消息状态及逐实例覆盖，避免把消息 freshness 误读为链路健康。
- 遥控与链路页主视觉布局：≥1280 px 桌面为 12 列网格，双摇杆/逻辑轴 6/6、链路质量/失控保护 8/4；760–1279 px 为 6 列网格，两行分别为 3/3、4/2；低于 760 px 单列。MockLink UI Integration 在 1000/900 px 断言中屏两行比例和同排等高，1600 px 断言桌面 6/6 + 8/4，700/400 px 断言单列与父容器边界。
- 接收机失效保护颜色由 `rcInputAvailable` freshness 门控：新鲜且触发红色、未触发绿色、超时琥珀色、等待/未上报中性灰；MockLink 断言主视觉和详情行一致，并覆盖触发→未触发恢复。
- RC 页摘要带将 `linkAvailable` family freshness 明确标记为「链路消息」，辅助说明显示「链路状态消息接收年龄」；「链路实例」独立显示逐实例覆盖率，MockLink 验证两者不混淆。
- 链路质量 HeroPanel 的「链路状态分布」按新鲜实例分别统计健康/降级/丢失/恢复中/初始化/未知；覆盖三实例健康、部分丢失、全部丢失、恢复中、全部恢复及混合超时，验证摘要文字与红/琥珀/绿/中性灰状态颜色，并确认 freshness 覆盖率仍独立显示。
- 全部链路实例报告丢失但各实例消息仍新鲜时，状态分布必须为红色，实例 freshness 覆盖值与颜色仍为 3/3 新鲜/绿色，linkAvailable family 消息仍为已接收；三者不得被同一个严重度颜色覆盖。
### G15 总览整机状态安全优先级（2026-11-15）

- 飞行模式是左侧整机状态主视觉，模式值用中性前景色；operation freshness 另行显示，不混用颜色语义。
- 解锁状态独立显示 FCU arm state；状态胶囊只表达 freshness，不把 armed/disarmed 本身解释为故障。
- 失效保护等级必须使用全宽优先状态条，正常/降级/需要动作/终止分别沿用 _failsafeColor() 的绿/琥珀/橙/红映射；operation 等待或超时时只显示 freshness 状态，不显示旧 failsafe 等级。
- 控制权作为次级信息行，严格显示 FCU authority 枚举。
- 六域 freshness 仅在中栏拓扑节点/连线独立显示状态和接收年龄；左栏不重复展示六域 HealthCell 矩阵，不合成综合健康分。
- FlightTelemetryPageUITest 通过 MockLink 验证初始 WAIT、fresh 的「定点/解锁中/飞手/正常」、stale 后全部业务字段门控及安全条颜色，并检查 1600/1000/400 px 内容边界。本轮不运行 CI/Unit/Integration/Windows Release，新增断言为 NOT_RUN。



### G19 Flight 存储与云台配置页（2026-11-15）

- 设计与实现规范：[Flight 存储与云台配置页规范](Flight-存储与云台功能设计规范.md)。
- 存储页从 `vehicle.flightTelemetry` 读取真实 FCU_STORAGE_STATUS Facts；SD/NOR/free-space 分别按 validity bit 0/1/2 门控；通信丢失立即隐藏缓存值，恢复链路后必须等待新的 storageTimeBootUs 帧才恢复展示，storage receive age 超过 5500 ms 时显示 stale。
- 存储页不推算总容量、使用率或剩余时间；当前固件没有 MAVLink Onboard Log 列表/读取/擦除支持，因此不加载通用日志下载器，也不发送未支持的日志命令。
- 云台页复用 GimbalController，显示 Gimbal Manager 发现的设备和实时姿态；必须由本机明确取得 primary control 后才开放控制；其他控制端持有时禁止接管。
- VehicleLinkManager 报告通信丢失后，GimbalController 清除缓存的控制权、使 manager/attitude 接收时间戳失效并停止周期发送；恢复后等待新的 GIMBAL_MANAGER_STATUS 确认所有权，并等待新的 GIMBAL_DEVICE_ATTITUDE_STATUS 后才开放姿态命令。
- GIMBAL_MANAGER_STATUS 使用 6500 ms 接收年龄门控（覆盖控制器回退到 0.2 Hz 的 5000 ms 周期及余量）；GIMBAL_DEVICE_ATTITUDE_STATUS 使用 3500 ms 门控。任一流超时都禁止姿态命令；姿态流超时隐藏缓存角度，管理状态超时禁止获取控制权。
- MockLink Configure 遵循 0 清空、-1 保持、-2 设为发送方、-3 owner-pair 校验释放、正整数显式设置；部分 -3/非法 ID 被拒绝。GIMBAL_MANAGER_PITCHYAW 仅允许 primary/secondary owner 发送。
- 拖动姿态滑块时按 5 Hz 发送目标，松开后提交最后目标并停止 Timer；yaw-lock/retract 控件按设备 capability flags 门控。
- MockLink Gimbal Manager Configure 的 -3 释放哨兵只允许释放与发送方 system/component ID 匹配的 owner pair，并在共享状态上加锁。
- MockLink 多设备模拟以同一个 manager component 管理独立的 device 154/155/3；各设备分别维护能力、姿态、速率、消息间隔和 owner pair。设备 155 的姿态状态由 component 155 发布并使用 gimbal_device_id=0；设备 3 的姿态状态由 manager component 发布并携带 gimbal_device_id=3，覆盖 Gimbal v2 两种地址形式。 UI Integration 还在设备 3 上执行 Configure/PITCHYAW 位置控制回读，确认设备 154/155 的姿态和 owner pair 不受影响。
- 单台设备可独立离线/恢复；离线时停止该设备的状态流并清空 owner/rate，另一台设备继续工作；恢复时重新发布 information 并恢复状态流，不继承旧控制权。
- `MAV_CMD_SET_MESSAGE_INTERVAL` 的 component 0 广播应用于所有模拟设备，ACK 使用 manager component；输入 message ID/interval 非有限或非整数时拒绝。
- FlightStorageGimbalPageUITest 增加真实 MockLink 三设备发现、Gimbal v2 两种设备寻址、姿态/角速度/owner 隔离、未知设备拒绝、广播 interval 停止/恢复与非法参数拒绝、单设备离线/重新发现与重新获取控制权覆盖。按用户要求本轮不运行 CI、Unit、Integration 或 Windows Release；上述新增断言保持 NOT_RUN，集中验收执行后再判 PASS。
- 新增 `FlightStorageGimbalPageUITest` 覆盖存储等待/validity/通信丢失/恢复后新帧门控/stale/无虚构容量、云台空态/设备发现/三轴姿态回读/控制权/能力标志门控/滑块目标与姿态回读/链路恢复/外部 owner/拖动 Timer。按用户要求本轮不运行 CI、Unit、Integration 或 Windows Release；G19 新增断言保持 NOT_RUN，集中验收执行后再判 PASS。

### G15 总览告警首要事件视觉层级（2026-11-15）

- 事件总数必须与当前启动周期 recentEvents 长度一致；列表继续按严重度降序、同级按本地接收序号降序，最多显示四项。
- 首项使用主告警卡：严重度徽标、语义色、原因主标题及「最高优先级」标签；未知严重度显示「严重度未知」，不得伪装为已知最高级。
- 事件来源信息保留本地序号、FCU event ID、来源 ID 和 FCU 启动相对时间；不将 time_boot_us 标记为 UTC。
- 空列表显示明确空态，不生成示例事件。
- FlightTelemetryPageUITest 验证首要事件属性/颜色/原因/标签/事件计数，以及 700/400 px 下原因、元信息、时间、标签均位于告警卡边界内。本轮不运行 CI/Unit/Integration/Windows Release，新增断言为 NOT_RUN。

### G15 总览三栏高度与响应式重心（2026-11-15）

- 桌面 1600/1400 px 维持左/中/右 3/5/4 列，中屏 1000 px 为 2/2/2 列；三张 HeroPanel 同行、顶部对齐、实际高度一致。
- 900/700/400 px 主视觉单列堆叠，不强制面板等高；各面板内部内容必须位于自身边界内。
- FlightTelemetryPageUITest 通过 overviewHeroPanelsSameHeight 验证 1600/1400/1000 px 等高，并保留原有顺序、宽度和 900/700/400 px 堆叠断言。本轮不运行 CI/Unit/Integration/Windows Release，新增断言为 NOT_RUN。



### G15 RC 轴读数 freshness 与有效性颜色（2026-11-15）

- AxisGauge fresh/valid 数值使用 `#E6F0F4`，validity bit 清零/越界/非有限使用 `#F0B94B`，未上报使用 `#8298A2`；family WAIT 使用 `#8A9A94`，family stale 沿用琥珀 freshness 色。
- StickVisualizer 两轴均有效时读数为 `#9DD9D0`；任一轴无效或 stale 为 `#F0B94B`；等待/未上报为 `#8A9A94`。Marker 只在两轴都有效时显示。
- FlightTelemetryPageUITest 通过 MockLink 断言 WAIT、有效、无效三种颜色及摇杆 marker 行为；既有三种手型映射与真实通道驱动断言必须继续通过。
- Mode 2 下即使 RC validity mask 全置位，横滚归一化值 1.50 仍必须被判为无效：横滚 AxisGauge 显示无效/琥珀色、右摇杆 marker 隐藏且 readout 无效，左摇杆保持可见；恢复到 0.25 后横滚仪表和右摇杆 marker/readout 同步恢复。
- 本轮按用户要求不运行 CI/Unit/Integration/Windows Release；新增断言保持 NOT_RUN，集中验收时执行。

### G15 总览 KPI 与三栏信息所有权（2026-11-15）

- 总览页顶部摘要带仅显示水平位置 σ、速度 σ、航向 σ三项导航估计标准差；值与状态必须继续遵循 navigation family freshness、capability mask 和字段有效性门控，不新增定位阈值。
- 三项 KPI 位于顶部摘要带；中栏「系统拓扑与遥测域状态」只显示六域拓扑节点/独立接收年龄，不再重复 MetricReadout 导航标准差。
- 左栏保留飞行模式、解锁状态、失效保护等级、控制权；移除与中栏重叠的六域 HealthCell 矩阵；右栏保持告警首要事件。
- 总览页隐藏六项 SummaryTile；飞行健康/导航时序/遥控与链路页切换后恢复各自原有六项摘要卡。
- FlightTelemetryPageUITest 通过 MockLink 与布局断言检查三项 KPI、页面切换显隐、1600/1400/1000/900/700/400 px 内容边界，以及三栏信息不重复。本轮不运行 CI/Unit/Integration/Windows Release，新增断言为 NOT_RUN。

### G15 总览六域拓扑 freshness 图例（2026-11-15）

- 总览中栏拓扑节点下方显示「状态图例」及三种独立状态：已接收、数据超时、等待遥测；颜色分别为 #45D39A、#F0B94B、#8A9A94。
- 图例色点与文字颜色必须一致，且只解释六域 family freshness；不得生成全局健康结论，也不得把全链路通信丢失状态传播到各遥测域。
- FlightTelemetryPageUITest 通过稳定 objectName 检查图例标题、三项文字、色点颜色，并在 1600/1400/1000/900/700/400 px 下检查图例及各项均位于拓扑 HeroPanel 边界内。
- 按用户要求本轮不运行 CI/Unit/Integration/Windows Release；新增断言保持 NOT_RUN，集中验收时执行。


### G15 飞行健康页全链路通信丢失提示补充（2026-11-15）

- MockLink 触发 VehicleLinkManager.communicationLost 后，飞行健康页概览 headline 必须优先显示红色「通信丢失」，说明行明确指出各遥测域仍按独立接收年龄判定。
- 通信丢失不能把 13 个 family 的 freshness 统一改写为 stale；矩阵继续显示各自真实状态。链路恢复后提示隐藏，headline 回到 family freshness 汇总。
- Integration 覆盖丢失、恢复两个状态，检查 headline 文案/颜色和说明行可见性；本轮不运行 CI/Unit/Integration/Windows Release，新增断言为 NOT_RUN。

### G15 总览导航标准差 KPI 数值与 freshness 状态分色（2026-11-15）

- 总览顶部水平位置 σ、速度 σ、航向 σ三项 KPI 在 family fresh 且 capability 具备时，数值使用中性前景色 #E6F0F4；状态文字独立使用 freshness 绿色 #45D39A。
- navigation family stale 时不得显示缓存标准差数值，数值/状态提示使用琥珀色 #F0B94B；capability 未具备或等待时数值占位为中性灰 #A8BBC3，状态文字按 capability/freshness 语义显示。
- FlightTelemetryPageUITest 使用 MockLink 覆盖 fresh、capability unavailable、stale、recovered 四种状态，检查 valueColor/statusColor 分离；三项 KPI 共享同一颜色规则。
- 不增加导航精度阈值，不推断定位等级，不修改 capability mask、family freshness、趋势采样或飞控控制行为。
- 按用户要求本轮不运行 CI/Unit/Integration/Windows Release；新增断言保持 NOT_RUN，集中验收时执行。

### G15 总览告警空态与实时严重度排序（2026-11-15）

- 无 FCU_EVENT 时告警空态必须显示，标题为当前 event family freshness 状态，说明提示收到 FCU_EVENT 后按严重度排序；首条事件到达后空态隐藏。
- MockLink UI Integration 通过真实 MAVLink 路径注入严重度 3、1、3 三条事件，验证事件总数、最新同级最高严重度置顶、同严重度按本地接收序号降序，以及严重度 1 的琥珀色语义。
- 页面优先级提示文案为「最高严重度置顶 · 同级按本地接收序号」，必须与 UI Integration 预期一致。
- 新增断言覆盖 waiting empty state、首条事件到达后的空态隐藏、实时列表重排；本轮按用户要求不运行 CI/Unit/Integration/Windows Release，状态保持 NOT_RUN。


### G15 飞行健康页预检覆盖与资源快照分层（2026-11-15）

- 「安全监测」是预检结果（通过/警告/失败/未知）的唯一摘要位置；维护横条显示预检 fresh/stale/missing 覆盖数量，覆盖色只表达完整度，不被 checkState 的失败/警告污染。
- 维护横条必须显示 CPU 负载、任务 WCET、系统栈剩余、存储剩余；resource taskId=0 指标分别遵循对应 validity bit 和实例 freshness，storage 遵循 storage family freshness/validity。
- FlightTelemetryPageUITest 使用 MockLink 验证 15 项完整覆盖与失败/警告结论分色、四项资源数值和颜色，并在 1600/1000/900/700/400 px 验证资源子网格所有指标位于父边界内。
- 按用户要求不运行 CI/Unit/Integration/Windows Release；新增回归断言为 NOT_RUN，待集中验收。


### G15 导航时序时间纪律与时间戳有效性补充（2026-11-15）

- `LocalTimebaseDisciplineState` 必须独立于 `TimeQuality` 分色：NOMINAL/LOCKED 绿色、ACQUIRING/HOLDOVER/DEGRADED 琥珀色、INVALID 红色；未知/缺失枚举中性灰；time family WAIT/STALE 优先覆盖缓存枚举状态，分别使用中性灰/琥珀。
- FCU 单调时间线只对 family fresh 且处于 JS 安全整数范围内的非负 `time_boot_us` 排序；有效时间戳按早到晚排列，缺失/无效/超时项置后。无效时间戳及其序号/标记使用红色，family stale 使用琥珀，等待/未上报使用中性灰。
- MockLink `FlightTelemetryPageUITest` 覆盖 discipline 六态、time family stale/recovered，以及 `2^53` 越界事件时间戳的无效显示、红色语义和有效点之后排序。
- 本轮按用户要求不运行 CI/Unit/Integration/Windows Release；新增断言保持 NOT_RUN。集中验收时执行 exact-HEAD Windows Release、Unit、Integration 和本地 1672×941 / 100%/125%/150% DPI 视觉检查。

### G15 总览告警摘要与事件历史展示范围（2026-11-15）

- 优先告警摘要最多显示 4 条，按严重度降序、同级按地面站本地接收序号降序；事件历史显示最近 8 条，两个展示上限由独立 QML 常量维护。
- 有事件时显示动态范围说明「优先摘要 X / 总数 条 · 历史最近 Y 条」；无事件时范围说明隐藏，空态不显示伪统计。
- MockLink UI Integration 注入严重度 3、1、3、2、0 五条事件，验证总数、前 4 条优先摘要顺序、第 5 条仍保留在事件历史，以及 700/400 px 范围说明布局边界。
- 本轮按用户要求不运行 CI/Unit/Integration/Windows Release；新增断言状态保持 NOT_RUN。


### G15 FCU 单调时间线相同时间戳排序补充（2026-11-15）

- 有效 `time_boot_us` 按数值升序；时间戳相同时必须按 navigation → time → event 固定顺序排列，不依赖 JS 引擎 sort 稳定性。
- 无效、缺失、超时项均置于有效项之后；无效仍红色、超时琥珀、等待/未上报中性灰。
- MockLink UI Integration 验证三个域同时间戳时的固定排序，以及后续无效时间戳的置后规则。
- 按用户要求本轮不运行 CI/Unit/Integration/Windows Release；集中验收时执行 exact-HEAD 完整回归和视觉检查。

### G15 六域拓扑实时 freshness 数量统计（2026-11-15）

- 中央拓扑状态统计应显示六域中已接收、数据超时、等待遥测的独立数量；计数必须与六个拓扑节点和连线使用相同的 family freshness 状态，不得推导或展示合成整机健康等级。
- MockLink UI Integration 验证初始 0/0/6、三域收到有效状态后 3/0/3，并在导航 family freshness 超时和恢复时同步更新统计。
- 现有 1600/1400/1000/900/700/400 px 响应式检查覆盖统计文字不越界；统计复用原图例行，不增加拓扑卡片高度。
- 按用户要求本轮不运行 CI、Unit、Integration、Windows Release；新增断言保持 NOT_RUN。


### G15 导航页面响应式断点精确验收（2026-11-15）

- 在 1280、1279、760、759 px 四个边界点验证导航页主网格列数分别为 12、6、6、1。
- 验证 1280 px 桌面布局的 4/8 + 8/4 列宽比例和同行等高；1279/760/759 px 四个主面板按顺序纵向堆叠、等宽对齐。
- 验证能力、时间和域状态子网格在 760 px 处由双列切换为单列。
- 本轮按用户要求不运行 CI/Unit/Integration/Windows Release；集中验收时执行 exact-HEAD 回归与 DPI 视觉检查。


### G15 飞行健康页传感器、预检与资源详情层级收口（2026-11-15）

- 传感器实例以独立卡片呈现编号、健康状态、动作、原因/间隙/拒绝计数和地面站接收年龄；状态色遵循实例健康/freshness。
- 预检元信息在 760 px 及以上双列、以下单列；逐项检查卡片保留名称、结果、原因与恢复条件，状态色条遵循 checkState/freshness。
- 每个资源任务使用 CPU 摘要和 WCET/栈剩余/截止期错过/AXI 2×2 指标格；每项独立 freshness/validity gating。
- MockLink UI Integration 验证四项资源的原始值和独立 validity mask、传感器与预检卡片，以及 1600/1000/900/700/400 px 子卡片边界。
- 按用户要求不运行 CI/Unit/Integration/Windows Release；新增断言为 NOT_RUN，待集中验收。


### G15 新 boot 首包非操作域时趋势会话隔离验收（2026-11-15）

- 使用 MockLink 在已有趋势数据后先发送新 boot_id 的 FCU_NAV_STATUS，不先发送 OPERATION_STATUS。
- 在下一次 1 Hz 趋势采样周期内，断言旧趋势缓冲区被清空、趋势 boot ID 暂时为空，且不会跨 boot epoch 连线。
- 新 OPERATION_STATUS 到达前，趋势保持 GAP；导航域显示新会话时间戳，时间同步和事件域仍显示等待遥测，不得显示旧会话时间戳。
- 新会话各域恢复后继续验证原有 FCU 时间线排序、无效时间戳颜色和数据 freshness 门控。
- 按用户要求本轮不运行 CI/Unit/Integration/Windows Release；集中验收时执行 exact-HEAD 完整回归。

### G15 六域拓扑遥测流向与节点标签（2026-11-15）

- 中央拓扑六个端点分别显示传感器系统、导航定位、数据链路、飞控系统、执行器、时间同步；下方节点卡名称必须与端点一一对应。
- 六条连线箭头统一指向 FCU 中心，语义为各域遥测上报；不得将箭头解释为控制指令方向，保留示意不代表飞行器姿态/空间位置说明。
- MockLink UI Integration 验证六个 family 的 label、节点卡文本和 toFcu 方向元数据完全匹配；各 family freshness 颜色保持独立。
- 通过 QFontMetricsF 检查 1600/1400/1000/900/700/400 px 六个页面宽度下标签均在 Canvas 内且不相互重叠；同时验证节点卡和统计图例不越界。
- 本轮按用户要求不运行 CI、Unit、Integration、Windows Release；新增断言为 NOT_RUN，集中验收时执行 exact-HEAD 完整回归。


### G15 Phase 2.53 飞行健康页高 DPI 与窄屏细节优化（2026-11-15）

- 传感器实例头部在 760 px 及以上为编号/状态/动作同一行，759 px 及以下为编号与状态并排、动作占满第二行；UI 回归精确覆盖 760/759 px。
- 资源 WCET、栈剩余、截止期错过、AXI 四项指标共用 ScreenTools 缩放的 ResourceMetricTile；内边距、间距、最小高度跟随字体像素高度，数值使用 secondary 字号并允许换行。
- 预检检查项名称/结果/说明采用一致的 ScreenTools 字体层级和缩放间距。
- MockLink UI Integration 验证 1600/1000/900/760/759/700/400 px 传感器头部重排、四项指标卡片与值标签的父子边界；新增断言未运行。
- 按用户要求不运行 CI/Unit/Integration/Windows Release；新增回归保持 NOT_RUN，待集中验收。

### G15 1672×941 主视觉三栏平衡验收（2026-11-15）

- 在 1672×941 紧凑垂直模式下，左侧整机状态、中间系统拓扑、右侧遥控输入三张 HeroPanel 应同排、同高、无重叠且完全位于 heroGrid 内。
- 保持左/中/右宽度顺序 3/4/5，12 列桌面结构不因纵向密度模式改变；同时验证 1672×1100 标准垂直模式恢复。
- 在同一 1672×941 紧凑模式下依次切换总览、飞行健康、导航时序、遥控与链路，逐页断言主视觉网格与详情网格为 12 列、趋势网格为 4 列、共享间距与紧凑 token 一致、趋势绘图区至少 64 px；最后恢复总览页。
- 该断言与 1672×941 垂直密度、页签、趋势绘图区和详情标题保留断言共同执行；本轮按用户要求不运行 CI/Unit/Integration/Windows Release，新增断言为 NOT_RUN。


### G15 连续多次 FCU 重启退役会话隔离（2026-11-15）

- 使用 MockLink 连续发送 12 个不同 Boot ID 的 FCU_NAV_STATUS，逐个建立新会话，时间戳每次从低值重新开始。
- 随后按旧会话顺序重放前 11 个 Boot ID，故意使用大于当前会话的 FCU 时间戳；所有退役会话帧必须被拒绝，当前导航时间戳不得回滚或切换。
- 验证最后一个活动 Boot ID 仍可正常更新；退役集合保持 64 个 ID 的有界容量。
- 事件域与 BUILD_INFO 域仍需通过各自首包触发 epoch 切换，并拒绝退役会话的事件/构建分块。
- 本轮按用户要求不运行 CI/Unit/Integration/Windows Release；集中验收时执行 exact-HEAD 完整回归。


### G15 飞行健康页详情卡片内容高度与缩放内边距（2026-11-15）

- 传感器实例、逐项预检、任务资源卡片的外层高度必须包含内容 implicitHeight 与两倍 ScreenTools 缩放内边距；禁止固定像素补偿造成高 DPI 裁切。
- UI Integration 使用内容容器 objectName 检查实际高度与上下内边距，并在 1600/1000/900/760/759/700/400 px 全断点验证。
- 新增断言尚未运行；按用户要求 CI/Unit/Integration/Windows Release 继续 NOT_RUN，待集中验收。


### G15 总览告警严重度分布实时摘要（2026-11-15）

- 使用完整当前 recentEvents 缓存统计严重、错误、警告、信息、未知五类事件数量；统计不得只使用优先摘要前 4 条或历史展示最近 8 条。
- severity=3/2/1/0 分别映射严重/错误/警告/信息；负数、超范围、null、缺失或非整数必须计入未知，不得并入信息或严重。
- 告警 family 非 fresh 或事件缓存为空时不显示实时分布；首条 fresh FCU_EVENT 到达后显示并更新对应类别数量。
- MockLink UI Integration 覆盖单事件、五条混合严重度事件、未知字段计数，以及 1600/1400/1000/900/700/400 px 五个 chip 的列数与父子边界；六列中屏告警卡使用三列，其余桌面/堆叠布局使用五列。
- 本轮按用户要求不运行 CI、Unit、Integration、Windows Release；新增断言保持 NOT_RUN，集中验收时执行 exact-HEAD 完整回归。


### G15 遥控通道映射标签与链路质量比例条（2026-11-15）

- 左右摇杆卡在 Mode 1/2/3 切换后，分别显示正确的横向/纵向 CH 编号；标签必须来自页面当前共享手型映射，不得硬编码固定模式。
- 质量条仅在 link family fresh、实例 fresh、validity bit 9 有效且 `linkQualityPermille` 位于 0–1000 时显示；填充宽度 / 轨道宽度必须等于原始 permille / 1000。
- 以 700 permille 验证 70% 填充；以 validity bit 清零、等待/超时、null/空值、非有限或 1200 permille 越界验证不显示填充。1200 permille 与 validity bit 置位但值缺失的情况必须显示无效/琥珀色；恢复到 700 后数字、颜色、条形同步恢复。
- 质量条与链路质量趋势卡当前值/采样共用同一 0–1000 范围门控；超范围或缺失值不得进入趋势序列，形成既有 GAP。质量条只表达 FCU 上报的链路质量字段，不按 RSSI、linkState 或其他实例推导；不得修改链路趋势选源、RC 输入、协议或飞控控制行为。
- MockLink UI Integration 覆盖三种手型标签和质量条 70% → 越界无效 → 70% 恢复；本轮按用户要求不运行 CI/Unit/Integration/Windows Release，新增断言保持 NOT_RUN。


### G15 导航时序时间线字体度量与紧凑大屏适配（2026-11-15）

- FCU 单调时间线三条记录的行高、行距、序号/标记/连接线几何、时间值列宽和横向间距必须随 ScreenTools 字体像素宽高缩放；默认密度保持原视觉基线，不允许固定像素值在高 DPI 下裁切文本。
- 时间戳完整显示优先于消息标签；标签允许省略，时间值不允许省略/换行；排序序号与实际 row y 位置使用同一动态 row pitch。
- FlightTelemetryPageUITest 验证 1672×941 紧凑大屏及 1600/1000/900/700/400 px 宽度下的行高、排序位置、时间值完整宽度、文本列不重叠和面板边界。
- Windows 实机仍需提供 100%/125%/150% DPI 截图；UI Integration 通过不替代像素级视觉签收。
- 本轮按用户要求不运行 CI/Unit/Integration/Windows Release；新增断言保持 NOT_RUN，集中验收时执行 exact-HEAD 回归。


## 4.81 — 2026-11-15

- G15 增加飞行健康页 HealthCell 内容驱动高度、ScreenTools 缩放内边距、维护横条长中文文本自然换行与外层动态增高验收。
- 按用户要求不运行 CI、Unit、Integration 或 Windows Release；新增断言保持 NOT_RUN，等待集中验收。


### G15 导航 UTC 时间戳全宽堆叠显示（2026-11-15）

- 导航页 FCU UTC 时间必须采用独立全宽堆叠行，值区固定宽度字体、自然换行，不得省略或截断日期、时分秒、六位微秒或 UTC 后缀。
- 双列时间同步网格中 UTC 时间戳行横跨两列；UTC 有效性与 UTC 不确定度并排；窄屏单列下时间戳仍占满内容宽度。
- MockLink UI Integration 覆盖 1672×941 紧凑垂直模式与 1600/1000/900/700/400 px，验证四块导航主视觉比例/等高/边界、时间戳行和值标签宽度、内容高度、UTC 时间同步卡片边界及原有 freshness/utc_valid 状态。
- 不改变 FCU_TIME_STATUS、时间服务、MAVLink、FactGroup、UTC 有效性和 freshness 语义；本轮按用户要求不运行 CI/Unit/Integration/Windows Release，新增断言保持 NOT_RUN。


### G15 导航时序辅助网格与指标文本高 DPI 适配（2026-11-15）

- 导航能力网格、时间同步网格、导航/时间域状态网格的 columnSpacing/rowSpacing 必须与 FlightTelemetry 根对象公开的 ScreenTools 缩放 token 一致；默认值分别保持 8/14/6/8 px，UI Scale 或 DPI 增大后按字体像素宽高同步扩展。
- MetricReadout 的内容间距必须与共享 ScreenTools token 一致；导航估计与能力指标的标签/状态文本均启用 WordWrap 且允许收缩到最小宽度，不得因中窄屏或高 DPI 溢出面板。
- MockLink FlightTelemetryPageUITest 在 1280/1279/760/759 px 检查子网格 spacing、指标间距与换行属性；同时保留 1672×941 时间线动态行高/排序、UTC 六位微秒全宽显示和原有数据 freshness/validity 门控。
- 不运行 CI、Unit、Integration 或 Windows Release；新增验收状态 NOT_RUN，集中验收时执行 exact-HEAD 回归并完成 100/125/150% DPI 实机截图签收。


### G15 导航估计指标与能力状态分级徽标（2026-11-15）

- 三项导航估计标准差以中性数值为主，单位与数值同行；family freshness/能力状态使用独立 FamilyStatusBadge，徽标文案和颜色必须与原状态函数一致。
- 四项导航能力状态使用紧凑徽标，不再以大号 metric 数值展示；未知、等待、未具备、超时、已接收等状态仍须保留原文案和颜色语义。
- 1280/1279/760/759 px 断点与 1600/1672×941 视口均验证所有估计/能力徽标可见且完整落在所属 MetricReadout 内；原数值门控、能力位和 freshness 回归继续有效。
- 本轮按用户要求不运行 CI/Unit/Integration/Windows Release；新增断言保持 NOT_RUN，集中验收时执行 exact-HEAD 回归并完成 Windows 100/125/150% DPI 实机截图签收。


### G15 飞行健康页 150% / 200% UI Scale 回归（2026-11-15）

- 必须通过 AppSettings.uiScalePercent 设置真实 UI 缩放，不得只改变页面逻辑尺寸；150%/200% 两档均检查 ScreenTools 派生内边距、HealthCell implicitHeight、矩阵单元内容边界和维护横条动态高度。
- 150% 使用 1672×941 物理视口对应的约 1115×627 逻辑尺寸，健康矩阵/维护横条双列；200% 使用 1280×720 物理视口对应的 640×360 逻辑尺寸，健康矩阵/维护横条单列。
- 200% 场景注入长中文异常摘要，验证文字换行、外层增高、列与内容边界；测试结束恢复原始 UI Scale 与页面视口。
- 按用户要求不运行 CI、Build、Unit、Integration 或 Windows Release；新增断言保持 NOT_RUN，等待集中 exact-HEAD 验收。


### G15 遥控与链路页高 DPI / 内容自适应收口（2026-11-15）

- RC 模式按钮、双摇杆网格、逻辑轴网格、链路实例与指标卡片的 spacing/inset/track/marker 尺寸必须与 FlightTelemetry 根对象公开的 ScreenTools token 一致；100% 基准密度保持当前设计值，125%/150% DPI 下按字体像素宽高同步扩展。
- 摇杆卡片高度必须覆盖内容隐式高度 + 双侧缩放内边距；摇杆舞台保持正方形、位置标记不越出舞台安全内缩范围；手型按钮/通道映射/实时读数可换行。
- RSSI、链路质量、丢包率三类指标卡片高度必须覆盖内容隐式高度 + 双侧缩放内边距；链路质量条高度随 ScreenTools 缩放；各链路实例 freshness、validity、receive-age 门控保持独立。
- MockLink FlightTelemetryPageUITest 在 1000/900/760/759/700/400 px 检查 RC 网格 spacing、AxisGauge track、StickVisualizer 内容高度、链路指标卡片父子边界和文本换行；保留 1600/1672×941 桌面/紧凑视口验收。
- 本轮按用户要求不运行、不触发 CI/Unit/Integration/Windows Release；新增验收保持 NOT_RUN，集中验收时执行 exact-HEAD 回归并完成 Windows 100/125/150% DPI 实机截图签收。


### G15 飞行健康页详情卡片与资源指标高缩放边界（2026-11-15）

- 150%/200% UI Scale 场景同时检查传感器实例、预检、任务资源详情卡内容高度和 ScreenTools 缩放内边距；传感器标题/状态/动作、预检编号/结果/详情及资源数值均不得越界。
- 维护横条 CPU/WCET/栈/存储四项指标必须位于资源网格边界内；200% 单列和长中文异常摘要不得导致维护面板裁切。
- 本轮新增断言未运行，按用户要求不执行 CI/Build/Unit/Integration/Windows Release；等待集中 exact-HEAD 验收。


### G15 飞控配置外壳高 DPI 缩放（2026-11-15）

- Flight 配置顶栏与 FlyViewToolBar 内容高度在 100% UI Scale 下为 56 px；125%/150%/200% 下按 ScreenTools.defaultFontPixelHeight * 5.6 同步增长，二者高度必须一致。
- Flight 左侧导航在 100% 下宽度为 160 px，125% 为 200 px，150%/200% 上限 240 px；导航项间距、分组间距、顶部/左右/底部内边距和按钮 padding/leftPadding 按 ScreenTools 字体度量缩放。
- 常规 VehicleConfigView 与非 Flight 工具栏维持原有布局；参数中心全屏布局不显示侧栏时不得因缩放 token 改变全屏边距。
- MockLink FlightTelemetryPageUITest 使用真实 uiScalePercent Fact 验证 100%/150%/200% 顶栏高度、侧栏宽度、参数按钮 padding 和恢复原始 UI Scale；新增验收 NOT_RUN，后续集中验收时执行 exact-HEAD 回归与 Windows 实机截图。
- 按用户要求本轮不运行、不触发 CI/Build/Unit/Integration/Windows Release。
