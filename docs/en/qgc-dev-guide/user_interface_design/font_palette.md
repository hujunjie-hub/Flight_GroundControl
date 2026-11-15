# Font and Palette

Flight GroundControl uses one shared font family strategy, semantic typography scale, and palette. QML pages should import:

```qml
import QGroundControl.Controls
```

Use the shared controls and `ScreenTools` tokens instead of defining page-local typography systems.

## QGCPalette

QGCPalette provides light and dark theme colors. Standard UI text, buttons, borders, selected states, and disabled states should use palette colors rather than hard-coded colors that bypass theme changes.

Map overlays use QGCMapPalette because satellite and street maps require different contrast.

## ScreenTools

ScreenTools is the single source for UI font families and sizes. Before QML loads, QGCApplication sets this family as the application default so raw Qt Quick Text items inherit the same baseline:

- `normalFontFamily`: normal UI font for the selected interface language.
- `fixedFontFamily`: monospaced font for consoles, code, and raw data.
- `defaultFontPointSize`: default body point size, scaled by the application UI Scale.
- `microFontPointSize`, `captionFontPointSize`, `secondaryFontPointSize`, `compactBodyFontPointSize`: compact information hierarchy.
- `bodyFontPointSize`, `subtitleFontPointSize`, `titleFontPointSize`, `metricFontPointSize`, `heroFontPointSize`: body, heading, and primary metric hierarchy.

Prefer QGCLabel semantic roles:

```qml
QGCLabel {
    text: qsTr("Flight Diagnostics")
    typographyRole: "hero"
}
```

Available roles: `micro`, `caption`, `eyebrow`, `secondary`, `compact-body`, `body`, `subtitle`, `title`, `metric`, and `hero`.

## CJK Font Fallback

- Open Sans is the default for English and other default locales.
- Korean uses the bundled Nanum Gothic family.
- Chinese UI prefers an installed platform CJK UI font: Microsoft YaHei UI / Microsoft YaHei on Windows, PingFang SC on macOS, and Noto Sans CJK SC or similar on Linux/Android.
- When the language setting is System, font selection follows the system locale; an explicitly selected language takes precedence.
- If no preferred family is installed, Open Sans remains the primary family and Qt/platform glyph fallback handles missing glyphs.
- Pages must not hard-code family names. New bundled fonts require license, package-size, and installer-deployment review.

## Antialiasing, Hinting, and DPI

- Standard text controls use `Font.PreferDefaultHinting` and antialiasing.
- Use point sizes for UI text; avoid fixed pixel sizes.
- Windows 100%, 125%, and 150% DPI use the same semantic typography roles. Fix layout clipping through layout constraints rather than shrinking fonts for a specific resolution.
- See the [project-wide typography and rendering specification](../../../../spec/UI-字体系统与渲染规范.md) for full rules and acceptance gates.

## QGCMapPalette

QGCMapPalette is used for text, markers, and overlays drawn over maps. Since the basemap may be satellite imagery or a street map, use map palette colors that preserve contrast against the selected basemap.
