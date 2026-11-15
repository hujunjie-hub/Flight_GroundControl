# 设置侧栏字体与状态颜色规范

版本：1.0  
日期：2026-11-15  
适用范围：`src/QmlControls/AppSettings.qml`、`src/QmlControls/SettingsButton.qml` 及设置侧栏导航项。

## 1. 目标

设置侧栏的一级页面项与二级分区项必须具有一致的基础字体渲染和状态颜色语义。层级通过图标、缩进、间距和选中背景表达，不通过无意的字号缩放或透明度差异表达。

本规范只约束设置侧栏，不改变应用其他区域的字体层级设计。

## 2. 字体规则

- 一级页面项和二级分区项统一使用 `ScreenTools.normalFontFamily`。
- 两级菜单统一使用 `ScreenTools.defaultFontPointSize`，不得为二级菜单额外乘以 `0.9` 等缩放系数。
- 两级菜单统一使用 `Font.Normal`；如未来确需强调层级，必须同时更新设计规范和 UI 回归测试。
- 通过 `QGCLabel` 使用项目统一的字体抗锯齿设置；不得为不同菜单层级分别关闭抗锯齿。
- 字号使用 ScreenTools 提供的点尺寸，不写死屏幕像素值，不通过修改 Windows 显示分辨率掩盖字体样式问题。

## 3. 状态颜色规则

一级和二级菜单必须按各自控件的 `enabled` 状态选择同一套 `QGCPalette` 颜色组：

| 状态 | 文字颜色 | 背景 |
|---|---|---|
| 普通/未选中 | `QGCPalette.buttonText` | 透明 |
| 选中/按下 | `QGCPalette.buttonHighlightText` | `QGCPalette.buttonHighlight` |
| 悬停 | 保持普通文字色 | `buttonHighlight` 低透明度 |
| 禁用 | 使用 disabled palette 颜色组 | 不显示交互高亮 |

- 不得在菜单项中硬编码白色、灰色或透明度来替代调色板。
- 二级菜单的 palette 必须以二级按钮自身的 `enabled` 为 `colorGroupEnabled`，不得复用一个始终启用的外层颜色组。
- 选中、悬停、禁用状态不能改变字体族、字号或字重。

## 4. DPI 与分辨率

- 字体尺寸由 `ScreenTools.defaultFontPointSize` 提供；行高、图标和边距由 `ScreenTools.defaultFontPixelHeight/Width` 提供。
- 不为 100%、125%、150% Windows 缩放单独设置不同字号，也不添加分辨率专用字体覆盖。
- Qt 高 DPI 缩放应保持两级菜单使用同一字体规格。若某个缩放比例下出现裁切或布局溢出，应修复控件布局，不应再次缩小字体。
- Windows 100%/125%/150% 缩放下的实际屏幕观感检查属于本地视觉验收；没有对应截图和环境记录时必须标记 NOT_RUN。

## 5. 回归验收

`TopLevelViewsTest::_testSettingsSidebarTypographyConsistency` 必须验证：

1. 一级页面项与二级分区项的字体族一致。
2. 一级页面项与二级分区项的 point size 一致。
3. 一级页面项与二级分区项的字重一致。
4. 两级菜单在未选中状态使用相同 palette 文字颜色。
5. 两级菜单在选中状态使用相同 palette highlight 文字颜色。
6. 现有页面导航、分区展开/折叠、搜索和页面内容加载行为保持不变。

CI 的 Integration 测试验证 QML 字体属性和调色板状态；真实 Windows 显示器的多 DPI 视觉检查不能由该测试替代。
