# 全项目字体系统与渲染规范

版本：1.0  
日期：2026-11-15  
适用仓库：`hujunjie-hub/Flight_GroundControl`  
适用范围：`src/**/*.qml`、`src/QmlControls`、Flight 专用页面及所有新增/修改的地面站 UI。

## 1. 目标与基本原则

建立全项目唯一的字体与文字渲染基线，消除页面之间意外的字体族、字号、字重、字形清晰度和亮度差异。

- 统一的是字体规则和语义层级，不是把所有文字改成相同字号。
- 标题、指标、正文、辅助说明可以有不同层级，但必须使用 `ScreenTools` 语义令牌。
- 层级通过字号角色、字重、布局和间距表达；不得通过随意缩放或降低透明度制造层级。
- UI 的字体变化不得影响遥测协议、Fact、飞控命令、ARM、FMS、Permit 或执行器授权逻辑。

## 2. 字体族与语言回退

`ScreenTools.normalFontFamily` 是 UI 字体族的唯一入口。语言设置为 System 时按系统 locale 选择；显式语言设置优先：

| 界面语言 | 首选字体 | 回退 |
|---|---|---|
| 英文及其他默认语言 | Open Sans | Qt/操作系统字形回退 |
| 韩文 | Nanum Gothic（项目内置） | Qt/操作系统字形回退 |
| 简体中文 | Windows: Microsoft YaHei UI / Microsoft YaHei / SimHei / SimSun；macOS: PingFang SC / Heiti SC；Linux/Android: Noto Sans CJK SC / Noto Sans SC / Source Han Sans SC / WenQuanYi Micro Hei | 按系统已安装字体列表选择；无匹配时回退 Open Sans 并由 Qt 执行缺字字形回退 |

规则：

1. 页面和可复用控件不得硬编码字体族名称；使用 `ScreenTools.normalFontFamily`。
2. 等宽数据、MAVLink 控制台和代码视图可使用 `ScreenTools.fixedFontFamily`，不得把等宽字体用于普通 UI。
3. 中文字体从系统已安装字体中选择，不强制把体积较大的 CJK 字体打包进当前 Windows 安装包。
4. 混合语言/用户输入内容在主字体缺少字形时，由 Qt 平台字形回退处理。
5. 新增字体文件必须检查许可证、包体积、安装包部署和字体加载失败行为，并更新本规范。

## 3. 语义字号令牌

所有字号由 `ScreenTools.qml` 提供。页面不得复制下列系数或自行建立局部字号体系。

| 角色 | ScreenTools 令牌 | 用途 |
|---|---|---|
| micro | `microFontPointSize` | 极密集状态、次级单位和紧凑辅助信息 |
| caption / eyebrow | `captionFontPointSize` | 卡片标签、说明、眉题 |
| secondary | `secondaryFontPointSize` | 次级正文、数据注释 |
| compact-body | `compactBodyFontPointSize` | 紧凑布局正文 |
| body | `bodyFontPointSize` | 默认正文、菜单、按钮、输入框 |
| body-emphasis | `bodyEmphasisFontPointSize` | 强调型正文 |
| caption-emphasis | `captionEmphasisFontPointSize` | 强调型辅助说明 |
| section-heading | `sectionHeadingFontPointSize` | 设置分组标题 |
| world-overlay | `worldOverlayFontPointSize` | 三维视图世界空间标注 |
| subtitle | `subtitleFontPointSize` | 分区副标题 |
| title | `titleFontPointSize` | 页面/卡片标题 |
| metric | `metricFontPointSize` | 重点数值 |
| hero | `heroFontPointSize` | 页面主指标/主视觉数值 |

- 默认字号仍由 `ScreenTools.defaultFontPointSize` 和应用 UI Scale 计算。
- micro/caption 有最小 point-size 下限，避免用户缩放时辅助文字无限缩小。
- 使用 `QGCLabel.typographyRole` 选择语义角色；确有特殊控件约束时可直接使用上述 ScreenTools 令牌。
- 禁止在页面中出现 `font.pointSize: ScreenTools.defaultFontPointSize * 0.8` 之类的局部系数。
- 禁止用固定 `font.pixelSize` 代替点字号；尺寸必须跟随 Qt 高 DPI 与应用 UI Scale。
- 旧 `smallFontPointSize` / `mediumFontPointSize` / `largeFontPointSize` 仅保留为兼容别名，项目内部 QML 必须使用语义令牌。

## 4. 字重、抗锯齿与 Hinting

统一字重：

| 角色 | 字重 |
|---|---|
| Regular | `Font.Normal` |
| Emphasis | `Font.DemiBold` |
| Strong | `Font.Bold` |

统一渲染规则：

- QGCApplication 在 QML 加载前设置应用默认字体族和 Hinting；未显式指定字体的 Qt Quick Text 也继承同一基线。
- `QGCLabel` 默认使用 `ScreenTools.normalFontFamily`、语义字号、语义字重。
- 文本控件统一使用 `Font.PreferDefaultHinting`。
- 普通 UI 文本开启 `antialiasing: true`。
- 自定义 `Text` 必须明确继承 `ScreenTools.normalFontFamily` 或父控件字体；不得把字号、颜色或其他数值误赋给 `font.family`。
- RadioButton、CheckBox、ComboBox、Button、TabButton、MenuItem、TextField 等共享控件必须与 QGCLabel 使用同一字体族和 Hinting 基线。
- 数字读数如需等宽对齐，必须明确登记为 fixed-font 用途，不得因局部显示效果随意切换字体。

## 5. 文字颜色与亮度

- 普通 QGC 页面继续使用 `QGCPalette`，不得硬编码颜色覆盖主题状态。
- 选中、悬停、禁用、警告、错误状态由对应控件自己的 palette 状态组决定。
- 飞控诊断大屏允许使用其已登记的深色指挥中心语义色板；同一语义角色在页面内必须保持固定颜色，不得用文字透明度或不同灰度重复表达同一状态。
- 状态颜色只能表达来源数据的真实状态；字体调整不得把未知、无效、过期状态渲染为健康。

## 6. DPI 与分辨率

- 字号使用 point size；行高、图标、控件尺寸和间距使用 `ScreenTools.defaultFontPixelHeight/Width` 等像素度量令牌。
- Windows 100%、125%、150% 缩放均使用同一字号角色和相同 UI Scale 逻辑。
- 不得为某个分辨率硬编码字号，不得通过降低字号来掩盖控件裁切；应修复布局、换行、最小宽度和响应式断点。
- 应用 `uiScalePercent` 变化后，字号令牌必须同步更新。
- CI 的 QML 属性断言不能替代真实显示器视觉检查。

## 7. 代码审查与例外

允许的例外：

1. 仪表盘主指标、卡片标题、辅助说明等明确的语义角色差异。
2. 代码/终端/原始遥测数据等有明确理由的等宽字体。
3. 地图罗盘/遥测覆盖层允许依据地图尺寸动态缩放，但基准必须来自 `ScreenTools` 语义字号；对应例外限定在 `src/FlightMap/Widgets/CompassDial.qml`、`FlightTelemetryCompass.qml` 和 `QGCCompassWidget.qml`。
4. `src/AppSettings/DebugWindow.qml` 的字体预览样本允许列出显式 point size；这仅用于调试预览，不得用于业务页面。
5. 三维视图的世界空间标注使用 `worldOverlayFontPointSize` / `heroFontPointSize`，必须使用 point size，不得使用固定 pixel size。

任何例外都必须在对应页面设计规范中登记，并优先使用 ScreenTools 令牌。禁止为了单个截图临时写入字号系数、透明度或系统字体名。

## 8. 自动化与视觉验收

必需检查：

1. ScreenTools 包含全部语义字号和字重角色。
2. 字体静态检查递归覆盖 `src/**/*.qml` 全部页面（当前 459 个 QML 文件），不再只检查共享控件白名单。
3. QGCLabel 通过 typographyRole 选择字号/字重。
4. QGCRadioButton 的 contentItem 字体族必须绑定 `control.font.family`。
5. 核心共享文本控件使用统一字体族和 `Font.PreferDefaultHinting`。
6. 中文界面按显式语言或 System locale 选择已安装的 CJK 字体。
7. FlightTelemetry 不允许出现散落的字号乘数。
8. TopLevelViewsTest 验证设置侧栏字体族、字号、字重、Hinting/抗锯齿，以及中文字体选择。
9. Windows 实机截图覆盖 100%、125%、150% DPI，并记录系统缩放、应用 UI Scale、屏幕分辨率、页面和截图。

Windows DPI 视觉检查在本地完成并登记 RUN_ID；没有截图和环境记录时必须标记 `NOT_RUN`。

## 9. 问题关联

- FGC-0046：设置侧栏一级/二级菜单字体亮度不一致。
- FGC-0047：全项目字体体系、中文字体回退与渲染一致性。

本规范是后续全项目 UI 字体审查、修改和验收的唯一基线。
