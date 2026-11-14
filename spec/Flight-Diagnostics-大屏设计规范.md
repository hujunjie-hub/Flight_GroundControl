# 飞控诊断高级大屏设计规范

- 版本：2.0
- 日期：2026-11-14
- 状态：第二阶段设计已批准，QML 实施中；第一阶段和第二阶段均需按精确 main HEAD 验收
- 适用仓库：`hujunjie-hub/Flight_GroundControl`
- 页面：`src/FirmwarePlugin/Flight/FlightTelemetry.qml`
- 跟踪 Issue：[FGC-0044 飞控诊断页高级大屏布局与可视化改造](https://github.com/hujunjie-hub/Flight_GroundControl/issues/44)
- 设计依据：用户于 2026-11-14 提供的当前飞控诊断页截图

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

## 2. 页面信息架构

### 2.1 顶部标题与连接摘要

页面顶部固定展示：

- 主标题：`飞控诊断`，配合小号英文标识 `FLIGHT DIAGNOSTICS`。
- 只读标识：`READ ONLY`。
- 当前 FCU 遥测接收状态：未连接、等待 FCU 遥测、已收到 FCU 遥测。
- 一句简短提示：遥测接收不等于所有数据组均新鲜或健康。

`telemetryAvailable` 仅表示已经收到 FCU 遥测，不代表所有消息组都已收到、仍然新鲜或状态健康。顶部不得据此显示“系统健康”。

### 2.2 关键摘要区

标题区下方放置四张关键摘要卡：

| 摘要卡 | 数据来源 | 展示规则 |
|---|---|---|
| FCU 遥测 | `telemetryAvailable`、接收状态 | 明确区分未连接、等待遥测、已收到遥测；提示分组新鲜度需查看下方 |
| 飞行状态 | `armState`、`flightMode`、`authority` | 仅在 operation family 可用时显示真实值；否则显示未上报 |
| 失效保护 | `failsafeLevel`、`operationReasonCode` | 只映射协议明确枚举；未知/无效值不能映射为正常 |
| 导航估计 | `positionStdM`、`velocityStdMps`、`headingStdRad` | 显示标准差原值和单位，不自行给出“定位可靠”结论 |

摘要卡不是全局健康评分。任何 family 未上报时，必须显示未知/未上报，不能回退为 0 或健康。

### 2.3 诊断卡片区

按操作优先级组织现有数据域：

| 顺序 | 卡片 | 内容 |
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

不得因卡片重新布局而删除原有字段、Validity 检查、原因码解释或 per-instance 接收年龄。

### 2.4 指挥中心主视觉区（Phase 2）

在四张摘要卡之后、完整诊断卡片之前增加三块真实数据驱动的主视觉面板：

| 面板 | 主要数据 | 展示方式 | 限制 |
|---|---|---|---|
| 飞行态势 | armState、flightMode、authority、failsafeLevel、operation family 状态 | 大字号模式/解锁状态、控制权和失效保护等级 | 不模拟姿态，不将遥测接收等同于可飞 |
| 导航估计 | positionStdM、velocityStdMps、headingStdRad、navigation family 状态 | 三个并列精度指标，显示原始值和单位 | 没有 ICD 阈值时不显示“优/良/差”或可靠度评分 |
| RC 输入 | rcRollNorm、rcPitchNorm、rcYawNorm、rcThrottleNorm、rcValidAxesMask | 四轴真实输入条；横滚/俯仰/偏航以 0 为中心，油门按 0..1 显示 | 严格按 valid-axis mask 显示；无效轴不画成零；明确标记为“遥控输入”，不能称为飞行器姿态 |

桌面 12 列时三块面板各占 4 列；6 列时各占 3 列；窄屏单列。主视觉区只突出已收到的数据，所有未知/无效/超时值保留其原有状态语义。

### 2.5 13 类遥测健康矩阵（Phase 2）

增加一张全宽状态矩阵，逐项显示 operation、navigation、sensor health、control、actuator、link、storage、resource、time、event、pre-arm、RC input、build info 共 13 个数据域。每项独立显示状态文字、接收年龄和对应状态色。

- 这是消息域接收/新鲜度矩阵，不是系统健康评分。
- event 为异步事件域，不使用周期数据超时语义。
- 传感器、链路和资源列表仍须保留实例级接收年龄；family 状态不得掩盖实例超时。
- 任意单项正常不得推导其他数据域正常。

### 2.6 遥测趋势（Phase 2）

增加地面站本地采样趋势区，使用现有页面 500 ms 年龄刷新 Timer 的每第二次 tick 采样（1 Hz），不新增 Timer。最多保留最近 60 个采样点，页面隐藏时停止采样。

| 趋势 | 来源 | 采样准入 |
|---|---|---|
| 位置标准差 | positionStdM | navigation family 新鲜、数值有限且非负 |
| 速度标准差 | velocityStdMps | navigation family 新鲜、数值有限且非负 |
| 航向标准差 | headingStdRad | navigation family 新鲜、数值有限且非负 |
| 控制求解耗时 | controlSolveTimeUs | control family 新鲜、数值有限且非负 |

- 每个采样点附带有效/缺失状态；family 未收到、超时、Fact 无效或数值非法时写入断点。
- 图表不得跨断点连线，不得用 0、上一次有效值或插值填补缺失样本。
- 缓冲区最多 60 点；切换 Vehicle 或检测到 opBootIdHex 启动 epoch 变化时清空，禁止跨飞行器/启动周期拼接曲线。
- 图表标题和说明必须明确标注“地面站 1 Hz 采样 / 最近 60 点”；采样点是 GCS 观察值，不是 FCU 原生历史时间序列。
- 样本不足 2 个有效点时显示“采集中”，不能画出貌似完整的趋势。
- 趋势仅供诊断观察，不构成安全阈值、告警或控制依据。
## 3. 响应式布局规范

布局依据 `SetupPage.availableWidth`，不是屏幕物理分辨率。页面在已有 SetupView 可用区域内自适应。

| 可用宽度 | GridLayout 列数 | 普通卡片跨度 | 用途 |
|---|---:|---:|---|
| ≥ 1450 px | 12 | 4 列 | 桌面/大屏，三张普通卡并排 |
| 760–1449 px | 6 | 3 列 | 中等窗口，两张普通卡并排 |
| < 760 px | 1 | 1 列 | 窄窗口/小屏，单列纵向滚动 |

特殊卡片跨度：

- 解锁前检查：宽屏 8 列；中等宽度 6 列；窄屏 1 列。
- 最新事件：宽屏 4 列；中等宽度 6 列；窄屏 1 列。
- 最近事件历史：宽屏 8 列；中等宽度 6 列；窄屏 1 列。
- 固件与版本：宽屏 12 列；中等宽度 6 列；窄屏 1 列。

卡片必须允许内容自然增高；不得固定高度裁切长原因、hash、逐项 prearm 或实例列表。窄屏允许页面纵向滚动，不允许通过压缩字号隐藏字段。

## 4. 视觉规范

### 4.1 视觉方向

采用深色工业控制台风格：低反射深背景、分层卡片、青绿色主强调、明确的状态色和统一的网格间距。避免大面积渐变、发光描边、装饰性动画和与飞控数据无关的视觉噪声。

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

- 主卡片圆角建议 8 px，边框 1 px，内边距 14 px。
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

- 页面标题、只读标识、四张摘要卡、主诊断 GridLayout 和主要卡片具有稳定 `objectName`。
- Phase 2 飞行态势、导航估计、RC 输入、13 类健康矩阵和四个趋势面板均有稳定 `objectName`，由 UI Integration 测试断言。
- RC 四轴面板按有效轴掩码显示；无效轴必须显示“无效/未上报”且不绘制有效填充。
- 健康矩阵必须覆盖 13 个 family，逐项显示独立状态和接收年龄；event 按异步域处理。
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
6. 摘要字段必须受对应 family availability 保护。
7. 失效保护摘要和详情行的颜色必须跟随 operation family freshness；过期的最后值不得继续显示为绿色。

### 8.3 响应式布局

- 主视觉区在 12/6/1 列布局下按设计跨度排列；健康矩阵和趋势图在宽/中/窄布局下均不横向裁切。
- 1600 px 页面宽度使用 12 列。
- 1000 px 页面宽度使用 6 列。
- 700 px 页面宽度使用 1 列。
- 改变宽度时卡片跨度同步改变，不能出现 0 列、负跨度或超出列数的跨度。

### 8.4 趋势采样与状态矩阵

- 无遥测时趋势采样不生成有效点，矩阵所有未收到 family 显示等待/未收到。
- 真实导航/控制帧进入 MockLink wire path 后，趋势有效样本计数增加，数值与 Fact 一致。
- family 超时/无效后写入断点；Canvas 不跨断点连线。
- 样本上限为 60；Vehicle/boot epoch 变化清空历史。
- 页面隐藏时停止采样，不增加额外 Timer。
- RC 有效轴按 mask 显示，清除 mask 位后对应轴不得继续显示最后有效位置。

### 8.5 安全与回归

- 不新增任何控制命令或写入路径。
- 既有 13 类遥测解码、分发、freshness、timestamp、boot epoch、profile 和多 Vehicle 隔离测试必须继续通过。
- `FlightTelemetryPageUITest` 保持 Integration / NoSanitizer，覆盖真实 wire path 和页面展示。
- 最终在精确 main HEAD 上通过 Windows x64 MSVC Release、Unit、Integration、Executable、Installer、Clean Install、Installed Runtime 和证据归档。

## 9. 实施分期

### Phase 1 — 当前实施

- 更新顶部标题与只读标识。
- 增加四张摘要卡。
- 重构卡片视觉体系和 family 状态徽标。
- 建立 12/6/1 列响应式布局。
- 保留所有现有诊断字段和数据语义。
- 扩展 `FlightTelemetryPageUITest` 验证页面结构、摘要未知态和响应式列数。

### Phase 2 — 指挥中心视觉与真实数据可视化（当前实施）

- 新增飞行态势、导航估计和 RC 输入三块主视觉面板；只使用现有有效 Fact 和 validity mask。
- 新增 13 类 family 独立状态/接收年龄矩阵，不生成综合健康分。
- 基于当前真实 Fact 增加 1 Hz GCS 本地采样趋势，最多 60 点；使用现有 500 ms Timer 的每第二个 tick，不创建新 Timer。
- 趋势数据严格 freshness/finite/non-negative gating；无效/超时样本形成断点，切换 Vehicle/boot epoch 清空缓冲。
- 重新建立主视觉层级、紧凑摘要带、健康矩阵、趋势区和原有详细诊断卡片之间的版式。
- 保留全部 14 张诊断卡片、字段、FactGroup、原有 objectName 和 MockLink wire-path 测试。
- 没有姿态角遥测，因此不绘制人工地平仪；RC 输入条不能被误认为飞机姿态。

### Phase 3 — 原生姿态/更多时序遥测（未来独立评估）

只有 Flight ICD 和发布端提供真实姿态角、原生采样时间序列及明确有效性语义后，才评估真实人工地平仪、航迹/姿态曲线和更多原生趋势。必须先完成 Flight 与 GroundControl 的协议一致性及回归测试。

## 10. 变更边界

第二阶段允许修改：

- `src/FirmwarePlugin/Flight/FlightTelemetry.qml`
- `test/QmlUITests/FlightTelemetryPageUITest.cc`
- `doc/gcs/FCU_TELEMETRY.md`
- 本设计规范、Windows Release 验收规范和机器可读 manifest
- 必要的 QML UI 测试辅助文件
- `doc/gcs/FCU_TELEMETRY.md`
- 本设计规范、验收规范和机器可读 manifest

未经单独批准，不修改：

- `src/FirmwarePlugin/Flight/FlightTelemetryFactGroup.*`
- MAVLink 消息定义、Flight ICD 和生成头文件
- Flight firmware 发布端
- 控制权限、ARM、FMS、Permit、Failsafe 或执行器逻辑
