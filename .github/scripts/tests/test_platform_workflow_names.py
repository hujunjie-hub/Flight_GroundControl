#!/usr/bin/env python3
"""Detect drift between the Windows x64 build workflow and its consumers."""
from __future__ import annotations
import json
from typing import TYPE_CHECKING
import yaml
from _helpers import REPO_ROOT
if TYPE_CHECKING:
    from pathlib import Path
WORKFLOWS_DIR = REPO_ROOT / ".github" / "workflows"
BUILD_CONFIG_JSON = REPO_ROOT / ".github" / "build-config.json"
RELEASE_YML = WORKFLOWS_DIR / "release.yml"
BUILD_RESULTS_YML = WORKFLOWS_DIR / "build-results.yml"
EXTRA_TRIGGERS: frozenset[str] = frozenset({"pre-commit"})
PLATFORM_FILES: dict[str, str] = {"Windows": "windows.yml"}
def _workflow_name(path: Path) -> str:
    return yaml.safe_load(path.read_text(encoding="utf-8")).get("name", "")
def _platform_workflows() -> list[str]:
    raw = json.loads(BUILD_CONFIG_JSON.read_text(encoding="utf-8")).get("build", {}).get("platform_workflows", "")
    assert raw, "platform_workflows missing from build-config.json"
    return [name.strip() for name in raw.split(",") if name.strip()]
def test_only_windows_x64_is_configured_for_platform_builds() -> None:
    assert _platform_workflows() == ["Windows"]
    assert sorted(_platform_workflows()) == sorted(PLATFORM_FILES)
    assert _workflow_name(WORKFLOWS_DIR / "windows.yml") == "Windows"
    assert not (WORKFLOWS_DIR / "linux.yml").exists()
def test_build_results_trigger_matches_build_config() -> None:
    doc = yaml.safe_load(BUILD_RESULTS_YML.read_text(encoding="utf-8"))
    on = doc.get("on") or doc.get(True)
    assert {str(name) for name in on["workflow_run"]["workflows"]} == set(_platform_workflows()) | EXTRA_TRIGGERS
def test_release_wait_for_builds_lists_match_platforms() -> None:
    doc = yaml.safe_load(RELEASE_YML.read_text(encoding="utf-8"))
    steps = doc["jobs"]["wait-for-builds"]["steps"]
    names_block = next(step["with"]["workflow-names"] for step in steps if "workflow-names" in step.get("with", {}))
    assert {line.strip() for line in str(names_block).splitlines() if line.strip()} == set(_platform_workflows())
