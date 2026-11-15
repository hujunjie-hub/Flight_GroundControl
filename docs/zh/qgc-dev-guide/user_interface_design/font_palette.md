# 字体和调色板

Flight GroundControl 使用统一的字体族、字号角色和调色板。所有 QML 页面应通过：

```qml
import QGroundControl.Controls
```

使用标准控件和 `ScreenTools` 字体令牌，不应在页面中自行定义字体体系。

## QGCPalette（QGC 调色板）

QGCPalette 提供浅色和深色主题的颜色。普通 UI 文字、按钮、边框、选中和禁用状态应使用调色板颜色，不要硬编码颜色覆盖主题状态。

地图覆盖层使用 QGCMapPalette，因为卫星图和街道图需要不同的对比度。

## ScreenTools（屏幕工具）

ScreenTools 是全项目字体族和字号的唯一入口；QGCApplication 会在 QML 加载前将该字体族设置为应用默认字体，使未显式指定字体的 Qt Quick Text 也继承统一基线：

- `normalFontFamily`：当前界面语言的普通字体族。
- `fixedFontFamily`：控制台、代码和原始数据使用的等宽字体族。
- `defaultFontPointSize`：默认正文 point size，受应用 UI Scale 控制。
- `microFontPointSize`、`captionFontPointSize`、`secondaryFontPointSize`、`compactBodyFontPointSize`：紧凑信息层级。
- `bodyFontPointSize`、`subtitleFontPointSize`、`titleFontPointSize`、`metricFontPointSize`、`heroFontPointSize`：正文、标题和主指标层级。

推荐使用 QGCLabel 的语义角色：

```qml
QGCLabel {
    text: qsTr("飞控诊断中心")
    typographyRole: "hero"
}
```

可用角色：`micro`、`caption`、`eyebrow`、`secondary`、`compact-body`、`body`、`subtitle`、`title`、`metric`、`hero`。

## 中文与其他语言字体

- 英文默认使用 Open Sans。
- 韩文使用项目内置 Nanum Gothic。
- 中文界面优先选择操作系统已安装的 CJK UI 字体：Windows 优先 Microsoft YaHei UI / Microsoft YaHei；macOS 优先 PingFang SC；Linux/Android 优先 Noto Sans CJK SC 等。
- 当语言设置为“System”时，字体选择跟随系统 locale；明确选择语言时按该界面语言选择。
- 若没有匹配字体，使用 Open Sans，并由 Qt/操作系统处理缺失字形回退。
- 页面不得硬编码字体名称。新增内置字体必须检查许可证、包体积和安装包部署。

## 抗锯齿、Hinting 与 DPI

- 标准文字控件使用 `Font.PreferDefaultHinting` 和抗锯齿。
- 普通文本使用 point size，不要使用固定 pixel size。
- 100%、125%、150% Windows DPI 使用同一套语义字号角色；布局裁切应通过布局修复，不得通过单独缩小字体掩盖。
- 完整规则和验收门槛见 [全项目字体系统与渲染规范](../../../../spec/UI-字体系统与渲染规范.md)。

## QGCMapPalette（地图调色板）

QGCMapPalette 用于地图上的文字、标记和覆盖图形。地图底图可能是卫星图或街道图，因此应选择适合底图对比度的地图调色板颜色。
