#!/usr/bin/env python3
"""Detect drift between the active Windows workflow and its consumers."""

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
WINDOWS_YML = WORKFLOWS_DIR / "windows.yml"
LINUX_YML = WORKFLOWS_DIR / "linux.yml"

EXTRA_TRIGGERS: frozenset[str] = frozenset({"pre-commit"})
PLATFORM_FILES: dict[str, str] = {"Windows": "windows.yml"}


def _workflow_name(path: Path) -> str:
    return yaml.safe_load(path.read_text(encoding="utf-8")).get("name", "")


def _workflow_on(doc: dict[str, object]) -> dict[str, object]:
    # PyYAML's YAML 1.1 loader parses the unquoted key "on" as boolean True.
    return doc.get("on", doc.get(True, {}))  # type: ignore[arg-type,return-value]


def _platform_workflows() -> list[str]:
    raw = json.loads(BUILD_CONFIG_JSON.read_text(encoding="utf-8")).get("build", {}).get(
        "platform_workflows", ""
    )
    assert raw, "platform_workflows missing from build-config.json"
    return [name.strip() for name in raw.split(",") if name.strip()]


def test_configured_platform_is_windows_only() -> None:
    configured = _platform_workflows()
    assert configured == ["Windows"]
    assert sorted(configured) == sorted(PLATFORM_FILES)
    for expected_name, filename in PLATFORM_FILES.items():
        assert _workflow_name(WORKFLOWS_DIR / filename) == expected_name


def test_linux_build_workflow_is_disabled() -> None:
    doc = yaml.safe_load(LINUX_YML.read_text(encoding="utf-8"))
    triggers = _workflow_on(doc)
    assert set(triggers) == {"workflow_dispatch"}
    assert doc["jobs"]["linux-build"]["if"] is False


def test_windows_workflow_is_release_only() -> None:
    doc = yaml.safe_load(WINDOWS_YML.read_text(encoding="utf-8"))
    matrix = doc["jobs"]["build"]["strategy"]["matrix"]
    assert matrix["build_type"] == ["Release"]
    dispatch = _workflow_on(doc)["workflow_dispatch"]
    assert dispatch is None or "inputs" not in dispatch


def test_build_results_trigger_matches_build_config() -> None:
    doc = yaml.safe_load(BUILD_RESULTS_YML.read_text(encoding="utf-8"))
    on = _workflow_on(doc)
    workflows = on["workflow_run"]["workflows"]
    trigger = {str(name) for name in workflows}
    assert trigger == set(_platform_workflows()) | EXTRA_TRIGGERS


def test_release_wait_for_builds_lists_match_platforms() -> None:
    doc = yaml.safe_load(RELEASE_YML.read_text(encoding="utf-8"))
    steps = doc["jobs"]["wait-for-builds"]["steps"]
    names_block = next(
        step["with"]["workflow-names"]
        for step in steps
        if "workflow-names" in step.get("with", {})
    )
    listed = {line.strip() for line in str(names_block).splitlines() if line.strip()}
    assert listed == set(_platform_workflows())
