#!/usr/bin/env python3
"""Detect whether Linux x64 or Windows x64 CI builds are needed from changed files."""

from __future__ import annotations

import argparse
import os
import re
from typing import TYPE_CHECKING

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.gh_actions import gh_warning, write_github_output
from common.git import run_git

if TYPE_CHECKING:
    from collections.abc import Sequence

_COMMON_PATTERNS = [
    r"^src/",
    r"^CMakeLists\.txt$",
    r"^cmake/",
    r"^libs/",
    r"^resources/",
    r"^translations/",
    r"^.*\.qrc$",
    r"^test/",
    r"^\.github/actions/",
    r"^\.github/scripts/",
    r"^\.github/build-config\.json$",
    r"^\.github/build-config\.schema\.json$",
]

_PLATFORM_PATTERNS: dict[str, list[str]] = {
    "linux": [
        r"^deploy/linux/",
        r"^tools/setup/.*debian",
        r"^tools/setup/install_dependencies/",
    ],
    "windows": [
        r"^deploy/windows/",
        r"^tools/setup/.*windows",
    ],
}


def workflow_name_for_platform(platform: str) -> str:
    """Return the workflow filename stem for a supported build platform."""
    if platform not in _PLATFORM_PATTERNS:
        raise ValueError(f"Unsupported build platform: {platform!r}; expected linux or windows")
    return platform


def build_patterns(platform: str) -> list[re.Pattern[str]]:
    """Compile common and platform-specific build-input patterns."""
    workflow_name = workflow_name_for_platform(platform)
    raw = [
        *_COMMON_PATTERNS,
        rf"^\.github/workflows/{re.escape(workflow_name)}\.yml$",
        *_PLATFORM_PATTERNS[platform],
    ]
    return [re.compile(pattern) for pattern in raw]


def has_relevant_changes(files: Sequence[str], platform: str) -> bool:
    """Return True when any changed path is relevant to the requested platform."""
    patterns = build_patterns(platform)
    return any(path and any(pattern.search(path) for pattern in patterns) for path in files)


_NULL_SHA = "0" * 40


def _ensure_commit(sha: str) -> bool:
    if not sha:
        return True
    if run_git("cat-file", "-e", f"{sha}^{{commit}}").returncode == 0:
        return True
    run_git("fetch", "--no-tags", "--depth=1", "origin", sha)
    return run_git("cat-file", "-e", f"{sha}^{{commit}}").returncode == 0


def _diff_names(base: str, head: str) -> list[str] | None:
    result = run_git("diff", "--name-only", base, head)
    return result.stdout.strip().splitlines() if result.returncode == 0 else None


def _tree_names(sha: str) -> list[str]:
    result = run_git("diff-tree", "--no-commit-id", "--name-only", "-r", sha)
    return result.stdout.strip().splitlines() if result.returncode == 0 else []


def get_changed_files() -> list[str] | None:
    """Read the event-specific Git diff; None means the diff was not reliable."""
    event = os.environ.get("EVENT_NAME", "")
    if event == "pull_request":
        base, head = os.environ.get("PR_BASE_SHA", ""), os.environ.get("PR_HEAD_SHA", "")
        if not _ensure_commit(base) or not _ensure_commit(head):
            return None
        return _diff_names(base, head)
    if event == "push":
        before, current = os.environ.get("PUSH_BEFORE_SHA", ""), os.environ.get("CURRENT_SHA", "")
        if before and before != _NULL_SHA:
            if not _ensure_commit(before) or not _ensure_commit(current):
                return None
            return _diff_names(before, current)
        return _tree_names(current)
    if event == "merge_group":
        base, current = os.environ.get("MERGE_BASE_SHA", ""), os.environ.get("CURRENT_SHA", "")
        if base:
            if not _ensure_commit(base) or not _ensure_commit(current):
                return None
            return _diff_names(base, current)
    current = os.environ.get("CURRENT_SHA", "")
    return _tree_names(current) if current else None


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Detect Linux x64 and Windows x64 CI-relevant changes."
    )
    parser.add_argument("--platform", nargs="+", choices=["linux", "windows"])
    parser.add_argument("--non-pr-passthrough", action="store_true")
    args = parser.parse_args(argv)
    if not args.non_pr_passthrough and not args.platform:
        parser.error("--platform linux|windows is required")
    return args


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    if args.non_pr_passthrough:
        write_github_output(
            {"should_build": "true", "linux": "true", "windows": "true", "any": "true"}
        )
        return 0

    platforms: list[str] = args.platform
    changed = get_changed_files()
    if changed is None:
        gh_warning("Unable to compute changed files reliably; forcing requested platform build(s).")
        outputs = dict.fromkeys(platforms, "true")
        outputs["any"] = "true"
        write_github_output(outputs)
        return 0

    print("Changed files:")
    for path in changed:
        print(f"  {path}")

    outputs = {}
    for platform in platforms:
        result = has_relevant_changes(changed, platform)
        outputs[platform] = "true" if result else "false"
        print(f"  {platform}: {result}")
    outputs["any"] = outputs[platforms[0]]
    write_github_output(outputs)
    print(f"Results: {outputs}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
