"""Tests for check_baseline_ready.py."""

from __future__ import annotations

from check_baseline_ready import evaluate_readiness

PLATFORMS = ["Windows"]


def _run(
    name: str,
    *,
    status: str = "completed",
    conclusion: str = "success",
    event: str = "push",
    created_at: str = "2026-02-24T00:00:00Z",
) -> dict[str, str]:
    return {
        "name": name,
        "status": status,
        "conclusion": conclusion,
        "event": event,
        "created_at": created_at,
    }


def test_evaluate_readiness_ready_when_windows_release_succeeds() -> None:
    runs = [_run("Windows")]
    ready, missing, incomplete, failed = evaluate_readiness(runs, PLATFORMS, "push")
    assert ready is True
    assert missing == []
    assert incomplete == []
    assert failed == []


def test_evaluate_readiness_reports_missing_windows_workflow() -> None:
    ready, missing, incomplete, failed = evaluate_readiness([], PLATFORMS, "push")
    assert ready is False
    assert missing == ["Windows"]
    assert incomplete == []
    assert failed == []


def test_evaluate_readiness_reports_incomplete_windows_workflow() -> None:
    runs = [_run("Windows", status="in_progress")]
    ready, missing, incomplete, failed = evaluate_readiness(runs, PLATFORMS, "push")
    assert ready is False
    assert missing == []
    assert incomplete == ["Windows"]
    assert failed == []


def test_evaluate_readiness_reports_failed_windows_workflow() -> None:
    runs = [_run("Windows", conclusion="failure")]
    ready, missing, incomplete, failed = evaluate_readiness(runs, PLATFORMS, "push")
    assert ready is False
    assert missing == []
    assert incomplete == []
    assert failed == ["Windows"]


def test_evaluate_readiness_uses_latest_windows_run() -> None:
    runs = [
        _run("Windows", conclusion="failure", created_at="2026-02-24T00:00:00Z"),
        _run("Windows", conclusion="success", created_at="2026-02-24T01:00:00Z"),
    ]
    ready, missing, incomplete, failed = evaluate_readiness(runs, PLATFORMS, "push")
    assert ready is True
    assert missing == []
    assert incomplete == []
    assert failed == []


def test_evaluate_readiness_filters_by_event() -> None:
    runs = [_run("Windows", event="pull_request")]
    ready, missing, incomplete, failed = evaluate_readiness(runs, PLATFORMS, "push")
    assert ready is False
    assert missing == ["Windows"]
    assert incomplete == []
    assert failed == []
