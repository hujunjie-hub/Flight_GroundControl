#!/usr/bin/env python3
"""Detect whether the Windows x64 CI build is needed from changed files."""

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

# Changes to these paths can affect every Windows x64 build.
_COMMON_PATTERNS: tuple[str, ...] = (
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
)

# Windows x64-only build inputs.
_WINDOWS_PATTERNS: tuple[str, ...] = (
    r"^tools/setup/.*windows",
    r"^deploy/windows/",
    r"^\.github/workflows/windows\.yml$",
)


def build_patterns(platform: str = "windows") -> list[re.Pattern[str]]:
    """Return the Windows x64 build-trigger patterns.

    Reject other platforms explicitly so a stale caller cannot silently
    reintroduce non-Windows build jobs.
    """
    if platform != "windows":
        raise ValueError(
            f"Unsupported build platform: {platform!r}; only Windows x64 is supported"
        )
    return [re.compile(pattern) for pattern in (*_COMMON_PATTERNS, *_WINDOWS_PATTERNS)]


def has_relevant_changes(files: Sequence[str], platform: str = "windows") -> bool:
    """Return whether any changed path affects the Windows x64 build."""
    patterns = build_patterns(platform)
    return any(path and any(pattern.search(path) for pattern in patterns) for path in files)


_NULL_SHA = "0" * 40


def _ensure_commit(sha: str) -> bool:
    """Ensure a commit exists locally; fetch it shallowly if needed."""
    if not sha:
        return True
    if run_git("cat-file", "-e", f"{sha}^{{commit}}").returncode == 0:
        return True
    run_git("fetch", "--no-tags", "--depth=1", "origin", sha)
    return run_git("cat-file", "-e", f"{sha}^{{commit}}").returncode == 0


def _diff_names(sha_a: str, sha_b: str) -> list[str] | None:
    """Return changed file names between two commits, or None on failure."""
    result = run_git("diff", "--name-only", sha_a, sha_b)
    if result.returncode != 0:
        return None
    return result.stdout.strip().splitlines()


def _tree_names(sha: str) -> list[str]:
    """List files changed in a single commit."""
    result = run_git("diff-tree", "--no-commit-id", "--name-only", "-r", sha)
    return result.stdout.strip().splitlines() if result.returncode == 0 else []


def get_changed_files() -> list[str] | None:
    """Determine changed files from GitHub Actions event variables.

    Return None when a diff cannot be computed reliably; the caller then
    forces the Windows x64 build rather than skipping validation.
    """
    event = os.environ.get("EVENT_NAME", "")

    if event == "pull_request":
        base = os.environ.get("PR_BASE_SHA", "")
        head = os.environ.get("PR_HEAD_SHA", "")
        if not _ensure_commit(base) or not _ensure_commit(head):
            return None
        return _diff_names(base, head)

    if event == "push":
        before = os.environ.get("PUSH_BEFORE_SHA", "")
        current = os.environ.get("CURRENT_SHA", "")
        if before and before != _NULL_SHA:
            if not _ensure_commit(before) or not _ensure_commit(current):
                return None
            return _diff_names(before, current)
        return _tree_names(current)

    if event == "merge_group":
        merge_base = os.environ.get("MERGE_BASE_SHA", "")
        current = os.environ.get("CURRENT_SHA", "")
        if merge_base:
            if not _ensure_commit(merge_base) or not _ensure_commit(current):
                return None
            return _diff_names(merge_base, current)

    current = os.environ.get("CURRENT_SHA", "")
    return _tree_names(current) if current else None


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Detect CI-relevant file changes for the Windows x64 build."
    )
    parser.add_argument(
        "--platform",
        nargs="+",
        choices=("windows",),
        default=["windows"],
        help="Build platform (only windows x64 is supported)",
    )
    parser.add_argument(
        "--non-pr-passthrough",
        action="store_true",
        help="Always schedule the Windows x64 build for non-PR events",
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)

    if args.non_pr_passthrough:
        write_github_output({"should_build": "true", "windows": "true", "any": "true"})
        return 0

    changed = get_changed_files()
    if changed is None:
        gh_warning("Unable to compute changed files reliably; forcing Windows x64 build.")
        write_github_output({"windows": "true", "any": "true"})
        return 0

    print("Changed files:")
    for path in changed:
        print(f"  {path}")

    should_build = has_relevant_changes(changed, "windows")
    outputs = {
        "windows": "true" if should_build else "false",
        "any": "true" if should_build else "false",
    }
    write_github_output(outputs)
    print(f"Results: {outputs}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
