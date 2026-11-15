# 飞控诊断高级大屏设计规范

- 版本：7.12
- 日期：2026-11-15
- 状态：四页主体与应用外壳已实现；四页结构、遥测语义与响应式适配持续完善；事件历史/告警元信息、预检、资源实例与 RC/链路 freshness 已收口；导航时序页与遥控链路页持续完成大屏布局适配；总览页全链路通信丢失提示与恢复颜色已补齐；六域拓扑连线与节点按独立 freshness 着色；拓扑状态图例同步显示六域实时已接收/超时/等待数量，不合成整机健康分；趋势卡显式区分有效采样、历史断点与最新样本状态；链路主视觉与通信详情按数值/诊断上下文去重并移除多实例聚合徽标；遥控页 RC family 徽标只在主输入监视面板显示，逻辑轴与下方遥控详情卡隐藏重复徽标并保留 freshness 色条；接收机失效保护按触发/未触发及 RC family freshness 独立着色；失控保护面板不重复显示 linkAvailable 消息状态，通信详情卡是该消息状态唯一呈现位置；整机状态概览已将飞行模式/解锁状态作为主视觉、失效保护作为优先安全状态条、控制权降为次级信息；右侧告警区突出首要严重事件、分离严重度/原因/来源/FCU 启动时间，并提供事件数量、严重度分布实时计数和明确空态；动态标注优先摘要 4 条与事件历史最近 8 条的展示范围；空态与首条事件到达/实时严重度重排均有 MockLink 覆盖；总览三栏主视觉沿用桌面 3/5/4、中屏 2/2/2 列宽，桌面/中屏等高对齐、窄屏自然堆叠；总览顶部摘要已改为水平位置σ/速度σ/航向σ三项导航估计 KPI，拓扑区只显示六域 freshness，左/中/右分别聚焦飞行安全、系统拓扑与告警事件；飞行健康页全链路通信丢失时在概览区突出红色提示，并保留各域独立 freshness；飞行健康页预检结果与覆盖率分层、资源快照扩展；集中 CI 与像素级截图/DPI 验收后置；RC 轴读数按 freshness 与字段有效性分层着色，等待/未上报不再误用警告色；总览六域拓扑增加已接收/超时/等待三态图例，Canvas 端点文字统一继承 ScreenTools 字体族与 micro 点字号，拓扑矢量几何按 ScreenTools 密度缩放；左侧整机安全状态条与控制权行按 ScreenTools 内容高度自适应，图例与节点及连线状态色一致；总览导航标准差 KPI 有效数值使用中性前景色，freshness/capability 状态独立着色；导航时序页补齐本地时钟纪律六态分色与 FCU 时间戳安全整数有效性，时间线标记颜色与排序状态一致。；飞行健康页传感器实例、预检逐项结果与任务资源详情完成卡片化和指标分格，保留原始字段与 freshness/validity 语义；Phase 2.53 飞行健康页传感器实例头部、资源指标卡片与预检排版按 ScreenTools 字体度量适配高 DPI 和窄屏，所有诊断值保持换行完整；Phase 2.55 传感器、预检与资源详情卡片高度严格跟随 ScreenTools 缩放内边距和内容隐式高度，避免高 DPI 下内容裁切；Phase 2.61 健康矩阵单元与维护横条统一使用 ScreenTools 缩放内边距、内容驱动高度和长诊断文本换行；Phase 2.63 导航时序页能力/时间/域状态子网格间距及 MetricReadout 内部间距统一按 ScreenTools 字体度量缩放，指标标签与状态文字支持窄宽度换行；Phase 2.66 遥控手型切换、双摇杆、逻辑轴刻度与多链路指标卡片按 ScreenTools 字体度量缩放，摇杆/指标卡片内容驱动高度并保留窄屏换行
- 适用仓库：`hujunjie-hub/Flight_GroundControl`
- 页面：`src/FirmwarePlugin/Flight/FlightTelemetry.qml`
- 跟踪 Issue：[FGC-0044 飞控诊断页高级大屏布局与可视化改造](https://github.com/hujunjie-hub/Flight_GroundControl/issues/44)
- 设计依据：用户于 2026-11-15 上传的四张 1672×941 原始参考图（总览总控、飞行健康、导航时序、遥控与链路）；四图共用顶部状态栏、左侧导航、标题区和四个页面入口

## 1. 目标与范围

将现有「飞控诊断」Setup 页面升级为高密度、专业、可快速识别异常的飞行器实时监控大屏。页面应当首先呈现飞控运行状态和关键诊断摘要，再按功能域组织完整遥测细节。

本次是展示层重构，不是遥测协议或飞控控制链路重构。现有 `FlightTelemetryFactGroup`、13 类 FCU 遥测消息、Fact 命名、枚举、validity mask、family freshness、boot/profile 隔离和数据分发路径保持不变。

### 1.1 设计目标

1. 一屏先掌握关键状态：FCU 通信状态、解锁/飞行模式、失效保护等级、导航估计摘要。
2. 通过卡片层级、状态徽标和颜色优先级，快速定位等待、超时、无效、警告和故障。
3. 保留现有所有诊断域和详细字段，不以少量 KPI 取代原始诊断数据。
4. 桌面宽屏充分利用空间；中等窗口和窄窗口自动降列，不产生横向裁切。
5. 继续使用现有单调时钟接收年龄和 500 ms 页面刷新机制，不引入高频轮询。
6. 所有状态同时使用文字和颜色表达，不能只依赖颜色。

### 1.2 非目标

- 不修改 MAVLink 消息、Flight ICD、FactGroup 数据结构或解码/分发逻辑。
- 不添加 ARM、解锁、模式切换、Permit、Failsafe 清除、执行器输出等控制操作。
- 不生成“综合健康分”“安全百分比”或单一绿灯结论。
- 当前遥测接口没有提供 FCU 原生历史时序缓存。第二阶段允许地面站在页面可见时，对当前已接收且新鲜的真实 Fact 做 1 Hz 有界采样；图表必须明确标注“地面站采样”，不能冒充 FCU 原生历史、不能插值跨越无效/超时区间，也不能用于飞行安全判断。
- 当前 13 类遥测不包含飞行器姿态角（roll/pitch/yaw）遥测；不得使用 RC 输入或导航标准差伪造人工地平仪/姿态仪。
- 不在地面站推断传感器实例 ID 的物理类型。Flight 发布端与 ICD 的传感器 ID 顺序未统一前，继续展示原始实例 ID。
- 不将 MockLink / Host UI 通过解释为 Flight UDP SIL、H723 实机或真实飞行验收。

## 2. 四页面大屏布局（用户确认四张参考图 v4.0）

本节以用户上传的四张参考图为唯一视觉验收标准。它们是同一诊断大屏内的四个真实页面，不是四张静态装饰图。顶部应用外壳保持 GroundControl 原样；标题、四个可点击页签、六项 KPI 和主内容随当前页面切换。每张图的参考画布均为 1672×941，像素级复刻以相同可用内容区比例、分区位置、卡片尺寸、边框、色彩、字体层级和间距为目标。窗口缩放时按本规范的响应式规则重排，不强行裁切。

### 2.1 应用外壳与页面标题

- 复用 GroundControl 现有顶部状态栏、`MainStatusIndicator`、`FlightModeIndicator`、GPS/电池真实指示器和 VehicleConfigView 功能导航；只对 Flight 固件配置视图应用参考图风格，不改写底层状态来源或导航行为。
- 桌面 Flight 配置顶栏高度为 56 px，采用深青蓝背景；在现有 QGC Logo 旁增加「无人机飞行地面控制站 / UAV GROUND CONTROL STATION」品牌锁定区。准备状态、飞行模式、GPS 和电池仍由现有真实组件提供；不得补造 HDOP、基站状态、时间或不存在的按钮状态。
- Flight 配置左侧导航宽度为 160 px，采用深青蓝底色、常态暗色按钮和青色选中描边；继续展示真实存在的 Flight 组件入口，不创建无实现的装饰性导航项。
- 在 1672×941 参考画布下，左侧内容分界目标约为 x=181 px；面板内容从顶栏下方留出稳定内边距。
- 页面内容区顶部使用大标题「飞控诊断总览」和英文副标题 `FLIGHT DIAGNOSTICS OVERVIEW`。
- 右侧保留「只读监控」状态徽标与 FCU 通信状态提示。
- 页面标题、只读状态、连接状态必须响应式排列，不遮挡、不裁切。

### 2.2 四个交互页面入口

标题下方使用四个真实页面入口：「总览总控」「飞行健康」「导航时序」「遥控与链路」。

- 四个页签图标统一使用 `resources/InstrumentValueIcons/` 中的 SVG 资源（dashboard、heart、location-current、radio），由 `QGCColoredImage` 渲染；禁止使用依赖系统字体回退的 Unicode 图形字符，避免不同 DPI/平台出现字形粗细、基线和占位不一致。
- 图标使用固定 20×20 布局盒并随选中态切换强调色；图标不得挤压页签标题，也不得改变页面切换行为。
- 页签支持轻量悬停反馈：未选中悬停时背景/边框提升一档，移出恢复默认态；选中态始终优先于悬停态。背景和边框颜色使用 140 ms 过渡，不改变点击切页逻辑。

- 四个页签均可点击，选中态使用青色描边/底部强调线；未选中态保持深青蓝面板。
- 点击后只切换 FlightTelemetry.qml 内的页面内容，不创建新 Setup 路由，不改变遥测协议或数据模型。
- 当前默认进入「总览总控」；页面标题、英文副标题、六项 KPI 和趋势图标题随页签同步变化。
- 页签必须保留稳定 objectName，UI Integration 必须逐项点击验证页面切换。

### 2.3 六项关键 KPI 带

四个页面共用六格 KPI 带，但每页按当前功能域切换标签和值；所有 KPI 仍必须绑定真实 Fact、family freshness 和 validity/capability mask。

| 页面 | 六项 KPI | 展示约束 |
|---|---|---|
| 总览总控 | FCU 通信状态、飞行状态、失效保护等级、导航估计、数据链路、时间同步 | 保留 operation/navigation/link/time family 的原有门控 |
| 飞行健康 | 飞控操作、传感器健康、控制器状态、执行器状态、系统资源、事件记录 | 不计算总分；事件数量只来自当前 boot epoch 的真实 recentEvents |
| 导航时序 | 导航状态、水平位置 σ、速度 σ、航向 σ、时间质量、UTC 不确定度 | 位置/速度/航向分别服从 capability bit3、bit5+bit6、bit2；UTC 不确定度仅在 UTC 有效时显示 |
| 遥控与链路 | 遥控输入、有效输入轴、RC 链路状态、数据链路、链路实例、遥控接收年龄 | 有效轴由 rcValidAxesMask 决定；链路指标只显示 validityMask 标记有效的字段 |

KPI 是信息索引，不是综合健康评分。任何 family 未收到、超时、无效或能力位缺失时，必须显示对应状态，不能用 0、绿色或上一次有效值伪装为当前值。 导航标准差 KPI 的数值是测量值而非健康结论：新鲜且能力具备时数值使用中性前景色 #E6F0F4，freshness/capability 状态文字独立使用对应状态色；超时、等待或能力缺失时不得展示旧数值。KPI 主值、辅助说明和强调色必须服从同一 family freshness、实例 receiveAgeMs 与字段 validity/capability 规则；不得出现主值已超时但辅助文字仍显示旧的有效/健康/未触发状态。KPI 只显示当前页有意义的指标，禁止为了对齐参考图而伪造电池、卫星、HDOP、振动、温度、SNR、RTT、吞吐量或不存在的原始 RC 通道。

### 2.4 三栏主视觉区

桌面宽屏按参考图组织为左窄、中宽、右侧告警三栏；三个面板同一行对齐。

| 区域 | 标题/内容 | 实现要求 |
|---|---|---|
| 左栏 | 整机状态概览 | 突出当前飞行模式与解锁状态；失效保护等级使用优先安全状态条，控制权作为次级信息；不重复展示六域 freshness 矩阵 |
| 中栏 | 系统拓扑与遥测域状态 | 中央绘制静态多旋翼拓扑示意；六条连接线与六个节点分别绑定传感器、导航、链路、控制、执行器、时间 family 的真实 freshness，已接收为绿色、超时为琥珀色、等待为中性灰；下方节点卡同步显示状态与接收年龄，并以文字+色点提供三态 freshness 图例；拓扑区不重复展示导航标准差，三项导航估计由顶部 KPI 独立负责 |
| 右栏 | 告警事件（优先级） | 显示当前 boot epoch 最近事件、严重度、事件序号/ID、来源和原因；RC 输入监视移至独立的「遥控与链路」页面 |

- 左栏不得显示 96/100 等虚构健康分，不得将 family 状态计数包装成健康百分比。
- 中栏拓扑图是静态系统连接示意，不代表飞行器实时位置、姿态、航向或飞行轨迹；当前接口没有姿态角，不得绘制人工地平仪。六条连线和节点必须按各自 family freshness 独立着色，不得将某一 family 状态或全链路通信状态扩散成整张拓扑的统一健康结论。节点区下方必须提供「已接收 / 数据超时 / 等待遥测」三态图例，颜色分别复用 #45D39A、#F0B94B、#8A9A94，图例同时显示文字，不承担状态汇总或健康评分职责。
- 右栏事件必须来自 `recentEvents`；没有事件时明确显示空态/等待状态，不生成示例告警。
- 左栏整机状态概览和中栏系统拓扑与遥测域状态均为多域复合面板，标题区不得显示某一个 family 的单域状态徽标；各子项与拓扑节点独立显示自己的 freshness。右栏告警面板可以保留 event family 状态。
- 总览页不重复显示 RC 操纵输入；横滚/俯仰/偏航/油门及双摇杆可视化统一放在「遥控与链路」页面，并按 `rcValidAxesMask` 和 FlightRadio 手型映射显示，无效轴不绘制成零值。
- 不为匹配参考图而伪造电池、卫星数、HDOP、振动或链路吞吐量；当前 FlightTelemetry 接口未提供的字段不展示为实时指标。

### 2.5 关键指标趋势带

主视觉区下方按当前页面显示四张并列趋势卡，采样数据全部来自新鲜、有效的真实 Fact：

| 趋势 | 来源 | 采样准入 |
|---|---|---|
| 水平位置标准差 | positionStdM | navigation 新鲜、能力 bit3 存在、数值有限且非负 |
| 速度标准差 | velocityStdMps | navigation 新鲜、能力 bit5 与 bit6 同时存在、数值有限且非负 |
| 航向标准差 | headingStdRad | navigation 新鲜、能力 bit2 存在、数值有限且非负 |
| 控制求解耗时 | controlSolveTimeUs | control 新鲜、数值有限且非负；总览/健康页显示 |
| UTC 时间不确定度 | utcUncertaintyUs | time 新鲜且 utcValid=true；导航时序页替换第 4 张图 |
| CPU 负载 / WCET / 栈剩余 / 存储剩余 | resourceStatuses、storageFreeKib | health/resource/storage family 新鲜；taskId=0 实例 receiveAgeMs 在限值内；resource validity bit0/1/2 和 storage bit2 有效；飞行健康页 |
| RSSI / 链路质量 / 丢包率 / RC 接收年龄 | linkStatuses、rcAgeUs | link/RC family 新鲜且相应 validity mask 有效；链路实例 receiveAgeMs 在限值内；遥控与链路页替换四张图 |

继续使用页面现有 500 ms 年龄刷新 Timer 的每第二次 tick 进行 1 Hz 地面站本地采样，最多保留 60 点；页面隐藏时停止采样。图表明确标注 GCS 1 Hz / 最近 60 点，缺失、过期、无效和 capability 缺失均形成断点，禁止插值或跨断点连线。每张趋势卡底部独立显示窗口内断点数量和最新样本状态（WAIT/NOW/GAP）：断点数量统计窗口内全部缺失/无效样本，最新样本恢复为 NOW 不得清除历史断点；刻度范围、断点数量和最新样本状态不得合并成含混的单一状态。

### 2.6 完整诊断信息保留区

趋势带下方继续保留原有 14 张详细诊断卡片。13 类遥测域状态矩阵只在「飞行健康」页作为首屏主视觉呈现，总览页不重复显示矩阵。所有页面均可纵向滚动查看完整 14 张详细诊断卡片，不允许删除、折叠丢失或只保留摘要。

| 顺序 | 卡片 | 必须保留的内容 |
|---|---|---|
| 第一组 | 操作与飞行模式 | 解锁状态、模式、控制权、失效保护、任务、boot/profile、各代次、请求 ID、原因、FCU 时间 |
| 第一组 | 导航状态 | 状态代次、原点、能力掩码、位置/速度/航向标准差、数据年龄、原因、FCU 时间 |
| 第一组 | 传感器健康 | 上报实例数量、每实例健康状态/动作/原因/间隙/拒绝计数/接收年龄 |
| 第二组 | 控制器状态 | 求解器、控制序号、求解耗时、可控/降级 DOF、原因、FCU 时间 |
| 第二组 | 执行器状态 | 控制序号、输出许可代次、可用/饱和掩码、输出策略、反馈能力、原因 |
| 第二组 | 通信链路 | RC/GCS/Companion/其他链路状态、RSSI、质量、丢包、FCU 年龄和地面站接收年龄 |
| 第三组 | 解锁前检查 | 检查代次、接收项数量、适用模式掩码、逐项状态、原因和恢复条件 |
| 第三组 | 最新事件 | 事件 ID、严重度、来源、原因命名空间/代码、本地接收序号、FCU 时间 |
| 第四组 | 实时资源 | 系统/任务 CPU、WCET、栈、截止期错过、AXI 可用空间、接收年龄 |
| 第四组 | 存储状态 | SD/NOR 状态、有效剩余空间、事件/日志丢弃数、设置代次、原因 |
| 第四组 | 时间同步 | 时间质量、时钟纪律、UTC 有效性和不确定度、事件不确定度、时间代次 |
| 第五组 | 遥控输入 | 横滚/俯仰/偏航/油门、有效轴掩码、接收机 failsafe、链路状态和输入年龄 |
| 第五组 | 最近事件历史 | 当前 boot epoch 内最近事件，保持有界历史 |
| 底部 | 固件与版本 | 产品/接口/Schema、ICD 兼容性、Profile 匹配、摘要 hash、源码提交和 FCU 时间 |

飞行健康页的 13 类 family 矩阵逐项显示 operation、navigation、sensor health、control、actuator、link、storage、resource、time、event、pre-arm、RC input、build info 的独立状态和接收年龄；宽屏/中屏/窄屏分别为 4/2/1 列。event 是异步域，不使用周期数据超时语义。

### 2.7 信息完整性与安全边界

- 所有原有字段、Fact 命名、Validity/capability mask、family freshness、boot/profile 隔离、per-instance 接收年龄、reason code 和 objectName 测试选择器保持不变。
- 不修改 `FlightTelemetryFactGroup`、MAVLink、Flight ICD、消息解码/分发或 Flight 发布端。
- 不新增 ARM、模式切换、Permit、Failsafe 清除、执行器输出等控制操作；页面始终只读。
- 不添加综合健康分、虚构电池/卫星/HDOP/振动/吞吐量指标，不把示意拓扑解释为真实位置/姿态。


### 2.8 飞行健康页面

- 主体按左/中/右三栏排列：左侧健康状态概览（逐项状态计数，不显示 96/100 等综合健康分）、中间 13 类遥测域矩阵、右侧安全监测。
- 健康矩阵逐项显示 operation、navigation、sensor health、control、actuator、link、storage、resource、time、event、pre-arm、RC input、build info 的独立状态和接收年龄。
- 矩阵宽屏 4 列、中屏 2 列、窄屏 1 列；单元内容随文本自然增高。
- 矩阵下方显示维护与可靠性横条：启动 ID、最近真实事件、预检覆盖数（有效/超时/未收到）和 CPU/WCET/系统栈/存储四项真实资源快照；预检通过/警告/失败/未知结论只在安全监测区显示，避免同一结论重复。
- 页面底部复用地面站本地 1 Hz 趋势，使用真实 CPU 负载、WCET、栈剩余和存储剩余；resource taskId=0 实例超时或 validity bit 缺失时形成断点。不把健康分、振动 RMS、电池温度或电压一致性等未提供字段伪装成遥测。
- VehicleLinkManager.communicationLost 为 true 时，健康概览 headline 立即显示红色「通信丢失」，并说明遥测矩阵仍按各 family 的独立接收年龄判定；不得把全链路丢失机械扩散为 13 个遥测域统一 stale。链路恢复后隐藏提示，矩阵继续沿用各 family 自身 freshness。
- 「安全监测」只显示预检通过/警告/失败/未知结论及其安全语义色；维护横条只显示预检覆盖数（fresh/stale/missing），覆盖色仅表达数据完整度：全部 15 项 fresh 为绿、存在 stale 为琥珀、不完整/等待为灰，不因检查失败或警告而改变覆盖色。
- 维护横条的系统资源快照复用 taskId=0 的 CPU/WCET/系统栈剩余和 storage family 的剩余空间，逐项沿用 resource/storage family freshness、实例年龄和 validity mask；底部四张趋势图仍表达这些量的本地 1 Hz 历史，不以缓存值填补无效/过期样本。

### 2.9 导航时序页面

- 主体按导航估计总览、导航能力状态、时间同步状态、FCU 单调时序四块组织；桌面 12 列网格采用两行不等宽布局：第一行导航估计不确定度 4 列 + 导航能力状态 8 列，第二行时间同步状态 8 列 + FCU 单调时序 4 列；每行内面板等高。760–1279 px 使用 6 列整行逐块堆叠，低于 760 px 单列。
- 导航估计总览只展示 positionStdM、velocityStdMps、headingStdRad；能力状态区独立展示水平位置、水平速度、垂直速度、航向四项能力状态，按 navigation family freshness → capability mask 顺序门控，缺少能力位显示不可用，不重复展示同一能力状态。
- FCU 单调时序面板分别显示 navigation 与 time 两个数据域的消息状态和接收年龄；有效 time_boot_us 按 FCU 启动时间升序排列，缺失/无效/超时项置后，不得使用 navigation 单域徽标代表导航与时间同步整体正常。
- 时间同步主视觉只显示 timeQuality、timeDisciplineState、utcValid、FCU unix_time_us 转换的 UTC 绝对时间和 utcUncertaintyUs；UTC 绝对时间/不确定度仅在 time family 新鲜且 utcValid=true 时显示，UTC 无效、未上报或超时不得显示旧值。eventUncertaintyUs、timeEpoch、sourceMappingEpoch 和地面站接收年龄保留在时间诊断详情；FCU 单调发布时间只在时序面板显示。timeDisciplineState 使用独立于 TimeQuality 的颜色映射：NOMINAL/LOCKED 绿色、ACQUIRING/HOLDOVER/DEGRADED 琥珀、INVALID 红色；time family 等待/超时优先覆盖缓存枚举颜色。
- FCU 单调时序仅将 family fresh 且处于 JavaScript 安全整数范围内的非负 time_boot_us 作为有效时间点；有效点按升序排列，相同时间戳固定按 navigation → time → event 顺序打破平局，缺失/无效/超时项置后。有效点使用对应数据域强调色，时间戳无效红色、family 超时琥珀色、等待/未上报中性灰；UTC 无效不影响 FCU 单调时间。趋势图使用水平位置、速度、航向和 UTC 时间不确定度；UTC 无效/超时产生断点。
- 当前 FlightTelemetry 未提供卫星数、HDOP、每颗卫星方位/仰角，禁止绘制虚构卫星天图或填入示例值。

### 2.10 遥控与链路页面

- 主体按左侧双摇杆、逻辑轴实时输入、链路质量、失控保护四块组织。
- 双摇杆手型直接读取 FlightRadio 的持久化 Mode 1/2/3 设置，按其 channel-to-axis 映射绑定横滚/俯仰/偏航/油门；marker 仅在对应两个轴均新鲜、validity bit 有效且归一化数值处于轴范围内时显示。轴读数分别保留等待、超时、未上报和无效语义；超范围输入不得夹取到边界或折算为 0，逻辑轴仪表与对应摇杆 marker/readout 必须同步转为无效并在新鲜有效值恢复后同步恢复。
- 当前协议只提供四个归一化逻辑轴，不提供 CH1–CH8 原始 PWM；页面明确显示四个逻辑轴，不虚构额外通道。
- 链路质量逐实例显示 linkKind、linkState、RSSI、质量、丢包、原因和接收年龄；RSSI、质量、丢包分别成格，并分别满足 link family 新鲜、实例 receiveAgeMs 未超时、validityMask 对应位有效。
- 链路质量主视觉增加「链路状态分布」摘要：只统计 linkAvailable family 与实例 freshness 均新鲜的当前状态，分别计数健康、降级、丢失、恢复中、初始化和未知；超时/等待实例不冒充当前断链，并单独保留超时/等待计数。状态颜色优先级为丢失红色、降级/恢复中或部分超时琥珀色、未知/无新鲜实例中性灰、全部健康绿色；链路实例新鲜度覆盖仍独立显示。
- 链路趋势优先使用新鲜 GCS（kind 2）、伴随计算机（kind 3）或其他数据链路（kind 4）；存在非 RC 实例但均超时时保留超时/断点，不以健康 RC 接收机冒充数据链路；仅无非 RC 实例时回退 RC（kind 1）。趋势标题标明当前选中链路类型。
- 失控保护与多实例链路质量面板不使用单一 operation/link family 徽标代表整张面板；各域和各实例状态独立表达。
- 接收机失效保护颜色独立服从 `rcInputAvailable` freshness：新鲜且触发为红色、未触发为绿色；RC family 超时为琥珀色，等待/未上报/未知为中性灰；不得借用 RC 链路状态或飞控 failsafe 等级颜色。
- 「失控保护」主视觉只呈现 operation failsafe、接收机 failsafe、RC 链路和 RC 输入年龄；`linkAvailable` 是链路状态消息 family freshness，不是链路健康结论，只在下方「通信链路」详情卡的「消息」行显示，并与逐实例状态/覆盖率并列。
- 遥控页摘要带将 `linkAvailable` 明确命名为「链路消息」，并以「链路状态消息接收年龄」解释其 freshness；「链路实例」摘要单独表达逐实例新鲜度覆盖，不把消息接收状态误读为链路健康。
- 桌面（≥1280 px）采用两行不等宽布局：第一行双摇杆/逻辑轴为 6/6，第二行链路质量/失控保护为 8/4；中屏（760–1279 px）使用 6 列网格，分别按 3/3、4/2 两行排列；低于 760 px 四块主面板单列堆叠。
- 底部四张趋势图按页切换为优先数据链路 RSSI、链路质量、丢包率和 RC 接收年龄；无效/过期样本形成断点。


## 3. 响应式布局规范

布局依据 SetupPage.availableWidth，页面外壳和左侧主导航由 GroundControl 现有应用框架提供。

| 可用宽度 | 主页面网格 | 健康矩阵（健康页） | KPI | 页签 |
|---|---:|---:|---:|---:|
| ≥1280 px | 12 列 | 4 列 | 6 列 | 4 列 |
| 1000–1279 px | 6 列 | 2 列 | 3 列 | 4 列 |
| 760–999 px | 总览/明细 1 列；健康/导航/RC 页面网格 6 列（导航主面板全宽堆叠；RC 按 3/3 + 4/2 两行排列） | 2 列 | 3 列 | 4 列 |
| 420–759 px | 1 列 | 1 列 | 2 列 | 2 列 |
| <420 px | 1 列 | 1 列 | 1 列 | 2 列 |

- 总览主视觉宽屏使用左 3 / 中 5 / 右 4 列；健康页使用左 3 / 中 6 / 右 3 列；导航时序页使用 4/8 + 8/4 两行布局；遥控链路页使用 6/6 + 8/4 两行布局。
- 1000–1279 px 与 760–999 px 时遥控链路页均使用 6 列网格：双摇杆/逻辑轴各占 3 列同排，链路质量/失控保护分别占 4/2 列同排；导航时序页主面板全宽纵向排列。总览/明细在 760–999 px 使用单列；低于 760 px 时所有主网格单列。
- 13 类健康矩阵只在「飞行健康」页面显示，宽屏 4 列、中屏 2 列、窄屏 1 列；总览页不重复显示矩阵。
- 健康页维护信息横条宽屏 3 列、中屏 2 列、窄屏 1 列，文字换行时自然增高。
- 页面标题/只读徽标在 760 px 以下纵向排列；KPI、主视觉、趋势、矩阵和明细卡自然增高，不得裁切长原因或 hash。
- UI Integration 覆盖 1600/1400/1000/900/700/400 px，验证页签、主网格、健康矩阵、标题徽标和内容边界。

## 4. 视觉规范

### 4.1 视觉方向

严格采用用户上传四张参考图的深色航空航天 HUD 大屏风格；四个页面共享应用外壳、标题带、页签和 KPI 带：深海军蓝/蓝黑背景、深青色分层面板、青蓝色高亮描边与局部光感、细密分隔线、清晰网格和紧凑高密度信息布局。保留绿色正常、黄色注意、橙色警告、红色故障的状态色。视觉效果要有专业指挥中心的层次与科技感，但不允许用装饰图形替代真实数据。

### 4.2 色彩令牌

| 令牌 | 色值 | 用途 |
|---|---|---|
| 页面背景 | `#031A20` | Flight 配置与大屏背景 |
| 卡片背景 | `#06232B` | 主诊断卡片 |
| 卡片内层/摘要背景 | `#082B33` | 摘要卡、徽标底色 |
| 边框/分隔线 | `#0B6873` | 卡片边界和细分隔 |
| 主强调青绿 | `#18DAD4` | 选中焦点、边框强调；状态绿仍独立使用 |
| 新鲜/已接收 | `#45D39A` | 数据组接收新鲜 |
| 超时/注意 | `#F0B94B` | 数据超时、降级/注意 |
| 警告/需动作 | `#F08B45` | 明确的警告或需要动作状态 |
| 严重/故障 | `#F06464` | 明确的故障、终止或无效值 |
| 未知/未上报 | `#8A9A94` | 未知、未上报、等待数据 |

颜色仅用于表达已知状态，不得通过视觉颜色把未知值包装成正常状态。文字标签必须同时存在。

### 4.3 卡片与排版

- 主视觉、趋势和详细诊断卡统一 8 px 圆角，边框 1 px；健康矩阵单元使用 6 px 次级圆角。面板采用 `#06232B` 深青蓝底色和 `#0B6873` 青色边界。
- KPI 带必须是一个连续的带边框面板，桌面宽屏 6 等分，图标/标签/数值水平组合，分隔线只用于桌面 6 列；中小屏按 3/2/1 列自然重排，长说明只在窄屏显示。桌面单行高度目标 66–70 px。三栏主视觉使用左窄/中宽/右侧告警的稳定比例；趋势卡使用统一高度和绘图区。
- 卡片标题采用“英文小类目 + 中文主标题”两级层次；英文类目仅作为辅助定位。
- 卡片头部右侧显示该 family 的独立接收徽标。
- 徽标使用小圆点、状态文字和淡色底；不要用大面积高饱和背景。
- 字段标签低对比度，字段值高对比度并右对齐；单位紧跟数值。
- 关键状态值可使用较大字号；普通诊断字段保持紧凑且可读。
- 保留字段之间的轻量分隔线；卡片间距 12–16 px。
- 趋势图绘图区高度不低于 64 px；绘制低对比网格、连续有效样本段的半透明面积填充、圆角折线和最新有效样本亮点。无效样本必须切断折线与面积填充，最新样本无效时不显示“当前点”；仅有一个有效样本时显示单点，不伪造趋势线或刻度范围。
- 趋势时间轴明确标注“较早/最新”，刻度与单位保持独立；状态标记使用 NOW/GAP/WAIT，GAP 时保留前序有效段但不跨越缺口连接。
- 字体和间距基于 `ScreenTools` 缩放，不硬编码整页字号。

## 5. 状态表达与数据语义

### 5.1 Family 接收状态

| 状态 | 文字 | 视觉 | 规则 |
|---|---|---|---|
| 已接收且在 freshness limit 内 | 已接收 | 绿色 | 仅说明该 family 的地面站接收年龄满足现有阈值 |
| 超过 freshness limit | 数据超时 | 黄色 | 保留最后值的诊断上下文，但必须显式标记过期 |
| 尚未收到/不可用 | 等待遥测/未收到 | 灰色 | 不得显示为 0、正常或健康 |
| 无效值/Validity bit 不满足 | 无效 | 红色 | 保留“无效”语义，不把原始 payload 数值显示为有效数据 |
| 未知枚举/保留值 | 未知（原始值） | 灰色或红色强调 | 保留原始数值，不能映射为有效枚举 |

各卡片使用自己的 family 状态。不得因顶部有遥测、某一个 family 更新或某一个传感器实例新鲜而把其他 family/实例显示为新鲜。

现有 freshness limit、接收年龄、per-instance age 和 monotonic clock 语义保持不变。

### 5.2 明确状态枚举

- Failsafe：NORMAL 绿色、DEGRADED 黄色、ACTION_REQUIRED 橙色、TERMINATE 红色；未上报/未知保持灰色。
- 传感器健康：HEALTHY 绿色、SUSPECT 黄色、DEGRADED 橙色、ISOLATED/FAULT 红色、UNKNOWN/INITIALIZING 灰色。
- 求解器、链路、存储、UTC、prearm 等状态必须按各自协议枚举显示，不得套用其他域的颜色含义。
- 状态颜色是对明确枚举的视觉编码，不构成地面站安全判定。

## 6. 实时刷新与性能

- 保留页面现有 500 ms 年龄刷新 tick；不再为每张卡片创建独立 Timer。
- 接收年龄和 freshness 状态继续由 `FlightTelemetryFactGroup` 的单调时钟和通知机制提供。
- QML 不在每个刷新 tick 中扫描或复制全部 Fact；只读取现有属性和有限列表。
- 最近事件保持现有有界历史，页面只显示最近有限条记录。
- 不增加无界缓存、后台线程、网络请求或额外遥测订阅。
- 页面隐藏时停止页面级年龄刷新 tick，保持现有行为。

## 7. 交互规范

- 页面为只读诊断视图，不出现可发送飞控命令的按钮。
- 卡片内容全部可见且可纵向滚动；不得因窗口缩窄而横向裁切。
- 诊断信息使用统一卡片头、状态徽标和字段行。
- 第二阶段趋势图为只读静态绘制，不增加图表交互、告警确认或状态清除操作；这些功能需要独立的产品需求和权限审查。
- RC 轴可视化仅显示真实 FCU 遥测输入，不提供摇杆控制或校准入口。

## 8. 测试与验收

### 8.1 页面结构

- Flight 配置顶栏 56 px、左侧导航 160 px，保留现有真实状态指示器和组件导航；诊断页内部包含动态标题、只读标识、四个真实交互页签、单一连续 KPI 带（六项页面专属 KPI）、当前页主体、四张趋势卡和原有 14 张详细诊断卡。
- 四个页签分别为「总览总控」「飞行健康」「导航时序」「遥控与链路」，通过 `_activeDashboardPage` 切换当前内容，不新增 Setup 路由或数据模型。页签选择器为 `flightTelemetry_visualTab_0..3`。
- 四个页面主体分别使用 `flightTelemetry_heroGrid`、`flightTelemetry_healthPageGrid`、`flightTelemetry_navigationPageGrid`、`flightTelemetry_rcLinkPageGrid`；页面标题、副标题、六项 KPI 和趋势 valueKey 必须随页签同步变化。
- 总览页展示飞控操作/飞行状态、系统拓扑与导航估计、当前启动周期告警事件；不重复显示 13 类健康矩阵，也不显示 RC 摇杆/四轴输入。
- 飞行健康页展示 13 类 family 独立状态矩阵、健康状态概览、安全监测和维护信息横条；不生成综合健康分。矩阵单元 objectName 使用 `flightTelemetry_healthPage_<family>`。
- 导航时序页只显示真实导航能力、位置/速度/航向精度、时间纪律、UTC 有效性与不确定度；不伪造卫星数量、HDOP 或卫星天图。
- 遥控与链路页展示双摇杆、四个逻辑轴、链路质量和失控保护；双摇杆手型读取与 FlightRadio 相同的持久化 Mode 1/2/3 设置，输入轴受 freshness、有效轴掩码和数值范围约束。
- 六项 KPI 根据当前页面切换标签和值；内容高度随文字自然增高，不能裁切原因、状态或 hash。
- 四张趋势卡保留稳定 objectName，按页面显示真实数据：总览导航/控制、健康 CPU/WCET/栈/存储、导航位置/速度/航向/UTC、RC RSSI/质量/丢包/接收年龄。
- 原有 14 张详细诊断卡片、字段、FactGroup、既有 objectName 和 MockLink wire-path 覆盖必须完整保留；13 类健康矩阵只在飞行健康页呈现。
- 不为贴近参考图而伪造健康分、电池、卫星、HDOP、振动、温度、CH5–CH8、SNR、RTT、吞吐量或不存在的姿态数据。

### 8.2 状态语义

继续使用现有真实路径：

`MockLink raw MAVLink bytes → MAVLink parser → Vehicle → FlightTelemetryFactGroup → QML`

必须验证：

1. 无 FCU 自定义遥测时，摘要显示等待/未上报，不能显示绿色健康。
2. operation family 新鲜时显示已接收。
3. operation family 超时后显示数据超时。
4. 新的严格递增帧到达后恢复已接收。
5. storage validity mask 无效时显示无效，不能显示 payload 中的伪有效数值。
6. 摘要字段必须受对应 family availability 和 freshness 保护；过期后显示“数据超时”，不得继续把最后一次有效值显示成当前值。
7. 失效保护摘要和详情行的颜色必须跟随 operation family freshness；过期的最后值不得继续显示为绿色。

### 8.3 响应式布局

- 四页主体主网格在 1600 px 使用 12 列、1000 px 使用 6 列、700 px 和 400 px 使用 1 列；所有卡片按各页面板跨度自然换行。
- 飞行健康页 13 类矩阵在 1600/1000/700/400 px 下分别使用 4/2/1/1 列；13 个单元必须全部存在且 x/y/width/height 均位于矩阵网格边界内。
- 飞行健康页维护信息横条在 1600/1000/700/400 px 下分别使用 3/2/1/1 列，文字换行时自然增高，不允许横向溢出。
- 飞行健康页 HealthCell 单元高度不得固定为像素常量；必须按内容 implicitHeight 与两倍 ScreenTools 缩放内边距计算，内部文字容器在 1672×941/1100 及 1600/1000/900/760/759/700/400 px 下完整位于单元内。
- 飞行健康页维护横条外层高度必须等于内容隐式高度加两倍 ScreenTools 面板内边距；长中文异常摘要应触发自然换行和外层增高，三列/两列/单列布局均不得裁切或横向溢出。
- 页签网格在 1600/1000 px 使用 4 列，在 700/400 px 使用 2 列；KPI 连续带内网格分别使用 6/3/2/1 列，六项内容必须完全落在带内；趋势网格分别使用 4/2/1/1 列。
- 页面标题与只读徽标在 760 px 以下纵向排列；KPI、主视觉、趋势、矩阵和详细卡片自然增高，不得裁切长原因或 hash。
- UI Integration 必须覆盖 1600/1000/700/400 px，验证四页主网格、健康矩阵、维护横条、页签、KPI 和趋势网格列数，并检查 13 个矩阵单元边界；RC 页还要验证 Mode 2 下双摇杆读数与逻辑轴一致，以及 roll validity bit 清除后右摇杆 marker 隐藏。

### 8.4 趋势采样与状态矩阵

- 无遥测时趋势采样不生成有效点；飞行健康页 13 类矩阵中的未收到 family 显示等待/未收到。
- 真实导航/控制帧进入 MockLink wire path 后，总览/导航趋势有效样本计数增加，数值与 Fact 一致。
- 健康趋势只使用真实 resource taskId=0 的 CPU/WCET/stack 与 storageFreeKib；resource 实例年龄超时或 validity bit 缺失时形成断点。
- RC 链路趋势只使用有效链路实例 RSSI/质量/丢包和真实 RC 接收年龄；family freshness、实例 receiveAgeMs 或字段 validity mask 不满足时形成断点。
- family 超时/无效或对应导航能力 bit 缺失后写入断点；Canvas 不跨断点连线。
- 水平位置标准差要求 bit3，速度标准差要求 bit5 与 bit6 同时置位，航向标准差要求 bit2；新鲜消息但能力缺失时显示“未具备”，不显示数值。
- 样本上限为 60；通过切换 active Vehicle 和 boot epoch 两种真实路径验证清空历史。
- 页面隐藏时停止采样，不增加额外 Timer；趋势区明确标注 GCS 1 Hz 采样，不得标注为 FCU 原生历史。
- RC 有效轴按 mask 显示，清除 mask 位后对应轴不得继续显示最后有效位置。

### 8.5 安全与回归

- 不新增任何控制命令或写入路径。
- 既有 13 类遥测解码、分发、freshness、timestamp、boot epoch、profile 和多 Vehicle 隔离测试必须继续通过。
- `FlightTelemetryPageUITest` 保持 Integration / NoSanitizer，覆盖真实 wire path 和页面展示；导航 capability gap 测试必须在确认新 mask 到达 QML 后再记录采样计数基线，并等待后续 1 Hz tick，避免旧 mask 样本误满足断点断言。
- 最终在精确 main HEAD 上通过 Windows x64 MSVC Release、Unit、Integration、Executable、Installer、Clean Install、Installed Runtime 和证据归档。

## 9. 实施分期

> 当前页面结构和验收以第 2 节及 Phase 2.4 四页规范为准；以下 Phase 2/2.3 内容记录四页拆分前的单页基线与历史实现，不再作为当前页面布局要求。

### Phase 1 — 已实现（需随 main HEAD 持续回归）

- 更新顶部标题与只读标识。
- 增加四张摘要卡。
- 重构卡片视觉体系和 family 状态徽标。
- 建立 12/6/1 列响应式布局。
- 保留所有现有诊断字段和数据语义。
- 扩展 `FlightTelemetryPageUITest` 验证页面结构、摘要未知态和响应式列数。

### Phase 2 — 指挥中心视觉与真实数据可视化（代码已实现，验收待完成）

- 新增飞行态势、导航估计和 RC 输入三块主视觉面板；只使用现有有效 Fact 和 validity mask。
- 新增 13 类 family 独立状态/接收年龄矩阵，不生成综合健康分。
- 基于当前真实 Fact 增加 1 Hz GCS 本地采样趋势，最多 60 点；使用现有 500 ms Timer 的每第二个 tick，不创建新 Timer。
- 趋势数据严格 freshness/finite/non-negative gating；导航指标还必须遵守 capability mask：水平位置 bit3、水平+垂直速度 bit5+bit6、航向 bit2；缺失能力显示“未具备”并形成断点。
- 通过 active Vehicle 切换与 boot epoch 变化两条 Integration 路径验证趋势缓冲清空。
- 重新建立主视觉层级、紧凑摘要带、健康矩阵、趋势区和原有详细诊断卡片之间的版式。Phase 2.1 将矩阵优化为 7/4/2/1 列，并压缩摘要卡、状态单元、趋势卡与主视觉卡留白。
- 保留全部 14 张诊断卡片、字段、FactGroup、原有 objectName 和 MockLink wire-path 测试。
- 没有姿态角遥测，因此不绘制人工地平仪；RC 输入条不能被误认为飞机姿态。

### Phase 2.3 — 单页参考图布局迁移（历史方案，已由 Phase 2.4 四页重构替代）

- 直接把用户确认的参考图布局移植到现有单页：标题/分区条/六项 KPI/左中右三栏主视觉/趋势带。
- 左栏呈现飞行状态和六个关键遥测域；中栏呈现静态系统拓扑、六个真实状态节点和导航估计；右栏呈现真实事件优先级与 RC 四轴输入。
- 趋势带下方保留 13 类状态矩阵、状态图例和原有 14 张详细诊断卡，不删除任何已有字段。
- 四段分区条仅用于视觉复刻，不创建四个子页面，不改导航结构。
- 禁止为贴近参考图而伪造综合健康分、电池、卫星数、HDOP、振动或吞吐量。
- UI Integration 增加参考图布局选择器和 1600/1000/700/400 px KPI/分区条断点断言；所有真实数据状态与旧测试继续保留。

### Phase 3 — 原生姿态/更多时序遥测（未来独立评估）

只有 Flight ICD 和发布端提供真实姿态角、原生采样时间序列及明确有效性语义后，才评估真实人工地平仪、航迹/姿态曲线和更多原生趋势。必须先完成 Flight 与 GroundControl 的协议一致性及回归测试。

## 10. 变更边界

第二阶段允许修改：

- `src/FirmwarePlugin/Flight/FlightTelemetry.qml`
- `src/MainWindow/MainWindow.qml`（仅 Flight 配置工具栏高度/底色与选择器）
- `src/Toolbar/FlyViewToolBar.qml`、`src/Toolbar/MainStatusIndicator.qml`（仅 Flight 配置品牌与状态背景皮肤）
- `src/Vehicle/VehicleSetup/VehicleConfigView.qml`、`src/QmlControls/SettingsButton.qml`（仅 Flight 配置侧栏皮肤）
- `test/QmlUITests/FlightTelemetryPageUITest.cc`
- `doc/gcs/FCU_TELEMETRY.md`
- 本设计规范、Windows Release 验收规范和机器可读 manifest
- 必要的 QML UI 测试辅助文件

未经单独批准，不修改：

- `src/FirmwarePlugin/Flight/FlightTelemetryFactGroup.*`
- MAVLink 消息定义、Flight ICD 和生成头文件
- Flight firmware 发布端
- 控制权限、ARM、FMS、Permit、Failsafe 或执行器逻辑

## 11. Phase 2 实施记录（2026-11-14）

- `FlightTelemetry.qml`：增加飞行态势、导航估计、四轴 RC 输入、13 类遥测域矩阵和 4 个趋势卡片；页面采用深色指挥中心视觉体系。
- 趋势采样复用现有 500 ms 页面 Timer 的每第二次 tick，采样频率 1 Hz；仅保留最近 60 个 GCS 观察点。
- 趋势采样必须同时满足 operation boot epoch 新鲜、对应 family 新鲜、Fact 有效、数值有限且非负；导航精度还必须满足水平位置 bit3、速度 bit5+bit6、航向 bit2；不满足时写入断点。
- `FlightTelemetryPageUITest` 验证 capability mask 缺失时摘要/主视觉/详细行不显示数值，并验证对应趋势断线；采样断点断言以新 mask 已到达 QML 后的样本计数为基线；另验证切换 active Vehicle 后缓存清空。
- Vehicle 或 operation boot ID 变化时清空本地趋势缓存；不跨飞行器或启动周期连接曲线。
- RC 输入按四轴有效位掩码显示；无效轴不绘制填充，不将无效值当作 0。
- `FlightTelemetryPageUITest` 通过 MockLink 原始 MAVLink wire path 验证四轴数值、mask 清除、13 类 family 展示、四条趋势数据、过期断点和 boot epoch 缓存重置。
- 当前阶段不提供姿态角遥测，因此明确不渲染人工地平仪；不新增任何飞行控制/校准/确认操作。
- 当前状态：代码和测试已实现；摘要在显示新鲜值时仍保留“已接收”状态说明，过期时改为“数据超时”；Windows Release / Unit / Integration / Installer 等最终验收结果必须以本次提交对应的最新 CI 和本地 RUN_ID 证据为准。

## 12. Phase 2.1 紧凑布局实施记录（2026-11-14）

- 13 类遥测状态矩阵改为宽屏 7 列、中屏 4 列、窄屏 2 列、极窄屏 1 列，宽屏从 3 行压缩为 2 行。
- 状态单元高度调整为 54 px；摘要卡高度 76 px；趋势卡高度 126 px、绘图区 56 px；主视觉卡内边距 12 px；页面区块垂直间距 12 px。
- FlightTelemetryPageUITest 增加 1600/1000/700/400 px 四档矩阵列数断言，并逐项检查 13 个单元未越出矩阵网格边界。
- 本轮仅调整 QML 展示密度和 UI 回归，不改变 FactGroup、MAVLink/ICD、遥测 freshness、validity/capability mask、趋势采样和只读安全边界。
- 精确 main HEAD 的 Windows Release、Unit、Integration、Installer、clean-install、installed-runtime 和归档证据仍须等待新提交的 CI/本地结果；不沿用旧 HEAD 的取消或成功结果。


## 13. Phase 2.2 Windows Integration 失败定位与测试修正（2026-11-14）

main@866a8fbc9518249daf8824a850c017ebd487b188 的 Windows Release 结果：

- Configure：PASS。
- Release Build：PASS。
- Unit：198/198 PASS。
- Integration：69/72 PASS，失败项为 ToolbarIndicatorUITest、FlightTelemetryPageUITest、FlightRadioPageUITest。
- FlightTelemetryPageUITest 的失败断言是 `normalizedValue` 将 MAVLink `float` 转为 `double` 后仍使用精确相等比较；实际油门值 0.40000000596 与期望 0.40 在数值语义上相符。测试改为 1e-4 容差，不改变 RC 数据或 QML 显示逻辑。
- ToolbarIndicatorUITest 的 APM RemoteID 展开按钮在 500 ms 内未出现。测试现在先等待 Loader 页面确实提供 `_showExpand` 能力，再等待按钮可见；若失败会输出 `showExpand` 和 `expandedComponent` 诊断，避免把页面加载时序误判成产品缺陷。
- 本轮仅修正测试精度、测试时序和验收断言；不修改飞控诊断 QML、遥测 FactGroup、MAVLink/ICD 或只读边界。
- 新 HEAD 必须重新通过 FlightTelemetryPageUITest、ToolbarIndicatorUITest、FlightRadioPageUITest 和完整 Windows Release 门禁；本次旧 HEAD 的 Integration 失败不能被覆盖或标记为通过。


## 14. 字体与文字渲染

飞控诊断指挥中心必须遵循 [全项目字体系统与渲染规范](UI-字体系统与渲染规范.md)：

- 页面主标题和主视觉数值使用 `QGCLabel.typographyRole` 的 `hero` 角色。
- 英文眉题使用 `eyebrow`，辅助说明使用 `caption` / `secondary`，次级正文使用 `compact-body`。
- 不在页面内直接计算 `defaultFontPointSize * ratio` 或 `largeFontPointSize * ratio`。
- 深色指挥中心的既有语义颜色可以保留，但同一文字角色的亮度和状态颜色必须一致。
- 100%/125%/150% DPI 实机截图检查与响应式布局验收分开记录；CI 属性测试不得替代视觉验收。


## 15. Phase 2.3 单页参考图布局迁移记录（历史方案，已由 Phase 2.4 四页重构替代）

- 设计基线改为用户确认的「飞控诊断总览」参考图；本次只重构现有 FlightTelemetry 单页的布局与视觉，不拆分诊断页面。
- 页面顺序：标题/只读状态 → 四段静态视觉分区条 → 六项真实 KPI → 左/中/右三栏主视觉 → 四条真实 GCS 趋势 → 13 类遥测状态矩阵 → 原有 14 张详细诊断卡。
- 左栏不显示虚构健康分；中栏拓扑仅为静态连接示意；右栏事件来自 recentEvents，RC 输入继续受 validity mask 和 freshness 约束。
- 当前接口未提供的电池、卫星、HDOP、振动、吞吐量等指标不得为了模仿参考图而展示。
- 代码和 UI Integration 断言随本轮提交更新。Windows Release/Unit/Integration/Installer 等最终验收结果以新提交的 exact-HEAD CI 和用户本地 RUN_ID 为准；未取得证据前不得标记 PASS。

## 16. 2026-11-15 四页面重构实施状态

- 当前实现以 GitHub `main` 最新 HEAD 为准；本阶段将用户提供的四张 1672×941 参考图落实为四个可交互页面，而非单页静态分区条。
- 总览页聚焦飞行状态、系统拓扑/导航和真实事件；飞行健康页聚焦 13 类矩阵、安全监测和维护信息；导航时序页聚焦导航能力与时间同步；遥控与链路页聚焦双摇杆、四个逻辑轴、链路质量与失控保护。
- 代码提交成功不代表构建或测试通过；Windows x64 MSVC Release、Unit、Integration、Installer、clean-install、installed-runtime 和证据归档必须按 exact-HEAD CI 记录。
- 用户本地视觉验收需逐页使用 1672×941 参考画布，并分别记录 100%/125%/150% DPI；重点检查各页布局比例、中文字体清晰度、卡片内容边界、趋势断点和 14 张明细卡完整性。
- Windows 实机截图与环境记录尚未提交前，像素级视觉验收保持 NOT_RUN；不得用 CI 布局断言代替实际 DPI 观感签收。

## 17. Phase 2.4 内容自适应与可读性加固（2026-11-15）

- 六项 KPI 卡片从固定 76 px 高度改为内容驱动高度，76 px 仅作为最小高度；标题、数值和详情文字换行时卡片自然增高。
- 为六张 KPI 卡片内部内容布局增加稳定选择器，UI Integration 检查 1600 px 与 1400 px 宽屏下内容高度和边界，避免文字被压缩或溢出。
- 右侧告警事件条目改为根据原因文本的实际布局高度增高，避免多行原因与下一条事件重叠。
- 不改变遥测来源、family freshness、validity/capability mask、趋势采样、只读边界或现有 14 张详细诊断卡。
- 本阶段 Windows x64 MSVC Release、Unit、Integration、Installer、clean-install 和 installed-runtime 仍须以新提交的 exact-HEAD 证据判定；CI 未完成前不标记 PASS。


## 18. Phase 2.4 四张参考图像素级复刻（2026-11-15）

- 用户上传四张 1672×941 原图，分别定义「总览总控」「飞行健康」「导航时序」「遥控与链路」四页；页签是真实交互，不是静态装饰。
- FlightTelemetry.qml 增加 _activeDashboardPage，四个页签可点击；页面标题、英文副标题、六项 KPI 和底部趋势内容随页面同步切换。
- 总览页保留真实飞行状态、系统拓扑和最近事件；RC 操纵输入迁移到独立遥控页。
- 飞行健康页采用左侧状态概览、中间 13 类矩阵、右侧安全监测和维护横条；不显示参考图中未由协议提供的 96/100 健康分。
- 导航时序页仅使用导航能力、精度、UTC 和时钟纪律字段；不伪造卫星数量、HDOP 或卫星天图。
- 遥控与链路页使用 FlightRadio 同一 FlightRadio.mode 设置和 Mode 1/2/3 通道映射；仅显示四个逻辑轴和 validityMask 有效的链路指标，不伪造 CH5–CH8、SNR、RTT 或吞吐量。
- 趋势面板复用现有 1 Hz / 60 点采样器：总览页显示导航/控制指标，健康页显示 resource CPU/WCET/stack 和 storage free，导航页第 4 张切换为 UTC 不确定度，RC 页切换为 RSSI/质量/丢包/RC 接收年龄；resource/link 实例过期时必须断点。
- UI Integration 对四页切换、各页主要面板、响应式 12/6/1 列、健康矩阵在 1600/1000/700/400 px 下的 4/2/1/1 列与 13 个单元边界、维护横条 3/2/1/1 列、Mode 1/2/3 映射、Mode 2 双摇杆读数和 RC marker mask 清除行为进行回归。
- 视觉验收仍需在 Windows x64 实际运行应用，对照四张原图逐页检查 1672×941、100%/125%/150% DPI；CI 布局断言不能替代像素级人工验收。
- Exact HEAD Windows Release、Unit、Integration、Installer、clean-install、installed-runtime 和本地 RUN_ID 证据必须重新执行；未完成前 FGC-0044 保持 RETESTING。


## 19. 本轮视觉构建增量（2026-11-15）

- Flight 配置工具栏桌面高度调整为 56 px，使用深青蓝背景；在真实 QGC Logo 与原有状态指示器之间增加中英品牌锁定区。
- Flight 配置左侧导航宽度调整为 160 px，背景和选中态采用参考图深青蓝/青色边框；原有组件入口、点击行为和权限约束保持不变，不添加无功能的导航项。
- 六项 KPI 从六张独立浮动卡片改为一个连续边框带，桌面 6 等分，使用资源内 SVG 图标；窄屏仍保留 3/2/1 列响应式布局。
- HeroPanel、StatusCard、HealthCell、TrendCard、RC 摇杆轨道采用更接近参考图的深青蓝面板；主面板圆角后续统一收敛到 8 px，矩阵单元保持 6 px。
- `FlightTelemetryPageUITest` 新增工具栏高度、品牌锁定区、侧栏宽度/Flight 专属按钮皮肤，以及 1600/1400/1000/900/700/400 px KPI 带容器与内部网格边界断言；原有真实 MockLink 数据链路、freshness/validity/capability 语义和只读边界不变。
- 本轮不宣称 1:1 像素验收完成；必须在 1672×941 与 100/125/150% DPI 下取得运行截图，进行叠加差异检查后才能关闭 FGC-0044。


## 20. Phase 2.4 桌面视觉密度与面板几何微调（2026-11-15）

- 以四张 1672×941 参考图为基准，桌面诊断内容根布局段间距统一调整为 12 px，避免标题、页签、KPI 和主面板之间的节奏过松或不一致。
- KPI 连续指标带与主 HeroPanel 圆角统一为 8 px；HeroPanel 内边距调整为 9 px，内容行间距调整为 7 px，尽量贴近参考图的紧凑指挥中心密度。
- 不改变四页的 3/5/4、3/6/3、3/4/3/2 等桌面列宽分配、真实数据来源、状态语义或 RC 手型映射。
- FlightTelemetryPageUITest 在 1600 px 桌面视口新增内容根间距、KPI 带圆角和首个主面板圆角断言；既有四页切换、13 类健康域、响应式边界和双摇杆测试继续保留。
- 该增量属于视觉几何收敛，不等同于像素级验收。必须继续使用 Windows 实际运行截图，在 1672×941 下与四张参考图叠加比较，并分别验收 100%/125%/150% DPI；没有截图证据不得关闭 FGC-0044。


## 21. Phase 2.4 页签图标资源统一（2026-11-15）

- 四个交互页签由 Unicode 字符图标改为项目资源内 SVG：总览使用 dashboard、飞行健康使用 heart、导航时序使用 location-current、遥控与链路使用 radio。
- 统一通过 `QGCColoredImage` 以 20×20 布局盒渲染，选中/未选中颜色沿用页签强调色；图标不再依赖系统字体 glyph fallback，减少 Windows/Linux、不同 DPI 下的字形差异。
- `FlightTelemetryPageUITest` 为每个页签验证 SVG 资源路径与图标尺寸，同时保留四页切换、真实 MockLink 数据、状态语义、只读边界和原有响应式断点。
- 该修改是图标一致性收敛，不代表四张参考图已完成 1:1 像素验收；1672×941 截图叠加和 100%/125%/150% DPI 实机验收仍为必需项。


## 22. Phase 2.4 趋势图表现与面板几何精修（2026-11-15）

- 趋势图改为分段面积折线：每一段连续有效样本独立填充渐变和绘制圆角折线；遇到 null/NaN/无效样本立即结束当前段，不跨缺口补线、不插值。
- 最新样本有效时绘制浅色核心点与同色柔和光晕；最新样本无效时只保留前序有效段并显示 GAP，不把旧样本伪装为当前值。只有一个有效样本时绘制单点，刻度范围仍等待至少两个有效样本。
- 绘图区高度提升至 64 px，增加五条时间分隔线、三条值参考线，以及“较早/最新”时间轴；保留 1 Hz、60 点上限、真实 Fact、freshness/capability/validity 准入规则。
- 主视觉卡、趋势卡、详细诊断卡和页签统一为 8 px 圆角；13 类健康矩阵单元使用 6 px 次级圆角；RC 双摇杆卡使用 8 px。
- `TrendCard` 暴露 `hasRenderableSeries` 和 `gapCount` 只读属性，便于 UI Integration 验证数据可绘制状态与断点数量；不改变任何遥测值或采样器行为。
- 当前阶段按用户要求暂缓逐次 CI；后续集中执行 Windows Release、Unit、Integration、Installer、clean-install、installed-runtime 和证据归档。该安排不降低正式验收门槛，也不将未运行测试记录为 PASS。


## 23. Phase 2.4 页签交互反馈（2026-11-15）

- 四个页面入口增加轻量 hover 背景/描边反馈，未选中页签悬停时提升对比度，指针移出恢复默认态；当前选中态优先，不会被 hover 覆盖。
- 背景色与边框色采用 140 ms 过渡，鼠标仍使用指针样式；点击只修改 _activeDashboardPage，不创建路由或触发飞控命令。
- UI Integration 检查每个页签 MouseArea 开启 hover、未选中 hover/idle 颜色和点击后的 active 颜色，避免交互态在后续样式调整中退化。


## 24. Phase 2.4 标题锁定区响应式排版（2026-11-15）

- 1672 px 桌面标题区保留中文主标题与英文副标题同一行，副标题在空间不足时省略，不挤压只读状态徽标。
- 760–1279 px 中等宽度将英文副标题移至主标题下方独立一行，保留页面英文识别信息，同时避免与中文标题和右侧只读状态徽标争抢横向空间。
- 小于 760 px 时隐藏英文副标题，优先保证中文页面标题与只读状态完整显示。
- FlightTelemetryPageUITest 覆盖 1600/1000/900/700 px 的副标题显示策略，并将当前可见标题元素纳入 header bounds 检查。
- 该改动仅调整排版，不改变页面标题、状态字段、数据来源、四页切换和只读安全边界；CI 继续按用户要求集中执行。


## 25. Phase 2.4 四页诊断明细归属与去重（2026-11-15）

- 14 张详细诊断卡按当前四个业务页签归属，不再在每个页面底部重复展示全部卡片；切换页签时，明细标题和可见卡片同步切换。
- 总览总控：操作与飞行模式、最新事件、最近事件历史、固件与版本（4 张）。
- 飞行健康：传感器健康、控制器状态、执行器状态、解锁前检查、实时资源、存储状态（6 张）。
- 导航时序：导航状态、时间同步（2 张）。
- 遥控与链路：通信链路、遥控输入（2 张）。
- 14 张卡片的原始 Fact、validity/capability/freshness 门控、状态颜色、只读行为和 objectName 均保留；仅调整可见性和归属，不删除字段或更改协议语义。
- UI Integration 对四页分别断言 4/6/2/2 张卡片的可见集合及动态明细标题，防止重复展示或遗漏诊断内容。
- 该调整用于降低每页信息噪声和纵向滚动长度；CI 与 Windows 运行视觉验收继续集中后置，不将源码断言视为像素级验收。


## 26. Phase 2.4 导航与遥控明细双列平衡（2026-11-15）

- 导航时序页的「导航状态」与「时间同步」明细卡在 12 列桌面网格各占 6 列，左右等宽并同排；在 6 列中屏各占 3 列，在窄屏各占 1 列纵向排列。
- 遥控与链路页的「通信链路」与「遥控输入」明细卡采用相同 6/3/1 列规则，充分利用桌面横向空间，避免两张卡仅占内容区三分之二。
- UI Integration 在导航页与 RC 页分别检查两张卡同排、左右顺序及宽度至少占内容网格 45%，同时继续验证 4/6/2/2 卡片可见集合。
- 仅调整卡片宽度，不改变诊断字段、有效性门控、遥控通道映射或安全边界。


## 27. Phase 2.5 健康矩阵语义与详情卡空间平衡（2026-11-15）

- 健康页顶部三块区域分别为「遥测域概览」「遥测域接收矩阵」「安全监测」。不再用单一 operation/sensor/prearm 数据域的状态徽标代表多域综合状态；这三块面板不显示单域状态徽标，各字段与矩阵单元继续独立显示自身状态。
- 13 域矩阵的语义明确为消息可用性与接收新鲜度：已接收、数据超时、等待遥测。矩阵不代表传感器内部健康、字段有效性、解锁前检查或设备故障结论；这些信息以对应明细为准。
- 健康详情卡第一行传感器/控制器/执行器各占 4/12 列；第二行解锁前检查占 6/12 列，实时资源与存储各占 3/12 列。6 列中屏时解锁前检查占整行、资源和存储各半行；窄屏单列堆叠。
- UI Integration 验证矩阵语义标题、三种真实 freshness 状态、字段语义说明、无单域冒充综合状态的徽标，以及 1600/1000/700/400 px 下详情卡行列与宽度。
- 仅调整诊断展示层，不修改 13 类消息、Fact、有效性掩码、freshness 门控、解锁逻辑、控制权限或只读边界。


## 28. Phase 2.5 主视觉卡片同排等高（2026-11-15）

- 总览页整机状态、系统拓扑与导航估计、告警事件三个主视觉面板在桌面宽屏同排等高，面板内部内容保持各自自然高度与顺序，不裁切、不隐藏。
- 飞行健康页遥测域概览、13 域接收矩阵、安全监测三个主面板在桌面宽屏同排等高，避免卡片底边参差。
- 等高能力通过 HeroPanel 的 fillRowHeight 显式启用，仅作用于上述六个主视觉面板；其他页面与诊断卡保持原有自然高度和顶部对齐。
- UI Integration 对总览与健康页分别验证三个面板同排、等高；中屏与窄屏继续由现有 6/1 列布局自然堆叠。


## 29. Phase 2.5 导航时序主面板平衡与跨域状态校验（2026-11-15）

- 导航时序页桌面主视觉调整为四块等宽面板：导航估计总览、导航能力与定位精度、时间同步、状态校验，各占 12 列网格中的 3 列；通过 HeroPanel.fillRowHeight 保证同排等高。
- 状态校验面板同时覆盖 navigation 与 time 两个数据域，不再用 navigation 单域徽标代表跨域状态；改为两个独立 HealthCell，分别展示消息可用性与地面站接收年龄，并保留位置/速度/航向能力和 UTC 状态检查。
- 时间同步面板与详细卡补充 sourceMappingEpoch（时钟源映射代次），与 timeEpoch、FCU 发布时间和接收年龄一起用于观察时钟映射更新，不推导或伪造 UTC 偏差。
- 12 列桌面下四块主面板同排等宽等高；6 列中等宽度与单列窄屏下按全宽纵向排列。导航状态和时间同步两张详细卡继续遵循 6/6、3/3、1/1 列规则。
- UI Integration 增加导航页四块主面板同排/等宽/等高、跨域状态单元无单域徽标、sourceMappingEpoch 字段，以及 1600/1000/700/400 px 下主面板与明细卡的排列断言。
- 仅调整 FlightTelemetry 展示层和 UI Integration 断言，不修改 MAVLink、Flight ICD、FactGroup 数据分发、能力掩码、family freshness 或只读安全边界；按用户要求本轮不触发 CI，后续集中执行正式验证。


## 30. Phase 2.5 飞行健康页中屏与窄屏专项适配（2026-11-15）

- 飞行健康页顶部三张主面板在桌面宽屏（availableWidth ≥1280）采用 3/6/3 的 12 列布局；中屏（760–1279）采用「遥测域概览 3 列 + 遥测域接收矩阵 3 列」同排，安全监测 6 列独占下一行；窄屏（<760）三张面板按 1/1/1 单列堆叠。
- 飞行健康页专属详情网格在 activeDashboardPage=1 时采用与页面一致的 12/6/1 列断点（1280/760）；其余三个页面仍沿用原有 dashboardColumns 规则，不因本次健康页适配改变。
- 健康详情卡桌面继续采用传感器/控制器/执行器 4/4/4，解锁前检查/实时资源/存储 6/3/3；中屏采用 3/3/3 与 6/3/3；窄屏所有详情卡单列堆叠。
- 解锁前检查每项的原因和恢复条件改为完整换行展示，不再使用单行省略，避免隐藏安全恢复条件。
- 遥测域概览标题使用中性色，避免把 operation 单一数据域颜色误用为 13 域汇总状态颜色；各域单元和详情仍使用自身真实状态颜色。
- UI Integration 在 1600/1000/900/700/400 px 验证健康页主面板 3/6/1 列策略、详情网格 12/6/1 列策略、各详情卡行列关系、13 个矩阵单元边界、预检原因完整换行及概览标题中性色。
- 本轮只调整飞行健康页布局与呈现；不修改遥测协议、字段定义、状态 freshness 门控、传感器实例健康判定、解锁逻辑、安全权限、趋势采样来源或其他三个页面。


## 31. Phase 2.5 健康详情的实例新鲜度门控（2026-11-15）

- 实时资源详情逐实例检查 resourceAvailable 域状态和实例 receiveAgeMs；只有域状态已接收且实例年龄在 resource freshness 窗口内，才显示 CPU/WCET/栈/截止期/AXI 数值。实例超时、等待或域级数据不可用时，数值区域明确显示对应状态，避免把旧样本误读为实时值。
- 安全监测中的「执行器可用掩码」受 actuatorAvailable 域新鲜度门控；执行器消息未接收或超时，显示域状态，不沿用旧掩码作为当前可用能力。
- resource 趋势仍沿用同一实例 freshness 与 validity mask 规则；未改变 taskId=0 系统汇总选择、字段 validity bit、趋势断点和采样来源。
- UI Integration 增加执行器安全摘要与执行器主域状态一致性断言；资源明细提供 taskId 级 objectName，便于后续针对实例年龄边界扩展验证。
- 本轮仅影响飞行健康页的资源详情和安全摘要，不修改资源协议、其他页面的链路/遥控/导航逻辑或飞控安全执行行为。


## 32. Phase 2.6 总览总控页二次适配（2026-11-15）

- 本阶段仅调整「总览总控」页的展示结构，不重做「飞行健康」「导航时序」「遥控与链路」三个页面，不改变四页入口、KPI 语义或页面专属详情归属。
- 中央系统拓扑从单纯静态机体线框升级为六个真实遥测域状态节点：传感器、导航、链路、控制、执行器和时间；节点连接线只表达系统关系，机体图仍是示意图，不表示实时位置、姿态或航迹。
- 六个节点的颜色直接绑定各自 family freshness，节点标签同时显示状态与地面站接收年龄；状态随现有 500 ms 页面年龄刷新同步重绘，不增加轮询频率。
- 节点网格按内容宽度采用 3/2/3/2 列响应式策略；导航精度指标在宽屏三列、中等窄栏单列、堆叠宽屏三列、小屏单列显示，避免中等宽度下内容挤压。
- 右侧告警列表按真实严重度降序（严重、错误、警告、信息），同级按地面站本地接收序号降序；最多显示四条。底层 recentEvents 的最新优先顺序不被修改，事件历史与最新事件详情仍保留原有时间顺序。
- 总览三栏保持 3:5:4 桌面列宽和同排等高；中等宽度按六列网格重排，窄屏单列堆叠。六项 KPI、四张真实趋势图、四张总览详情卡和全部原始字段继续保留。左侧「整机状态概览」是多域复合面板，不再以 operationAvailable 单域徽标代表整机状态；飞控操作、传感器、导航、链路、资源和时间仍各自显示真实状态。
- FlightTelemetryPageUITest 增加告警优先级/同级序号排序断言、六个拓扑域状态/接收年龄节点断言、三栏宽度顺序与边界、拓扑节点高度和 3/2/3/2 响应式断言；本轮进一步验证 1600/1400/1000/900/700/400 px 六档下每个节点均在网格边界内、导航节点 fresh → stale → recovered 的状态颜色变化，并断言整机多域复合面板不显示 operation 单域徽标。
- 遥测模型、Fact、MAVLink/Flight ICD、family freshness、validity/capability mask、boot/profile 隔离和只读安全边界均不变。
- 按用户要求，本阶段不运行、不触发 CI；新增回归断言保留，集中 CI 与 Windows 1672×941 / 100%/125%/150% DPI 视觉验收后置。当前测试状态为 NOT_RUN，不宣称 PASS 或像素级验收完成。


## 33. Phase 2.6 遥控与链路页均衡与语义收尾（2026-11-15）

- 遥控输入/逻辑轴与链路质量/失控保护按两行组织：桌面 12 列网格分别采用 6/6、8/4；760–1279 px 的 6 列网格分别采用 3/3、4/2；低于 760 px 单列排列。
- 同一行的两张 HeroPanel 显式启用 `HeroPanel.fillRowHeight`，分别验证等高；两行宽度分别验证 1:1 与 2:1 比例，不得产生横向裁切。
- RC 摇杆读数保留 RC family freshness 与逐轴 validity mask 的语义：等待、超时、未上报、无效均明确显示；无效轴不绘制 marker。
- 链路质量面板不使用 family 单状态徽标概括多个链路实例；失控保护面板不使用 operation 单域状态冒充 RC/链路综合状态。
- 每个链路实例以三个独立指标格显示 RSSI、链路质量和丢包率；每格独立服从实例接收年龄和字段 validity mask。
- 链路趋势选择顺序为新鲜 GCS、伴随计算机、其他；若已有非 RC 链路实例但全部超时，保留超时/断点；只有没有非 RC 实例时才回退 RC。趋势标题同步标明实际链路类型。
- MockLink Integration 覆盖 RC/GCS/其他三类实例、有效与无效指标、GCS 优先于 RC 的趋势选择、非 RC 链路过期时不回退 RC、四面板桌面均衡和中窄屏重排。仅更新 GroundControl 展示层，不修改 MAVLink、FlightTelemetryFactGroup、ICD、飞控控制或校准行为。
- 本轮不执行 CI；Windows Release、Unit、Integration、Installer 和 1672×941 / 100/125/150% DPI 视觉验收集中后置，未执行前均不得标记 PASS。


## 34. Phase 2.6 导航时序 UTC 绝对时间与趋势有效性闭环（2026-11-15）

- 导航时序主面板和时间同步明细卡增加 FCU UTC 绝对时间，来源仅为 FCU_TIME_STATUS.unix_time_us；按 UTC ISO-8601 形式展示，并保留微秒精度。
- UTC 时间展示同时受 time family freshness 与 utc_valid 门控：域未接收/超时显示对应状态，utc_valid=false 显示无效，缺少字段显示未上报；不得沿用旧 UTC 时间作为当前值。
- UTC 不确定度继续只在 time family 新鲜且 utc_valid=true 时显示；时间趋势与 UTC 绝对时间共用有效性条件，无效样本产生断点，不插值、不跨段连接。
- UI Integration 使用 MockLink 发送 FCU_TIME_STATUS，验证 UTC 时间格式、微秒保留、无效时隐藏时间、恢复有效后重新显示，以及时间趋势有效/断点/恢复闭环。
- 仅修改地面站 QML 展示、UI Integration 回归断言与 SPEC；不修改 FCU_TIME_STATUS 协议、FactGroup 解码、时间服务、飞控时间纪律算法或控制权限。
- 按用户要求本轮不运行、不触发 CI；新增回归断言保留到集中 CI 执行，未执行前状态为 NOT_RUN。


## 35. Phase 2.6 UTC 有效性状态颜色与 freshness 语义统一（2026-11-15）

- 导航时序主面板、跨域状态校验和时间同步明细卡统一使用 UTC 有效性文案与颜色：time family 已接收且 utc_valid=true 为绿色；utc_valid=false 为红色；time family 超时为琥珀色；等待/未上报为中性灰。
- UTC 有效性文字和 UTC 绝对时间均服从 time family freshness，不再在 time 消息超时后显示旧的“UTC 有效”布尔值或旧时间戳。
- UTC 不确定度与 UTC 时间戳采用同一状态颜色门控，避免 time family 新鲜但 UTC 无效时仍显示绿色。
- UI Integration 验证 UTC 有效、无效、恢复三态的文字和颜色，并继续验证 UTC 绝对时间与时间趋势断点。
- 仅修改 GroundControl 展示层、UI Integration 断言与 SPEC，不修改协议、FactGroup、时间服务或飞控逻辑；本轮 CI 仍后置集中执行。


## 36. Phase 2.7 传感器与解锁前检查逐实例新鲜度（2026-11-15）

- 传感器实例状态、动作、原因和 gap/reject 计数明确区分当前有效值与最近一次上报值；实例超过 sensorHealth freshness 窗口后显示「数据超时 · 最近状态：…」，原因/动作改为「最近上报」，保留诊断上下文但不冒充当前状态。
- 传感器实例覆盖率只将 family 与实例 receiveAgeMs 均新鲜的数据计为有效，并单独显示超时和等待实例数；不再把已缓存实例数量直接等同于当前健康实例数。
- FCU_PREARM_STATUS 按 checkId 独立记录本地 receiveElapsedMs；FactGroup getter 动态导出 receiveAgeMs，QML 使用既有 500 ms age tick 刷新逐项年龄，不新增定时器或通信轮询。
- 解锁前检查每项独立执行 prearm freshness 门控；过期结果显示「数据超时 · 最近结果：通过/失败/警告」，原因与恢复条件明确标注为最近上报；摘要与覆盖率只统计当前新鲜项，分别报告超时和未收到项。
- MockLink Integration 注入真实 FCU_SENSOR_HEALTH 与 FCU_PREARM_STATUS 帧，验证 per-instance/per-check receiveAgeMs、fresh/stale 文案、健康/通过结果、原因与恢复条件、1/4 传感器覆盖率和 1/15 预检覆盖率。
- 本阶段仅调整 GroundControl 接收时间记录与飞行健康页展示；不改变 MAVLink 字段、飞控检查判定、预检恢复条件、安全权限、传感器健康策略或其他三个诊断页面。


## 37. Phase 2.7 控制器与执行器详情 freshness 闭环（2026-11-15）

- 控制器详情卡的求解器状态、控制序号、求解耗时、可控/降级 DOF、原因和 FCU 发布时间统一受 controlAvailable family freshness 门控；超时/等待时显示对应状态，不把缓存 Fact 当作当前控制状态。
- 执行器详情卡的控制序号、输出许可代次、可用/饱和掩码、输出策略、反馈能力、原因和 FCU 发布时间统一受 actuatorAvailable freshness 门控。
- 安全监测面板中的控制求解器摘要与控制器详情使用相同 freshness 语义。
- 新增 _familyFreshValue 展示层 helper，不修改 freshness 窗口、Fact、MAVLink、Flight ICD 或飞控状态判定。
- 集中验收需覆盖 control/actuator fresh → stale → recovered；本轮 CI 未运行，状态 NOT_RUN。
- 仅修改飞行健康页展示与 SPEC，不影响其他三个页面。


## 38. Phase 2.8 总览告警事件时间定位（2026-11-15）

- 总览优先级告警列表与最近事件历史均展示 FCU_EVENT.time_boot_us 对应的 FCU 启动相对时间，格式为「FCU +x.xxx s」；该字段是 FCU 单调启动时间，不是 UTC 或地面站本地时间。
- 告警条目保留严重度、地面站本地接收序号、事件 ID、来源 ID 和原因；新增时间只增强定位能力，不改变严重度降序/同级序号降序规则。
- 事件历史仍按 recentEvents 的原始最新接收顺序显示；时间字段独立列出，不改变事件排序或底层缓存顺序。
- time_boot_us 缺失或超出 JavaScript 安全整数范围时明确显示未上报/无效，不格式化为伪造时间；事件本身仍保留在列表中。
- MockLink UI Integration 注入 FCU_EVENT，覆盖有效、未上报、越界时间输入，并验证优先级告警与事件历史显示同一 FCU 启动相对时间。
- 仅修改地面站 QML 展示、MockLink UI Integration 断言和 SPEC/验收清单；不修改 MAVLink、FlightTelemetryFactGroup、ICD、飞控事件生成、UTC 时间服务或控制行为。
- 按用户要求本轮不运行、不触发 CI；新增断言状态为 NOT_RUN，待集中验证。


## 39. Phase 2.8 事件历史严重度与原因可读性（2026-11-15）

- 最近事件历史采用两层信息结构：首行显示本地序号、FCU 启动相对时间和严重度；次行完整显示事件 ID、来源 ID 与原因。
- 历史事件严重度复用优先级告警相同的颜色语义：信息青色、警告琥珀、错误橙色、严重红色、未知中性灰；不以颜色替代严重度文字。
- 原因改为自然换行，不再因单行省略而丢失故障定位上下文；窄屏下允许条目增高，避免横向裁切。
- UI Integration 通过 MockLink FCU_EVENT 验证严重度文案/颜色、原因文本和启动相对时间；测试状态 NOT_RUN，等待集中验证。
- 仅调整地面站事件历史展示和对应回归断言，不改变 recentEvents 原始顺序、严重度排序、事件缓存、协议或飞控行为。


## 40. Phase 2.8 事件序号的本地接收语义明确化（2026-11-15）

- FCU_EVENT 协议本身不提供地面站列表使用的 sequence；recentEvents.sequence 是 FlightTelemetryFactGroup 按接收顺序生成的地面站本地接收序号，不得误读为 FCU 原生事件序号。
- 总览告警列表、最近事件历史和排序提示统一标注「本地序号 / 本地接收序号」；同级严重度排序仍按该本地序号降序。
- 最新事件详情卡继续使用「本地接收序号」字段名，三处文案保持一致；事件 ID、来源 ID、FCU 启动相对时间与原因保持独立展示。
- MockLink UI Integration 验证告警列表和历史列表均显示本地序号，并保留原严重度排序结果。
- 仅调整 GroundControl 展示文案与 UI 回归断言，不修改 FCU_EVENT 协议、FactGroup 的序号生成或事件缓存顺序。


## 41. 旧版 SYS_STATUS 传感器状态 freshness 与通信状态联动（FGC-0049）

主状态指示器中的 Sensor Status 是不支持 Health and Arming Checks 飞控的兼容回退显示，数据来自 MAVLink SYS_STATUS。它必须遵循诊断大屏相同的“过期/缺失数据不得显示为健康”原则：

- SYS_STATUS 独立维护接收新鲜度，默认超时 5500 ms；该阈值与诊断大屏 sensor-health family freshness limit 对齐。
- VehicleLinkManager.communicationLost 变为 true 时，立即将缓存的传感器状态标记为过期并停止旧计时器。
- 通信恢复本身不能恢复传感器健康显示；必须收到新的 SYS_STATUS 后才允许重新显示 Normal、Error 或 Disabled。
- 数据过期时保留传感器名称，所有传感器状态统一显示 Stale；状态抽屉顶部明确说明通信丢失、数据过期或等待首次 SYS_STATUS。
- 即使 HEARTBEAT 仍在到达，只要 SYS_STATUS 超过 5500 ms 超时阈值，也必须显示过期状态。
- 该兼容回退显示仅用于地面站诊断，不授予 ARM、Permit、FMS 或执行器权限。

回归覆盖：
- SysStatusSensorInfoTest 使用短注入超时验证正常 → Stale，并验证通信丢失立即失效、通信恢复后等待新 SYS_STATUS 才恢复。
- VehicleLinkManagerTest::_simpleCommLossTest 通过真实 MockLink/VehicleLinkManager 信号路径验证断链时立即 stale，重连后由新 SYS_STATUS 恢复 fresh。
- ToolbarIndicatorUITest::_testLegacySensorStatusStaleOnCommLoss 验证主状态抽屉在 fresh 时隐藏提示、全链路断开时显示通信丢失提示、重连并收到新 SYS_STATUS 后再次隐藏提示。
- 本轮未运行 Unit、Integration、Windows Release 或 CI；相关回归状态均为 NOT_RUN，待集中验收。


## 42. Phase 2.9 KPI 辅助值 freshness 与状态颜色一致性（2026-11-15）

- 六项 KPI 的主值、辅助说明和强调色共享同一数据有效性语义；任何 family 未收到/超时、字段未上报或 UTC 无效时，辅助说明不得继续显示缓存的旧状态。
- 飞行健康页的传感器覆盖率复用逐实例 freshness 计数，显示有效、超时和等待实例数；不再把缓存实例总数当作当前有效数量。
- 执行器掩码辅助说明只有 actuator family 新鲜且掩码已上报时才显示；等待或超时显示对应状态。
- 系统资源辅助说明按 taskId 实例 receiveAgeMs 统计新鲜/超时/等待实例；family 不新鲜时不展示旧实例数。
- 导航页 UTC 辅助说明统一经过 time family freshness 与 utcValid 门控；UTC 无效使用红色，time family 超时使用琥珀色，恢复有效后回到绿色。
- 遥控页接收机 failsafe 辅助说明受 rcInput family freshness 门控；RC 超时后不保留旧的“未触发”文字。
- 链路实例摘要分别统计新鲜/超时/等待实例，避免将缓存实例数量解释为当前在线链路数量。
- MockLink Integration 增加 UTC 有效/无效/超时/恢复、RC failsafe 新鲜/超时、传感器覆盖率、资源实例覆盖率、执行器未接收和多链路实例新鲜度断言。
- 仅调整地面站 KPI 呈现和 UI Integration；不修改 FlightTelemetry 协议、FactGroup 解码、飞控健康判定、控制权限或任何飞行控制行为。本阶段按用户要求不运行 CI，所有新增回归待集中验收。


## 43. Phase 2.9 时间质量枚举与 UTC 有效性分色（2026-11-15）

- 「时间质量」强调色由 time family freshness 与 timeQuality 枚举共同决定：仅本地时钟为中性灰、同步中为琥珀色、锁定为绿色、保持/降级为琥珀色、无效为红色。
- time family 等待/超时时优先显示 family 状态颜色，不允许旧的 timeQuality 枚举覆盖等待/超时状态。
- 「UTC 不确定度」仍独立服从 utcValid：UTC 无效为红色，time family 超时为琥珀色，等待/未上报为中性灰；UTC validity 与 timeQuality 不互相代替。
- 总览页与导航时序页的时间质量 KPI 使用同一颜色映射；UTC 不确定度 KPI 使用独立 UTC validity 颜色映射。
- MockLink Integration 增加 FCU_TIME_STATUS.time_quality=locked/invalid 且 utc_valid=true 的组合断言，并验证 time family stale 时两项 KPI 均转为琥珀色。
- 仅修改地面站 QML、MockLink UI Integration 与 SPEC；不改变 FCU_TIME_STATUS、FactGroup、时间同步服务、飞控时间纪律算法或控制行为。本轮不运行 CI，新增回归留待集中验收。


## 44. Phase 2.9 总览与遥控页链路 KPI 实例覆盖率与状态颜色（2026-11-15）

- 总览「数据链路」和遥控与链路页「链路实例」KPI 主值统一显示新鲜实例数/已知实例总数，不再把缓存实例总数直接表达为当前在线链路数量。
- 辅助说明并列显示新鲜、超时、等待实例数；总览页额外保留链路 family 接收年龄，保留故障定位上下文。
- KPI 颜色按实例状态聚合：任一新鲜实例为 lost 时红色；存在超时/降级/恢复中时琥珀色；全实例健康且新鲜时绿色；等待/未知为中性灰。family stale/等待优先于实例缓存状态。
- 遥控与链路页的链路实例 KPI 复用同一聚合颜色规则，避免同一组链路在总览与专页呈现不同的健康色。
- MockLink Integration 覆盖三实例全部新鲜、GCS/其他实例超时而 RC 仍新鲜的 3/3 → 1/3 数量变化与颜色降级。
- 仅修改 GroundControl QML、UI Integration 与 SPEC/G15；不修改 MAVLink、FactGroup、链路状态生成或飞控控制逻辑。本轮不运行 CI，待集中验收。


## 45. Phase 2.9 顶部 KPI 窄屏辅助说明自适应（2026-11-15）

- 六项顶部 KPI 在内容宽度低于 760 px 时显示辅助说明；辅助说明使用 WordWrap，最多显示两行，超出两行才允许尾部省略。
- KPI Tile 高度由标题、主值、辅助说明的实际 implicitHeight 加内边距决定，不再仅使用固定 82 px 高度；桌面仍保持不低于 66 px 的基准高度。
- 400 px 单列布局和 700 px 双列布局必须保证文字区域位于 Tile 边界内，不能因固定高度裁掉第二行。
- 为辅助说明增加稳定 objectName 后缀 _detail，用于回归测试，不改变既有 selector。
- MockLink UI Integration 在 700/400 px 两个断点检查辅助说明可见、最大两行、wrapMode 生效，以及全部六个 KPI tile/content 不溢出。
- 仅修改 QML 布局、UI Integration 与验收文档；不改变遥测数据、健康判定、MAVLink 协议或控制行为。本轮不运行 CI，回归待集中验收。


## 46. SYS_STATUS 过期行保留最近状态上下文（FGC-0049）（2026-11-15）

- 通信丢失或 5500 ms freshness timeout 后，传感器行显示「Stale · Last: Normal/Error/Disabled」，保留最近一次状态作为历史上下文。
- 最近状态不得解释为当前健康；所有 stale 行统一使用琥珀色，顶部 freshness notice 明确说明数据已过期。
- 过期状态行顺序必须与传感器名称顺序一致：unhealthy → healthy → disabled。
- 新增 SysStatusSensorInfoTest 验证 stale 后保留 sensor names、last-known state 与顺序；ToolbarIndicatorUITest 验证主状态抽屉 stale notice 和状态行警告色。
- 不改变 SYS_STATUS 协议、VehicleLinkManager 丢包判定、ARM/Permit/FMS/执行器权限；本轮不运行 Unit、Integration、Windows Release 或 CI，新增回归为 NOT_RUN。


## 47. Phase 2.9 遥控链路实例指标窄屏重排（2026-11-15）

- 每条链路的 RSSI、链路质量、丢包率三个指标格按内容宽度响应式排列：>=760 px 三列，420–759 px 两列，<420 px 单列。
- 两列布局中丢包率指标跨两列占满第二行，避免产生半行空白；单列布局中三个指标各占整行。
- 三个指标仍独立使用各自 validity mask 和实例 freshness，不合并、不降级、不隐藏。
- MockLink UI Integration 使用真实 GCS 链路实例验证 700 px 时两列与丢包格整行，400 px 时三格全宽；恢复原始窗口宽度后恢复桌面布局。
- 仅修改 GroundControl QML 布局、UI Integration 和 SPEC/G15；不修改链路协议、FactGroup、链路状态语义或飞控控制行为。本轮不运行 CI，回归待集中验收。


## 48. Phase 2.10 飞行健康页 freshness 与状态表达补强（2026-11-15）

- 存储卡的 SD/NOR 状态、剩余空间、事件/日志丢弃计数、设置代次、原因和 FCU 发布时间统一受 storageAvailable family freshness 门控；字段本身继续遵守 storageValidityMask。family 等待或超时后不得展示旧启动周期或旧消息缓存值。
- 预检卡的检查代次、适用模式掩码、最近检查原因和 FCU 发布时间统一受 prearmAvailable family freshness 门控；逐项 checkId 的 receiveAgeMs 规则保持不变。
- 预检列表使用响应式 12 列网格：桌面/中屏按检查名称 3 列、状态 3 列、原因与恢复条件 6 列排列；小于 760 px 时改为单列纵向排列，三项均完整换行，不再挤压原因/恢复条件文本。
- 维护横条的最近事件颜色直接对应最新事件严重度；预检摘要仅在 15 项全部新鲜且全部通过时显示健康绿，失败红色、警告/超时琥珀色、等待/未完整上报中性灰。
- 系统汇总 CPU 和资源实例 CPU/WCET/栈/AXI 使用青色表示“有效测量值”，字段无效使用中性灰、family/实例超时使用琥珀色；颜色不代表 CPU 负载处于健康阈值，也不引入未定义的资源告警门限。
- 安全监测中的预检摘要与维护横条使用同一预检状态颜色 helper，避免同一数据在同页出现冲突颜色。
- MockLink UI Integration 覆盖 storage 新鲜无效 → family 超时、prearm 元数据超时、事件严重度颜色、预检未完整/失败/警告/超时颜色、资源 CPU 有效/无效字段颜色，以及 1600/1000/900/700/400 px 的预检行布局。
- 仅修改飞行健康页 QML、对应 UI Integration 断言与 SPEC/验收清单；不修改 MAVLink、FactGroup、Flight ICD、数据 freshness 阈值、控制行为或只读边界。本轮不运行 CI，新增断言状态 NOT_RUN，等待集中验收。


## 49. RC 页面 400 px 面板与双摇杆边界回归（2026-11-15）

- 四参考页响应式矩阵已覆盖 1600/1000/900/700/400 px；本轮补齐 RC 页面在 1000/900/700/400 px 下四个主视觉面板的垂直顺序、统一左边界和等宽约束。
- 在每个窄屏宽度下检查左右摇杆都完整位于「遥控输入监视」面板内部，且保持并排，不得越界或被裁切。
- 该补充只增强 UI Integration 断言，不改变 RC 摇杆绘制、FlightRadio 手型映射、有效轴 mask 或遥控输入数据。
- 本轮未运行 UI Integration、Windows Release 或 CI；新增断言为 NOT_RUN，待集中验收。


## 50. Phase 2.10 事件历史元信息响应式布局（2026-11-15）

- 最近事件历史的本地接收序号、FCU 启动相对时间、严重程度从固定宽度 RowLayout 改为响应式 GridLayout。
- 内容宽度 >=520 px 时三列同排；低于 520 px 时两列，序号与启动时间保持同排，严重程度移动至下一行右对齐。
- 序号与时间取消固定 preferredWidth 和 ElideRight，使用自然文本宽度参与网格分配，禁止单行省略；严重程度继续使用原有严重度颜色映射。
- 事件 ID、来源 ID 和原因详情仍独立保留并 WordWrap，不修改事件排序、接收序号含义、FCU 启动相对时间或事件严重程度。
- MockLink UI Integration 在 700 px 检查三列同排，在 400 px 检查两列及严重程度换行，并验证序号/时间宽度不小于文本 implicitWidth。
- 仅修改 GroundControl QML、UI Integration 与 SPEC/G15；不修改 MAVLink、FactGroup、事件缓存、协议时间戳或飞控行为。本轮不运行 CI，新增回归待集中验收。


## 51. 预检摘要完整覆盖率与 freshness 表达（2026-11-15）

- 解锁前检查摘要始终以 15 项为完整覆盖基准；只收到部分 checkId 时，必须同时显示有效项数、待更新项数和通过/警告/失败/未知计数，不得只显示已收到项的结果而隐去缺失项。
- 预检 family 未新鲜时，摘要和覆盖率显示 family 状态及最近缓存项数量，不得把旧缓存结果计入当前有效检查。
- family 新鲜但无任何逐项检查时，明确显示 0/15 有效、15 项未收到；不以单独的「已接收」代替检查覆盖状态。
- 只有 15 项全部新鲜且 checkState 全部为通过时，摘要才使用健康绿色；任一失败为红色，警告/逐项超时为琥珀色，等待、未知或覆盖不完整为中性灰。
- MockLink UI Integration 增加部分覆盖摘要、family stale 摘要、15/15 全通过绿色、单项警告转琥珀色的断言；本轮不运行 CI，新增断言为 NOT_RUN。
- 仅修改飞行健康页 QML、对应 UI Integration 和 SPEC/G15 验收清单；不修改 FCU_PREARM_STATUS 协议、FactGroup 数据源、检查规则或飞控解锁决策。


## 52. Phase 2.11 告警事件元信息无省略布局（2026-11-15）

- 总览告警事件列表的本地接收序号、事件 ID、来源 ID、FCU 启动相对时间与原因详情全部允许换行；不再对序号/ID/来源和 FCU 时间使用 ElideRight。
- 告警事件内容列与文本项允许收缩到可用宽度，避免长元信息撑破面板；换行后的 RowLayout implicitHeight 驱动告警卡片高度增长。
- 保留严重度标签、严重度排序、同级本地接收序号排序、FCU 时间语义、原因文本和只读边界，不改变事件数据或缓存。
- MockLink UI Integration 在 700/400 px 检查元信息和时间 wrapMode、告警卡片内容高度及文本右下边界不超出告警面板。
- 本轮只调整 FlightTelemetry QML、对应 UI Integration 与 SPEC/G15；未运行 CI，新增回归待集中验收。


## 53. 资源实例新鲜度覆盖表达（2026-11-15）

- 实时资源卡与传感器卡采用一致的实例新鲜度表达：显示新鲜、超时、等待三类实例数量，不只显示缓存数组总数。
- resourceAvailable family stale/等待时，覆盖行直接显示 family 状态；family fresh 时由现有逐实例 receiveAgeMs 聚合新鲜/超时/等待数量。
- 资源实例明细继续保留最近缓存行，但 CPU/WCET/栈/截止期/AXI 数值仍分别按实例 receiveAgeMs 与 validityMask 独立门控。
- MockLink UI Integration 验证单实例新鲜覆盖文本及 resourceAvailable family stale 转换；_resourceCoverageTextFor 的独立样本断言继续覆盖新鲜/超时/等待聚合。
- 仅修改飞行健康页 QML、对应 UI Integration 与 SPEC/G15；不修改资源消息协议、FactGroup 数据缓存或飞控资源统计。本轮不运行 CI，新增断言为 NOT_RUN。


## 54. Phase 2.11 六项 KPI 辅助说明逐项布局与强制双行回归（2026-11-15）

- 六项顶部 KPI 辅助说明均使用稳定的 `_detail` objectName；700 px 双列与 400 px 单列断点逐项验证可见性、WordWrap、最多两行、实际 lineCount 和 Tile 内边界。
- 通过测试注入一段长链路说明，强制产生两行文本，验证 `SummaryTile.implicitHeight` 随 `summaryContent.implicitHeight` 增长，内容容器和辅助说明均不越界、不裁切。
- 该测试只覆盖真实 QML SummaryTile 的布局行为，不改变 KPI 文本来源、遥测值、链路选择、状态颜色或控制逻辑。
- 本轮仅新增 MockLink UI Integration 断言和验收条目；按用户要求不运行 CI/Unit/Integration，所有新回归为 NOT_RUN，待集中验收。


## 55. Phase 2.13 操作、导航、时间与 RC 动态详情 freshness 收口（2026-11-15）

- 操作详情中的解锁状态、飞行模式、控制权、失效保护、任务状态、启动/配置 ID、代次、输出许可、请求 ID、原因和 FCU 发布时间统一服从 operation family freshness；等待/超时不保留旧值。
- 导航状态代次、原点 ID、能力掩码、位置/速度/航向标准差、数据年龄、原因和 FCU 发布时间统一服从 navigation family freshness；先检查 family freshness，再检查 capability mask 和字段有效性。
- 时间质量、本地时钟纪律、事件不确定度、时间代次、时钟源映射代次和 FCU 发布时间统一服从 time family freshness；时间质量强调色同步服从 freshness，UTC 字段继续额外服从 utc_valid。
- RC 轴值、有效轴掩码、接收机 failsafe、RC 链路状态、输入年龄和 FCU 发布时间统一服从 rcInput family freshness；链路状态在等待/超时时使用 family 状态色，摇杆和 AxisGauge 保留既有 family freshness + axis validity 双门控。
- MockLink UI Integration 覆盖操作/导航/时间 fresh → stale → recovered，以及 RC 5500 ms 超时与警告色；资源实例新鲜度覆盖已由上一阶段专项覆盖。
- 仅修改 GroundControl QML、UI Integration 与 SPEC/G15，不改变 MAVLink 协议、FactGroup freshness 阈值、飞控健康判定、控制权限或飞行控制行为。本轮不运行 CI，新增断言为 NOT_RUN。


## 56. Phase 2.14 遥控与链路详情 freshness 一致性闭环（2026-11-15）

- 遥控与链路页的「链路质量」主面板和下方「通信链路」详情卡统一使用相同的链路 family freshness、实例 receiveAgeMs 与字段 validityMask 规则；详情卡不得绕过 freshness 直接显示缓存 RSSI、链路质量、丢包率或 FCU 数据年龄。
- 链路实例覆盖摘要使用新鲜实例数/已知实例总数；family 等待或超时时显示对应状态，不再将缓存数组长度标记为当前已上报实例数。
- 链路实例状态在新鲜时显示当前枚举；实例或 family 超时/等待时显示「数据超时/等待遥测 · 最近状态：…」，保留最近状态上下文但不得表达为当前健康。
- 链路原因在超时/等待时明确标注为最近原因；FCU 数据年龄仅在该链路实例新鲜时作为当前指标显示。
- 「链路质量」主面板增加 linkAvailable 状态徽标；状态文字、指标值和颜色共享实例 freshness 结果。
- 失控保护卡的 RC 接收年龄统一使用 rcInput family freshness；RC 数据超时后显示「数据超时」，不继续展示缓存的 FCU RC age。
- MockLink UI Integration 覆盖新鲜链路详情、实例超时、family 超时、最近状态上下文、详情指标超时，以及 RC family 超时后的接收年龄显示。
- 本轮仅调整 GroundControl QML、UI Integration 与本 SPEC；不修改 MAVLink 消息、FlightTelemetryFactGroup、Flight ICD、链路/RC 状态生成、飞控判定或任何控制行为。
- 按用户要求不运行、不触发 CI；本阶段新增断言状态为 NOT_RUN，待集中 CI 与本地 Windows x64 Release 验收。


## 57. 通信链路详情卡三态覆盖与指标 freshness 可视化（2026-11-15）

- 通信链路详情卡的实例覆盖统一显示新鲜、超时、等待三态数量，与传感器、资源和 RC 链路摘要保持一致；不再仅显示新鲜数/总数。
- 每个链路实例的 RSSI、链路质量、丢包率继续使用 per-instance freshness 与 validityMask；当实例超时/等待时不显示缓存指标数值。
- 指标行颜色与实例 freshness/字段 validity 对齐：超时琥珀色、无效字段琥珀色、未知字段中性灰、有效值使用中性前景色。
- MockLink UI Integration 覆盖 3/0/0 新鲜覆盖、1/2/0 stale 覆盖、实例状态“最近状态”标识、指标值超时替代及 stale 色彩。
- 仅修改 FlightTelemetry QML、对应 UI Integration 和 SPEC/G15；不修改 MAVLink 消息、FactGroup 缓存、链路选择规则或 RC/飞行控制行为。本轮不运行 CI，新增断言为 NOT_RUN。


## 58. RC 输入有效性与接收年龄语义统一（2026-11-15）

- 「有效输入轴」计数必须同时满足 rcValidAxesMask 对应位有效、归一化值为有限数且位于轴值范围；与 AxisGauge、StickVisualizer 使用相同的有效性判断，避免掩码置位但数值越界时仍显示 4/4。
- 顶部 KPI 的「RC 遥测接收年龄」表示地面站本地收到 rcInput 遥测族的年龄，辅助说明明确为「地面站接收 RC 状态」；失控保护详情中的「FCU 侧 RC 输入年龄」表示 FCU 上报的 rcAgeUs。两种年龄均保留各自 family freshness 门控，不得混用或使用相同含义标签。
- 遥控手型选择使用「1 日本手 / 2 美国手 / 3 中国手」标签，与 FlightRadio 当前手型映射保持一致；映射算法和设置持久化行为不变。
- 轴输入面板命名为「逻辑轴输入（实时）」，准确反映 FlightTelemetry 当前发布的是横滚、俯仰、偏航、油门四个归一化逻辑轴，而非原始 PWM 通道。
- MockLink UI Integration 覆盖正常轴 4/4、掩码置位但横滚数值越界时 3/4、恢复后 4/4；同时检查 KPI 年龄语义、FCU 侧年龄标签、逻辑轴标题及三种中文手型标签。
- 仅修改 GroundControl QML、对应 UI Integration 与本 SPEC；不修改 MAVLink、FlightTelemetryFactGroup、Flight ICD、FlightRadio 手型映射算法、飞控判定或控制行为。本轮不运行、不触发 CI，新增回归状态为 NOT_RUN。


## 59. Phase 2.14 导航/时间重复摘要面板与维护启动 ID freshness（2026-11-15）

- 导航页能力面板中的状态代次、原点 ID、能力掩码、FCU 数据年龄和原因服从 navigation family freshness，避免导航页主 KPI 已超时但同页摘要仍显示旧值。
- 导航页时间面板中的时间质量、本地时钟纪律、事件不确定度、时间代次和时钟源映射代次服从 time family freshness；时间质量颜色与时间质量卡一致，UTC 时间仍由 utc_valid 额外门控。
- 健康维护区的启动 ID 服从 operation family freshness；等待/超时明确显示对应状态，不把旧启动 ID 作为当前运行上下文。
- RC 链路摘要继续使用 _rcLinkStateText/_rcLinkStateColor，已按 rcInput family freshness 显示状态和颜色；本轮不重复修改。
- MockLink UI Integration 覆盖导航/时间摘要面板和启动 ID 的等待、接收、超时与恢复；本轮不运行 CI，新增断言为 NOT_RUN。
- 仅修改 GroundControl QML、UI Integration 与 SPEC/G15，不改变 MAVLink、FactGroup freshness 阈值或飞控控制行为。


## 60. Phase 2.15 导航时序页 FCU 单调时间线适配（2026-11-15）

- 导航时序页第四张主视觉面板由重复的能力/UTC 状态校验改为「导航与时间时序」；导航能力仍集中在「导航能力与定位精度」面板，UTC 有效性与 UTC 绝对时间仍集中在「时间同步」面板，避免同一字段在多个主面板重复呈现。
- 新增 FCU 单调时间线，展示导航状态发布（`navTimeBootUs`）、时间同步发布（`timeStatusTimeBootUs`）和最近事件（`eventTimeBootUs`）三个真实时间标记，并按当前 FCU 启动时间从早到晚排列。
- 导航与时间标记严格服从各自 family freshness：等待/超时时显示对应状态，不把缓存的旧发布时间作为当前数据；最近事件保留为最近一次已接收事件的历史标记。
- 时间线按三个消息各自的 `time_boot_us` 数值从早到晚动态排序；显示格式为「FCU +x.xxxxxx s」，保留微秒精度，避免亚毫秒时间戳在三位小数显示下无法区分。该时间只表达当前 FCU 启动周期内的单调时间先后，不得解释为 UTC、地面站时间、导航与时钟同步偏差或网络延迟；总览告警和事件历史继续沿用原有三位小数格式。
- `utc_valid=false` 只使 UTC 绝对时间/不确定度无效，不使 FCU 单调启动时间无效；UTC 时间仍只从 `FCU_TIME_STATUS.unix_time_us` 获取，并保留微秒精度。
- 保留导航/时间两个独立 freshness 状态单元、四张桌面等宽主视觉面板、现有 KPI/趋势/详情字段和只读边界；不修改 MAVLink、FlightTelemetryFactGroup、Flight ICD、时间同步服务或飞控控制行为。
- MockLink UI Integration 覆盖三个时间标记的微秒级真实数值、亚毫秒先后排序与新事件到达后的动态重排、UTC 无效时单调时间仍有效、time family fresh → stale → recovered，以及 1600/1000/900/700/400 px 下时间线内容不越界。
- 本轮只更新 GroundControl QML、UI Integration 与 SPEC；按用户要求不运行 CI/Unit/Integration/Windows Release，新增回归状态为 NOT_RUN，等待后续集中验收。


## 61. 固件构建快照与 Profile 兼容性双域 freshness（2026-11-15）

- 固件构建信息是本启动周期的快照；构建消息 family 超时后，产品版本、接口版本、Schema、哈希、源码提交和发布时间继续保留，但统一标注「最近已知」，不得冒充当前新鲜上报。
- ICD/Schema 兼容性结果在 buildInfo family stale 时标注为最近已知；未收到构建快照时保持「未收到版本」。
- Profile 匹配是当前运行状态与本启动周期构建快照的交叉结论：operation family 必须 fresh；BUILD_INFO 快照只要已收到且属于当前启动周期即可参与比对，family age 超时后标注「最近已知构建快照」。operation 等待/超时则显示「无法确认」；FactGroup 在新 boot epoch 会清空旧构建快照，未重新收到时保持「未验证」。
- MockLink UI Integration 增加四种 BUILD_INFO digest chunk，验证 fresh 时 schema/profile 匹配；buildInfo age stale 但 operation fresh 时仍可用同启动快照并标注最近已知；operation stale 时不得显示当前匹配；operation 恢复后恢复为「最近已知构建快照」匹配；新 boot epoch 清空旧快照并回到未验证。
- 仅修改 GroundControl QML、MockLink UI Integration 与 SPEC/G15；不修改 BUILD_INFO 协议、构建快照缓存、Profile ID 定义或飞控控制行为。本轮不运行 CI，新增断言为 NOT_RUN。


## 62. Phase 2.16 遥控与链路页大屏主视觉布局适配（2026-11-15）

- 本阶段适配对象是 Flight Diagnostics 第四页「遥控与链路」（FlightTelemetry.qml 的 flightTelemetry_rcLinkPageGrid），不是独立遥控校准页 FlightRadio.qml；后者保留既有校准、CRSF 绑定、微调量复制和原始 RC PWM 通道职责。
- 16:9 桌面大屏（availableWidth ≥ 1280）采用两行主视觉布局：第一行「遥控输入监视」与「逻辑轴输入（实时）」各占 6/12；第二行「链路质量」占 8/12、「失控保护」占 4/12。优先给双摇杆和链路实例指标足够宽度，避免四张卡片等宽横排造成信息拥挤。
- 中等宽度（760 ≤ availableWidth < 1280）使用 6 列网格：双摇杆与逻辑轴各占 3/6；链路质量占 4/6、失控保护占 2/6。窄屏（availableWidth < 760）恢复单列纵向顺序，确保无横向挤压。
- 双摇杆卡保留两种手型映射实时驱动、真实值回中、无效轴不绘制 marker 的既有语义；摇杆圆形轨迹随可用宽度增大，最大直径 176 px，窄宽度保持至少 112 px。
- 四个逻辑轴使用响应式仪表网格：中宽及以上两列两行，窄屏单列；不将四个归一化逻辑轴伪装成原始 PWM 通道。
- 链路实例指标在大屏三列、中/窄屏两列、极窄屏单列显示；RSSI、链路质量、丢包率仍严格服从 per-instance freshness 与 validityMask。链路质量面板宽度优先级高于失控保护摘要。
- 顶部 KPI、关键趋势、底部遥控与通信详情卡继续复用诊断大屏共享外壳、字体 token、状态色和卡片组件；不引入独立视觉体系，不重复构造顶部导航或连接状态。
- MockLink UI Integration 新增/更新 1600、1000、900、700、400 px 布局断点断言：验证宽屏 6/6 + 8/4、中屏 3/3 + 4/2、窄屏单列；验证逻辑轴仪表 2/1 列、链路指标 3/2/1 列、双摇杆不越界与宽屏轨迹放大。
- 宽屏与中屏均额外断言链路质量面板/失控保护面板宽度约 2:1，防止后续布局改动把链路主视觉压缩成与摘要面板等宽。
- 仅修改 FlightTelemetry.qml、FlightTelemetryPageUITest.cc 和本 SPEC；不修改 FlightRadio.qml、MAVLink 消息、FlightTelemetryFactGroup、Flight ICD、遥控手型映射算法、飞控判定或控制行为。
- 按用户要求不运行、不触发 CI/Unit/Integration/Windows Release；本阶段新增布局回归状态为 NOT_RUN，待后续集中验收。

## 63. Phase 2.17 总览总控页全链路通信丢失状态适配（2026-11-15）

- 总览页「FCU 通信状态」KPI 与页面只读监控状态徽标，区分未连接、等待 FCU 遥测、已收到 FCU 遥测和独立的「通信丢失」状态。
- 通信状态唯一依据为当前 Vehicle 的 `vehicleLinkManager.communicationLost`，即 VehicleLinkManager 对所有活动链路心跳的整体判定；不得用 `telemetryAvailable` 代替当前链路状态。该 Fact 只说明曾收到 FCU 遥测，不代表通信当前正常，也不代表各遥测域健康。
- 全链路通信丢失时，总览 KPI 和页头状态徽标立即切换为红色「通信丢失」，并显示「链路心跳超时；各遥测域仍按独立新鲜度判定」；链路恢复后，在 VehicleLinkManager 报告通信恢复时恢复为「已收到 FCU 遥测」。
- 六域拓扑节点、13 类健康矩阵、每个 family/instance freshness 与 validityMask 均保持各自数据源和状态，不因全链路状态徽标而被强制染色；缓存值不得被提升为当前值。
- 仅修改 GroundControl 总览 QML、MockLink UI Integration、G15 manifest 与 SPEC；不修改 VehicleLinkManager 心跳判定阈值、MAVLink、FactGroup、Flight 控制行为或只读边界。
- MockLink Integration 使用 `MockLink::setCommLost(true/false)` 触发真实 VehicleLinkManager 心跳丢失/恢复，验证 KPI 文案、页头状态、红/绿颜色及通信恢复；本轮按用户要求不运行 CI/Unit/Integration/Windows Release，新增回归为 NOT_RUN，等待集中验收。



## 64. Phase 2.18 导航时序时间线层级与中窄屏可读性（2026-11-15）

- FCU 单调时间线每条记录增加动态顺序编号（01/02/03），编号与纵向位置共用同一排序结果；导航、时间同步和事件颜色继续分别绑定各自 family freshness，不用编号或颜色代替状态。
- 每条记录改为单行横向信息结构：左侧消息名称、右侧固定宽度时间值；时间值使用统一等宽字体并右对齐，保留「FCU +x.xxxxxx s」微秒精度。标签在空间不足时只对自身省略，时间值不省略、不换行。
- 时间线说明明确标注「由上至下：最早 → 最近」，并保留 FCU 启动单调时间、非 UTC、非同步偏差的语义边界。
- MockLink UI Integration 增加初始时间排序编号、事件更新后的编号重排，以及 1600/1000/900/700/400 px 下编号/标签/时间值边界、两列不重叠和完整时间值宽度断言。
- 仅调整 FlightTelemetry QML、对应 UI Integration 与本 SPEC/进度登记；不修改 MAVLink、FlightTelemetryFactGroup、Flight ICD、freshness 阈值、时间同步服务或飞控控制行为。
- 按用户要求本轮不运行、不触发 CI/Unit/Integration/Windows Release；新增回归断言为 NOT_RUN，待后续集中验收。


## 65. Phase 2.19 遥控链路主数值与通信详情去重（2026-11-15）

- 遥控与链路页上方「链路质量」主视觉面板是链路数值的唯一主要呈现位置：每个链路实例分别展示 RSSI、链路质量、丢包率和 FCU 数据年龄；前三项分别服从 validityMask 对应 bit 8/9/10 与实例 receiveAgeMs，FCU 数据年龄服从实例 freshness。
- FCU 上报的链路数据年龄（ageUs）移入上方链路实例指标区，明确标注「FCU 数据年龄」；地面站本地接收年龄（receiveAgeMs）继续独立标注「地面站接收年龄」，两者不可混用。链路原因和地面站接收年龄只在下方通信详情的同一实例上下文组中呈现，不在上方主视觉重复。
- 下方「通信链路」详情卡不再重复显示 RSSI、质量、丢包率数值；保留 family 消息状态、链路实例新鲜/超时/等待覆盖、实例当前/最近状态、当前/最近原因、指标字段有效性摘要和地面站接收年龄。
- 指标字段有效性摘要逐实例统计 3 项指标：有效字段显示 N/3；无效字段明确列出 RSSI/链路质量/丢包率；未上报字段单独列出；实例 freshness 等待/超时时显示 freshness 状态，不得把旧指标标记为当前有效。
- 摘要颜色与实例 freshness/字段值对齐：超时琥珀色、无效字段琥珀色、未上报字段中性灰、有效字段使用中性辅助色。主视觉指标仍保留各字段独立颜色与数值门控。
- MockLink UI Integration 对全字段无效实例除验证 0/3 与无效字段清单外，还必须断言摘要文字使用警告琥珀色；有效摘要为中性辅助色，实例 stale 仍由 freshness 状态色覆盖。
- 保留既有 link detail objectName 选择器（包括 flightTelemetry_linkDetailReason_*、flightTelemetry_linkDetailMetrics_*、flightTelemetry_linkDetailReceiveAge_*），避免破坏既有 UI Integration 查找路径；主视觉不再创建重复的实例原因行或原因选择器；linkDetailMetrics 选择器现在承载指标有效性摘要，不再承载重复的指标数值。
- MockLink UI Integration 经原始 MAVLink wire path 覆盖：新鲜实例 3/3 有效、全字段无效实例 0/3、实例超时后的指标 freshness、FCU 数据年龄从有效值切换为超时状态、主视觉指标数值仍正确显示、原因只在下方详情出现且地面站接收年龄保留独立标签。
- 仅修改 FlightTelemetry.qml、FlightTelemetryPageUITest.cc、本设计 SPEC、Windows Release 验收指南与 G15 机器可读 manifest；不修改 MAVLink、FlightTelemetryFactGroup、Flight ICD、链路状态生成、飞控判定或控制行为。
- 按用户要求不运行、不触发 CI/Unit/Integration/Windows Release；本阶段新增回归状态为 NOT_RUN，待后续集中验收。


## 66. Phase 2.20 多实例链路状态去聚合徽标（2026-11-15）

- 遥控与链路页的「链路质量」主视觉 HeroPanel 与下方「通信链路」详情 StatusCard 均不使用 `linkAvailable` 作为整卡 `statusFamily`；避免 family 级 freshness 被误读为所有链路实例均健康。
- 两张卡片的左侧状态色条与标题区保持中性，不生成 family-wide 健康徽标；每个链路实例在主视觉内独立显示当前/最近状态与各指标 freshness/validity，在详情内独立显示当前/最近状态、原因、指标有效性摘要和地面站接收年龄。
- 详情卡保留显式「消息」行展示 linkAvailable family 接收状态，并保留新鲜/超时/等待实例覆盖统计；这是消息接收事实与实例覆盖，不是综合健康评分或整卡健康结论。
- `FlightTelemetryPageUITest` 使用真实 MockLink 数据验证主视觉链路面板与详情卡 `statusFamily` 均为空、失控保护卡也不显示 operation family 聚合徽标；实例状态、覆盖数、字段有效性与接收年龄继续独立断言。
- 仅修改 FlightTelemetry.qml、FlightTelemetryPageUITest.cc 与本 SPEC；不修改链路实例状态生成、VehicleLinkManager、MAVLink、FactGroup、Flight ICD、飞控判定或控制行为。
- 按用户要求不运行、不触发 CI/Unit/Integration/Windows Release；本阶段新增回归断言为 NOT_RUN，待后续集中验收。

## 67. Phase 2.21 总览系统拓扑六域连线状态映射与复合面板徽标语义（2026-11-15）

- 中栏拓扑 Canvas 新增 `familyConnectionStates`，直接从 `_overviewTopologyFamilies` 派生六域的 family、availability、当前状态和状态颜色；画布连线与节点读取同一状态快照，避免节点颜色更新而连接线仍保持固定装饰色。
- 六条中心连接线和六个节点 halo/边框按各自 family freshness 独立着色：新鲜绿色、超时琥珀色、等待中性灰；新鲜连线强调度略高，超时/等待降低亮度。不得用全链路 communicationLost 或 navigation 单域状态覆盖其余 family。
- 中央示意增加 FCU 核心标识和轻量中心光晕；该图仍只表示系统连接关系，不表示实时姿态、位置或飞行轨迹。
- 六个节点卡增加与节点同源的状态色背景、左侧状态色条和加粗状态文字；原有 family 接收年龄继续保留，卡片状态与 Canvas 连线共用同一 freshness 语义。
- 「系统拓扑与导航估计」为六域复合面板，移除 `statusFamily: navigationAvailable`，不再以导航单域状态为整张面板生成状态徽标；左侧整机多域面板同样保持无单域徽标。右侧告警面板仍可按 event family 表达状态。
- `FlightTelemetryPageUITest` 增加六条连接状态映射、family 名称/状态颜色绑定、导航 fresh→stale→recovered 的节点与连线状态同步，以及拓扑复合面板无导航单域徽标断言；保留 1600/1400/1000/900/700/400 px 节点边界断言。
- 仅修改 FlightTelemetry QML、对应 UI Integration、G15 manifest、Windows Release 验收指南和本 SPEC；不修改 MAVLink、FactGroup、Flight ICD、freshness 阈值、VehicleLinkManager 或飞控控制行为。
- 按用户要求不运行、不触发 CI/Unit/Integration/Windows Release；本阶段新增回归断言为 NOT_RUN，等待集中验收。



## 68. Phase 2.22 链路消息与实例接收年龄语义区分（2026-11-15）

- 下方通信详情的 family 级 `linkAvailable` 接收年龄使用「链路状态消息接收年龄」标签，来源为 `_receiveAgeText("link")`，表示地面站最近收到链路状态消息的年龄。
- 每个链路实例的 `receiveAgeMs` 使用「地面站接收年龄」标签，来源为该实例自身的接收年龄；它与 family 级消息年龄、FCU 上报的 `ageUs`（「FCU 数据年龄」）是三个不同量，不能共用同一标签或互相替代。
- 为 family 级接收年龄增加稳定 `flightTelemetry_linkDetailFamilyReceiveAge` objectName；原有 per-instance `flightTelemetry_linkDetailReceiveAge_*` 与主视觉 `flightTelemetry_linkMetricAge_*` 保持不变。
- MockLink UI Integration 分别断言 family 消息年龄、实例接收年龄和 FCU 数据年龄的标签/显示位置；本轮不运行 CI/Unit/Integration/Windows Release，新增断言为 NOT_RUN。
- 仅修改 FlightTelemetry.qml、FlightTelemetryPageUITest.cc 与本 SPEC；不修改 MAVLink、FactGroup、VehicleLinkManager、Flight ICD 或飞控控制行为。

## 69. Phase 2.23 趋势卡显式断点计数与最新样本状态（2026-11-15）

- 每张 `TrendCard` 在底部独立显示「断点 N」计数；计数直接来自窗口内 `gapCount`，即该趋势序列中缺失、过期、无效或 capability 不满足而未采样的点，不插值、不补造数据。
- 断点数大于零时使用琥珀色，等于零时使用中性辅助色；历史断点在后续采样恢复后仍保留，直到对应样本离开最近 60 点窗口或 Vehicle/boot epoch 重置。
- 最新样本状态单独显示 WAIT/NOW/GAP：没有有效样本为 WAIT，最新样本有效为 NOW，最新样本无效为 GAP。该状态只反映最后一个采样点，不得覆盖或清零历史断点计数。
- 原有刻度范围/有效点采集说明、较早/最新时间轴和 1 Hz/60 点采样规则保持不变；趋势线仍按连续有效段绘制，不跨断点连线。
- `FlightTelemetryPageUITest` 验证无数据时断点 0/WAIT、数据过期后断点计数增长并显示 GAP、数据恢复后最新状态回到 NOW 但历史断点仍保留；同时验证趋势卡 footer 标签在 1600 px 与 400 px 下位于卡片边界内。
- 仅修改 TrendCard QML、对应 UI Integration、G15 manifest、Windows Release 验收指南与本 SPEC；不修改采样周期、FactGroup、MAVLink、Flight ICD 或飞控控制行为。
- 按用户要求不运行、不触发 CI/Unit/Integration/Windows Release；本阶段新增回归断言为 NOT_RUN，等待集中验收。



## 70. Phase 2.24 导航页主视觉层级与诊断详情去重（2026-11-15）

- 导航页四张主视觉面板按信息密度采用桌面 12 列网格的 4/8 + 8/4 布局：第一行「导航估计不确定度」4 列、「导航能力状态」8 列；第二行「时间同步状态」8 列、「FCU 单调时序」4 列。760–1279 px 使用 6 列整行堆叠，低于 760 px 使用单列。
- 「导航估计不确定度」只展示水平位置、速度和航向标准差。标准差数值使用中性前景色，独立状态行使用能力/新鲜度语义色；绿色仅表示对应能力位具备且 family fresh，不代表精度达到某个阈值。
- 「导航能力状态」以四个独立能力状态为主，不重复显示同一状态两次；能力状态仅表达数据能力可用性，不解释为估计精度。导航状态代次、原点 ID、能力掩码、数据年龄和原因统一移入下方「导航诊断详情」。
- 「时间同步状态」只展示时间质量、本地时钟纪律、UTC 有效性、FCU UTC 时间和 UTC 不确定度；宽度 ≥760 px 时使用两列主状态网格，窄屏单列。事件时间不确定度、时间代次、时钟源映射代次与地面站接收年龄统一移入「时间诊断详情」。FCU 单调发布时间只在右侧时间线显示，不在详情卡重复。
- 导航与时间主视觉、详情字段继续按各自 family freshness 门控；能力指标继续按 navigation family freshness → capability mask 顺序门控；UTC 有效性继续独立于 timeQuality 枚举，UTC 无效不影响 FCU 单调启动时间。
- 导航/时间详情卡取消重复的 family-wide 状态徽标，保留显式消息状态和地面站接收年龄；导航主视觉仅由导航估计面板表达 family 状态，能力位单独表达 capability 状态，时间面板表达 time family 状态，时序面板保留导航/时间两个独立 freshness 单元。
- MockLink UI Integration 增加 1600 px 下 4/8 + 8/4 面板位置、宽度比例、行高对齐和时间线边界断言；1000/900 px 验证 6 列堆叠与能力/时间/域状态双列；700/400 px 验证单列；验证估计数值中性颜色、能力状态语义色、四项能力状态不重复显示，以及主视觉与详情卡不重复呈现同一字段。
- 为兼容既有 UI Integration 选择器，已迁移字段保留不可见的旧选择器；其值仍按原 family freshness 更新，但不参与布局、不产生第二份可见诊断信息。
- 仅修改 FlightTelemetry.qml、FlightTelemetryPageUITest.cc、本设计 SPEC、spec/README.md 与 Windows Release G15 manifest；不修改 MAVLink、FlightTelemetryFactGroup、Flight ICD、freshness 阈值、时间同步服务、VehicleLinkManager 或飞控控制行为。
- 按用户要求不运行、不触发 CI/Unit/Integration/Windows Release；新增回归断言为 NOT_RUN，待后续集中验收。


## 71. Phase 2.25 遥控页 RC family 状态徽标去重（2026-11-15）

- `HeroPanel` 新增 `showStatusBadge` 显示开关，默认 `true`，不改变既有页面；状态色条仍由 `statusFamily` 控制，徽标可独立隐藏。
- 遥控输入监视 `flightTelemetry_rcStickMonitor` 是 RC family 状态徽标的唯一主视觉归属；逻辑轴输入 `flightTelemetry_rcChannelMonitor` 设置 `showStatusBadge: false`，保留 `statusFamily: "rcInputAvailable"`，因此仍保留一致的 freshness 色条，但不重复显示「已接收/超时」徽标。
- 横滚、俯仰、偏航、油门各 AxisGauge 继续按 `rcValidAxesMask` 独立显示字段有效性；本次不改变逻辑轴数据、手型映射、family freshness、接收年龄或失控保护判断。
- MockLink `FlightTelemetryPageUITest` 在 1600 px 主视觉测试及 1000/900/700/400 px 响应式循环中断言主输入面板徽标可见、逻辑轴面板徽标隐藏，且逻辑轴面板仍绑定 `rcInputAvailable`，防止去重时误删 freshness 语义。
- 仅修改 `FlightTelemetry.qml`、`FlightTelemetryPageUITest.cc`、本 SPEC、Windows Release 验收指南、G15 manifest 与项目 SPEC 索引；不修改 MAVLink、FlightTelemetryFactGroup、FlightRadio 或飞控控制行为。
- 按用户要求不运行、不触发 CI/Unit/Integration/Windows Release；新增回归断言为 NOT_RUN，待后续集中验收。

## 72. Phase 2.26 总览整机状态安全优先级与操作信息分层（2026-11-15）

- 左侧「整机状态概览」以当前飞行模式作为第一视觉焦点；模式值使用中性前景色，operation family freshness 作为独立小状态显示，避免把数据新鲜度误读为飞行模式等级。
- 解锁状态保留为独立状态胶囊，显示 FCU 原始 arm state 枚举；胶囊描边/底纹只表达 operation family freshness，不根据「已解锁」推断安全或故障。
- 失效保护等级从原先与控制权并列的普通字段提升为全宽安全状态条；背景、边框、侧边色条和主数值共用 _failsafeColor()，严格映射 FCU 上报的正常/降级/需要动作/终止等级。operation family 等待/超时时，主值只显示等待/超时，颜色回退到 freshness 语义，不展示缓存的旧等级。
- 控制权下沉为独立次级信息行，保留 FCU authority 枚举；不增加任何地面站推断或控制权修改操作。
- 六个关键系统 HealthCell 继续按各自 family freshness 显示状态和接收年龄；不计算、不显示综合健康分，也不受 failsafe 等级影响。
- MockLink UI Integration 验证无遥测 WAIT、fresh 状态下「定点/解锁中/飞手/正常」与正常绿色、operation stale 后整机模式/解锁/控制权/失效保护均显示 stale 且安全条转为琥珀色；验证 1600/1000/400 px 下主视觉、失效保护条、控制权和六域矩阵均不越界。
- 仅修改 FlightTelemetry QML、FlightTelemetryPageUITest、设计 SPEC、SPEC 入口、Windows Release G15 和机器可读 manifest；不修改 MAVLink、FactGroup、Flight ICD、VehicleLinkManager、状态枚举、freshness 阈值或飞控控制行为。
- 按用户要求不运行、不触发 CI/Unit/Integration/Windows Release；本阶段新增 UI 回归为 NOT_RUN，等待集中验收。

## 73. Phase 2.27 总览告警事件首要级别与信息分组（2026-11-15）

- 右侧告警区显示当前启动周期事件总数；优先级规则保持严重度降序、同级按地面站本地接收序号降序，列表最多显示前四项，不改变事件排序或历史记录语义。
- 排序首项采用主告警卡视觉：severity 色条、较强边框/底纹、放大的严重度徽标、原因主标题和「最高优先级」标识；后续事件采用紧凑卡片，保持同一信息顺序但降低视觉权重。
- 事件信息按原因、地面站本地序号/FCU event ID/来源 ID、FCU 启动相对时间分组；时间继续使用 FCU + 秒，不标记为 UTC。严重度未知时显示「严重度未知」，不伪装为最高等级。
- 无事件时显示独立空态，说明当前启动周期暂无事件以及收到 FCU_EVENT 后的排序方式；不生成示例事件或模拟告警。
- MockLink UI Integration 验证首要事件 isPrimary、严重度颜色、原因主标题、最高优先级标签、事件计数，以及 700/400 px 下原因/元信息/时间/优先级标签边界；既有排序函数和事件历史一致性断言保留。
- 仅修改 FlightTelemetry 告警区 QML、FlightTelemetryPageUITest、设计 SPEC、SPEC 入口、Windows Release G15 与机器可读 manifest；不修改事件协议、严重度枚举、事件排序规则、FCU_EVENT 时间语义或飞控控制行为。
- 按用户要求不运行、不触发 CI/Unit/Integration/Windows Release；本阶段新增 UI 回归为 NOT_RUN，等待集中验收。

## 74. Phase 2.28 总览三栏主视觉等高与响应式重心一致性（2026-11-15）

- 总览桌面主视觉继续使用 12 列网格，左侧整机概览/中栏系统拓扑/右侧告警分别占 3/5/4 列，顺序与宽度层级保持稳定；三栏统一使用 HeroPanel 的标题、分隔线、边框、圆角和 12 px 网格间距。
- 桌面 1600/1400 px 与中屏 1000 px，三个主视觉面板共享同一 GridLayout 行高，保证顶部对齐、底部齐平；左侧安全状态条与右侧主告警卡增加内容后，不允许某一栏单独变矮或溢出。
- 低于 900 px 后主视觉改为单列堆叠，不强制不同内容面板等高；每张卡保持自身内容高度和自然阅读顺序。
- FlightTelemetryPageUITest 增加 overviewHeroPanelsSameHeight 断言，覆盖桌面/中屏等高与既有三栏位置/宽度/边界；窄屏继续验证单列和各面板内容不越界。
- 本阶段只补充布局回归断言和文档，不修改三栏现有 3/5/4、2/2/2 布局比例、HeroPanel 默认高度策略或飞控遥测语义。
- 按用户要求不运行、不触发 CI/Unit/Integration/Windows Release；新增等高断言为 NOT_RUN，等待集中验收。



## 75. Phase 2.29 时间同步指标末行完整占宽与内容边界回归（2026-11-15）

- 时间同步主状态保持 2 列（宽度 ≥760 px）/1 列（窄屏）响应式结构；第五项「UTC 不确定度」在 2 列布局下横跨两列，形成完整的底部信息行，避免最后一项孤立在左侧。
- UTC 不确定度仍独立服从 UTC validity 与 time family freshness 门控；本次只改变布局，不改变数值、颜色、有效性判定和时间质量枚举语义。
- MockLink UI Integration 增加 1600 px 桌面和 1000/900/700/400 px 响应式断言：双列时末行宽度至少为普通指标行的 1.8 倍，单列时与普通指标行等宽；末行位于 UTC 时间戳之后，五项时间指标均完整落在时间同步主视觉卡片边界内。
- 仅修改 FlightTelemetry.qml、FlightTelemetryPageUITest.cc、本设计 SPEC、spec/README.md 与 Windows Release G15 manifest；不修改 MAVLink、FlightTelemetryFactGroup、Flight ICD、时间同步服务、freshness 阈值、VehicleLinkManager 或飞控控制行为。
- 按用户要求不运行、不触发 CI/Unit/Integration/Windows Release；新增回归断言为 NOT_RUN，待后续集中验收。


## 76. Phase 2.30 RC 轴读数 freshness 与有效性颜色分层（2026-11-15）

- `AxisGauge` 数值颜色与数据语义一致：fresh 且有效时使用中性白 `#E6F0F4`；fresh 但 validity bit 清零或数值越界/非有限时使用警告琥珀 `#F0B94B`；字段未上报使用中性灰 `#8298A2`；family 未收到时沿用 family 状态色（等待中性灰、超时琥珀），不把所有非有效状态一律涂成警告色。
- `StickVisualizer` 组合读数按两轴状态表达：两轴均有效使用薄荷绿 `#9DD9D0`；任一轴无效或 family stale 使用琥珀；等待遥测或轴未上报使用中性灰。摇杆 marker 仍仅在水平/垂直轴都有效时显示，不绘制缓存值或零值替代。
- 仅改变 QML 展示颜色，不改变 `_rcAxisStatusTextForChannel`、`rcValidAxesMask`、family freshness 阈值、手型映射、通道顺序或摇杆数值；FlightRadio 与 FlightTelemetry 仍共用 `FlightRadio` Settings 中的 mode。
- MockLink `FlightTelemetryPageUITest` 验证等待时 AxisGauge/摇杆读数为中性灰、有效轴数值为中性白/有效双轴读数为薄荷绿、validity bit 清零后数值与摇杆组合读数变为琥珀；现有 marker 隐藏/显示和三种手型通道映射断言保留。
- 仅修改 `FlightTelemetry.qml`、`FlightTelemetryPageUITest.cc`、本 SPEC、Windows Release 验收指南、G15 manifest 与项目 SPEC 索引；不修改 MAVLink、FlightTelemetryFactGroup、FlightRadio 或飞控控制行为。
- 按用户要求不运行、不触发 CI/Unit/Integration/Windows Release；新增回归断言为 NOT_RUN，待后续集中验收。


## 77. Phase 2.31 四页主视觉与诊断详情卡片视觉基线统一（2026-11-15）

- 新增 dashboard panel 语义令牌，统一四页 HeroPanel 与 StatusCard 的圆角、背景、边框、眉题颜色、标题颜色、分隔线颜色、内边距、标题区间距和内容区间距；SummaryTile 外围摘要带、TrendCard、StickVisualizer 与 HealthCell 复用对应边框/圆角令牌；内边距与间距由 ScreenTools.defaultFontPixelHeight/Width 度量并随 UI Scale 缩放。
- HeroPanel 与 StatusCard 统一使用同一 FamilyStatusBadge：24 px 基准高度并按字体实际高度扩展、12 px 基准圆角、状态点/间距按 ScreenTools 度量缩放、caption 字号和强字重；状态文字、前景色、底色和边框色仍绑定各自真实 telemetry family freshness，不跨 family 聚合。
- 主视觉与详情卡片统一使用同一标题/眉题语义字号、字重与颜色；趋势图仍保留独立绘图区和背景层级，健康矩阵单元仍使用 6 px 圆角，KPI 连续摘要带不改为独立卡片。
- 总览、健康、导航时序、遥控链路四个页面网格及下方详情网格统一使用同一 ScreenTools.defaultFontPixelHeight 缩放间距令牌（基准约 12 px）；内层能力格、趋势图和健康矩阵保留各自信息密度所需的专用间距。
- FlightTelemetryPageUITest 增加总览主视觉与详情卡片跨组件断言，检查标题/眉题/徽标字体属性（point size、weight、color）、徽标尺寸/圆角、状态文本/颜色、卡片背景/圆角和内边距/区块间距一致。
- 本阶段仅修改 FlightTelemetry.qml、FlightTelemetryPageUITest.cc、本设计 SPEC、SPEC 入口、Windows Release G15 与机器可读 manifest；不修改 MAVLink、FactGroup、Flight ICD、freshness 阈值、VehicleLinkManager、飞控控制行为或其他页面导航。
- 按用户要求不运行、不触发 CI/Unit/Integration/Windows Release；新增跨组件视觉回归断言为 NOT_RUN，待后续集中验收。

## 78. Phase 2.32 总览顶部 KPI 与三栏主视觉信息去重（2026-11-15）

- 总览页顶部摘要带不再重复展示 FCU 通信、飞行操作、失效保护、链路和时间质量状态；改为三项导航估计关键指标：水平位置标准差 σ、速度标准差 σ、航向标准差 σ。
- 三项 KPI 沿用既有 navigation family freshness、capability mask、字段有效性与标准差原值，不引入阈值或推断定位等级；桌面/中屏为三列，700 px 为两列，400 px 为单列。
- 三项 MetricReadout 从中栏「系统拓扑与导航估计」迁移至顶部摘要带；中栏只负责六个遥测域拓扑节点、连接状态与各 family 独立接收年龄，不重复展示导航标准差数值。
- 左栏保留飞行模式、解锁状态、失效保护等级、控制权；移除与中栏拓扑重叠的六域 HealthCell 矩阵，保留 FCU 控制权只读说明。右栏继续专注最高优先级告警事件。
- 形成明确职责：顶部 KPI = 导航估计关键数值；左栏 = 飞行操作与安全状态；中栏 = 六域系统拓扑与 freshness；右栏 = 告警事件与原因。
- 飞行健康/导航时序/遥控与链路三个详情页的六项 SummaryTile 保持原有内容和响应式规则；只有总览页使用三项导航 KPI。
- MockLink UI Integration 验证总览页三项 KPI 显示、其他页面摘要卡切换、1600/1400/1000/900/700/400 px KPI 边界，以及拓扑面板不再重复导航标准差；左栏不再渲染重复六域矩阵。
- 仅修改 FlightTelemetry.qml、FlightTelemetryPageUITest.cc、设计 SPEC、SPEC 入口、Windows Release G15 和机器可读 manifest；不修改 MAVLink、FlightTelemetryFactGroup、Flight ICD、freshness 阈值、VehicleLinkManager 或飞控控制行为。
- 按用户要求不运行、不触发 CI/Unit/Integration/Windows Release；本阶段新增 UI 回归为 NOT_RUN，等待集中验收。

## 79. Phase 2.33 总览拓扑 freshness 图例与语义可读性（2026-11-15）

- 中栏六域拓扑节点下方新增独立三态图例，明确标识「已接收」「数据超时」「等待遥测」。
- 图例色值分别复用 freshness 状态色 #45D39A、#F0B94B、#8A9A94；文字与色点同时显示，不仅依赖颜色。
- 图例只解释六域 family freshness，不生成汇总健康状态，不改变 Canvas 连接线/节点各自独立的 freshness 映射，也不把全链路通信丢失扩散为所有节点状态。
- 400–1600 px 响应式宽度下，图例标题和三项状态均必须留在拓扑 HeroPanel 内，不裁切、不横向溢出；既有节点状态/接收年龄与导航 KPI 所有权保持不变。
- FlightTelemetryPageUITest 增加图例文字、色点颜色、状态色一致性及 1600/1400/1000/900/700/400 px 面板边界断言。
- 本阶段只修改 FlightTelemetry.qml、FlightTelemetryPageUITest.cc、设计 SPEC、项目 SPEC 索引、Windows Release 验收指南和 G15 manifest；不修改 MAVLink、FlightTelemetryFactGroup、Flight ICD、freshness 阈值、VehicleLinkManager 或飞控控制行为。
- 按用户要求不运行、不触发 CI/Unit/Integration/Windows Release；新增断言状态为 NOT_RUN，等待后续集中验收。


## 80. Phase 2.34 飞行健康页全链路通信丢失提示（2026-11-15）

- 飞行健康页「遥测域概览」直接读取 VehicleLinkManager.communicationLost；全链路心跳丢失时 headline 优先显示红色「通信丢失」，不再继续显示普通「遥测域状态已接收」。
- 增加说明行：链路心跳已丢失时，13 域矩阵仍按各自 family 接收年龄独立判定，缓存状态不代表链路在线；不把全链路丢失机械扩散为所有 family stale。
- 链路恢复后说明行隐藏，headline 恢复由各域 freshness 计数决定的中性状态；原有 family/instance freshness、validity mask、传感器与预检逐项年龄规则均不改变。
- MockLink UI Integration 在真实 communicationLost 信号路径下验证健康页红色 headline、说明行可见，以及恢复后提示隐藏和中性色恢复。
- 仅修改 FlightTelemetry.qml、FlightTelemetryPageUITest.cc、本设计 SPEC、项目 SPEC 入口、Windows Release G15 与机器可读 manifest；不修改 MAVLink、FlightTelemetryFactGroup、Flight ICD、VehicleLinkManager、freshness 阈值或飞控控制行为。
- 按用户要求不运行、不触发 CI/Unit/Integration/Windows Release；新增回归为 NOT_RUN，等待集中验收。


## 81. Phase 2.35 遥控输入详情卡状态徽标去重（2026-11-15）

- RC family `rcInputAvailable` 的状态徽标全页只在「遥控输入监视」主视觉面板显示；「逻辑轴输入（实时）」和下方「遥控输入」详情卡均不再重复显示同一状态徽标。
- `StatusCard` 增加 `showStatusBadge` 开关，默认开启以保持其他诊断详情卡行为不变；遥控输入详情卡设置 `showStatusBadge: false`，继续保留 `statusFamily: "rcInputAvailable"` 与左侧 freshness 状态色条，并保留显式「消息」行和全部诊断字段。
- 不移除遥控输入详情卡，不隐藏其 freshness/validity 数据，也不改变 RC 输入、手型映射、链路或失控保护逻辑。
- MockLink `FlightTelemetryPageUITest` 在 1600/1000/900/700/400 px 下断言主输入面板徽标可见、逻辑轴与遥控详情卡徽标隐藏；两个次级面板仍绑定 `rcInputAvailable`，避免为了去重丢失 freshness 状态语义。
- 仅修改 `FlightTelemetry.qml`、`FlightTelemetryPageUITest.cc`、本 SPEC、Windows Release 验收指南、G15 manifest 与项目 SPEC 索引；不修改 MAVLink、FlightTelemetryFactGroup、FlightRadio、Flight ICD 或飞控控制行为。
- 按用户要求不运行 CI、Unit、Integration 或 Windows Release；新增回归断言保持 NOT_RUN，待集中验收。

## 82. Phase 2.36 总览导航 KPI 数值与状态颜色分离（2026-11-15）

- 总览顶部水平位置 σ、速度 σ、航向 σ 三项 KPI 与导航时序页使用同一颜色语义：新鲜且能力具备时，标准差数值使用中性前景色 #E6F0F4，状态文字独立使用 freshness 绿 #45D39A。
- family 超时时，数值不显示缓存值，状态与不可用提示使用琥珀色 #F0B94B；能力未具备/等待时，数值占位保持中性灰，状态文字按对应 capability/freshness 语义显示。
- 不新增导航精度阈值、不推断定位等级，不修改导航标准差原值、capability mask、family freshness 或趋势采样准入规则。
- FlightTelemetryPageUITest 增加总览 KPI fresh、capability unavailable、stale、recovered 四种状态的 valueColor/statusColor 断言；三项指标均使用同一颜色规则。
- 本阶段只修改 FlightTelemetry.qml、FlightTelemetryPageUITest.cc、设计 SPEC、项目 SPEC 索引、Windows Release 验收指南和 G15 manifest；不修改 MAVLink、FlightTelemetryFactGroup、Flight ICD、VehicleLinkManager 或飞控控制行为。
- 按用户要求不运行、不触发 CI/Unit/Integration/Windows Release；新增断言状态为 NOT_RUN，等待后续集中验收。


## 83. Phase 2.37 遥控失控保护与数据链路消息状态归属分离（2026-11-15）

- 「失控保护」HeroPanel 只保留飞控 failsafe 等级、接收机 failsafe、RC 链路状态、FCU 侧 RC 输入年龄和 operation 消息接收年龄；移除重复的「数据链路」family 状态行。
- `linkAvailable` 只表示 FCU 链路状态消息 family 的接收 freshness，不等价于某个链路实例健康；该消息状态统一由下方「通信链路」StatusCard 的「消息」行显示，实例状态与新鲜/超时/等待覆盖继续逐实例表达。
- 为通信详情「消息」行增加稳定 objectName `flightTelemetry_linkDetailMessage`；保留 family 接收年龄、实例接收年龄、FCU 数据年龄三种不同时间语义。
- MockLink `FlightTelemetryPageUITest` 断言通信详情「消息」行存在且正确显示 family 接收状态；失控保护卡不再承担 linkAvailable 消息状态展示职责。
- 不改变 link family/instance freshness、链路趋势选择、RC failsafe 判定、MAVLink、FlightTelemetryFactGroup、Flight ICD 或飞控控制行为。
- 按用户要求不运行 CI、Unit、Integration 或 Windows Release；新增回归断言保持 NOT_RUN，待集中验收。

## 84. Phase 2.38 总览告警空态与实时优先级回归收口（2026-11-15）

- 告警空态标题与说明行增加稳定 objectName：flightTelemetry_alarmEmptyTitle、flightTelemetry_alarmEmptyDescription；不改变原有等待/空列表文案及空态可见规则。
- MockLink UI Integration 在未收到 FCU_EVENT 时验证告警空态可见、标题显示等待遥测、说明明确提示事件接收后按严重度排序；第一条事件到达后验证空态立即隐藏。
- 在真实 MockLink → MAVLink → Vehicle → FactGroup → QML 路径中注入严重度 3、1、3 三条事件，验证总数更新为 3、最新同级最高严重度事件置顶、同严重度按本地接收序号降序，严重度 1 事件仍按琥珀色显示。
- 修正 UI Integration 中告警优先级提示的预期文案，使其与页面实际「最高严重度置顶 · 同级按本地接收序号」一致；不改变严重度排序算法、事件序号生成、FCU_EVENT 字段或历史容量。
- 不修改 MAVLink、FlightTelemetryFactGroup、Flight ICD、VehicleLinkManager 或飞控控制行为；总览仍只读，不授予 ARM、FMS、Permit 或执行器权限。
- 按用户要求不运行 CI、Unit、Integration 或 Windows Release；新增回归断言保持 NOT_RUN，待后续集中验收。


## 85. Phase 2.39 飞行健康页预检覆盖与资源快照信息分层（2026-11-15）

- 「安全监测」保留唯一的预检结果摘要，继续显示通过/警告/失败/未知结论及风险语义色；维护横条不再重复同一结果摘要，改为「预检覆盖」并展示有效/超时/未收到数量。
- 新增 _prearmCheckCoverageColor()：全部 15 项 fresh 才显示绿色；存在 stale 显示琥珀；缺项/等待显示灰色。该颜色只表达数据覆盖，不受 checkState 的失败/警告影响；安全结论仍由 _prearmSummaryColor() 独立着色。
- 维护横条增加 CPU 负载、任务 WCET、系统栈剩余、存储剩余四项资源快照；taskId=0 resource 指标按各自 validity bit 和实例 freshness 门控，storage 按 storage family freshness 与 validity bit 门控；趋势仍保留四项指标的本地 1 Hz 历史。
- 增加 resource snapshot 子网格稳定 objectName，并在 1600/1000/900/700/400 px 检查四个指标均位于父网格边界内；MockLink 断言覆盖资源值、字段有效性颜色、预检覆盖/结论分色。
- 保留原有预检摘要 objectName flightTelemetry_healthMaintenancePrearmSummary 以兼容现有 UI 选择器，但其语义改为覆盖摘要；安全结论唯一显示在 flightTelemetry_healthSafetyPrearmSummary。
- 仅修改 FlightTelemetry.qml、FlightTelemetryPageUITest.cc、设计 SPEC、项目 SPEC 入口、Windows Release G15 和机器可读 manifest；不修改 MAVLink、FlightTelemetryFactGroup、Flight ICD、VehicleLinkManager、预检判定、资源统计或飞控控制行为。
- 按用户要求不运行、不触发 CI/Unit/Integration/Windows Release；新增回归为 NOT_RUN，等待集中验收。


## 86. Phase 2.40 接收机失效保护严重度颜色与 freshness 分离（2026-11-15）

- 新增 `_rcReceiverFailsafeColor()`，只在 `rcInputAvailable` family 新鲜时解释 `rcReceiverFailsafe`：触发红色 `#F06464`、未触发绿色 `#45D39A`。
- RC family 等待/未上报/未知采用中性灰；RC family 超时沿用 freshness 琥珀色 `#F0B94B`，不再将缓存的触发/未触发布尔值显示成当前状态。
- 飞行健康「安全监测」、遥控链路「失控保护」主卡与「遥控输入」详情卡统一使用该颜色映射；不改变接收机 failsafe 事实、RC linkState、operation failsafe 等级或飞控控制逻辑。
- MockLink `FlightTelemetryPageUITest` 覆盖未触发绿色、触发红色、恢复绿色及 family 超时琥珀色，并同时断言主视觉与详情行颜色一致。
- 按用户要求不运行 CI、Unit、Integration 或 Windows Release；新增回归断言保持 NOT_RUN，待集中验收。

## 87. Phase 2.41 导航时序字段有效性与时间纪律颜色闭环（2026-11-15）

- `LocalTimebaseDisciplineState` 独立分色：NOMINAL(0)/LOCKED(2) 绿色，ACQUIRING(1)/HOLDOVER(3)/DEGRADED(4) 琥珀色，INVALID(5) 红色；未知/缺失枚举中性灰，time family 等待使用中性灰、超时使用琥珀色并覆盖缓存枚举颜色。不得复用 `TimeQuality` 的状态 0 映射，因为 LOCAL_ONLY 与 NOMINAL 语义不同。
- 导航时序颜色与时间戳有效性一致：family fresh 且非负整数处于 JS 安全整数范围内时使用对应域强调色；缺失/未上报使用中性灰；超出安全整数范围或数值非法使用红色；family 超时使用琥珀色。无效/未上报/超时项不参与有效时间排序并置于有效时间戳之后。
- 时序说明明确只对有效 FCU `time_boot_us` 按早到晚排序；UTC 有效性与 FCU 单调启动时间保持独立。
- MockLink Integration 增加 LocalTimebaseDisciplineState 六态颜色、time family waiting/stale/recovered 覆盖，以及 `2^53` 超出 JS 安全整数边界的 FCU_EVENT 时间戳红色标记、置后排序断言。
- 仅修改 FlightTelemetry.qml、FlightTelemetryPageUITest.cc、本设计 SPEC、SPEC 入口、Windows Release G15 验收指南和机器可读 manifest；不修改 MAVLink、FlightTelemetryFactGroup、Flight ICD、时间同步服务、freshness 阈值或飞控控制行为。
- 按用户要求不运行、不触发 CI/Unit/Integration/Windows Release；新增回归断言为 NOT_RUN，待后续集中验收。

## 88. Phase 2.42 总览告警摘要与事件历史展示范围明确化（2026-11-15）

- 将总览优先告警摘要上限 4 条、下方事件历史上限 8 条提取为独立 QML 常量，两个列表共用明确的展示范围定义；不改变 FactGroup 的事件历史容量、FCU_EVENT 接收、严重度排序或本地接收序号。
- 告警区增加动态范围说明：显示「优先摘要 X / 总数 条 · 历史最近 Y 条」，其中 X 为优先级列表实际展示数量，Y 为事件历史实际展示数量；无事件时隐藏该说明，避免空态产生伪统计。
- MockLink UI Integration 注入严重度 3、1、3、2、0 五条事件，验证总数为 5、优先摘要按严重度/本地序号排序且只展示前 4 条，第 5 条仍出现在下方最近事件历史；同时验证 700/400 px 窄屏下范围说明不越界。
- 本阶段只调整 FlightTelemetry.qml、FlightTelemetryPageUITest.cc、设计 SPEC、项目 SPEC 索引、Windows Release 验收指南和 G15 manifest；不修改 MAVLink、FlightTelemetryFactGroup、Flight ICD、VehicleLinkManager 或飞控控制行为。
- 按用户要求不运行 CI、Unit、Integration 或 Windows Release；新增回归断言保持 NOT_RUN，待后续集中验收。


## 89. Phase 2.43 遥控摘要区区分链路消息 freshness 与链路实例健康（2026-11-15）

- 遥控页摘要带将原「数据链路」标签改为「链路消息」，其值继续表示 `linkAvailable` family 消息 freshness；辅助说明明确为「链路状态消息接收年龄」。
- 「链路实例」摘要继续显示逐实例新鲜/超时/等待覆盖率；两项指标分别表达消息是否收到、各链路实例是否新鲜，不合并成单一链路健康结论。
- 保留稳定的 `flightTelemetry_summaryNavigation` / `flightTelemetry_summaryLink` objectName，避免破坏既有 UI 选择器；不修改任何链路实例状态、趋势选择、freshness 阈值或协议字段。
- MockLink `FlightTelemetryPageUITest` 验证 RC 页摘要标题、family 消息状态值与接收年龄说明，并继续验证链路实例覆盖率独立显示。
- 按用户要求不运行 CI、Unit、Integration 或 Windows Release；新增回归断言保持 NOT_RUN，待集中验收。


## 90. Phase 2.44 相同 FCU 时间戳的确定性排序（2026-11-15）

- FCU 单调时间线先按有效 `time_boot_us` 升序；当两个或多个有效时间戳相等时，固定按 navigation → time → event 的域优先序排列，不依赖 JavaScript 引擎的排序稳定性。
- 缺失、无效、超时项继续排在所有有效时间戳之后；它们之间保留 navigation → time → event 的确定性域顺序。
- MockLink UI Integration 在新 boot epoch 中注入三个完全相同的 FCU 时间戳，断言显示顺序为 navigation 01、time 02、event 03；随后继续验证超出 JS 安全整数范围的事件时间戳被标红并置后。
- 仅修改 FlightTelemetry.qml、FlightTelemetryPageUITest.cc、本设计 SPEC、SPEC 入口、Windows Release 验收指南和 G15 manifest；不改变协议、FactGroup 数据源、freshness 阈值或飞控控制行为。
- 按用户要求不运行 CI、Unit、Integration 或 Windows Release；新增回归断言保持 NOT_RUN，待集中验收。


## 91. Phase 2.45 多链路状态分布与断链恢复闭环（2026-11-15）

- 链路质量 HeroPanel 新增 `flightTelemetry_linkInstanceStateSummary`，对新鲜实例的当前 `linkState` 汇总健康、降级、丢失、恢复中、初始化和未知数量。
- 超时/等待实例不计入当前 linkState 分布，但在摘要中保留 freshness 计数；避免把最后一次缓存的「健康」误判为当前在线，也避免把数据超时直接推断为链路丢失。
- 颜色优先级：任一新鲜实例丢失为红色；降级/恢复中或存在超时实例为琥珀色；全部健康为绿色；未知或无新鲜实例为中性灰。linkAvailable family 本身等待/超时时继续服从 family freshness。
- 「链路实例」摘要值与颜色只表达 freshness 覆盖：全部实例新鲜即为绿色，即使所有实例当前 linkState 均为丢失；部分超时为琥珀色，存在等待实例为中性灰。链路状态分布摘要独立表达 linkState 严重度，全部丢失/部分丢失为红色；linkAvailable 消息 freshness 也独立保持显示，避免三种语义互相覆盖。
- MockLink `FlightTelemetryPageUITest` 覆盖三实例同时健康、单实例丢失、全部丢失、全部恢复中、全部恢复健康，以及一条新鲜/两条超时的混合状态；同时验证逐实例状态文字与颜色。
- 不修改 MAVLink 字段、链路实例 ID、freshness 阈值、主数据链路选择策略或飞控行为；按用户要求不运行 CI、Unit、Integration 或 Windows Release，新增断言保持 NOT_RUN。

## 92. Phase 2.46 六域拓扑实时 freshness 统计（2026-11-15）

- 中央系统拓扑状态图例由静态颜色说明升级为实时数量统计，分别显示已接收、数据超时、等待遥测的域数量；数量只统计六个独立遥测 family，不合成总健康等级，也不覆盖单域状态。
- 复用原有状态图例行显示数量，不新增垂直布局占用；计数直接来自与拓扑连线/节点相同的 family freshness 状态，随接收年龄变化同步更新。
- MockLink UI Integration 增加初始 0/0/6、三域接收后 3/0/3、导航 freshness 超时与恢复时统计同步断言；并使用现有 1600/1400/1000/900/700/400 px 拓扑图例越界检查覆盖更长标签。
- 不修改 FCU 协议、遥测 FactGroup、状态阈值、飞行控制行为或六域拓扑连线映射。
- 按用户要求不运行 CI、Unit、Integration 或 Windows Release；新增断言保持 NOT_RUN，等待集中验收。


## 93. Phase 2.47 导航页面布局断点精确回归（2026-11-15）

- 导航页桌面布局断点为 1280 px：1280 px 及以上采用 12 列 4/8 + 8/4 两行布局；1279 px 起回落到 6 列整行堆叠。
- 中屏/窄屏断点为 760 px：760 px 及以上保持 6 列网格、能力/时间/域状态子网格双列；759 px 及以下单列。
- MockLink UI Integration 精确覆盖 1280、1279、760、759 px 四个边界点，断言主网格列数、子网格列数、桌面两行宽度比例与等高、以及中窄屏四面板纵向堆叠和等宽。
- 不改变现有设计比例、遥测数据、控件交互或页面路由；本轮不运行 CI/Unit/Integration/Windows Release，新增断言保持 NOT_RUN。


## 94. Phase 2.48 飞行健康页传感器、预检与资源详情层级收口（2026-11-15）

- 传感器健康实例由连续文本组改为独立实例卡片：实例编号/健康状态/建议动作位于首行，原因与间隙/拒绝计数作为诊断说明，地面站接收年龄作为次级元信息；左侧状态色条严格沿用实例健康与 freshness 语义。
- 解锁前检查元信息使用响应式双列网格（760 px 及以上双列，以下单列）；逐项检查改为带状态色条的独立卡片，保留检查项名称、通过/失败/警告/未知结果、原因与恢复条件，桌面/中屏维持 3/3/6 列信息比例，窄屏纵向堆叠。
- 任务资源详情由压缩单行改为每任务独立卡片：CPU 作为任务摘要，WCET、系统栈剩余、截止期错过、AXI 可用采用 2×2 指标格；每个指标分别沿用对应 validity bit 和 task instance freshness，截止期错过仅在实例 fresh 且数值有限非负时显示有效强调色。
- 新增 MockLink UI Integration 对传感器实例卡片、预检元信息列数、逐项检查卡片、四项资源值/颜色/独立 validity mask 和 1600/1000/900/700/400 px 子卡片边界的断言。
- 保留原有传感器状态/详情、预检检查行/名称/状态/详情、资源 CPU/WCET 等稳定 objectName 选择器；未删减传感器、预检、资源、原因、恢复条件或接收年龄字段。
- 仅修改 FlightTelemetry.qml、FlightTelemetryPageUITest.cc、设计 SPEC、项目 SPEC 入口、Windows Release G15 和机器可读 manifest；不修改 MAVLink、FlightTelemetryFactGroup、Flight ICD、VehicleLinkManager、飞控控制行为或数据判定阈值。
- 按用户要求不运行、不触发 CI/Unit/Integration/Windows Release；新增回归为 NOT_RUN，等待集中验收。


## 95. Phase 2.49 双摇杆与逻辑轴超范围输入一致性（2026-11-15）

- 归一化轴数值范围与 validity mask 双重门控：横滚/俯仰/偏航必须位于 [-1, 1]，油门必须位于 [0, 1]；即使对应 validity bit 置位，超范围数值仍为无效。
- 同一轴事实同时驱动逻辑轴 AxisGauge 与 Mode 1/2/3 摇杆映射；任一摇杆轴无效时隐藏该摇杆 marker、readout 显示无效并采用琥珀色；另一只摇杆只要映射的两个轴有效仍保持可用。
- MockLink `FlightTelemetryPageUITest` 在 Mode 2 下发送 roll=1.50 且 validity mask 全置位，断言横滚仪表无效、右摇杆 marker 隐藏、左摇杆 marker 保持可见；随后恢复有效 roll 并断言仪表与右摇杆同步恢复。
- 不改变 RC 归一化协议、手型映射、飞控输入或控制行为；按用户要求不运行 CI、Unit、Integration 或 Windows Release，新增断言保持 NOT_RUN。


## 96. Phase 2.50 新会话首包非操作域时趋势缓存下一采样周期清空（2026-11-15）

- FCU boot_id 是随机会话标识，任意通过校验的遥测域都可能成为新 boot epoch 的首包；不能假设 OPERATION_STATUS 一定先于导航、时间、事件或其他遥测域到达。
- FlightTelemetryFactGroup 在切换 boot epoch 时会清空各域 availability，并清空 opBootIdHex；QML 趋势采样器在已有旧趋势会话、operationAvailable 已被清空且尚无新 opBootIdHex 时，必须在下一次 1 Hz 趋势采样时清空旧趋势缓冲区和采样序号。
- 新 OPERATION_STATUS 到达前继续写入 GAP，不允许把上一会话的趋势样本与新会话数据连接；新操作状态到达后再绑定新的 Boot ID。
- MockLink UI Integration 改为先发送新 boot 的 FCU_NAV_STATUS，再发送 OPERATION_STATUS；断言趋势会话 ID 立即清空、旧样本数量下降、导航时间戳显示新会话值，而尚未收到新帧的时间同步/事件时间线显示等待遥测；随后验证新会话各域恢复与既有时间排序/无效时间戳检查。
- 原始 Fact 数值允许保留作内部缓存，但 QML 必须通过各自 family availability 门控后才能展示；不需要清零所有原始 Fact，也不修改协议、freshness 阈值或飞控控制行为。
- 按用户要求本轮不运行 CI、Unit、Integration 或 Windows Release；新增回归断言保持 NOT_RUN，待集中验收。

## 97. Phase 2.51 六域拓扑遥测流向与节点标签（2026-11-15）

- 中央拓扑六条遥测域连线增加指向 FCU 中心的箭头，明确传感器、导航、链路、飞控、执行器和时间同步向 FCU 汇报的方向；不表达控制命令下发方向。
- 六个拓扑端点显示对应域中文名称，并与下方六张 freshness 节点卡保持同一 family/label 映射；节点卡继续承载完整状态与接收年龄，Canvas 只做轻量身份标注。
- 连接元数据显式暴露 label 与 toFcu 方向，颜色仍逐域遵循原 freshness 状态，不合成整机健康分；保留「示意不代表飞行器姿态或空间位置」说明。
- MockLink UI Integration 校验六域名称、节点卡标签与连接元数据一一对应，六条连接方向均为 toFcu；使用字体度量验证 Canvas 标签在 1600/1400/1000/900/700/400 px 响应式宽度内不越界且互不重叠。
- 侧向标签向画布内侧布置，顶部/底部标签避开中心 FCU 标识；不增加 Canvas 高度，不改变原有三栏 Hero 布局。
- 不修改 MAVLink 协议、FactGroup、遥测阈值、状态映射、飞控行为或只读边界。按用户要求本轮不运行 CI、Unit、Integration、Windows Release，新增断言保持 NOT_RUN。


## 98. Phase 2.52 1672×941 大屏垂直密度自适应（2026-11-15）

- 垂直紧凑模式由 SetupPage 的 `availableHeight < 960` 决定；1672×941 参考视口进入紧凑模式，较高视口恢复标准密度，不依赖固定显示器 DPI 或字体像素值。
- 紧凑模式仅收敛装饰性空间：内容根间距 12→8 px、页签高度 46→42 px、标题区内边距/分隔间距、摘要带内边距/图标尺寸、共享 HeroPanel/StatusCard 内边距与段间距、趋势卡外边距及趋势网格间距；共享 token 同时作用于四页和下方详情卡，避免页面各自出现不同密度。
- 紧凑模式仅隐藏详情区英文 eyebrow，保留中文详情标题、所有遥测字段、14 张详情卡、四张趋势卡和页签；趋势 Canvas 绘图区继续保持至少 64 px，不缩小趋势信息区或改变 1 Hz/60 点采样规则。
- 标准模式恢复原有 12 px 根间距、46 px 页签、标准卡片内边距和 134 px 趋势卡最小高度；紧凑模式趋势卡最小高度 128 px，最终仍由内容 implicitHeight 决定，不裁切图例、刻度、断点或状态。
- `FlightTelemetryPageUITest` 增加 1672×941 与 1672×1100 两种高度的密度切换断言；保持 1672×941 紧凑态遍历总览、健康、导航时序、遥控链路四页及其详情网格，检查页签、趋势绘图区、详情标题保留和共享卡片间距；测试结束恢复原始页面高度，该 MockLink UI Integration 断言按用户要求保持 NOT_RUN。
- 1672×941 的运行截图与 100%/125%/150% DPI 像素级人工验收仍须在 Windows 实机集中执行；本阶段不宣称像素级签收，不执行 CI/Unit/Integration/Windows Release。


## 99. Phase 2.53 飞行健康页高 DPI 与窄屏细节优化（2026-11-15）

- 传感器实例头部改为响应式 GridLayout：760 px 及以上使用 12 列 3/5/4 排列实例编号、健康状态、建议动作；760 px 以下使用双列，编号与状态并排、动作占满第二行，避免窄屏三段文字横向挤压。
- 任务资源四项指标统一抽取为 ResourceMetricTile 组件；内边距、列/行间距和最小高度均由 ScreenTools.defaultFontPixelHeight 派生，标题使用 micro 字号，数值使用 ScreenTools.secondaryFontPointSize，长状态文字允许完整换行并由内容高度驱动卡片高度。
- 预检逐项卡片的内边距、列间距和行间距改为 ScreenTools 字体度量；检查项名称与结果使用 secondary 字号，原因/恢复说明保持 caption 字号，状态色和 freshness 语义不变。
- 新增 MockLink UI Integration 断言：传感器头部在 1000/900/760 px 保持三段同一行、759/700/400 px 转为双列加动作次行；资源四个指标卡片及其数值在 1600/1000/900/760/759/700/400 px 均完整位于父卡片内，动态高度不得裁切数值。
- 保留 sensor/prearm/resource 原有 objectName 选择器、字段、接收年龄、独立 validity bit、freshness gating 和颜色语义；不改变遥测解析、MAVLink、FactGroup、飞控行为或阈值。
- 本轮不运行 CI、Unit、Integration 或 Windows Release；新增回归状态为 NOT_RUN，等待集中验收。

## 100. Phase 2.54 1672×941 主视觉三栏平衡回归（2026-11-15）

- 在 1672×941 紧凑垂直模式下验证总览三张主视觉卡片：左侧整机状态、中间系统拓扑、右侧告警保持同一行，三卡高度一致；左/中/右列跨度固定为 3/5/4，按宽度从窄到宽依次为左侧整机状态（3）、右侧告警（4）、中间系统拓扑（5）。
- 断言三张卡片在 heroGrid 内不越界、不重叠，且 12 列桌面布局未因垂直密度压缩而改变横向信息层级。
- 与 1672×941 / 1672×1100 垂直密度切换测试并行覆盖；不隐藏遥测字段、不改变 12/6/1 横向断点，不改变卡片数据、状态颜色或只读边界。
- 按用户要求不运行 CI、Unit、Integration、Windows Release；新增 UI Integration 断言保持 NOT_RUN，待 exact-HEAD 集中验收。


## 101. Phase 2.55 飞行健康页详情卡片内容高度与缩放内边距收口（2026-11-15）

- 传感器实例卡片、逐项预检卡片、任务资源卡片的外层 implicitHeight 不再使用固定的 +12/+14 px；统一按各自内容布局的 implicitHeight 加上两倍 ScreenTools 缩放内边距计算。
- 传感器实例内容容器与任务资源内容容器增加稳定 objectName，供 MockLink UI Integration 精确检查实际内容容器与父卡片边界。
- UI Integration 增加内容高度契约：内容容器实际高度不得小于 implicitHeight，内容顶部/底部必须保留完整缩放内边距；预检 GridLayout 同样验证，覆盖 1600/1000/900/760/759/700/400 px。
- 该修正只处理 UI 几何，不修改字段、文案、状态色、freshness/validity 门控、MAVLink、FactGroup、协议、控制行为或安全权限。
- 按用户要求不运行 CI、Unit、Integration 或 Windows Release；新增回归保持 NOT_RUN，等待集中验收。

## 102. Phase 2.56 连续多次 FCU 重启退役会话隔离（2026-11-15）

- FCU boot_id 是随机会话标识，不能按数值大小判断新旧；FactGroup 使用有界退役 ID 集合拦截延迟到达的旧会话消息。
- 原退役集合容量为 8，连续多次重启后最早的旧会话可能被提前淘汰，导致其延迟帧重新被当作新会话接受。容量提升至 64，仍保持有界内存。
- 新增 FlightTelemetryDispatchTest：连续切换 12 个不同 Boot ID，每次由 FCU_NAV_STATUS 首包建立会话；随后以更大的 FCU 时间戳重放全部 11 个退役会话，必须全部拒绝，且当前会话仍可继续更新。
- 已有事件域与 BUILD_INFO 域首包切换测试继续覆盖：新会话允许复用时间戳/事件 ID，旧会话事件与构建快照分块不得污染新会话。
- 长时间断链 freshness 及恢复仍由既有独立 family freshness 测试覆盖；本轮不运行 CI、Unit、Integration 或 Windows Release，新增回归保持 NOT_RUN，待集中验收。



## 103. Phase 2.57 总览告警严重度分布实时摘要（2026-11-15）

- 总览右侧告警 HeroPanel 新增当前事件缓存的严重度分布摘要，分别统计严重、错误、警告、信息和未知五类数量；统计范围使用完整 recentEvents 缓存，不局限于优先摘要前 4 条或历史展示最近 8 条。
- 数量直接依据 FCU_EVENT 原始 severity 值分类：3 严重、2 错误、1 警告、0 信息；负数、超范围、空值、缺失或非整数均归入未知，不将未知级别伪装为信息或严重事件。
- 分布摘要仅在 event family freshness 为「已接收」且当前事件缓存非空时显示；无事件或数据超时时隐藏，避免将缓存旧值呈现为实时计数。
- 视觉使用紧凑状态色 chip：六列中屏主视觉下告警卡较窄，采用三列；桌面 12 列布局和 1 列堆叠布局采用五列。断点以实际 `_dashboardColumns`/HeroPanel 宽度层级为依据，而不是简单按页面宽度判断。
- MockLink UI Integration 覆盖纯函数计数（含负数/null/缺失 severity）、首条严重事件到达、五条混合严重度事件实时更新，以及 1600/1400/1000/900/700/400 px 下 chip 列数与文本/计数父子边界。
- 不修改 MAVLink、FlightTelemetryFactGroup、事件协议、飞控行为或安全权限；按用户要求不运行 CI、Unit、Integration 或 Windows Release，新增回归保持 NOT_RUN，等待集中验收。


## 104. Phase 2.58 遥控通道映射可见性与链路质量量化指示（2026-11-15）

- 双摇杆卡在名称下方显示当前手型的横向/纵向通道映射，例如「横向 CH4 · 纵向 CH3」；映射直接读取共享的 `FlightRadio.mode` 与 `_stickChannels`，Mode 1/2/3 切换后同步更新，不另设一套通道定义。
- 通道映射标签使用 ScreenTools micro 字号和字体度量；摇杆卡片隐式高度随新增映射行自适应，圆形轨迹保持正圆，400 px 双摇杆并排及父卡片边界约束继续有效。
- 链路质量指标增加 0–100% 细条，仅当 link family 与该实例均 fresh、validity bit 9 置位且 `linkQualityPermille` 为有限的 0–1000 值时显示；填充比例严格为 permille / 1000，不按 RSSI 或链路状态推算。
- validity bit 缺失/清零、family/实例等待或超时、null/空值、非有限值和超出 0–1000 的质量值均不绘制质量条；超范围或缺失值明确显示「无效」并使用琥珀色，恢复有效值后同步恢复数字、颜色和填充比例。
- 链路质量趋势卡的当前值文本与趋势采样共用相同的 0–1000 范围门控；超范围、缺失或无效质量值不得进入趋势序列，并按既有规则形成 GAP，不允许趋势图继续绘制无效值。
- MockLink `FlightTelemetryPageUITest` 主动切换 Mode 1/2/3，验证左右摇杆通道标签；以 700/1000 验证质量条和趋势值均为 70%，以 1200/1000 验证无效值隐藏质量条、趋势值无效且趋势采样返回 GAP，再恢复 70% 验证回归。
- 不修改 MAVLink、FlightTelemetryFactGroup、Flight ICD、RC 输入值/validity mask、链路 freshness 阈值、趋势数据源、飞控控制行为或只读安全边界。按用户要求不运行 CI、Unit、Integration 或 Windows Release；新增回归保持 NOT_RUN，等待集中验收。


## 105. Phase 2.59 导航时序时间线高 DPI 与紧凑大屏适配（2026-11-15）

- FCU 单调时间线的三条记录不再依赖固定 36 px 行高、38 px 行距、144/124 px 时间列宽；行高、行距、编号/标记尺寸、轨道宽度、时间值列宽和横向间距统一由 `ScreenTools.defaultFontPixelHeight/Width` 派生。默认缩放保留现有紧凑视觉基线，字体或 DPI 放大时同步扩展。
- 时间线容器高度由三行动态行高与两段行距计算；每条记录的纵向位置由同一动态 row pitch 乘以排序序号，确保排序编号、连接线、行高和时间戳列在缩放后仍保持一致。
- 时间值列优先保证完整的 `FCU +x.xxxxxx s` 微秒字符串，消息标签仅在空间不足时省略；不降低微秒精度、不把 FCU 单调时间转换为 UTC，也不改变有效/无效/超时排序和颜色语义。
- MockLink UI Integration 在 1672×941 紧凑模式与 1600/1000/900/700/400 px 宽度下验证时间线动态行高/行距、编号位置、时间值完整显示、标签/时间列不重叠和三行均位于面板边界内。Windows 100%/125%/150% DPI 截图仍作为人工视觉签收项。
- 仅修改 FlightTelemetry QML、FlightTelemetryPageUITest、设计 SPEC、SPEC 入口、Windows Release G15 验收指南和机器可读 manifest；不修改 MAVLink、FlightTelemetryFactGroup、Flight ICD、freshness 阈值、时间同步服务或飞控控制行为。
- 按用户要求本轮不运行、不触发 CI/Unit/Integration/Windows Release；新增回归断言状态为 NOT_RUN，等待后续集中验收。


## 106. Phase 2.60 总览拓扑文字字体系统与高 DPI 度量统一（2026-11-15）

- 中央拓扑 Canvas 的 FCU 标识与六个遥测域端点标签不再使用硬编码 `sans-serif` 和固定 px 字号；统一读取 `ScreenTools.normalFontFamily` 与 `ScreenTools.microFontPointSize`，以 point size 绘制，跟随应用字体族、UI Scale 和高 DPI 字体配置。
- Canvas 暴露只读字体族/点字号属性，并在字体属性变化时请求重绘，避免 UI Scale 或字体设置变化后继续显示旧字形。
- MockLink UI Integration 的 QFontMetricsF 几何检查直接读取 Canvas 实际字体族与 point size，验证 FCU/六域标签在 1600/1400/1000/900/700/400 px 下均不越界、不重叠；不再以独立硬编码像素字号模拟绘制字体。
- 仅改变总览拓扑文字渲染与对应几何回归；不改变拓扑节点、箭头方向、family freshness、状态统计、三栏布局、遥测字段或只读边界。
- 按用户要求不运行/触发 CI、Unit、Integration 或 Windows Release；新增静态检查与 UI Integration 断言保持 NOT_RUN，等待集中 exact-HEAD 验收。


## 107. Phase 2.61 飞行健康页状态单元与维护横条高 DPI 内容适配（2026-11-15）

- 13 域健康矩阵和左侧重点域概览共用的 HealthCell 移除固定 54 px 高度与 7 px 内边距；最小高度随 ScreenTools 字体像素高度缩放，实际高度按内容 implicitHeight 加两倍面板内边距计算。
- HealthCell 内部标签、状态和接收年龄均允许在有限宽度内换行，内部内容容器提供稳定 objectName，便于检查顶部/底部/左右内边距和内容高度。
- 维护与可靠性横条的外层高度改为内容 implicitHeight + 两倍 ScreenTools 面板内边距，锚定内边距使用同一 token；三列、两列和单列间距统一使用现有 dashboard spacing tokens。
- 启动 ID 支持任意位置换行；最近异常与预检覆盖文本显式允许最小宽度收缩并换行；资源指标网格间距改用 ScreenTools 缩放 token。
- MockLink UI Integration 覆盖矩阵单元内容高度、维护横条上下内边距、三列/两列/单列列布局、长中文异常摘要引发的自然增高，以及 1672×941 紧凑/1100 px 标准垂直模式切换。
- 只调整 FlightTelemetry.qml 展示层和 UI Integration；不改变 13 类遥测字段、family/instance freshness、validity mask、状态判定、MAVLink、FactGroup、飞控控制行为或只读安全边界。
- 按用户要求不运行 CI、Unit、Integration 或 Windows Release；新增验收状态为 NOT_RUN，等待后续集中 exact-HEAD 验收。


## 108. Phase 2.62 导航 UTC 时间戳全宽堆叠显示（2026-11-15）

- 导航页「FCU UTC 时间」从普通左右分栏 TelemetryRow 改为独立的全宽堆叠时间戳行：字段标签单独一行，UTC 时间值使用固定宽度字体与轻量深色底框，完整保留 `yyyy-MM-dd HH:mm:ss.ffffff UTC` 六位微秒与 UTC 标识。
- 时间值允许按空格自然换行，不使用省略号、不截断微秒、不改写时间值；UTC 有效/无效/等待/超时仍由原 `timeAvailable` freshness 与 `utc_valid` 门控决定，强调色仍复用 `_utcValidityColor()`。
- 760 px 及以上时间同步网格为双列时，UTC 时间戳行横跨两列；UTC 不确定度回到第二行右侧，与 UTC 有效性并排，避免长时间戳挤占半列并保持原三行信息密度。低于 760 px 时继续单列全宽排列。
- MockLink UI Integration 在 1672×941 紧凑垂直模式及 1600/1000/900/700/400 px 下检查导航四块主视觉的 4/8 + 8/4 比例、同排等高、卡片边界、UTC 时间戳行/值标签宽度和内容高度；既有 UTC 有效/无效/超时/恢复与微秒值断言继续生效。
- 仅修改 FlightTelemetry.qml、FlightTelemetryPageUITest、设计 SPEC、SPEC 入口、Windows Release G15 验收指南和机器可读 manifest；不修改 MAVLink、FlightTelemetryFactGroup、Flight ICD、UTC 时间服务、freshness 阈值或飞控控制行为。
- 按用户要求本轮不运行、不触发 CI/Unit/Integration/Windows Release；新增回归状态为 NOT_RUN，等待后续集中验收。


## 109. Phase 2.63 导航时序页辅助网格与指标文本高 DPI 适配（2026-11-15）

- 导航能力、时间同步、导航/时间域状态三组子网格的列间距与行间距不再使用固定 8/14/6 px，统一由 `ScreenTools.defaultFontPixelWidth/Height` 派生；默认字号保持 8/14/6/8 px 的既有视觉基线，并设置最小间距，应用 UI Scale 与高 DPI 放大时同步扩展。
- 共用 `MetricReadout` 的标签/状态行内部间距改为 ScreenTools 缩放 token；标签和状态文本设置最小宽度为 0 并允许 WordWrap，数值与单位仍保持独立横排，避免窄面板或放大字号时标签/状态被挤出卡片。
- MockLink UI Integration 在导航页 1280/1279/760/759 px 断点检查三组子网格实际 spacing 与 ScreenTools token 一致，并检查导航估计/能力指标使用相同缩放间距、标签/状态启用换行；既有 1672×941 时间线几何、UTC 六位微秒和 freshness/validity 回归保持不变。
- 仅调整 FlightTelemetry.qml 展示层与对应 UI Integration；不修改 MAVLink、FlightTelemetryFactGroup、Flight ICD、时间戳排序、UTC 转换、freshness 阈值、能力位、时间质量枚举或飞控控制行为。
- 按用户要求不运行 CI、Unit、Integration 或 Windows Release；新增验收保持 NOT_RUN，等待后续集中 exact-HEAD 验收。


## 110. Phase 2.64 总览拓扑矢量几何与六域节点卡片高 DPI 自适应（2026-11-15）

- 中央拓扑 Canvas 的图形密度比例由可用宽度与 `ScreenTools.defaultFontPixelHeight` 共同决定；画布高度按同一字体像素度量扩展，避免 125%/150% DPI 下文字增大而机体线框、节点、箭头和连线仍保持固定像素。
- FCU 中心圆、四旋翼机臂/旋翼、六域节点光环/节点环/中心点、连线粗细、箭头尺寸、十字参考线和六个标签偏移统一使用 Canvas 暴露的 `geometryScale`，字体仍使用 `ScreenTools.normalFontFamily` 与 `ScreenTools.microFontPointSize`。
- 六域节点网格行列间距、内容左右/上下内边距、首行指示点尺寸、标签状态间距、节点内部行距和统计图例点/间距统一由 `ScreenTools.defaultFontPixelWidth/Height` 派生；节点卡片高度按内容隐式高度加两倍缩放垂直内边距计算。
- MockLink UI Integration 读取 Canvas 实际 density/geometry scale、画布目标高度和标签字体度量；在 1600/1400/1000/900/700/400 px 验证六个标签不越界、不重叠、六张节点卡片父子边界正确、节点网格间距与 ScreenTools token 一致。
- 不改变六域 family freshness、连线颜色/箭头语义、状态统计、拓扑节点数量、遥测字段、KPI、三栏 3/5/4 布局或只读安全边界。
- 按用户要求不运行/触发 CI、Unit、Integration 或 Windows Release；新增静态检查与 UI Integration 断言保持 NOT_RUN，等待集中 exact-HEAD 验收。


## 111. Phase 2.65 导航估计指标与能力状态分级徽标（2026-11-15）

- 导航估计不确定度卡片的水平位置、速度、航向标准差继续以中性高对比数值作为主视觉，单位与数值同行；对应的 family freshness/能力状态从普通 micro 文本改为独立 FamilyStatusBadge，使用原有状态文案与状态色，不把标准差数值本身误染成健康结论。
- 导航能力卡片的航向估计、水平位置、水平速度、垂直速度状态不再以大号 metric 数字显示；统一使用紧凑 FamilyStatusBadge，标签仍为 caption 字号，已接收/未具备/未知/等待/超时文案和颜色完全复用 `_navigationMetricStatus()` / `_navigationMetricColor()`。
- MetricReadout 新增可选 `valueAsStatusBadge` 与 `statusAsBadge` 呈现模式，默认关闭，不改变总览 KPI 等其他使用点；新增 value/unit/status badge objectName 便于自动化验收。数值字段仍由原 capability mask、family freshness 与字段有效性门控决定，不新增合成状态。
- MockLink UI Integration 验证三项估计值保持可见、状态徽标与原 statusText/statusColor 一致、旧状态文本隐藏；四项能力状态以徽标呈现、原大号状态文本隐藏，且在 1280/1279/760/759 px 断点与 1600/1672×941 视口下所有徽标均完整位于所属指标容器内。
- 仅修改 FlightTelemetry.qml、FlightTelemetryPageUITest、设计 SPEC、SPEC 入口、Windows Release G15 验收指南和机器可读 manifest；不修改 MAVLink、FlightTelemetryFactGroup、Flight ICD、导航 capability bit 定义、freshness 阈值、标准差字段精度或飞控控制行为。
- 按用户要求不运行、不触发 CI/Unit/Integration/Windows Release；新增回归状态为 NOT_RUN，等待后续集中 exact-HEAD 验收。


## 111. Phase 2.65 左侧整机状态概览高 DPI 与内容自适应（2026-11-15）

- 左侧整机状态概览的飞行模式主视觉行、operation freshness 辅助信息、解锁状态列与解锁胶囊统一使用 `ScreenTools.defaultFontPixelWidth/Height` 派生的行距、列距、徽标内边距与圆角；飞行模式仍为主视觉，解锁状态仍独立显示 FCU 原始枚举。
- 解锁状态胶囊的宽/高按实际文本 implicitWidth/implicitHeight 加双侧缩放内边距计算；不将「已解锁」解释为健康或安全结论。
- 失效保护等级保持全宽优先安全状态条；原 46 px 高度改为 ScreenTools 字体像素高度派生的最小高度，实际高度取内容 implicitHeight 加双侧垂直内边距的较大值。左右内边距、上下内边距、文字间距和侧边状态色条尺寸均使用 ScreenTools 像素度量。
- 控制权继续作为次级只读行；原 32 px 高度改为 ScreenTools 字体像素高度派生的最小高度，实际高度按内容 implicitHeight + 双侧垂直内边距计算。控制权长文本可收缩省略，说明文本保留自然换行。
- MockLink UI Integration 新增飞行模式行、解锁胶囊、失效保护状态条、控制权内容容器与只读说明的父子边界/隐式高度/内边距断言；覆盖 1600/1400/1000/900/700/400 px，并在恢复原窗口宽度后重复断言。
- 不改变 operation family freshness、飞行模式/arm/failsafe/authority 枚举、状态色、通信丢失优先级、只读安全边界或总览三栏 3/5/4、2/2/2 布局。
- 按用户要求不运行/触发 CI、Unit、Integration 或 Windows Release；新增静态检查与 UI Integration 断言保持 NOT_RUN，等待集中 exact-HEAD 验收。


## 112. Phase 2.66 飞行健康页 150% / 200% UI Scale 实际缩放验收（2026-11-15）

- 回归必须使用 AppSettings.uiScalePercent 实际切换 UI 缩放比例，不得仅通过修改页面 width/height 冒充 DPI 缩放；测试结束必须恢复原始设置，避免污染后续用例。
- 150% 场景以 1672×941 物理参考视口折算为约 1115×627 逻辑尺寸，健康矩阵与维护横条应自然重排为双列；13 个 HealthCell 内容容器必须完整保留 ScreenTools 缩放内边距和 implicitHeight。
- 200% 场景以 1280×720 物理视口折算为 640×360 逻辑尺寸，健康矩阵与维护横条应自然重排为单列；不得发生文字裁切、横向溢出或内容容器越界。
- 200% 单列场景继续注入长中文异常摘要，验证摘要换行、维护横条外层高度同步增长、内部内容和各列保持边界完整。
- 该验收只覆盖 UI Scale、布局断点、内容高度与可读性；不修改遥测字段、family/instance freshness、validity mask、状态颜色语义、MAVLink、FactGroup、飞控行为或只读安全边界。
- 按用户要求不运行 CI、Build、Unit、Integration 或 Windows Release；新增回归保持 NOT_RUN，等待后续集中 exact-HEAD 验收。


## 112. Phase 2.66 遥控与链路页高 DPI / 内容自适应收口（2026-11-15）

- RC 手型切换按钮的间距与最小高度按 ScreenTools 字体像素宽高缩放；按钮文案设置 WordWrap，按钮高度取缩放最小高度与实际文案隐式高度加双侧内边距的较大值。
- 双摇杆网格间距、摇杆卡片内边距/内部间距、舞台最小尺寸、摇杆位置标记直径/边缘内缩和十字参考线尺寸按 ScreenTools 缩放；通道映射与实时读数允许换行。摇杆卡片高度取缩放最小高度、宽度驱动高度和内容隐式高度加双侧内边距三者最大值，避免窄屏/高 DPI 裁切。
- 逻辑轴网格列/行间距、轴刻度条高度、中心线宽度及上下 overscan 按 ScreenTools 缩放；轴标签允许收缩与换行，保留原始四轴映射、validity mask 和 freshness 着色。
- 多链路实例行距、RSSI/链路质量/丢包率指标网格间距、卡片内边距/内部间距、最小卡片高度和质量条高度按 ScreenTools 缩放；指标卡片高度按内容隐式高度加双侧缩放内边距计算，不改变各链路独立 freshness、validity 与 receive-age 门控。
- MockLink UI Integration 在 1000/900/760/759/700/400 px 验证手型按钮、摇杆与通道映射、逻辑轴刻度、链路指标网格的缩放 token 一致性，以及摇杆/指标卡片内容完全落在父卡片内；既有 1600/1672×941 布局和链路有效性门控保持不变。
- 仅调整 FlightTelemetry.qml 展示层和对应 UI Integration；不修改 FlightRadio 手型映射、MAVLink、FlightTelemetryFactGroup、链路字段、quality permille/packet loss 计算、freshness 阈值或飞控控制行为。
- 按用户要求不运行、不触发 CI、Unit、Integration 或 Windows Release；新增验收状态 NOT_RUN，等待后续集中 exact-HEAD 验收。


## 113. Phase 2.67 飞行健康页详情卡片与资源指标 150% / 200% 缩放边界（2026-11-15）

- 在 150%/200% UI Scale 回归中，除 13 域健康矩阵和维护横条外，继续验证传感器实例卡、预检卡、任务资源卡的内容高度与缩放内边距。
- 传感器标题/状态/动作、预检编号/结果/详情、资源指标 tile 及其数值必须完整落在所属父容器内；维护资源 CPU/WCET/栈/存储指标必须完整位于资源网格内。
- MockLink UI Integration 在 150% 双列与 200% 单列视口下调用统一的 healthDetailCardsFit 与 maintenanceResourceMetricsFit 几何契约；原始 UI Scale 与视口必须恢复。
- 不改变遥测字段、freshness/validity 语义、状态色、MAVLink、FactGroup、飞控行为和只读边界；按用户要求不运行 CI/Build/Unit/Integration/Windows Release，新增回归保持 NOT_RUN。
