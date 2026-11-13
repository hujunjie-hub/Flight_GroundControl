# FGC-0031 — FlightTelemetry freshness, ordering, reboot and QML presentation

| Item | Value |
|---|---|
| GitHub Issue | [#31](https://github.com/hujunjie-hub/Flight_GroundControl/issues/31) |
| Severity | S2-MEDIUM / TEST_GAP |
| Branch | **fix/fgc-0031-telemetry-page-ui** |
| Base | **codex/phase2-fcu-telemetry** |
| Status | **FIXED_WAITING_RETEST** |

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

## Acceptance

- [x] Approved per-family timestamp and boot-epoch contract implemented in phase-2.
- [x] Real-wire parser/Vehicle dispatch regression coverage exists for timestamp ordering, reboot, disconnect, and freshness.
- [x] QML page exposes stable objectName selectors for the two asserted values.
- [x] Page-level UNKNOWN / INVALID / STALE / RECOVERY test added.
- [ ] Exact-head Windows Release CI passes the new QML UI test and full Unit/Integration suites.
- [ ] Review the final Windows CI artifacts and close Issue #31 only after exact-head validation.

Do not treat local test history as a substitute for the exact-head Windows Release result. Keep the issue Open / FIXED_WAITING_RETEST until that run succeeds.
