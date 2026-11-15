#!/usr/bin/env python3
"""Enforce the project-wide QML typography contract."""

from __future__ import annotations

import re
import sys
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class Violation:
    file: str
    line: int
    message: str


LEGACY_FONT_TOKEN = re.compile(
    r"\b(?:ScreenTools|root\.screenTools)\.(?:default|small|medium|large)FontPointSize\b"
)
FONT_FAMILY = re.compile(r"\bfont\.family\s*:\s*(.*?)\s*$")
FONT_POINT_SIZE = re.compile(r"\bfont\.pointSize\s*:\s*(.*?)\s*$")
FONT_PIXEL_SIZE = re.compile(r"\bfont\.pixelSize\s*:")
DYNAMIC_SCALE_EXCEPTIONS = {
    "src/AppSettings/DebugWindow.qml",
    "src/FlightMap/Widgets/CompassDial.qml",
    "src/FlightMap/Widgets/FlightTelemetryCompass.qml",
    "src/FlightMap/Widgets/QGCCompassWidget.qml",
}
POINT_SIZE_PREVIEW_EXCEPTIONS = {"src/AppSettings/DebugWindow.qml"}


def _relative(path: Path, root: Path) -> str:
    try:
        return path.resolve().relative_to(root.resolve()).as_posix()
    except ValueError:
        return path.as_posix()


def _contract_violations(path: Path, content: str) -> list[Violation]:
    issues: list[Violation] = []

    def require(condition: bool, message: str) -> None:
        if not condition:
            issues.append(Violation(str(path), 1, message))

    name = path.name
    if name == "ScreenTools.qml":
        for token in (
            "microFontPointSize",
            "captionFontPointSize",
            "secondaryFontPointSize",
            "compactBodyFontPointSize",
            "bodyFontPointSize",
            "bodyEmphasisFontPointSize",
            "captionEmphasisFontPointSize",
            "sectionHeadingFontPointSize",
            "subtitleFontPointSize",
            "titleFontPointSize",
            "metricFontPointSize",
            "heroFontPointSize",
            "worldOverlayFontPointSize",
            "fontPointSizeForRole",
            "fontWeightForRole",
        ):
            require(token in content, f"missing semantic typography token/function: {token}")
        require(
            "readonly property real smallFontPointSize:  captionFontPointSize" in content,
            "legacy small font size must remain a compatibility alias to caption",
        )
        require(
            "readonly property real mediumFontPointSize: subtitleFontPointSize" in content,
            "legacy medium font size must remain a compatibility alias to subtitle",
        )
        require(
            "readonly property real largeFontPointSize:  titleFontPointSize" in content,
            "legacy large font size must remain a compatibility alias to title",
        )
        require(
            "readonly property real subtitleFontPointSize:     defaultFontPointSize * mediumFontPointRatio" in content,
            "subtitle font size must derive from the base size, not the legacy alias",
        )
        require(
            "readonly property real titleFontPointSize:        defaultFontPointSize * largeFontPointRatio" in content,
            "title font size must derive from the base size, not the legacy alias",
        )

    elif name == "QGCLabel.qml":
        require('property string typographyRole: "body"' in content, "QGCLabel must expose typographyRole")
        require("ScreenTools.fontPointSizeForRole(typographyRole)" in content, "QGCLabel must use semantic point-size roles")
        require("ScreenTools.fontWeightForRole(typographyRole)" in content, "QGCLabel must use semantic weight roles")
        require("ScreenTools.normalFontFamily" in content, "QGCLabel must use the application font family")
        require("Font.PreferDefaultHinting" in content, "QGCLabel must use the shared hinting preference")
        require(re.search(r"antialiasing:\s*true", content) is not None, "QGCLabel antialiasing must remain enabled")

    elif name == "QGCRadioButton.qml":
        require(
            re.search(r"font\.family:\s*control\.font\.family", content) is not None,
            "radio button content must inherit control.font.family",
        )
        require(
            re.search(r"font\.family:\s*control\.font\.pointSize", content) is None,
            "radio button font.family must never be bound to pointSize",
        )
        require("Font.PreferDefaultHinting" in content, "radio button must use shared hinting preference")
        require(re.search(r"antialiasing:\s*true", content) is not None, "radio button text antialiasing must remain enabled")

    elif name == "QGCSlider.qml":
        require('typographyRole: "caption"' in content, "QGCSlider labels must use the shared caption role")
        require("QGCLabel {" in content, "QGCSlider must use QGCLabel for its displayed text")

    elif name in {
        "QGCTextField.qml",
        "QGCCheckBox.qml",
        "QGCComboBox.qml",
        "QGCButton.qml",
        "QGCTabButton.qml",
        "QGCMenuItem.qml",
        "QGCMarqueeLabel.qml",
    }:
        require("ScreenTools.normalFontFamily" in content, f"{name} must use the application font family")
        require("Font.PreferDefaultHinting" in content, f"{name} must use shared hinting preference")

    elif name == "FlightTelemetry.qml":
        require(
            re.search(
                r"font\.(?:pointSize|pixelSize)\s*:\s*ScreenTools\.(?:default|large|small|medium)FontPointSize\s*\*",
                content,
            )
            is None,
            "FlightTelemetry must use semantic ScreenTools tokens instead of inline font-size multipliers",
        )
        require('typographyRole: "hero"' in content, "FlightTelemetry hero values must use the shared hero role")
        require('readonly property string labelFontFamily: ScreenTools.normalFontFamily' in content,
                "FlightTelemetry Canvas labels must use the application font family")
        require('readonly property real labelFontPointSize: ScreenTools.microFontPointSize' in content,
                "FlightTelemetry Canvas labels must use the semantic micro point-size token")
        require('ctx.font = "bold " + topologyCanvas.labelFontPointSize + "pt' in content,
                "FlightTelemetry Canvas text must use a semantic point size")
        require('"sans-serif"' not in content,
                "FlightTelemetry must not hard-code the Canvas fallback font family")
        require('readonly property real densityScale: root._overviewTopologyDensityScale' in content and
                'readonly property real geometryScale:' in content and
                'ScreenTools.defaultFontPixelHeight / 16.0' in content,
                "FlightTelemetry topology geometry must use ScreenTools density scaling")
        require('Layout.preferredHeight: preferredCanvasHeight' in content,
                "FlightTelemetry topology Canvas height must follow the ScreenTools-scaled height token")
        require('topologyNodeContent.implicitHeight + 2 * contentVerticalInset' in content and
                'anchors.topMargin: parent.contentVerticalInset' in content,
                "FlightTelemetry topology node cards must use scaled insets and content-driven height")


    elif name == "FlightRadio.qml":
        require('typographyRole: "title"' in content, "FlightRadio page title must use the shared title role")

    elif name == "QGCApplication.cc":
        require("QGuiApplication::setFont(defaultFont)" in content, "application default font family must be set before QML loads")
        require("ScreenToolsController::normalFontFamily()" in content, "application default font must use ScreenToolsController family")
        require("QFont::PreferDefaultHinting" in content, "application default font must use shared hinting preference")

    elif name == "ScreenToolsController.cc":
        require("QLocale::Chinese" in content, "Chinese UI font selection must be explicit")
        require("QLocale::system().language()" in content, "System language selection must follow the system locale")
        require("Microsoft YaHei UI" in content, "Windows CJK font preference is missing")
        require("Noto Sans CJK SC" in content, "Linux/Android CJK font preference is missing")
        require("QFontDatabase::families()" in content, "CJK font selection must use the Qt 6.11 static font database API")
        require(not re.search(r"\\bQFontDatabase\\s+\\w+\\s*;", content), "QFontDatabase must not be instantiated; its API is static in Qt 6.11")

    return issues


def _scan_qml(path: Path, root: Path) -> list[Violation]:
    try:
        content = path.read_text(encoding="utf-8")
    except OSError as error:
        return [Violation(str(path), 1, f"cannot read file: {error}")]

    relative = _relative(path, root)
    # Remove block comments before scanning declarations; line comments are stripped per line.
    scan_content = re.sub(r"/\*.*?\*/", "", content, flags=re.DOTALL)
    issues = _contract_violations(path, content)

    for line_number, raw_line in enumerate(scan_content.splitlines(), start=1):
        line = raw_line.split("//", 1)[0].strip()
        if not line:
            continue

        if relative != "src/QmlControls/ScreenTools.qml" and LEGACY_FONT_TOKEN.search(line):
            issues.append(Violation(relative, line_number, "use a semantic ScreenTools font token; legacy size aliases are deprecated"))

        if FONT_PIXEL_SIZE.search(line):
            issues.append(Violation(relative, line_number, "font.pixelSize is not allowed; use a point-size semantic token for DPI scaling"))

        if "monospace" in line.lower():
            issues.append(Violation(relative, line_number, "use ScreenTools.fixedFontFamily instead of a hard-coded monospace family"))

        family_match = FONT_FAMILY.search(line)
        if family_match:
            value = family_match.group(1).strip().rstrip(";")
            allowed = (
                value in {
                    "ScreenTools.normalFontFamily",
                    "ScreenTools.fixedFontFamily",
                    "normalFontFamily",
                    "fixedFontFamily",
                }
                or value.endswith(".font.family")
                or "ScreenTools.normalFontFamily" in value
                or "ScreenTools.fixedFontFamily" in value
            )
            if not allowed:
                issues.append(Violation(relative, line_number, f"font.family must inherit ScreenTools or a parent/control font, got: {value}"))

        point_match = FONT_POINT_SIZE.search(line)
        if point_match:
            value = point_match.group(1).strip().rstrip(";")
            numeric_literal = re.fullmatch(r"[+-]?\d+(?:\.\d+)?", value) is not None
            if numeric_literal and relative not in POINT_SIZE_PREVIEW_EXCEPTIONS:
                issues.append(Violation(relative, line_number, f"hard-coded font.pointSize is not allowed: {value}"))
            if "*" in value and relative not in DYNAMIC_SCALE_EXCEPTIONS:
                issues.append(Violation(relative, line_number, "page-local font-size multipliers are not allowed; use a semantic role/token"))

    return issues


def main() -> int:
    root = Path.cwd()
    if "--all" in sys.argv[1:]:
        qml_files = sorted((root / "src").rglob("*.qml"))
        contract_files = [
            root / "src/QGCApplication.cc",
            root / "src/QmlControls/ScreenToolsController.cc",
        ]
        paths = qml_files + [path for path in contract_files if path.exists()]
    else:
        paths = [Path(argument) for argument in sys.argv[1:] if argument != "--all"]
        if not paths:
            paths = [Path(line.strip()) for line in sys.stdin if line.strip()]

    violations = [issue for path in paths for issue in _scan_qml(path, root)] if paths else []
    for issue in violations:
        print(f"{issue.file}:{issue.line}: error: {issue.message}")

    if violations:
        print(f"Found {len(violations)} QML typography violation(s).", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
