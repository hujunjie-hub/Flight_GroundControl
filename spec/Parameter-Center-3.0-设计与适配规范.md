# 参数中心 3.0 设计与适配规范

- 版本：1.0
- 日期：2026-11-15
- 状态：设计基线已确认，分阶段实施
- 适用仓库：hujunjie-hub/Flight_GroundControl
- 目标页面：Flight GroundControl 参数中心
- 视觉基线：用户确认的 1672 × 941 参考图“飞控参数中心_3.0_仪表盘.png”

## 1. 目标与范围

将现有通用参数表升级为围绕“当前唯一绑定飞行器”的专业调参工作台。页面采用已确认参考图的深色航空航天风格、单机型下拉选择、五步工作流、左侧参数域导航、中部数字孪生与控制架构、参数表、在线调参、响应验证和底部连接状态。

本规范定义页面信息架构、布局、视觉、交互、数据约束、安全边界、实施阶段和验收要求。实现必须保留现有参数读写能力，不以视觉重构为由删除参数、放宽写入条件或绕过 QGroundControl Fact / ParameterManager / FirmwarePlugin 架构。

### 1.1 本次范围

- 参数中心页面及其必要的可复用 UI 组件。
- 当前飞行器模型的唯一绑定上下文。
- FMS、CONTROL、INS、PLANT、ACTUATOR 等参数域的组织与筛选。
- 数字孪生模型、控制架构示意、参数健康检查。
- 参数表、在线调参、响应曲线、影响分析、仿真验证入口。
- 参数写入、回读确认、恢复上一次未确认修改的安全流程。
- 与参数中心直接相关的 SPEC、测试和本地验收记录。

### 1.2 明确不做

- 不增加多机型并排卡片、机型轮播或多飞行器参数对照工作区；一次只显示一个当前绑定机型。
- 不增加配置方案市场、方案库、版本管理、版本历史、版本发布或多方案切换面板。
- 不把通用日志、系统管理、飞行诊断大屏、任务规划页面内容搬入参数中心。
- 不新增独立参数数据库，不绕过现有 Fact / ParameterManager。
- 不以示意数据冒充实时遥测、真实参数、仿真结果或 AI 分析结果。
- 不在本轮视觉重构中改动飞控算法、MAVLink 参数协议、Flight FMS 授权或飞控安全逻辑。

## 2. 唯一视觉基线与还原原则

用户确认的参考图是本页面唯一视觉基线。后续实现不得自行改成普通管理后台、通用表格页或其他仪表盘风格。

参考画布为 1672 × 941。实施 Phase 0 时必须将用户确认的原始 PNG 原样归档到：

- 目标文件：spec/assets/Parameter-Center-3.0-Reference.png
- 源文件：本轮对话附件“飞控参数中心_3.0_仪表盘.png”

必须保留原始分辨率和原始像素，不得重新绘制、裁剪、压缩覆盖或用生成式新图替代。参考图是布局、比例、视觉层级和控件形态的对照依据；图中参数值、健康分数、遥测值、模型规格和曲线均为示意内容，不是固件默认值或真实测试结论。

像素级移植的定义：
1. 在 1672 × 941 基准视口下，应用外壳、导航、流程条、主面板位置、尺寸比例、间距、边框、圆角、文字层级和控件状态尽可能与参考图一致。
2. 动态数据必须来自真实 Fact、参数元数据、飞控遥测或真实仿真输出；不得为了截图一致而硬编码参考图中的数值。
3. 数据内容不同可以导致文字长度和数值变化，但不得改变面板顺序、主栅格、信息层级和视觉令牌。
4. 在其他 DPI / 分辨率下保持同一布局逻辑和视觉比例，允许按断点重排，不允许横向裁切或丢失功能。

## 3. 页面总体信息架构

### 3.1 顶部应用栏

- 沿用 GroundControl 现有应用导航和状态来源。
- 参数中心入口处于选中态。
- 右侧连接状态、电池、GPS/HDOP 等指标只在现有真实数据可用时显示；不得创建假状态。
- 不在参数中心顶部增加配置版本、配置方案、版本切换或历史管理入口。

### 3.2 当前飞行器唯一绑定区

顶部第二行左侧显示“当前飞行器（唯一绑定）”和一个下拉选择器。

规则：
- 当前参数中心只有一个 active vehicle profile / aircraft model binding。
- 下拉菜单列出 FirmwarePlugin 实际注册且当前固件可用的机型/构型；每次只允许选择一个调参 profile，参数表按该 profile 的 parameterNames / parameterDomains 过滤。
- 选择 profile 只改变参数调优上下文，不创建第二个 Vehicle，也不改变 activeVehicle 对应的真实硬件类型。Flight SIL 可单选注册的 SIM_ONLY / SIL_ONLY profile；实机 profile 必须补齐硬件证据、构型/电机映射、推力曲线和兼容性审查后才能进入可选列表。
- 当前绑定项显示机型名称与必要的构型标识，例如“多旋翼 · X 型六轴”。
- 机型切换只切换参数中心的模型上下文，不代表改变真实飞行器硬件、固件类型、混控或飞控控制律。
- 已连接飞控时，必须将所选模型与当前 Vehicle、FirmwarePlugin、飞控上报能力和参数元数据做兼容性检查；不匹配时阻止参数写入并清楚说明原因。
- 切换模型前若存在未应用修改，必须提示先应用、恢复或取消切换；不得静默丢弃修改。
- 任一时刻只显示一架当前绑定飞行器的数字孪生、参数和调参对象。
- 不新增多飞行器并列视图；QGC 原有多 Vehicle 能力不在本次被删除，但参数中心必须明确作用于当前 activeVehicle。

### 3.3 顶部五步流程条

固定顺序：
1. 选择机型
2. 选择参数域
3. 在线调参
4. 仿真验证
5. 应用与保存

交互规则：
- 当前步骤高亮，已完成步骤显示完成状态，未满足前置条件的步骤显示未就绪。
- 点击步骤可导航到对应工作区；不能通过点击流程条绕过校验、连接检查或写入确认。
- 流程条是任务进度和页面导航，不是配置方案或版本管理。
- 选择机型后才允许进入参数域；选择参数后才允许进入对应调参视图；仿真和应用步骤按真实能力、状态与校验结果开放。

### 3.4 左侧参数域导航

按参考图顺序展示：
1. 飞行器与模型（Aircraft & Model）
2. FMS 任务参数（FMS Mission Params）
3. CONTROL 控制器（Control Parameters）
4. INS 导航参数（INS Parameters）
5. Plant 动力学模型（Plant Dynamic Model）
6. 执行器配置（Actuator Configuration）
7. 在线调参（Online Tuning）
8. 仿真与验证（Simulation & Validation）

规则：
- 导航只切换当前绑定机型下的参数域或相关工作区，不改变当前机型绑定。
- 当前选中域采用青色高亮；其他项保持深色弱强调。
- 每个域只展示元数据确认适用的参数。
- 没有对应参数或固件能力时显示明确的“不支持 / 未提供 / 尚未配置”空态，不显示伪造参数。
- FMS、INS、CONTROL、PLANT 是参数域分类，不意味着本页面重新实现这些飞控模块。

## 4. 主内容区排版

### 4.1 基准尺寸与栅格

基准画布：1672 × 941。

- 顶部应用栏约 60 px 高。
- 当前飞行器与五步流程区位于顶部应用栏下方，整体约 76 px 高。
- 左侧参数域导航约 190 px 宽，纵向占据主内容高度。
- 主内容区使用两层主栅格：上层模型/架构/健康度，下层参数/调参/响应验证。
- 主内容区外边距约 10–12 px；卡片间距约 12 px；内部采用 8 px 间距基线。
- 底部状态栏约 54 px 高。
- 1672 × 941 下优先复现参考图中的面板位置、宽高、比例和视觉重心；不使用绝对坐标将布局锁死在单一分辨率。

### 4.2 上层三张卡片

左：当前飞行器数字孪生
- 显示当前唯一绑定机型的 3D 模型或可用的真实模型视图。
- 旁边显示已知的机型规格：轴距/翼展、质量、惯量、最大推力/动力能力、电机或舵机数量等。
- 仅显示当前模型元数据或飞控实际提供的数据；未知字段显示“未配置 / 未上报”，不得填入示例数值。
- 模型视图是配置与结构表达；若没有真实姿态遥测，不得让静态模型假装实时跟随飞行姿态。

中：飞控系统架构（当前机型）
- 根据当前机型和真实控制能力展示 FMS、MPC/位置外环、姿态控制、角速度控制、执行器、INS、Plant 等模块关系。
- 模块名称、连线和层级必须来自当前固件/模型的已知架构定义。
- 点击模块可切换左侧参数域或过滤参数表。
- 对未实现或未确认的控制模块必须标注未提供，不得用参考图的架构示意声称固件已经具备该能力。

右：参数健康度
- 展示当前参数集合的范围有效性、依赖关系一致性、模型匹配度、控制器约束检查和安全约束检查。
- 健康状态必须由可追溯的校验规则计算。
- 不存在正式评分算法时不得显示 96/100 等综合分数；改用检查项状态列表，保持参考图的面板结构。
- 每项状态必须能定位到具体参数、规则和原因。

### 4.3 下层三张主卡片

左：参数调校表
- 表格列：选择、参数名称、当前值、调节目标值、单位、允许范围、状态。
- 行选中态采用参考图的青色强调。
- 参数名、单位、范围、当前值来自 Fact 与参数元数据；不硬编码显示文本替代参数。
- 目标值为本地待应用值，必须与飞控当前值明确区分。
- 支持多行选择，以服务批量调参；批量修改前显示变更数量与校验结果。
- 现有搜索、修改项过滤、收藏、只读过滤、导入/导出和重置等能力不得删除；不常用操作收纳到参数表内部的紧凑工具入口，避免破坏参考图布局。
- 不在主页面增加版本历史、配置方案管理或配置库面板。

中：在线调参
- 显示调参模式、当前选中参数、参数说明、当前值、目标值、单位、上下限和调节控件。
- 控件根据参数类型选择数值输入、步进按钮、滑块或枚举选择；不得所有参数统一使用滑块。
- 显示“应用到飞控”“回读验证”“恢复上一次”操作。
- “恢复上一次”仅恢复本次工作会话内的上一次已知参数值/待应用修改，不等同于完整配置版本回滚。
- 选择参数表行后，在线调参面板必须同步切换到该参数。
- 调参模式至少保留参考图中的实时在线调参、试探调参、批量调参入口；每种模式是否可用由飞控能力、参数策略和安全状态决定。

右：响应与验证
- 顶部选择当前分析量/轴向，例如位置响应（北向）；可按真实数据支持切换位置、速度、姿态或角速度响应。
- 曲线必须标明真实数据来源、单位、时间轴和当前/目标/误差语义。
- 当前参数或目标值变化后，图表分析对象同步更新。
- 下方包含参数影响分析和 AI 调参建议两个次级面板。
- 影响分析只显示参数依赖图中明确定义的上下游关系及实际计算结果；无依赖定义时显示“暂无已定义影响关系”。
- AI 建议只作为可审阅建议，必须说明所用数据、约束和依据；不得自动写入飞控，不得伪造收益百分比或稳定性结论。

### 4.4 底部状态栏

只保留参数中心工作所需的即时状态：
- 飞控连接状态及通信是否正常。
- 当前唯一绑定飞行器。
- 当前控制架构（仅在已确认时显示）。
- 参数数量。
- 待修改数量。
- 参数同步/回读状态。

不得放置配置版本、配置方案、版本历史或与当前调参无关的系统信息。

## 5. 视觉系统

### 5.1 颜色基线

以用户确认图片为准，以下色值作为实现初始令牌，最终通过截图叠加微调：

| 角色 | 建议色值 | 用途 |
|---|---|---|
| 页面背景 | #04151D | 主背景 |
| 顶栏/底栏 | #061C26 | 应用外壳 |
| 主面板 | #082631 | 一级卡片 |
| 次级面板 | #0B303B | 内层卡片/输入区 |
| 面板边框 | #087783 | 卡片描边 |
| 主强调青色 | #18DDE2 | 当前项、焦点、主要操作 |
| 次强调蓝色 | #1689C5 | 次级按钮、图表辅助线 |
| 正常绿色 | #37DF8A | 有效、通过、已同步 |
| 注意琥珀色 | #E9B34F | 待应用、警告、部分有效 |
| 故障红色 | #EF6D78 | 错误、拒绝、失效 |
| 主文字 | #E8F4F6 | 标题、数值 |
| 次级文字 | #91ADB6 | 英文副标题、说明 |

### 5.2 卡片、按钮和光效

- 卡片采用深色填充、1 px 青蓝边框、约 8–10 px 圆角。
- 选中卡片使用青色边框、低强度内发光和轻量背景提亮；不使用大面积高亮色填充。
- 主操作按钮采用青色高亮；次操作使用深色按钮和细描边。
- 状态图标使用统一线性图标，不使用 Unicode 装饰字符替代图标资源。
- 光效仅用于焦点、选中和重要状态，不允许每个卡片同时高亮。
- 曲线颜色与状态颜色保持一致性，但必须有图例和文字标签，不能只靠颜色区分。

### 5.3 字体与 DPI

- 服从《全项目字体系统与渲染规范》，使用 ScreenTools 语义字号、normalFontFamily、fixedFontFamily 和现有 QGC 字体控件。
- 中英文标题采用主标题 + 英文辅助副标题层级。
- 参数名/数字可按现有数字对齐规则使用等宽字体；普通说明不使用等宽字体。
- 不使用固定 font.pixelSize，不使用页面局部字号系数。
- 100%、125%、150% DPI 下保持字号角色和层级一致，优先通过布局重排解决空间问题。

## 6. 参数数据组织与机型适配

### 6.1 当前源码基线

当前 Flight 参数元数据文件为 src/FirmwarePlugin/Flight/FlightParameterFactMetaData.json，来源 middleware/protocol/mavlink/parameters/ground_parameters.json。

当前读取到的元数据包含 49 个参数，现有 group 分布为：
- 机体模型：1
- MPC 权重：21
- 位置环回退：6
- 姿态角速度环：17
- SO3 姿态增益：3
- 仿真：1

参数元数据已升级到 schema v2，包含 name/type/group/描述/单位/范围/默认值/domain/modelProfiles，以及 vehicleFamily、modelVariants、controlLaw、tuningPolicy、updatePolicy、dependencies、validationRules 和 readOnly。`rebootRequired` 仅在固件源目录明确声明时输出；缺失表示未知，不得用默认 false 冒充“无需重启”。当前 QUAD_X4 的适用性与控制律由 vehicle_model.yaml 显式声明，不能从参数名前缀或 domain 推断。

跨仓库模型资料核查（2026-11-15）：
- Flight 仓库当前 config/vehicle_model.yaml 仅登记 profile=QUAD_X4，status=SIM_ONLY；文件明确说明这是 synthetic X4 fixture，电机编号和偏航力矩符号没有硬件证据，hardware_thrust_curve 仍为 TBD。
- FlightFirmwarePlugin 当前只把 gitHash=FLTSIL01 且固件版本不低于 1.9.2 的 Vehicle 识别为 SIL。
- Flight_GroundControl 已接入 FlightVehicleModelCatalog.json 注册表，但目前只登记 QUAD_X4 的 SIM_ONLY / SIL_ONLY profile；它仍是 synthetic fixture，不能当作已验证的实机模型，也不能向硬件飞行开放参数写入。
- 模型下拉框现已来自正式注册的 profile；SIL 专用 profile 只有在 Flight SIL 身份校验通过时才显示。实机 profile 仍须补齐硬件证据、构型/电机映射、推力曲线和兼容性审查后才能注册开放。

### 6.2 元数据扩展目标

保留原始参数身份、group、单位、范围和默认值；当前 schema v2 已接入下列适用性与调参策略字段。未明确声明的依赖或组合规则保持空数组，不推断：

| 字段 | 含义 |
|---|---|
| domain | VEHICLE_MODEL / FMS / CONTROL / INS / PLANT / ACTUATOR / SIMULATION |
| vehicleFamily | 适用飞行器大类 |
| modelVariants | 适用的具体构型 |
| controlLaw | 适用控制算法/控制架构 |
| section | 参数域内的功能分组（待接入） |
| tuningLevel | 基础 / 高级 / 专家（待接入） |
| tuningPolicy | 只读 / 地面可调 / 在线可调 / 受限飞行中可调 |
| updatePolicy | 即时生效 / 需飞控确认 / 需重启 / 仅离线 |
| dependencies | 已定义的参数依赖关系 |
| validationRules | 单参数及组合校验规则 |

- 未提供适用性信息的参数默认不跨机型推断，不允许因此扩大写入范围。当前 schema v2 对缺失的 vehicleFamily/modelVariants/controlLaw/tuningPolicy/updatePolicy 执行 fail-closed。
- 机型、控制律或固件能力未知时，相关参数显示为未知/未验证，默认只读。
- 参数域分类与原始 group 是不同概念；不得为了新界面破坏原始 group 和参数标识。
- 不删除现有 49 个参数，不更改参数名、单位、默认值、范围或来源协议，除非另有独立需求和证据。

## 7. 在线调参与安全流程

### 7.1 调参模式

- 实时在线调参：仅对固件明确支持即时更新且通过安全策略的参数开放。
- 试探调参：以受限小步长修改，记录修改前后值并检查响应。
- 批量调参：允许选择多个参数，但必须先做组合校验，再按协议和固件能力执行。
- 飞行中调参不是默认模式；只有明确白名单参数、固件端支持、当前飞行状态许可、变化率限制和恢复机制全部具备时才允许。未登记的参数在飞行中一律禁止写入。

### 7.2 单次参数应用流程

1. 确认 activeVehicle 存在且唯一绑定关系明确。
2. 检查通信链路、参数管理器状态、飞行/解锁状态和 pending writes。
3. 读取并记录当前值，保留本次修改前快照。
4. 检查参数适用机型、FirmwarePlugin 能力、范围、单位、依赖和 updatePolicy。
5. 将目标值发送给飞控，等待协议确认。
6. 再次读取参数并核对回读值。
7. 回读一致才标记“已应用”；超时、拒绝、回读不一致或通信丢失均显示失败/未知，不得显示成功。
8. 记录本次参数名、原值、目标值、实际回读值、时间和结果。
9. 在允许的策略内提供恢复上一次值；恢复同样必须经过校验、写入和回读。

### 7.3 关键安全约束

- 绝不绕过 Fact、ParameterManager、FirmwarePlugin 和现有 MAVLink 参数协议。
- 不以地面站 UI 状态替代飞控端安全许可。
- 失联、参数下载/写入未完成、Vehicle 销毁、activeVehicle 切换或连接身份变化时，立即停止新的写入操作并清楚显示状态。
- 需要 disarmed、重启或特定模式的参数必须严格遵循固件元数据和飞控返回结果。
- 飞行中在线调参必须由固件端能力和安全状态最终许可；地面站不能单方面授予。
- AI 建议、仿真结果和参数影响分析均不得直接触发参数写入。

## 8. 仿真、响应曲线和影响分析

### 8.1 响应曲线

- 支持显示真实目标值、实际反馈、误差或仿真预测值。
- 曲线图例必须明确区分 FCU 实测、地面站计算、SITL/仿真和预测曲线。
- 数据无效、超时、缺样或来源切换时必须断线或显示数据状态，不得插值制造连续性。
- 当前没有真实遥测或仿真输出时显示等待/不可用状态，不绘制装饰性曲线。

### 8.2 仿真验证

- 仿真入口只连接仓库实际存在且可调用的仿真能力。
- 仿真结果必须标记仿真环境、模型版本/输入参数、运行时间和数据来源。
- 不得把仿真通过标记为实机验证通过。
- 尚无可用仿真接口时保留面板布局和明确空态，后续阶段再接入，不伪造结果。

### 8.3 参数影响分析

- 仅依据显式 dependencies / control architecture mapping / 已验证模型计算影响关系。
- 每条影响路径显示来源参数、目标模块、方向和计算/规则依据。
- 没有正式关系定义时显示“暂无已定义影响关系”，不从参数名称或经验猜测影响。
- 参数影响百分比、性能提升或稳定性结论必须有可复现的计算/仿真证据。

### 8.4 AI 调参建议

- AI 面板是可选的辅助建议，不是参数控制器。
- 建议必须给出目标参数、当前值、建议区间、适用条件、证据和风险提示。
- 没有足够遥测/仿真数据时明确显示“数据不足，暂不建议”。
- AI 不自动写入飞控；用户必须手动确认并走完整参数校验/应用流程。

## 9. 现有功能保留与工程架构约束

- 复用现有 ParameterEditor、ParameterEditorController、Fact、ParameterManager 和 FirmwarePlugin 能力；先评估复用点，再决定是否拆分新组件。
- 保留现有搜索、修改项、收藏、只读过滤、参数导入/导出、默认值恢复和工具入口能力；通过紧凑工具栏/菜单收纳，避免主视觉拥挤。
- 新 UI 不建立第二套参数缓存或写入队列。
- 所有 Vehicle / activeVehicle 指针访问必须判空；遵守多 Vehicle 和 FirmwarePlugin 约束。
- QML 状态通过项目现有 Q_PROPERTY / Fact 暴露；不得在 QML 侧伪造参数写入成功状态。
- 不在本轮修改 Flight 固件端算法或安全逻辑。
- 参数中心与飞控诊断大屏保持页面职责分离；诊断页仍遵守其只读设计规范。

## 10. 分阶段实施计划

### Phase 0：设计资产归档与源码盘点

- 将用户确认的原始参考图原样归档至 spec/assets/Parameter-Center-3.0-Reference.png。
- 检查现有参数中心入口、VehicleSetup 路由、ParameterEditor、ParameterEditorController、Fact 元数据和参数读写调用链。
- 形成当前能力清单：已支持、部分支持、未支持、需要固件配合。
- 明确 activeVehicle / 唯一模型绑定的来源和切换规则。
- 本阶段只做资产和源码盘点，不改参数读写行为。

验收门槛：参考图已入库；现有参数能力、页面入口和安全边界有可追溯清单。

### Phase 0 当前源码盘点（main@de54b2f）

本次基于 main 分支源码完成只读盘点，未修改产品代码：

- VehicleConfigView.qml 将 QGroundControl.multiVehicleManager.activeVehicle 作为当前配置对象，并检查 parameterReadyVehicleAvailable、missingParameters 和 parameterDownloadSkipped；参数面板在参数可用后加载 SetupParameterEditor.qml。
- SetupParameterEditor.qml 当前只是对通用 ParameterEditor 的薄封装，尚无参数中心 3.0 的独立工作台布局。
- ParameterEditor.qml 使用 ParameterEditorController，现有 UI 包含搜索、全部/已修改/收藏筛选、隐藏只读、导入、工具菜单、刷新、默认值恢复、导出和参数差异预览等能力。
- ParameterEditorController 按 Fact 的 category / group 构建分类和分组，暴露 searchText、currentCategory、currentGroup、showModifiedOnly、showFavoritesOnly、hideReadOnly、favoriteParameterNames 等属性；参数文件差异发送最终通过 Fact::setRawValue 进入现有参数写入路径。
- ParameterManager 已提供 parametersReady、missingParameters、pendingWrites、loadProgress、parameterDownloadSkipped 等状态，以及参数刷新、批量刷新、读取、默认值恢复和现有 MAVLink 参数管理路径。当前参数写入确认等待常量为 1000 ms；新 UI 必须沿用现有 ParameterManager 行为，不能另建写入通道。
- FlightParameterMetaData::parseParameterJson 仅接受 schema v2、float 类型和七种受支持 domain；同时校验 vehicleFamily/modelVariants/controlLaw、tuningPolicy/updatePolicy、modelProfiles、dependencies、validationRules、readOnly/writeOnly/rebootRequired 与数值范围。非法或缺失的必需策略字段、未注册 profile、非法依赖、错误布尔类型、重复参数名均拒绝缓存并输出警告。通过校验的适用性、调参策略和更新策略映射到 FactMetaData / Fact，供控制器与 QML 读取；缺失 rebootRequired 保留未知态。实时参数仍可由通用 fallback 创建，但不会获得有效 domain，因此不会混入参数中心域过滤列表。
- FlightParameterFactMetaData.json 当前包含 49 个 Flight 参数，源文件标识为 middleware/protocol/mavlink/parameters/ground_parameters.json；group 分布为机体模型 1、MPC 权重 21、位置环回退 6、姿态角速度环 17、SO3 姿态增益 3、仿真 1。
- 当前唯一机型上下文应优先复用 activeVehicle 与对应 FirmwarePlugin / 参数元数据；页面下拉框不得创建第二套 Vehicle 选择/管理机制，也不得把切换 UI 模型误当作改变飞控硬件类型。

Phase 0 尚未完全结束：用户确认的原始 1672 × 941 PNG 尚未归档进仓库 spec/assets/Parameter-Center-3.0-Reference.png。完成该原图归档后，Phase 0 才可标记 PASS；当前状态为 PARTIAL。

### Phase 1：静态 UI 像素级还原

- 完成应用栏、唯一机型下拉区、五步流程条和左侧参数域导航。
- 完成上层数字孪生、控制架构、参数健康度三张卡片。
- 完成下层参数表、在线调参、响应验证及其影响分析/AI 建议区域。
- 完成底部状态栏。
- 先使用明确标注的 UI fixture / mock 数据进行静态布局开发；fixture 不得进入真实运行数据通路。
- 本阶段不宣称在线写入、仿真或 AI 能力已实现。

验收门槛：1672 × 941 基准截图的主栅格、面板比例、间距、字体层级、颜色和交互状态与参考图一致；现有参数读写代码路径未被破坏。

### Phase 2：唯一机型绑定与参数域动态适配

- 接入当前 activeVehicle 与唯一模型上下文。
- 完成机型注册信息、FirmwarePlugin 能力和参数元数据之间的适用性映射。
- 参数域导航驱动参数表、数字孪生规格、控制架构和参数健康检查。
- 机型切换时检查未应用修改、连接状态和参数兼容性。
- 未知或不适用参数不显示为可写。

验收门槛：任一时刻只有一个当前机型；切换机型后所有内容同步变化；不适用参数被隐藏或只读标识；没有丢失原始参数。

### Phase 3：在线调参闭环

- 参数表行与在线调参面板联动。
- 实现实时在线、试探、批量模式的能力门控。
- 完成当前值/目标值、范围校验、参数写入、协议确认、回读验证和恢复上一次。
- 处理超时、拒绝、失联、activeVehicle 切换、重复写入和参数管理器忙等异常。
- 飞行中调参只开放明确白名单，不默认开启。

验收门槛：每次写入均有协议确认和回读结果；失败不会显示成功；恢复操作也经过同样校验；没有旁路参数写入机制。

### Phase 4：响应验证、仿真和影响分析

- 接入真实遥测响应曲线。
- 接入仓库实际可用的仿真接口。
- 参数选中/修改后联动响应分析与影响关系。
- 无真实数据时显示空态，不生成示例曲线。
- AI 建议仅在数据来源和依据可审计时启用。

验收门槛：图表来源明确、无效数据不伪造连续性、仿真与实测清晰区分、影响分析可追溯。

### Phase 4 实施记录（首轮实测控制遥测趋势）

- 参数中心响应面板开始接入 Vehicle.flightTelemetry 的真实 FCU_CONTROL_STATUS 数据，采集 controlTimeBootUs、controlBootId、controlProfileId 和 controlSolveTimeUs。
- FlightTelemetryFactGroup 在每帧 FCU_CONTROL_STATUS 通过 boot/profile/时间戳/字段校验并完成 Fact 更新后发出 controlStatusUpdated；参数中心通过该信号逐帧采样，不再用 100 ms 地面站定时器轮询最新 Fact，避免高频控制遥测被轮询漏采或重复采样旧值。仅在 controlAvailable 为真且 familyStale.control=false 时记录，最多保留最近 120 个有效样本。
- 控制器 boot ID、controlProfileId 变化、飞控时间戳回退、GroundControl 机型调参上下文切换或活动飞行器切换时清空历史样本，避免跨启动/跨控制配置/跨机型上下文/跨飞行器拼接曲线。相邻有效样本间隔超过 FlightTelemetryFactGroup 的 3 秒 stale threshold 时断开曲线并单独绘制采样点，不跨缺样区间连线。
- 当前曲线明确标记为“控制求解耗时”，显示最近真实值；控制遥测失效时标注历史样本，样本不足两个时显示等待态。
- 曲线横轴按 FCU 启动时间戳等比例绘制，不按样本序号压缩时间；相邻有效样本间隔超过 3 秒时断开连线，并保留间隔两侧的真实采样点。实时可用状态同时要求 controlAvailable=true、链路正常且 familyStale.control=false。
- 曲线从当前有效样本计算最小值、平均值、峰值和实际时间窗；没有样本时显示等待态，不生成示例值。
- FlightTelemetryDispatchTest 增加 controlStatusUpdated 信号回归：合法新帧触发一次，重复时间戳和非法 solver_status 不触发，后续合法新帧继续触发。
- ParameterEditorController 仅在匹配的 PARAM_VALUE 写入确认成功后发出 parameterWriteConfirmed(componentId, parameterName, previousValue, confirmedValue)，保留飞控确认前后值；失败、超时或未授权写入不产生确认事件。
- 参数确认事件保留参数写入前值与飞控确认值，先记录当前 FCU boot ID、controlProfileId 与最后一个已接收控制时间戳，随后只在同一 boot ID / controlProfileId 下的下一帧更新 FCU_CONTROL_STATUS 到达时落为琥珀色时间标记；没有同会话/同控制配置遥测基线时只显示未关联说明，不伪造曲线标记。
- 参数标记与曲线样本在 boot ID、controlProfileId、时间戳回退、GroundControl 机型调参上下文或 activeVehicle 变化时一并清空；标记仅表示 PARAM_VALUE 确认与后续控制遥测的时间先后关系，不证明参数变化导致控制求解耗时变化。
- 仿真接口盘点：当前仓库的 `tools/simulation/mock_vehicle.py` 明确不模拟飞行动力学；`run-arducopter-sitl.sh` 启动的是 ArduCopter，不是 Flight SIL；`Flight-UDP-SIL-20261114.md` 记录的是外部 Flight Host SIL 经临时本地转发补丁接入的遥测验证，未提供 GroundControl 可调用的 Flight SIL 启停或参数注入 API。因此参数中心只读已连接 FCU 的真实遥测，不启动/控制仿真进程，也不将 ArduCopter SITL 冒充 Flight 模型仿真。
- 影响关系盘点：schema v2 已提供 dependencies 与 validationRules 字段，但当前 49 项均为空数组，尚无经审核的参数依赖图或组合规则；`FlightVehicleModelCatalog.json` 提供机型、适用性、控制律、域和参数白名单。影响分析继续显示依据不足空态，不从参数名称或 group 推断依赖关系。
- 当前曲线仍只表示控制计算耗时，不冒充参数目标/反馈误差响应曲线；真实目标/反馈量、参数变化前后统计检验、仿真预测、仿真接口、参数影响关系和 AI 建议仍待后续实现。
- ParameterEditorController 回归测试增加“非 Flight SIL 未授权写入不得产生 parameterWriteConfirmed 事件”断言；本轮不运行构建、QML UI 测试、单元测试或 CI，新增测试保持 NOT_RUN。

### Phase 5：视觉收口与集中验收

- 响应式布局基线：宽屏（≥1280 px）保持侧栏 + 三列仪表盘；中屏（900–1279 px）保留侧栏、上层采用双列、参数表跨列并纵向滚动；紧凑屏（<900 px）折叠左侧导航为参数域选择器，卡片改为单列纵向滚动。
- 视口高度不足时允许参数中心纵向滚动，不得压缩到不可操作或裁切关键控件。
- 按 1672 × 941 参考图完成截图叠加和区域级对比。
- 验收 100%、125%、150% Windows DPI。
- 验收 1600、1440、1280、1024、800、600 px 窗口宽度；低宽度允许纵向重排，但不得丢失参数编辑与安全状态。
- 完成 QML / UI Integration、Fact/ParameterManager 相关测试和在线参数异常场景测试。
- 记录真实 Windows x64 MSVC 2022 Release 验收结果和截图证据。

验收门槛：视觉、交互、参数数据、写入安全、异常恢复和响应式布局全部有证据；未执行项目必须标记 NOT_RUN，不得宣称完成。

## 11. 测试与验收矩阵

| 类别 | 验收内容 |
|---|---|
| 视觉 | 基准图叠加、面板边界、比例、颜色、字体、选中态 |
| 单机型绑定 | 单一绑定、切换确认、activeVehicle 改变、机型不匹配 |
| 参数域 | FMS/CONTROL/INS/PLANT/ACTUATOR 分类、空态、只读和不适用过滤 |
| 参数编辑 | 当前值/目标值/单位/范围/小数位/收藏/搜索/修改过滤 |
| 在线写入 | ACK、回读一致、超时、拒绝、失联、重复写入、busy |
| 安全状态 | armed/disarmed、重启要求、飞行中白名单、Vehicle 切换 |
| 恢复 | 恢复上一次值、回读确认、失败状态展示 |
| 曲线 | 实测/仿真/预测来源、单位、无效样本断点 |
| 仿真 | 实际接口、输入参数记录、仿真结果来源、NOT_RUN 空态 |
| 响应式 | 1672/1600/1440/1280/1024/800/600 px |
| DPI | 100%/125%/150% |
| 回归 | 现有 ParameterEditor、ParameterEditorController、ParameterManager 和 Fact 测试 |

## 12. 完成定义

- 唯一机型绑定规则清楚，参数内容与当前机型一致。
- 参考图主布局和视觉基线完成像素级还原。
- 不存在无关的配置版本、配置方案和版本管理内容。
- 所有现有参数能力保留，没有重复参数存储或旁路写入。
- 在线写入具备校验、协议确认、回读和恢复闭环。
- 实测、仿真、预测和 AI 建议的来源明确。
- 未知、无效、超时和不支持状态均被如实表达。
- 相关自动化测试和 Windows Release 验收有精确 HEAD 证据。
- SPEC 与实际实现同步；未完成阶段保持明确状态，不提前标记 PASS。

### Phase 1 实施记录（首轮）

- Flight 参数页切换为全宽参数中心工作台：仅在 Flight 的参数页隐藏旧配置侧栏和分隔线，其他配置页保留原导航。
- 顶部 Flight 工具栏在参数页显示“参数中心 3.0”标题和“参数调优 / 仿真验证 / 参数应用”工作区入口。
- 参数页新增唯一 activeVehicle 绑定区、五步流程条、八个参数域导航、上层三卡片、下层三卡片和底部状态栏。
- 数字孪生卡片只读取 Vehicle 已公开的机型、构型和电机数量；未提供的规格明确显示“未提供”。
- 参数健康度只显示真实连接状态、ParameterManager 参数就绪和待写入状态；不显示未经规则计算的综合分数。
- 参数表嵌入现有 ParameterEditor，继续使用 ParameterEditorController / Fact / ParameterManager，不建立第二套参数写入路径。
- 非 Flight 固件继续通过 Loader 回退到原生 ParameterEditor，不改变 PX4 / ArduPilot 等既有参数页布局。
- 在线调参按钮保持禁用并标明 Phase 3 待接入；响应曲线显示空态，不绘制伪造的实时/仿真数据；影响分析和 AI 建议显示数据不足空态。
- 原始 1672 × 941 参考图已审阅，但尚未成功写入 Git 仓库二进制资产路径；Phase 0 参考图归档仍待解决。

当前局限：
- Phase 1 首轮参数域点击仅切换 UI 选中态；Phase 2 已开始接入真实 domain 映射，详见 Phase 2 实施记录。
- 顶部工作区入口仅为视觉入口，不代表仿真或参数应用功能已实现。
- 当前机型信息来自 activeVehicle；独立机型模型注册/兼容性选择待 Phase 2。
- 本轮未运行 Windows 构建、QML UI 测试或 CI；必须在集中验收阶段验证。

### Phase 2 实施记录（首轮真实参数域映射）

本轮将当前 Flight 固件的真实参数元数据接入参数域导航，未扩大任何参数写入权限：

- Flight 仓库 tools/generate_qgc_parameter_metadata.py 已成为 schema v2 可重复生成入口：从 ground_parameters.json、vehicle_model.yaml、controller_tuning.yaml、estimator_tuning.yaml 生成 FlightParameterFactMetaData.json 与 FlightVehicleModelCatalog.json；参数项携带 domain/modelProfiles/vehicleFamily/modelVariants/controlLaw/tuningPolicy/updatePolicy/dependencies/validationRules，注册表保存同一 profile 描述与精确参数清单。生成器新增回归用例，未运行测试或 CI。
- 生成前强制校验 ground_parameters.json、vehicle_model.yaml、controller_tuning.yaml、estimator_tuning.yaml 的 profile ID 一致；任一来源机型不匹配即失败，不生成混合机型的参数元数据或注册表。

- FlightParameterFactMetaData.json 的 49 个参数使用 schema v2；原有 name、type、group、单位、范围、默认值、描述和来源标识全部保留，并增加机型适用性、控制律、调参/更新策略以及显式空 dependencies / validationRules。
- 当前 group → domain 映射为：机体模型 → VEHICLE_MODEL（1 项）；MPC 权重、位置环回退、姿态角速度环、SO3 姿态增益 → CONTROL（47 项）；仿真 → SIMULATION（1 项）。
- 当前 FMS、INS、PLANT、ACTUATOR 在这 49 项 Flight 参数元数据中没有登记参数；页面显示明确空态，不从其他域借用或伪造参数。
- FlightParameterMetaData::parseParameterJson 仅接受 schema v2，并对七种 domain、适用性、控制律、tuningPolicy/updatePolicy、依赖/规则数组、rebootRequired/readOnly/writeOnly 和数值范围执行 fail-closed 校验。合法字段映射到 FactMetaData / Fact；FactMetaData / Fact 保留 vehicleRebootRequiredKnown，缺失重启标志保持未知。原始 group 不变，ParameterEditorController 继续按 category / group 组织原生 Fact。
- ParameterEditor 新增 externalCategoryNavigation、hasCategory、selectCategoryByName：默认 false 时保持原有 PX4 / ArduPilot 等参数页行为；参数中心启用外层参数域选择，原生分组导航只显示当前域的 group，原搜索、全部/已修改/收藏、只读过滤、导入/导出和工具菜单继续保留。
- 外层参数域导航已联动真实 ParameterEditor category。当前域没有固件参数时显示“当前参数域暂无参数”，不显示其他域参数；当前 49 项的 tuningPolicy 为 GROUND_ADJUSTABLE，但 profile 的 SIL_ONLY 范围和解除锁定门控仍然优先，未授权的实机和飞行中写入保持锁定。
- ParameterEditorController 新增可选 categoryFilter；搜索、已修改、收藏和只读过滤均限制在当前 domain，切换参数域后不会因原生全局搜索而混入其他域参数。categoryFilter 默认为空，其他固件的原生参数编辑行为保持不变。
- FlightFirmwarePluginTest 新增元数据解析回归用例，覆盖合法 CONTROL 参数、未知 domain、非法 min/max 范围、不支持类型、rebootRequired 已声明/未声明/非法类型和重复参数名；测试代码已登记，本轮不执行构建或测试，集中验收时再运行。
- ParameterEditorController 通过 FactPanelController 在构造时捕获 activeVehicle；因此 activeVehicleChanged 时重建内部 ParameterEditor，避免切换飞行器后仍操作旧 Vehicle 的 Fact。
- 修正工具栏标题绑定：toolDrawerLoader 实际加载 VehicleConfigView.qml，标题现在读取其 _parameterCenterFullPage 状态，不再错误地检查外层 Loader 的 source 是否为 SetupParameterEditor.qml。
- 当前飞行器卡片的模型下拉由 FlightFirmwarePlugin → FirmwarePlugin::vehicleModelProfiles → Vehicle::vehicleModelProfiles 提供，不在 QML 内硬编码型号或重复判断固件身份。Flight SIL 通过身份校验后，插件返回注册的 SIM_ONLY / SIL_ONLY profiles；用户必须单选一个 profile 作为参数调优上下文，ParameterEditor 只显示该 profile 的参数名与参数域白名单。该选择不改变 activeVehicle 的真实硬件类型。实机或身份不明的 Flight 固件返回空注册列表，所有参数域不创建 ParameterEditor 并保持锁定。
- ParameterEditorController::setSelectedModelProfileId 在绑定前校验 profile 状态、hardwareEligible 布尔 false、SIL_ONLY 写入范围、vehicleFamily/modelVariants/controlLaw、参数名/参数域和完整 Fact 列表；每个 Fact 的实际 category、family、variants、controlLaw 必须与 profile 及参数元数据一致。参数列表不完整、适用性不匹配或注册表异常时清空旧绑定并锁定参数列表。每次写入还要重新检查当前参数 tuningPolicy/updatePolicy。
- 参数 profile 不再在页面启动或 activeVehicle 切换时自动预选首项；必须由操作者显式选择后才创建参数编辑器并解锁该 profile 的参数域。这样可避免把“注册表中存在唯一候选”误认为操作者已确认当前调参对象。无候选时显示“未登记可调机型”，有候选但未选择时显示“请选择机型配置”。
- 通信丢失时 ParameterEditor 禁止交互并显示链路异常遮罩；参数未就绪时不创建编辑器并显示等待状态。未改变现有 Fact / ParameterManager / FirmwarePlugin 写入路径。

当前局限与验收状态：
- domain 只解决参数归属，不等于机型兼容性。schema v2 已显式登记 vehicleFamily、modelVariants、controlLaw、tuningPolicy、updatePolicy；当前 dependencies / validationRules 为空数组，section / tuningLevel 尚待后续接入。
- 当前 49 项统计仅对应 FlightParameterFactMetaData.json，不代表未来所有机型或固件。
- SIL 单参数写入、PARAM_VALUE 确认、显式回读和恢复上一次已确认值已接入；实机在线写入、试探/批量调参、Flight SIL 参数注入、真实参数目标/反馈响应曲线仍未实现。
- 新增 ParameterEditorController categoryFilter 搜索隔离回归测试；测试代码已登记，但本轮未执行 Windows 构建、QML 运行、单元测试或 CI，必须在集中验收阶段验证。

### Phase 2 / Phase 3 安全边界补充

- Flight 参数元数据仅在固件源目录明确声明时生成 `rebootRequired`；生成器不再将缺失字段写成 false。字段存在但不是 JSON 布尔值时，整条元数据拒绝缓存；缺失时通过 `vehicleRebootRequiredKnown=false` 保留未知状态。
- 在线调参详情卡片从当前选中 Fact 的 `vehicleRebootRequiredKnown` 和 `vehicleRebootRequired` 显示“需要重启 / 无需重启 / 固件未声明”；不根据参数名推断重启要求，也不把写入确认等同于持久保存。当前 49 项 Flight 参数源目录没有提供重启要求，因此显示“固件未声明，请查阅固件说明”。
- 目标值输入在 QML 层先执行有限数值和 Fact min/max 边界校验；空值、非数值和越界值会显示原因并禁用“应用到飞控”。C++ 控制器仍独立执行 Fact 元数据转换/范围校验，前端校验不替代底层安全检查。
- 同一元数据文档中出现重复参数名时，所有同名条目均拒绝缓存，避免“先到先得”导致不确定的参数范围、单位或描述。
- 参数中心将 `ParameterManager::parameterDownloadSkipped` 视为参数未就绪；参数下载被主动跳过时，不创建可编辑的参数表。
- readOnly 参数允许查看和单参数回读，但禁止在线写入；writeOnly 参数因无法完成 PARAM_VALUE 回读确认，禁止进入在线调参闭环。详情卡片明确显示当前 Fact 的权限类型。
- 每次写入前再次从当前 Vehicle 的 profile 注册表核对 SIM_ONLY、hardwareEligible=false、SIL_ONLY、family/variant/controlLaw，以及当前参数名、domain、tuningPolicy 和 updatePolicy；不能只依赖首次绑定时缓存的资格。
- `FlightFirmwarePlugin::vehicleModelProfiles` 在向 UI 暴露机型前同时读取 schema v2 参数元数据与模型注册表，校验 source/SHA-256、profile ID、vehicleFamily/modelVariants/controlLaw、逐参数 modelProfiles/domain/tuningPolicy/updatePolicy、参数名和依赖引用；任一不匹配、未知依赖或权限字段类型错误时，整个注册表 fail-closed 并返回空列表。
- 新增模型注册表一致性回归用例，覆盖 SHA/来源路径不匹配、未知参数、参数域/适用性/控制律/variant/tuningPolicy 错配、未知或自引用依赖、逐参数 profile 错配、重复参数名和非法 hardwareEligible 类型；测试已登记但按集中验收要求保持 NOT_RUN。
- 参数工具菜单的飞控重启操作仅在已连接、解除锁定、未飞行且 ParameterManager 无待写入时启用，避免在飞行中、链路异常或参数同步过程中触发重启。
- C++ 单参数回读资格也检查 `parameterDownloadSkipped`，与写入资格和 QML 页面就绪门控保持一致，避免 UI 隐藏后仍可通过控制器调用回读路径。
- QML 编辑入口同时要求机型 profile 已成功绑定；profile 校验失败时，参数域过滤到空集并保持编辑锁定。
- 参数健康度卡片的“机型参数映射”读取真实 Loader / profile 绑定状态，区分等待选择、等待下载、校验中、校验通过和校验失败；不再显示已过期的“Phase 2 待接入”。
- 机型 profile 切换在控制器层与 QML 选择器层同时受待写入状态门控；存在单参数写入、回读或参数管理器待写入任务时，不允许切换调参上下文。
- 对应回归测试覆盖重启标志已声明/未声明/非法类型、适用性与策略字段映射、非法依赖/规则、profile 交叉匹配和未知依赖拒绝。本轮按集中验收要求未运行构建、单元测试或 CI，结果保持 NOT_RUN。

### Phase 3 实施记录（首轮 SIL 单参数在线调参闭环）

本轮开始接入在线调参，但不开放实机写入，也不启用试探调参/批量调参：

- ParameterEditor 增加参数行选择能力；Flight 参数中心启用 safe mode，参数表值单元格不再直接打开原生写入对话框，重置操作禁用，参数文件仍可导入预览但禁止批量上传。
- 在线调参控制器复用 ParameterEditorController、Fact、ParameterManager 和既有 MAVLink PARAM_SET 状态机，不建立第二套参数缓存或写入队列。
- 写入资格必须同时满足：当前 activeVehicle 由 FirmwarePlugin 识别为 Flight SIL；用户当前单选 profile 为 SIM_ONLY / SIL_ONLY / hardwareEligible=false；参数名/domain/family/variant/controlLaw 与注册表匹配；tuningPolicy 为 GROUND_ADJUSTABLE，updatePolicy 为受支持的在线确认策略；链路正常、参数完整就绪、无待处理写入；飞行器已解除锁定且未飞行；参数不是只读/只写项。
- 目标值先经 Fact 元数据范围/类型校验，再通过 Fact::setCookedValue 进入原有 ParameterManager PARAM_SET 流程；只有收到匹配参数的 PARAM_VALUE 确认才标记应用成功。
- confirmedValue 只从 Fact 的 vehicleUpdated（飞控回传）更新，不把 UI 乐观目标值冒充当前确认值；失败后要求回读确认实际值，失败后的下一次写入必须先完成回读。
- “回读验证”使用 ParameterManager 的单参数 refresh 路径；“恢复上一次”只恢复当前会话内该参数上一次已确认的值，并重新经过相同资格检查、范围校验、写入确认。
- 实时在线为当前唯一开放模式；试探调参和批量调参保持禁用。所有实机 profile、未知固件、非活动 Vehicle、失联、参数未就绪、armed/flying 状态均 fail-closed。C++ 层同时禁止 Flight 参数页的批量导入写入和批量重置，避免仅依赖 QML 禁用态。

本轮仅登记实现与回归测试代码；Windows 构建、QML 运行、Unit/Integration 和 CI 均未执行。该实现是 QUAD_X4 SIL 的单参数调参试点，不构成实机在线调参放行。

## 13. 当前实施状态

- SPEC：已定义，持续与实现同步。
- Phase 0：PARTIAL（源码盘点已完成；原始参考图已审阅，仓库二进制归档待完成）。
- Phase 1：PARTIAL（首轮布局移植已提交；Windows 构建 / QML 运行 / 截图比对 NOT_RUN）。
- Phase 2：PARTIAL（schema v2 机型适用性与控制律元数据、tuningPolicy/updatePolicy、依赖与规则空态、模型注册表双向一致性校验、Vehicle/FirmwarePlugin 动态绑定、SIL-only 机型显式单选、重启要求未知态保留、元数据 fail-closed 和参数白名单交叉校验已接入；硬件模型注册、真实 dependencies/validationRules、section/tuningLevel 仍未完成；构建与测试 NOT_RUN）。
- Phase 3：PARTIAL（SIL-only 单参数调参闭环、参数选择、目标值范围校验、PARAM_VALUE 确认、显式回读、上一次已确认值恢复、C++/QML 双层批量写入保护，以及 family/variant/controlLaw/tuningPolicy/updatePolicy 双层门控和策略显示已接入；实机/试探/批量调参未开放；构建与测试 NOT_RUN）。
- Phase 4：PARTIAL（已接入基于 controlStatusUpdated 逐帧采样的真实控制求解耗时趋势、FCU 时间轴、3 秒缺样断线、遥测 freshness 门控、样本统计，以及携带参数前后值、按 boot ID/controlProfileId 隔离的 PARAM_VALUE 确认时间标记；boot/控制配置/机型上下文/时间戳/活动飞行器变化时清空历史；真实参数目标/反馈响应、Flight SIL 启停与参数注入、影响关系和 AI 建议未完成；构建与测试 NOT_RUN）。
- Phase 5：PARTIAL（已接入 1280/900 px 响应式断点、紧凑屏参数域选择器、上/下层卡片重排、纵向滚动、状态栏重排，以及 safe mode 参数表工具栏/列宽适配；截图叠加、DPI、Windows 构建、QML/单元测试与 CI 仍 NOT_RUN）。

### Phase 5 实施记录（首轮响应式适配）

- 1672 px 及以上保持参考图的侧栏 + 三列主卡片布局，不改变参数表、在线调参、响应验证的既有数据来源与写入路径。
- 900–1279 px 保留参数域侧栏，上层数字孪生/架构卡片双列排列，健康度卡片跨列；下层参数表跨列，在线调参与响应验证双列排列。
- 900 px 以下隐藏纵向侧栏，改用紧凑参数域选择器；上层和下层卡片改为单列，避免窄窗口下三列挤压和横向裁切。
- 参数中心内容改为纵向可滚动布局；低高度窗口保持控件最小可用高度，避免参数编辑区被压缩到无法操作。
- 五步流程条根据当前机型绑定、参数域、选中参数和飞控确认状态显示进度，并可导航到对应工作区；导航只切换页面上下文，不绕过参数写入安全门控。
- 机型参数注册表变化后清除原选择与遥测确认历史，要求操作者重新选择调参上下文；旧的“Phase 2 待接入”提示替换为实时映射状态。
- 本轮未运行构建、QML UI 测试、单元测试或 CI；截图叠加与 100%/125%/150% DPI 验收保持 NOT_RUN。

### Phase 5 实施记录（第二轮参数表操作适配）

- Flight 参数中心 safe mode 的搜索、清空筛选、隐藏只读、参数导入预览、工具和参数列表标签切换为中文；普通参数编辑器继续使用原有英文界面。
- safe mode 顶部操作区在窄面板下切换为双列，搜索框独占一行；宽面板保持单行工具栏。
- 参数分组导航按可用宽度收缩，在极窄面板隐藏分组栏并把参数表扩展到全宽；搜索状态下仍按原有逻辑隐藏分组栏。
- 参数表在紧凑宽度下改用收藏/名称/当前值/说明四列，单位列收起；宽度恢复后自动恢复五列。说明列最多显示两行并在单元格内换行，避免窄窗口横向裁切。
- Flight safe mode 工具菜单隐藏恢复默认值、RC 映射清理和飞行器重启等不属于参数调校主流程的操作；保留刷新、参数文件预览/导出和收藏管理。所有文件导入仍只允许预览，批量上传保持关闭。
- 本轮修改仅由 parameterCenterSafeMode 门控，不改变普通 ParameterEditor 的既有工具、列宽和写入行为。
- 本轮未运行 QML 构建、UI/单元测试或 CI；紧凑表格宽度、长描述换行、100%/125%/150% DPI 和普通参数编辑器兼容性保持 NOT_RUN。

本规范是参数中心 3.0 后续设计、实现和验收的唯一基线。若用户确认新的视觉变化，必须先更新本规范和参考图，再修改实现。
