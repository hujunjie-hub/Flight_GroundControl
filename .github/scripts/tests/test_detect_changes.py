#!/usr/bin/env python3
"""Tests for Windows x64 detect_changes.py."""
from __future__ import annotations
import pytest
from detect_changes import build_patterns, has_relevant_changes, workflow_name_for_platform
def test_only_windows_platform_is_supported() -> None:
    assert workflow_name_for_platform("windows") == "windows"
    for platform in ("linux","macos","android"):
        with pytest.raises(ValueError,match="only windows x64"): workflow_name_for_platform(platform)
def test_windows_patterns_are_included() -> None:
    patterns=[p.pattern for p in build_patterns("windows")]
    assert r"^src/" in patterns
    assert r"^CMakeLists\.txt$" in patterns
    assert r"^cmake/" in patterns
    assert r"^\.github/build-config\.json$" in patterns
    assert r"^\.github/build-config\.schema\.json$" in patterns
    assert r"^deploy/windows/" in patterns
    assert r"^\.github/workflows/windows\.yml$" in patterns
@pytest.mark.parametrize("path",("src/Vehicle/Vehicle.cc","CMakeLists.txt","cmake/FindFoo.cmake","libs/lib/CMakeLists.txt","resources/qgc.qrc","translations/qgc.ts","test/UnitTest.cc",".github/actions/test/action.yml",".github/scripts/detect_changes.py",".github/build-config.json",".github/build-config.schema.json",".github/workflows/windows.yml","deploy/windows/installer.nsi"))
def test_windows_changes_trigger_build(path: str) -> None:
    assert has_relevant_changes([path],"windows")
@pytest.mark.parametrize("path",("README.md","docs/guide.md","android/AndroidManifest.xml",".github/workflows/linux.yml","deploy/linux/AppImage.sh"))
def test_non_windows_changes_do_not_trigger_build(path: str) -> None:
    assert not has_relevant_changes([path],"windows")
def test_empty_paths_are_ignored() -> None:
    assert not has_relevant_changes(["",""],"windows")
def test_empty_file_list_does_not_trigger_build() -> None:
    assert not has_relevant_changes([],"windows")
def test_unsupported_platform_is_rejected() -> None:
    with pytest.raises(ValueError,match="only windows x64"): has_relevant_changes(["src/Vehicle/Vehicle.cc"],"android")
