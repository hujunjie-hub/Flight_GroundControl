#!/usr/bin/env python3
"""Check the shared QML typography contract for Flight GroundControl."""

from __future__ import annotations

import re
import sys
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class Violation:
    file: str
    message: str


def _has(content: str, pattern: str) -> bool:
    return re.search(pattern, content, re.MULTILINE) is not None


def analyze_file(path: Path) -> list[Violation]:
    try:
        content = path.read_text(encoding="utf-8")
    except OSError as error:
        return [Violation(str(path), f"cannot read file: {error}")]

    issues: list[Violation] = []

    def require(condition: bool, message: str) -> None:
        if not condition:
            issues.append(Violation(str(path), message))

    name = path.name
    if name == "ScreenTools.qml":
        for token in (
            "microFontPointSize",
            "captionFontPointSize",
            "secondaryFontPointSize",
            "compactBodyFontPointSize",
            "bodyFontPointSize",
            "subtitleFontPointSize",
            "titleFontPointSize",
            "metricFontPointSize",
            "heroFontPointSize",
            "fontPointSizeForRole",
            "fontWeightForRole",
        ):
            require(token in content, f"missing semantic typography token/function: {token}")

    elif name == "QGCLabel.qml":
        require('property string typographyRole: "body"' in content, "QGCLabel must expose typographyRole")
        require("ScreenTools.fontPointSizeForRole(typographyRole)" in content, "QGCLabel must use semantic point-size roles")
        require("ScreenTools.fontWeightForRole(typographyRole)" in content, "QGCLabel must use semantic weight roles")
        require("ScreenTools.normalFontFamily" in content, "QGCLabel must use the application font family")
        require("Font.PreferDefaultHinting" in content, "QGCLabel must use the shared hinting preference")
        require(_has(content, r"antialiasing:\s*true"), "QGCLabel antialiasing must remain enabled")

    elif name == "QGCRadioButton.qml":
        require(
            _has(content, r"font\.family:\s*control\.font\.family"),
            "radio button content must inherit control.font.family",
        )
        require(
            not _has(content, r"font\.family:\s*control\.font\.pointSize"),
            "radio button font.family must never be bound to pointSize",
        )
        require("Font.PreferDefaultHinting" in content, "radio button must use shared hinting preference")
        require(_has(content, r"antialiasing:\s*true"), "radio button text antialiasing must remain enabled")

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
            not _has(content, r"font\.(?:pointSize|pixelSize)\s*:\s*ScreenTools\.(?:default|large|small|medium)FontPointSize\s*\*"),
            "FlightTelemetry must use semantic ScreenTools tokens instead of inline font-size multipliers",
        )
        require('typographyRole: "hero"' in content, "FlightTelemetry hero values must use the shared hero role")

    elif name == "FlightRadio.qml":
        require('typographyRole: "title"' in content, "FlightRadio page title must use the shared title role")

    elif name == "ScreenToolsController.cc":
        require("QLocale::Chinese" in content, "Chinese UI font selection must be explicit")
        require("QLocale::system().language()" in content, "System language selection must follow the system locale")
        require("Microsoft YaHei UI" in content, "Windows CJK font preference is missing")
        require("Noto Sans CJK SC" in content, "Linux/Android CJK font preference is missing")
        require("fontDatabase.families()" in content, "CJK font selection must check installed families")

    return issues


def main() -> int:
    files = [Path(argument) for argument in sys.argv[1:]]
    if not files:
        files = [Path(line.strip()) for line in sys.stdin if line.strip()]

    violations = [violation for path in files for violation in analyze_file(path)]
    for violation in violations:
        print(f"{violation.file}: error: {violation.message}")

    if violations:
        print(f"Found {len(violations)} QML typography violation(s).", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
