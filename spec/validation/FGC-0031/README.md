# FGC-0031 — FlightTelemetry freshness, ordering, reboot and QML presentation

> Archived validation record: the page-level test and its stable selectors are now part of `main`
> (commit `de0d60b`). This file preserves the branch-scoped evidence history; current issue status
> is tracked in GitHub Issue #31.

| Item | Value |
|---|---|
| GitHub Issue | [#31](https://github.com/hujunjie-hub/Flight_GroundControl/issues/31) |
| Severity | S2-MEDIUM / TEST_GAP |
| Branch | **fix/fgc-0031-telemetry-page-ui** |
| Base | **codex/phase2-fcu-telemetry** |
| Status | **VERIFIED** |

## Approved telemetry contract

For each message family and boot epoch, the FlightTelemetryFactGroup accepts only strictly increasing FCU time_boot_us values:

- newer timestamp: accept and update the family Fact;
- equal timestamp: duplicate, reject without Fact update;
- lower timestamp: stale/out-of-order, reject without Fact update;
- confirmed nonzero boot_id change: establish a new epoch and timestamp baseline;
- delayed frames from retired boot IDs: reject;
- boot_id=0 is epoch-unknown and must not reset an established nonzero epoch or watermark.

Watermarks are independent per message family. Tests use raw MAVLink bytes through MockLink, MAVLinkProtocol, Vehicle dispatch, and FlightTelemetryFactGroup; they do not set Facts directly.

## Existing real-wire coverage

FlightTelemetryDispatchTest covers all 13 FCU message families, ordering, duplicate/stale timestamps, boot-epoch transition, delayed retired-epoch frames, unknown boot ID, disconnect/reconnect, and per-family freshness. The phase-2 implementation recorded 38/38 dispatch tests passing in local evidence, with Unit 198/198 and Integration 70/70.

## New QML page-level coverage

The previous gap was that helper functions and FactGroup properties were tested, but the actual FlightTelemetry setup page did not have an end-to-end presentation test.

FlightTelemetryPageUITest now opens the real FlightTelemetry setup page with a Generic MockLink and verifies the bound VehicleSummaryRow values:

1. UNKNOWN: before any FCU custom telemetry, operation status shows “等待遥测” and storage free space shows “未上报”.
2. INVALID: a real FCU_STORAGE_STATUS frame with storage family available and validity_mask bit 2 clear makes free space show “无效”.
3. STALE: after a valid FCU_OPERATION_STATUS frame ages beyond the page's 1000 ms freshness limit, the operation status shows “数据超时”.
4. RECOVERY: a strictly newer operation frame restores “已接收”.

The test uses raw MAVLink messages through the production dispatch path. The added QML objectName values are stable test selectors only; no telemetry behavior or freshness thresholds are changed.

CTest registration: Integration / NoSanitizer, 180-second timeout.

## Current mainline status (2026-11-14)

The page-level test and the telemetry dispatch implementation are present on `main`. The last completed full Windows Release run [43567619588](https://github.com/hujunjie-hub/Flight_GroundControl/actions/runs/43567619588) at HEAD `37035a38d8fcdecca6412b3696704d6416aa3600` passed Unit 198/198 and Integration 71/71. The later main baseline `77563b1` has a Windows Release run `43571983530` still IN_PROGRESS at the time of this snapshot. Issue #31 remains Open as an audit record; current dynamic status is in GitHub.

## Acceptance

- [x] Approved per-family timestamp and boot-epoch contract implemented in phase-2.
- [x] Real-wire parser/Vehicle dispatch regression coverage exists for timestamp ordering, reboot, disconnect, and freshness.
- [x] QML page exposes stable objectName selectors for the two asserted values.
- [x] Page-level UNKNOWN / INVALID / STALE / RECOVERY test added.
- [x] Exact-head Windows Release CI passes the new QML UI test and full Unit/Integration suites.
- [x] Review the final Windows CI artifacts; keep Issue #31 open as an audit record until the project consolidation pass.

Do not treat local test history as a substitute for the exact-head Windows Release result. Keep the issue Open / FIXED_WAITING_RETEST until that run succeeds.


## 2026-11-13 — exact-head Windows gate failure triage

Initial Windows Release run [43355557100](https://github.com/hujunjie-hub/Flight_GroundControl/actions/runs/43355557100), PR head `16b7941e3859ff02f1bdf403c8a416b79205bf42`, completed build successfully. The new `FlightTelemetryPageUITest` passed; the overall gate did not:

- Unit: 197/198. `SurveyComplexItemTest` hit its 60-second CTest timeout during the 12-way parallel Unit run. No product assertion failure was recorded. This remains a separate retest item; do not increase the timeout or classify it as fixed without repeat evidence.
- Integration: 70/71. `RequestMessageTest::_duplicateWhileQueued` observed 3 `MAV_CMD_REQUEST_MESSAGE` commands where the test expected 2. Its exact-count test paths did not disable the GimbalController and CameraManager, although the shared `_testCaseWorker` explicitly disables both because they issue background message requests. The direct duplicate/queue test paths are now isolated in commit [9e9d53d](https://github.com/hujunjie-hub/Flight_GroundControl/commit/9e9d53d26b7a8e545adb25f5cf764e5766df03d1) by disabling both background requesters in `_duplicateCommand`, `_duplicateWhileQueued`, and `_compIdAllFailure`.

The initial failing run's JUnit artifact confirms the observed mismatch (`Actual: 3`, `Expected: 2`). This source-level isolation correction is a test-harness fix, not a change to request-message production behavior. Exact-head Windows Release validation on the new commit is required before marking the gate green.

A failed-job rerun (attempt 2) was started for the original head, then cancelled automatically when commit `9e9d53d` updated the PR branch and superseded that run. It produced no new test result and is not acceptance evidence.


## R30 — exact-head Windows Release verification — 2026-11-13

Validated PR head: `38c9acd37da8b706869d778ed6891e1e97d3efc9`  
Windows Release run: [43374503873](https://github.com/hujunjie-hub/Flight_GroundControl/actions/runs/43374503873) — **SUCCESS**.

- Configure and Release build: PASS.
- Unit: **198/198 PASS**, 0 failures, 0 errors, 0 skipped.
- Integration: **71/71 PASS**, 0 failures, 0 errors, 0 skipped. The additional FlightTelemetryPageUITest is included.
- Executable verification, installer creation, clean installation, and installed executable verification: PASS.
- Artifacts: Unit JUnit `14015968464`; Integration JUnit `14016083112`; installer `14015894733`.

The new QML page-level test and the complete Windows Release gate passed on the exact PR head. **FGC-0031 status: VERIFIED**. Issue #31 remains open as an audit record; no PR merge or main-branch change is implied.
