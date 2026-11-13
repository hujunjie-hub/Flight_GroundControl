#!/usr/bin/env python3
"""Tests for Linux x64 and Windows x64 detect_changes.py."""

from __future__ import annotations

import pytest

from detect_changes import build_patterns, has_relevant_changes, workflow_name_for_platform


class TestWorkflowNameForPlatform:
    def test_supported_platforms(self) -> None:
        assert workflow_name_for_platform("linux") == "linux"
        assert workflow_name_for_platform("windows") == "windows"

    def test_unsupported_platform_is_rejected(self) -> None:
        with pytest.raises(ValueError, match="expected linux or windows"):
            workflow_name_for_platform("macos")


class TestBuildPatterns:
    @pytest.mark.parametrize("platform", ("linux", "windows"))
    def test_common_patterns_are_included(self, platform: str) -> None:
        patterns = [pattern.pattern for pattern in build_patterns(platform)]
        assert r"^src/" in patterns
        assert r"^CMakeLists\.txt$" in patterns
        assert r"^cmake/" in patterns
        assert r"^\.github/build-config\.json$" in patterns
        assert r"^\.github/build-config\.schema\.json$" in patterns

    def test_linux_patterns_are_included(self) -> None:
        patterns = [pattern.pattern for pattern in build_patterns("linux")]
        assert r"^deploy/linux/" in patterns
        assert r"^\.github/workflows/linux\.yml$" in patterns
        assert r"^tools/setup/.*debian" in patterns
        assert r"^tools/setup/install_dependencies/" in patterns

    def test_windows_patterns_are_included(self) -> None:
        patterns = [pattern.pattern for pattern in build_patterns("windows")]
        assert r"^deploy/windows/" in patterns
        assert r"^\.github/workflows/windows\.yml$" in patterns
        assert r"^tools/setup/.*windows" in patterns


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
    def test_common_changes_trigger_both_builds(self, path: str) -> None:
        assert has_relevant_changes([path], "linux")
        assert has_relevant_changes([path], "windows")

    @pytest.mark.parametrize(
        "path",
        (
            ".github/workflows/linux.yml",
            "deploy/linux/AppImage.sh",
            "tools/setup/install_dependencies/_debian.py",
            "tools/setup/foo-debian.sh",
        ),
    )
    def test_linux_specific_changes_only_trigger_linux(self, path: str) -> None:
        assert has_relevant_changes([path], "linux")
        assert not has_relevant_changes([path], "windows")

    @pytest.mark.parametrize(
        "path",
        (
            ".github/workflows/windows.yml",
            "deploy/windows/installer.nsi",
            "tools/setup/foo-windows.ps1",
        ),
    )
    def test_windows_specific_changes_only_trigger_windows(self, path: str) -> None:
        assert has_relevant_changes([path], "windows")
        assert not has_relevant_changes([path], "linux")

    @pytest.mark.parametrize("path", ("README.md", "docs/guide.md", "android/AndroidManifest.xml"))
    def test_unrelated_changes_do_not_trigger_builds(self, path: str) -> None:
        assert not has_relevant_changes([path], "linux")
        assert not has_relevant_changes([path], "windows")

    def test_empty_paths_are_ignored(self) -> None:
        assert not has_relevant_changes(["", ""], "linux")
        assert not has_relevant_changes(["", ""], "windows")

    def test_empty_file_list_does_not_trigger_build(self) -> None:
        assert not has_relevant_changes([], "linux")
        assert not has_relevant_changes([], "windows")

    def test_unsupported_platform_is_rejected(self) -> None:
        with pytest.raises(ValueError, match="expected linux or windows"):
            has_relevant_changes(["src/Vehicle/Vehicle.cc"], "android")
