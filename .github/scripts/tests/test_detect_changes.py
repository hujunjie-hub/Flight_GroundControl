#!/usr/bin/env python3
"""Tests for the Windows x64 detect_changes.py implementation."""

from __future__ import annotations

import pytest

from detect_changes import build_patterns, has_relevant_changes


class TestBuildPatterns:
    def test_common_patterns_are_included(self) -> None:
        patterns = [pattern.pattern for pattern in build_patterns("windows")]
        assert r"^src/" in patterns
        assert r"^CMakeLists\.txt$" in patterns
        assert r"^cmake/" in patterns
        assert r"^\.github/build-config\.json$" in patterns

    def test_windows_only_patterns_are_included(self) -> None:
        patterns = [pattern.pattern for pattern in build_patterns("windows")]
        assert r"^deploy/windows/" in patterns
        assert r"^\.github/workflows/windows\.yml$" in patterns
        assert r"^tools/setup/.*windows" in patterns

    @pytest.mark.parametrize(
        "platform",
        ("linux", "macos", "ios", "android", "docker-linux", "docker-android"),
    )
    def test_non_windows_platforms_are_rejected(self, platform: str) -> None:
        with pytest.raises(ValueError, match="only Windows x64 is supported"):
            build_patterns(platform)


class TestHasRelevantChanges:
    @pytest.mark.parametrize(
        "path",
        (
            "src/Vehicle/Vehicle.cc",
            "CMakeLists.txt",
            "cmake/FindFoo.cmake",
            "libs/some-library/CMakeLists.txt",
            "resources/qgc.qrc",
            "translations/qgc.ts",
            "test/UnitTest.cc",
            ".github/actions/qt-install/action.yml",
            ".github/scripts/detect_changes.py",
            ".github/build-config.json",
            ".github/build-config.schema.json",
        ),
    )
    def test_common_changes_trigger_windows_build(self, path: str) -> None:
        assert has_relevant_changes([path], "windows")

    @pytest.mark.parametrize(
        "path",
        (
            ".github/workflows/windows.yml",
            "deploy/windows/installer.nsi",
            "tools/setup/foo-windows.ps1",
        ),
    )
    def test_windows_specific_changes_trigger_build(self, path: str) -> None:
        assert has_relevant_changes([path], "windows")

    @pytest.mark.parametrize(
        "path",
        (
            "README.md",
            "docs/guide.md",
            ".github/workflows/linux.yml",
            ".github/workflows/macos.yml",
            ".github/workflows/android.yml",
            "android/AndroidManifest.xml",
            "deploy/linux/AppImage.sh",
            "deploy/macos/Info.plist",
            "tools/setup/foo-debian.sh",
        ),
    )
    def test_unrelated_and_non_windows_changes_do_not_trigger_build(self, path: str) -> None:
        assert not has_relevant_changes([path], "windows")

    def test_empty_paths_are_ignored(self) -> None:
        assert not has_relevant_changes(["", ""], "windows")

    def test_empty_file_list_does_not_trigger_build(self) -> None:
        assert not has_relevant_changes([], "windows")

    def test_non_windows_platform_argument_is_rejected(self) -> None:
        with pytest.raises(ValueError, match="only Windows x64 is supported"):
            has_relevant_changes(["src/Vehicle/Vehicle.cc"], "linux")
