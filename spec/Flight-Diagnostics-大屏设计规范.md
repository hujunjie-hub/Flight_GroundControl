# 飞控诊断高级大屏设计规范

- 版本：6.45
- 日期：2026-11-15
- 状态：四页主体与应用外壳已实现；四页结构、遥测语义与响应式适配持续完善；事件历史/告警元信息、预检、资源实例与 RC/链路 freshness 已收口；导航时序页与遥控链路页持续完成大屏布局适配；总览页全链路通信丢失提示与恢复颜色已补齐；链路主视觉与通信详情已按数值/诊断上下文去重，并移除多实例聚合徽标；链路 family 消息年龄与实例接收年龄已明确区分；集中 CI 与像素级截图/DPI 验收后置
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

KPI 是信息索引，不是综合健康评分。任何 family 未收到、超时、无效或能力位缺失时，必须显示对应状态，不能用 0、绿色或上一次有效值伪装为当前值。KPI 主值、辅助说明和强调色必须服从同一 family freshness、实例 receiveAgeMs 与字段 validity/capability 规则；不得出现主值已超时但辅助文字仍显示旧的有效/健康/未触发状态。KPI 只显示当前页有意义的指标，禁止为了对齐参考图而伪造电池、卫星、HDOP、振动、温度、SNR、RTT、吞吐量或不存在的原始 RC 通道。

### 2.4 三栏主视觉区

桌面宽屏按参考图组织为左窄、中宽、右侧告警三栏；三个面板同一行对齐。

| 区域 | 标题/内容 | 实现要求 |
|---|---|---|
| 左栏 | 整机状态概览 | 突出当前飞行模式、解锁状态、控制权、失效保护；下方显示飞控、传感器、导航、链路、资源、时间六个关键域的独立状态 |
| 中栏 | 系统拓扑与导航估计 | 中央绘制静态多旋翼拓扑示意；六条连接线与六个节点分别绑定传感器、导航、链路、控制、执行器、时间 family 的真实 freshness，已接收为绿色、超时为琥珀色、等待为中性灰；下方节点卡同步显示状态与接收年龄；底部保留水平位置、速度、航向标准差 |
| 右栏 | 告警事件（优先级） | 显示当前 boot epoch 最近事件、严重度、事件序号/ID、来源和原因；RC 输入监视移至独立的「遥控与链路」页面 |

- 左栏不得显示 96/100 等虚构健康分，不得将 family 状态计数包装成健康百分比。
- 中栏拓扑图是静态系统连接示意，不代表飞行器实时位置、姿态、航向或飞行轨迹；当前接口没有姿态角，不得绘制人工地平仪。六条连线和节点必须按各自 family freshness 独立着色，不得将某一 family 状态或全链路通信状态扩散成整张拓扑的统一健康结论。
- 右栏事件必须来自 `recentEvents`；没有事件时明确显示空态/等待状态，不生成示例告警。
- 左栏整机状态概览和中栏系统拓扑与导航估计均为多域复合面板，标题区不得显示某一个 family 的单域状态徽标；各子项与拓扑节点独立显示自己的 freshness。右栏告警面板可以保留 event family 状态。
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

继续使用页面现有 500 ms 年龄刷新 Timer 的每第二次 tick 进行 1 Hz 地面站本地采样，最多保留 60 点；页面隐藏时停止采样。图表明确标注 GCS 1 Hz / 最近 60 点，缺失、过期、无效和 capability 缺失均形成断点，禁止插值或跨断点连线。

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
- 矩阵下方显示维护与可靠性横条：启动 ID、最近真实事件、预检通过/警告/失败/未知计数、真实系统资源摘要。
- 页面底部复用地面站本地 1 Hz 趋势，使用真实 CPU 负载、WCET、栈剩余和存储剩余；resource taskId=0 实例超时或 validity bit 缺失时形成断点。不把健康分、振动 RMS、电池温度或电压一致性等未提供字段伪装成遥测。

### 2.9 导航时序页面

- 主体按导航估计总览、导航能力与定位精度、时间同步、状态校验四块组织；桌面宽屏采用 12 列网格，四块主面板各占 3 列并同排等高，避免状态校验被压缩成窄侧栏。
- 导航数值只展示 positionStdM、velocityStdMps、headingStdRad 和协议能力掩码；状态校验区同时显示水平位置、水平速度、垂直速度、航向能力，缺少能力位时显示不可用。
- 状态校验横跨 navigation 与 time 两个数据域，必须分别显示两域的消息状态和接收年龄，不得使用 navigation 单域徽标代表导航与时间同步整体正常。
- 时间面板显示 timeQuality、timeDisciplineState、utcValid、FCU unix_time_us 转换的 UTC 绝对时间、utcUncertaintyUs、eventUncertaintyUs、timeEpoch、sourceMappingEpoch 和接收年龄；UTC 绝对时间仅在 time family 新鲜且 utcValid=true 时显示，UTC 无效、未上报或超时不得显示旧时间；时间同步明细卡同时保留 FCU 发布时间。
- 趋势图使用水平位置、速度、航向和 UTC 时间不确定度；UTC 无效/超时产生断点。
- 当前 FlightTelemetry 未提供卫星数、HDOP、每颗卫星方位/仰角，禁止绘制虚构卫星天图或填入示例值。

### 2.10 遥控与链路页面

- 主体按左侧双摇杆、逻辑轴实时输入、链路质量、失控保护四块组织。
- 双摇杆手型直接读取 FlightRadio 的持久化 Mode 1/2/3 设置，按其 channel-to-axis 映射绑定横滚/俯仰/偏航/油门；marker 仅在对应两个轴均新鲜且 validity bit 有效时显示。轴读数分别保留等待、超时、未上报和无效语义，不把无效输入折算为 0。
- 当前协议只提供四个归一化逻辑轴，不提供 CH1–CH8 原始 PWM；页面明确显示四个逻辑轴，不虚构额外通道。
- 链路质量逐实例显示 linkKind、linkState、RSSI、质量、丢包、原因和接收年龄；RSSI、质量、丢包分别成格，并分别满足 link family 新鲜、实例 receiveAgeMs 未超时、validityMask 对应位有效。
- 链路趋势优先使用新鲜 GCS（kind 2）、伴随计算机（kind 3）或其他数据链路（kind 4）；存在非 RC 实例但均超时时保留超时/断点，不以健康 RC 接收机冒充数据链路；仅无非 RC 实例时回退 RC（kind 1）。趋势标题标明当前选中链路类型。
- 失控保护与多实例链路质量面板不使用单一 operation/link family 徽标代表整张面板；各域和各实例状态独立表达。
- 桌面四个主面板采用 3/3/3/3 等宽同排等高；760–1279 px 时每张面板占 6 列全宽换行；低于 760 px 单列堆叠。
- 底部四张趋势图按页切换为优先数据链路 RSSI、链路质量、丢包率和 RC 接收年龄；无效/过期样本形成断点。


## 3. 响应式布局规范

布局依据 SetupPage.availableWidth，页面外壳和左侧主导航由 GroundControl 现有应用框架提供。

| 可用宽度 | 主页面网格 | 健康矩阵（健康页） | KPI | 页签 |
|---|---:|---:|---:|---:|
| ≥1280 px | 12 列 | 4 列 | 6 列 | 4 列 |
| 1000–1279 px | 6 列 | 2 列 | 3 列 | 4 列 |
| 760–999 px | 总览/明细 1 列；健康/导航/RC 6 列（主面板全宽换行） | 2 列 | 3 列 | 4 列 |
| 420–759 px | 1 列 | 1 列 | 2 列 | 2 列 |
| <420 px | 1 列 | 1 列 | 1 列 | 2 列 |

- 总览主视觉宽屏使用左 3 / 中 5 / 右 4 列；健康页使用左 3 / 中 6 / 右 3 列；导航时序与遥控链路页均采用四块主面板各占 3 列的 3/3/3/3 布局。
- 1000–1279 px 时总览/明细主网格使用 6 列；导航/RC 主面板各占 6 列并全宽纵向排列。760–999 px 时总览/明细使用单列，健康/导航/RC 网格使用 6 列且各主面板全宽换行；低于 760 px 时所有主网格单列。
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

- 遥控输入、逻辑轴、链路质量、失控保护四个主视觉面板在桌面宽屏采用 3/3/3/3 等宽布局，并显式启用 HeroPanel.fillRowHeight；UI Integration 验证同排、等宽、等高。
- 760–1279 px 时四个面板各占 6 列逐行排列；低于 760 px 单列排列；不得产生横向裁切。
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



## 67. Phase 2.21 链路消息与实例接收年龄语义区分（2026-11-15）

- 下方通信详情的 family 级 `linkAvailable` 接收年龄使用「链路状态消息接收年龄」标签，来源为 `_receiveAgeText("link")`，表示地面站最近收到链路状态消息的年龄。
- 每个链路实例的 `receiveAgeMs` 使用「地面站接收年龄」标签，来源为该实例自身的接收年龄；它与 family 级消息年龄、FCU 上报的 `ageUs`（「FCU 数据年龄」）是三个不同量，不能共用同一标签或互相替代。
- 为 family 级接收年龄增加稳定 `flightTelemetry_linkDetailFamilyReceiveAge` objectName；原有 per-instance `flightTelemetry_linkDetailReceiveAge_*` 与主视觉 `flightTelemetry_linkMetricAge_*` 保持不变。
- MockLink UI Integration 分别断言 family 消息年龄、实例接收年龄和 FCU 数据年龄的标签/显示位置；本轮不运行 CI/Unit/Integration/Windows Release，新增断言为 NOT_RUN。
- 仅修改 FlightTelemetry.qml、FlightTelemetryPageUITest.cc 与本 SPEC；不修改 MAVLink、FactGroup、VehicleLinkManager、Flight ICD 或飞控控制行为。
