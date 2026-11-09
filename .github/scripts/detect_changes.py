#!/usr/bin/env python3
"""Detect whether the Windows x64 build is needed from changed files."""
from __future__ import annotations
import argparse, os, re
from typing import TYPE_CHECKING
from ci_bootstrap import ensure_tools_dir
ensure_tools_dir(__file__)
from common.gh_actions import gh_warning, write_github_output
from common.git import run_git
if TYPE_CHECKING:
    from collections.abc import Sequence
_COMMON_PATTERNS = [r"^src/",r"^CMakeLists\.txt$",r"^cmake/",r"^libs/",r"^resources/",r"^translations/",r"^.*\.qrc$",r"^test/",r"^\.github/actions/",r"^\.github/scripts/",r"^\.github/build-config\.json$",r"^\.github/build-config\.schema\.json$"]
_WINDOWS_PATTERNS = [r"^deploy/windows/",r"^tools/setup/.*windows",r"^\.github/workflows/windows\.yml$"]
def workflow_name_for_platform(platform: str) -> str:
    if platform != "windows": raise ValueError(f"Unsupported build platform: {platform!r}; only windows x64 is supported")
    return "windows"
def build_patterns(platform: str) -> list[re.Pattern[str]]:
    workflow_name_for_platform(platform)
    return [re.compile(p) for p in [*_COMMON_PATTERNS,*_WINDOWS_PATTERNS]]
def has_relevant_changes(files: Sequence[str], platform: str = "windows") -> bool:
    patterns=build_patterns(platform)
    return any(path and any(p.search(path) for p in patterns) for path in files)
_NULL_SHA="0"*40
def _ensure_commit(sha: str) -> bool:
    if not sha: return True
    if run_git("cat-file","-e",f"{sha}^{{commit}}").returncode==0: return True
    run_git("fetch","--no-tags","--depth=1","origin",sha)
    return run_git("cat-file","-e",f"{sha}^{{commit}}").returncode==0
def _diff_names(a: str,b: str) -> list[str] | None:
    r=run_git("diff","--name-only",a,b)
    return r.stdout.strip().splitlines() if r.returncode==0 else None
def _tree_names(sha: str) -> list[str]:
    r=run_git("diff-tree","--no-commit-id","--name-only","-r",sha)
    return r.stdout.strip().splitlines() if r.returncode==0 else []
def get_changed_files() -> list[str] | None:
    event=os.environ.get("EVENT_NAME","")
    if event=="pull_request":
        base,head=os.environ.get("PR_BASE_SHA",""),os.environ.get("PR_HEAD_SHA","")
        if not _ensure_commit(base) or not _ensure_commit(head): return None
        return _diff_names(base,head)
    if event=="push":
        before,current=os.environ.get("PUSH_BEFORE_SHA",""),os.environ.get("CURRENT_SHA","")
        if before and before!=_NULL_SHA:
            if not _ensure_commit(before) or not _ensure_commit(current): return None
            return _diff_names(before,current)
        return _tree_names(current)
    if event=="merge_group":
        base,current=os.environ.get("MERGE_BASE_SHA",""),os.environ.get("CURRENT_SHA","")
        if base:
            if not _ensure_commit(base) or not _ensure_commit(current): return None
            return _diff_names(base,current)
    current=os.environ.get("CURRENT_SHA","")
    return _tree_names(current) if current else None
def parse_args(argv: list[str] | None=None) -> argparse.Namespace:
    p=argparse.ArgumentParser(description="Detect Windows x64 CI-relevant changes.")
    p.add_argument("--platform",nargs="+",choices=["windows"])
    p.add_argument("--non-pr-passthrough",action="store_true")
    a=p.parse_args(argv)
    if not a.non_pr_passthrough and not a.platform: p.error("--platform windows is required")
    return a
def main(argv: list[str] | None=None) -> int:
    a=parse_args(argv)
    if a.non_pr_passthrough:
        write_github_output({"should_build":"true","windows":"true","any":"true"})
        return 0
    changed=get_changed_files()
    if changed is None:
        gh_warning("Unable to compute changed files reliably; forcing Windows x64 build.")
        write_github_output({"windows":"true","any":"true"})
        return 0
    print("Changed files:")
    for path in changed: print(f"  {path}")
    result=has_relevant_changes(changed,"windows")
    outputs={"windows":"true" if result else "false","any":"true" if result else "false"}
    write_github_output(outputs); print(f"Results: {outputs}")
    return 0
if __name__=="__main__": raise SystemExit(main())
