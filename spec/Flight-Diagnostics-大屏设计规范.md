# 飞控诊断高级大屏设计规范

- 版本：4.0
- 日期：2026-11-15
- 状态：四张参考图对应的四页面交互与主体布局已提交；等待新 HEAD CI、Windows 本地视觉和运行验收
- 适用仓库：`hujunjie-hub/Flight_GroundControl`
- 页面：`src/FirmwarePlugin/Flight/FlightTelemetry.qml`
- 跟踪 Issue：[FGC-0044 飞控诊断页高级大屏布局与可视化改造](https://github.com/hujunjie-hub/Flight_GroundControl/issues/44)
- 设计依据：用户于 2026-11-15 上传的四张 1672×941 原始参考图（总览总控、飞行健康、导航时序、遥控与链路）；四图共用顶部状态栏、左侧导航、标题区和四个页面入口

## 1. 目标与范围

将现有「飞控诊断」Setup 页面升级为高密度、专业、可快速识别异常的飞行器实时监控大屏。页面应当首先呈现飞控运行状态和关键诊断摘要，再按功能域组织完整遥测细节。

本次是展示层重构，不是遥测协议或飞控控制链路重构。现有 `FlightTelemetryFactGroup`、13 类 FCU 遥测消息、Fact 命名、枚举、validity mask、family freshness、boot/profile 隔离和数据分发路径保持不变。

### 1.1 设计目标

1. 一屏先掌握关键状态：FCU 遥测接收、解锁/飞行模式、失效保护等级、导航估计摘要。
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

- 继续使用 GroundControl 现有全局顶部状态栏和左侧主导航；本页面不重复绘制、不替换应用外壳。
- 页面内容区顶部使用大标题「飞控诊断总览」和英文副标题 `FLIGHT DIAGNOSTICS OVERVIEW`。
- 右侧保留「只读监控」状态徽标与 FCU 遥测接收提示。
- 页面标题、只读状态、连接状态必须响应式排列，不遮挡、不裁切。

### 2.2 四个交互页面入口

标题下方使用四个真实页面入口：「总览总控」「飞行健康」「导航时序」「遥控与链路」。

- 四个页签均可点击，选中态使用青色描边/底部强调线；未选中态保持深青蓝面板。
- 点击后只切换 FlightTelemetry.qml 内的页面内容，不创建新 Setup 路由，不改变遥测协议或数据模型。
- 当前默认进入「总览总控」；页面标题、英文副标题、六项 KPI 和趋势图标题随页签同步变化。
- 页签必须保留稳定 objectName，UI Integration 必须逐项点击验证页面切换。

### 2.3 六项关键 KPI 带

四个页面共用六格 KPI 带，但每页按当前功能域切换标签和值；所有 KPI 仍必须绑定真实 Fact、family freshness 和 validity/capability mask。

| 页面 | 六项 KPI | 展示约束 |
|---|---|---|
| 总览总控 | FCU 遥测接收、飞行状态、失效保护等级、导航估计、数据链路、时间同步 | 保留 operation/navigation/link/time family 的原有门控 |
| 飞行健康 | 飞控操作、传感器健康、控制器状态、执行器状态、系统资源、事件记录 | 不计算总分；事件数量只来自当前 boot epoch 的真实 recentEvents |
| 导航时序 | 导航状态、水平位置 σ、速度 σ、航向 σ、时间质量、UTC 不确定度 | 位置/速度/航向分别服从 capability bit3、bit5+bit6、bit2；UTC 不确定度仅在 UTC 有效时显示 |
| 遥控与链路 | 遥控输入、有效输入轴、RC 链路状态、数据链路、链路实例、遥控接收年龄 | 有效轴由 rcValidAxesMask 决定；链路指标只显示 validityMask 标记有效的字段 |

KPI 是信息索引，不是综合健康评分。任何 family 未收到、超时、无效或能力位缺失时，必须显示对应状态，不能用 0、绿色或上一次有效值伪装为当前值。KPI 只显示当前页有意义的指标，禁止为了对齐参考图而伪造电池、卫星、HDOP、振动、温度、SNR、RTT、吞吐量或不存在的原始 RC 通道。

### 2.4 三栏主视觉区

桌面宽屏按参考图组织为左窄、中宽、右侧告警三栏；三个面板同一行对齐。

| 区域 | 标题/内容 | 实现要求 |
|---|---|---|
| 左栏 | 整机状态概览 | 突出当前飞行模式、解锁状态、控制权、失效保护；下方显示飞控、传感器、导航、链路、资源、时间六个关键域的独立状态 |
| 中栏 | 系统拓扑与导航估计 | 中央绘制静态多旋翼拓扑示意，周围节点显示传感器、导航、链路、控制、执行器、时间六个 family 的真实状态；下方保留水平位置、速度、航向标准差 |
| 右栏 | 告警事件（优先级） | 显示当前 boot epoch 最近事件、严重度、事件序号/ID、来源和原因；RC 输入监视移至独立的「遥控与链路」页面 |

- 左栏不得显示 96/100 等虚构健康分，不得将 family 状态计数包装成健康百分比。
- 中栏拓扑图是静态系统连接示意，不代表飞行器实时位置、姿态、航向或飞行轨迹；当前接口没有姿态角，不得绘制人工地平仪。
- 右栏事件必须来自 `recentEvents`；没有事件时明确显示空态/等待状态，不生成示例告警。
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
| RSSI / 链路质量 / 丢包率 / RC 接收年龄 | linkStatuses、rcAgeUs | link/RC family 新鲜且相应 validity mask 有效；遥控与链路页替换四张图 |

继续使用页面现有 500 ms 年龄刷新 Timer 的每第二次 tick 进行 1 Hz 地面站本地采样，最多保留 60 点；页面隐藏时停止采样。图表明确标注 GCS 1 Hz / 最近 60 点，缺失、过期、无效和 capability 缺失均形成断点，禁止插值或跨断点连线。

### 2.6 完整诊断信息保留区

趋势带下方继续保留完整 13 类遥测域状态矩阵和原有 14 张详细诊断卡片。总览页的详细矩阵作为纵向补充；飞行健康页将 13 类矩阵提升为首屏主视觉。所有页面均可纵向滚动查看完整 14 张诊断卡，不允许删除、折叠丢失或只保留摘要。

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

13 类 family 矩阵仍逐项显示 operation、navigation、sensor health、control、actuator、link、storage、resource、time、event、pre-arm、RC input、build info 的独立状态和接收年龄。event 是异步域，不使用周期数据超时语义。

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
- 页面底部复用地面站本地 1 Hz 趋势；不把健康分、振动 RMS、电池温度或电压一致性等未提供字段伪装成遥测。

### 2.9 导航时序页面

- 主体按导航估计总览、导航能力与定位精度、时间同步、状态校验四块组织。
- 导航数值只展示 positionStdM、velocityStdMps、headingStdRad 和协议能力掩码；缺少能力位时显示不可用。
- 时间面板显示 timeQuality、timeDisciplineState、utcValid、utcUncertaintyUs、eventUncertaintyUs、timeEpoch 和接收年龄。
- 趋势图使用水平位置、速度、航向和 UTC 时间不确定度；UTC 无效/超时产生断点。
- 当前 FlightTelemetry 未提供卫星数、HDOP、每颗卫星方位/仰角，禁止绘制虚构卫星天图或填入示例值。

### 2.10 遥控与链路页面

- 主体按左侧双摇杆、逻辑轴实时输入、链路质量、失控保护四块组织。
- 双摇杆手型直接读取 FlightRadio 的持久化 Mode 1/2/3 设置，按其 channel-to-axis 映射绑定横滚/俯仰/偏航/油门；marker 仅在对应两个轴均新鲜且 validity bit 有效时显示。
- 当前协议只提供四个归一化逻辑轴，不提供 CH1–CH8 原始 PWM；页面明确显示四个逻辑轴，不虚构额外通道。
- 链路质量逐实例显示 linkKind、linkState、RSSI、质量、丢包、原因和接收年龄；指标必须同时满足 link family 新鲜、实例 receiveAgeMs 未超时、validityMask 对应位有效。
- 失控保护显示 failsafeLevel、rcReceiverFailsafe、rcLinkState、RC 接收年龄和 operation/link family 状态。
- 底部四张趋势图按页切换为 RSSI、链路质量、丢包率和 RC 接收年龄；无效/过期样本形成断点。


## 3. 响应式布局规范

布局依据 SetupPage.availableWidth，页面外壳和左侧主导航由 GroundControl 现有应用框架提供。

| 可用宽度 | 主页面网格 | 健康矩阵（健康页） | KPI | 页签 |
|---|---:|---:|---:|---:|
| ≥1280 px | 12 列 | 4 列 | 6 列 | 4 列 |
| 1000–1279 px | 6 列 | 2 列 | 3 列 | 4 列 |
| 760–999 px | 6 列 | 2 列 | 3 列 | 4 列 |
| 420–759 px | 1 列 | 1 列 | 2 列 | 2 列 |
| <420 px | 1 列 | 1 列 | 1 列 | 2 列 |

- 总览主视觉宽屏使用左 3 / 中 5 / 右 4 列；健康页使用左 3 / 中 6 / 右 3 列；导航和 RC 页按各自四块面板分配 12 列。
- 1000–1279 px 时主页面使用 6 列；760–999 px 时每页面板按 6 列换行；低于 760 px 时主页面单列纵向排列。
- 13 类健康矩阵只在「飞行健康」页面显示，宽屏 4 列、中屏 2 列、窄屏 1 列；总览页不重复显示矩阵。
- 页面标题/只读徽标在 420 px 以下纵向排列；KPI、主视觉、趋势、矩阵和明细卡自然增高，不得裁切长原因或 hash。
- UI Integration 覆盖 1600/1400/1000/900/700/400 px，验证页签、主网格、健康矩阵、标题徽标和内容边界。

## 4. 视觉规范

### 4.1 视觉方向

严格采用用户上传四张参考图的深色航空航天 HUD 大屏风格；四个页面共享应用外壳、标题带、页签和 KPI 带：深海军蓝/蓝黑背景、深青色分层面板、青蓝色高亮描边与局部光感、细密分隔线、清晰网格和紧凑高密度信息布局。保留绿色正常、黄色注意、橙色警告、红色故障的状态色。视觉效果要有专业指挥中心的层次与科技感，但不允许用装饰图形替代真实数据。

### 4.2 色彩令牌

| 令牌 | 色值 | 用途 |
|---|---|---|
| 页面背景 | `#111B18` | 大屏背景 |
| 卡片背景 | `#1B2A26` | 主诊断卡片 |
| 卡片内层/摘要背景 | `#20332E` | 摘要卡、徽标底色 |
| 边框/分隔线 | `#344A43` | 卡片边界和细分隔 |
| 主强调青绿 | `#42CFA8` | 只读界面强调、选中焦点 |
| 新鲜/已接收 | `#45D39A` | 数据组接收新鲜 |
| 超时/注意 | `#F0B94B` | 数据超时、降级/注意 |
| 警告/需动作 | `#F08B45` | 明确的警告或需要动作状态 |
| 严重/故障 | `#F06464` | 明确的故障、终止或无效值 |
| 未知/未上报 | `#8A9A94` | 未知、未上报、等待数据 |

颜色仅用于表达已知状态，不得通过视觉颜色把未知值包装成正常状态。文字标签必须同时存在。

### 4.3 卡片与排版

- 主卡片圆角 6–8 px，边框 1 px，采用深青蓝面板和低强度青色边界；主视觉卡可使用更明显的青色边界。
- KPI 带采用连续分隔的横向指标舱，最小高度 76 px，内容换行时自然增高；三栏主视觉使用左窄/中宽/右侧告警的稳定比例；趋势卡使用统一高度和绘图区。
- 卡片标题采用“英文小类目 + 中文主标题”两级层次；英文类目仅作为辅助定位。
- 卡片头部右侧显示该 family 的独立接收徽标。
- 徽标使用小圆点、状态文字和淡色底；不要用大面积高饱和背景。
- 字段标签低对比度，字段值高对比度并右对齐；单位紧跟数值。
- 关键状态值可使用较大字号；普通诊断字段保持紧凑且可读。
- 保留字段之间的轻量分隔线；卡片间距 12–16 px。
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

- 页面标题、只读标识、四段视觉分区条、六项 KPI、三栏主视觉、拓扑 Canvas、告警列表、趋势区、13 类状态矩阵和 14 张明细卡均具有稳定 `objectName`。
- 六项 KPI 卡片最小高度 76 px，实际高度必须随中文详情换行自然增高；内部内容不得超出卡片边界。
- 右侧告警事件条目按原因文本自然增高，长原因不得覆盖下一条事件。
- 四段视觉分区条仅是静态视觉元素，不允许触发路由或加载子页面。
- 宽屏主视觉必须按 3/5/4 列显示，中心面板宽于左右面板；右栏必须显示真实 recentEvents 和四轴 RC 输入。
- 拓扑示意不得使用位置/姿态遥测语义；不得出现合成健康分、电池、卫星、HDOP、振动等当前接口未提供的 KPI。
- Phase 2 飞行态势、导航估计、RC 输入、13 类健康矩阵和四个趋势面板均有稳定 `objectName`，由 UI Integration 测试断言。
- RC 四轴面板按有效轴掩码显示；无效轴必须显示“无效/未上报”且不绘制有效填充。
- 健康矩阵必须覆盖 13 个 family，逐项显示独立状态和接收年龄；event 按异步域处理。宽/中/窄/极窄宽度分别使用 7/4/2/1 列，所有单元不得越出网格边界。
- 趋势区明确标注 GCS 1 Hz 采样/最近 60 点，不得标注为 FCU 原生历史。
- 桌面 12 列布局中，解锁前检查与最新事件并排；资源/存储/时间同步并排；遥控输入与最近事件历史并排；固件与版本位于底部全宽。
- 原有 `flightTelemetry_page`、`flightTelemetry_operationMessageStatus`、`flightTelemetry_storageFreeKib` 等测试选择器必须保留。
- 页面完整保留所有现有诊断卡片和字段。

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

- 主视觉区在 12/6/1 列布局下按设计跨度排列；健康矩阵和趋势图在宽/中/窄布局下均不横向裁切。
- 1600 px 页面宽度使用 12 列。
- 1000 px 页面宽度使用 6 列。
- 700 px 页面宽度使用 1 列。
- 遥测域矩阵在 1600/1000/700/400 px 下分别使用 7/4/2/1 列；每个状态单元的 x/y/width/height 均位于矩阵网格边界内。
- 改变宽度时卡片跨度同步改变，不能出现 0 列、负跨度或超出列数的跨度。

### 8.4 趋势采样与状态矩阵

- 无遥测时趋势采样不生成有效点，矩阵所有未收到 family 显示等待/未收到。
- 真实导航/控制帧进入 MockLink wire path 后，趋势有效样本计数增加，数值与 Fact 一致。
- family 超时/无效或对应导航能力 bit 缺失后写入断点；Canvas 不跨断点连线。
- 水平位置标准差要求 bit3，速度标准差要求 bit5 与 bit6 同时置位，航向标准差要求 bit2；新鲜消息但能力缺失时显示“未具备”，不显示数值。
- 样本上限为 60；通过切换 active Vehicle 和 boot epoch 两种真实路径验证清空历史。
- 页面隐藏时停止采样，不增加额外 Timer。
- RC 有效轴按 mask 显示，清除 mask 位后对应轴不得继续显示最后有效位置。

### 8.5 安全与回归

- 不新增任何控制命令或写入路径。
- 既有 13 类遥测解码、分发、freshness、timestamp、boot epoch、profile 和多 Vehicle 隔离测试必须继续通过。
- `FlightTelemetryPageUITest` 保持 Integration / NoSanitizer，覆盖真实 wire path 和页面展示；导航 capability gap 测试必须在确认新 mask 到达 QML 后再记录采样计数基线，并等待后续 1 Hz tick，避免旧 mask 样本误满足断点断言。
- 最终在精确 main HEAD 上通过 Windows x64 MSVC Release、Unit、Integration、Executable、Installer、Clean Install、Installed Runtime 和证据归档。

## 9. 实施分期

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

### Phase 2.3 — 用户确认参考图布局迁移（本轮实施）

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


## 15. Phase 2.3 参考图布局迁移记录（2026-11-15）

- 设计基线改为用户确认的「飞控诊断总览」参考图；本次只重构现有 FlightTelemetry 单页的布局与视觉，不拆分诊断页面。
- 页面顺序：标题/只读状态 → 四段静态视觉分区条 → 六项真实 KPI → 左/中/右三栏主视觉 → 四条真实 GCS 趋势 → 13 类遥测状态矩阵 → 原有 14 张详细诊断卡。
- 左栏不显示虚构健康分；中栏拓扑仅为静态连接示意；右栏事件来自 recentEvents，RC 输入继续受 validity mask 和 freshness 约束。
- 当前接口未提供的电池、卫星、HDOP、振动、吞吐量等指标不得为了模仿参考图而展示。
- 代码和 UI Integration 断言随本轮提交更新。Windows Release/Unit/Integration/Installer 等最终验收结果以新提交的 exact-HEAD CI 和用户本地 RUN_ID 为准；未取得证据前不得标记 PASS。

## 16. 2026-11-15 参考图布局重构实施状态

- 当前工作提交以 GitHub main 的最新提交为准；本轮修改文件：`FlightTelemetry.qml`、`FlightTelemetryPageUITest.cc`、本规范和 `spec/README.md`。
- 本地 Windows x64 MSVC Release、Unit、Integration、Installer、clean-install、installed-runtime 尚未在本轮执行；不得将 GitHub 提交成功视为构建或测试通过。
- 用户本地复测需重点检查：宽屏三栏比例、中文字体清晰度、顶部 KPI 横向密度、右栏事件/RC 输入高度、趋势图断点、下方 14 张明细卡完整性，以及 100%/125%/150% DPI。


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
- 趋势面板复用现有 1 Hz / 60 点采样器：总览/健康页显示导航与控制指标，导航页第 4 张切换为 UTC 不确定度，RC 页切换为 RSSI/质量/丢包/RC 接收年龄。
- UI Integration 对四页切换、各页主要面板、响应式 12/6/1 列、健康矩阵 4/2/1 列、Mode 1/2/3 映射和 RC marker 有效性进行回归。
- 视觉验收仍需在 Windows x64 实际运行应用，对照四张原图逐页检查 1672×941、100%/125%/150% DPI；CI 布局断言不能替代像素级人工验收。
- Exact HEAD Windows Release、Unit、Integration、Installer、clean-install、installed-runtime 和本地 RUN_ID 证据必须重新执行；未完成前 FGC-0044 保持 RETESTING。
