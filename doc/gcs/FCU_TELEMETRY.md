# Flight FCU Telemetry Facts

## Source and scope

The wire contract is copied from `Flight/spec/contracts/FCU.xml` into
`src/MAVLink/flight.xml`. The QGC MAVLink `all` dialect is extended at CPM
configure time by `mavlink-flight-dialect.patch`; the patch adds both the
`flight.xml` include and the XML file itself to the pinned upstream MAVLink
source tree. Message IDs, field order, payload lengths and CRC_EXTRA remain
owned by the Flight repository.

All values are informational telemetry. They do not grant control authority,
approve arming, clear a failsafe, or authorize actuator output. FMS remains the
only authority for operational decisions.

## QML access

The per-vehicle FactGroup is exposed as:

```qml
vehicle.flightTelemetry
```

Read a value using:

```qml
vehicle.flightTelemetry.getFact("solverStatus").rawValue
```

Facts are intentionally flat within this group. Each message family has an
`<family>Available` boolean and its own `<family>TimeBootUs`,
`<family>BootId`, and `<family>ProfileId` facts where present. Consumers
must check the availability flag before displaying a value. A zero value is
not a substitute for checking validity.

The group accepts only messages from the connected vehicle's system and
autopilot component IDs. For periodic message families it rejects duplicate
or older `time_boot_us` samples. It tracks one active `boot_id`; a new
operation-status boot ID resets the family availability flags, digest values,
and timestamp watermarks. Event messages are deduplicated by event ID. Build-info
digest chunks are keyed by `digest_kind`, so all four hashes can be retained
even when they share a publication timestamp. Reserved enum values and invalid
boolean/mask ranges are rejected rather than interpreted as healthy.

## Message coverage

| ID | Message | Availability fact | Main fields |
|---:|---|---|---|
| 43000 | `FCU_OPERATION_STATUS` | `operationAvailable` | arm/mode/authority/failsafe/mission state and authority/directive/permit epochs |
| 43001 | `FCU_NAV_STATUS` | `navigationAvailable` | state epoch, origin, navigation capability mask, position/velocity/heading standard deviations, sample age |
| 43002 | `FCU_SENSOR_HEALTH` | `sensorHealthAvailable` | sensor instance, policy epoch, health state, action, reason, gap/reject counts |
| 43003 | `FCU_CONTROL_STATUS` | `controlAvailable` | control sequence, reference ID/epoch, controllable/degraded DOF masks, solve time, solver status, reason |
| 43004 | `FCU_ACTUATOR_STATUS` | `actuatorAvailable` | control sequence, permit epoch, available/saturated masks, output policy, feedback capability, reason |
| 43005 | `FCU_LINK_STATUS` | `linkAvailable` | link instance/kind/state, validity mask, packet loss, RSSI, quality, age, reason |
| 43006 | `FCU_STORAGE_STATUS` | `storageAvailable` | validity mask, settings generation, event/log drops, free KiB, SD/NOR states, reason |
| 43007 | `FCU_RESOURCE_STATUS` | `resourceAvailable` | validity mask, task ID, WCET, stack free, deadline misses, AXI free, CPU load |
| 43008 | `FCU_TIME_STATUS` | `timeAvailable` | time epoch, source mapping epoch, UTC value/uncertainties, time quality, discipline state, UTC-valid |
| 43009 | `FCU_EVENT` | `eventAvailable` | event ID, source, severity, reason namespace/code, authority/directive/permit epochs |
| 43010 | `FCU_PREARM_STATUS` | `prearmAvailable` | check epoch, check ID/state, mode mask, evidence time, reason namespace/code, recovery condition |
| 43011 | `FCU_RC_INPUT_STATUS` | `rcInputAvailable` | valid axes, normalized roll/pitch/yaw/throttle, sample age, receiver failsafe, link state |
| 43012 | `FCU_BUILD_INFO` | `buildInfoAvailable` | profile/product/port/schema revisions, digest kind and SHA-256 chunk, optional source commit |

## Fixed enum meanings

The values below follow the Flight ICD and safety contract. Unknown/reserved
values should be displayed as unknown, not silently mapped to healthy.

- Sensor health state: 0 UNKNOWN, 1 INITIALIZING, 2 HEALTHY, 3 SUSPECT,
  4 DEGRADED, 5 QUARANTINED, 6 FAILED, 7 RECOVERY_PROBATION.
- Solver status: 0 NOT_RUN, 1 SOLVED, 2 SOLVED_INACCURATE, 3 MAX_ITER,
  4 PRIMAL_INFEASIBLE, 5 DUAL_INFEASIBLE, 6 NUMERICAL_FAILURE,
  7 DEADLINE_EXCEEDED, 8 INVALID_INPUT.
- Link state: 0 UNKNOWN, 1 INITIALIZING, 2 HEALTHY, 3 DEGRADED, 4 LOST,
  5 RECOVERING. Link kind: 1 RC, 2 GCS, 3 COMPANION, 4 OTHER.
- Storage state: 0 UNKNOWN, 1 READY, 2 DEGRADED, 3 UNAVAILABLE.
- Time quality: 0 LOCAL_ONLY, 1 ACQUIRING, 2 LOCKED, 3 HOLDOVER,
  4 DEGRADED, 5 INVALID. Local timebase discipline uses the same 0..5
  ordering: NOMINAL, ACQUIRING, LOCKED, HOLDOVER, DEGRADED, INVALID.
- Event severity: 0 INFO, 1 WARNING, 2 ERROR, 3 CRITICAL. Reason namespace:
  0 common, 1 Guidance, 2 Control, 3 SensorConsistency.
- Pre-arm check state: 0 UNKNOWN, 1 PASS, 2 FAIL, 3 WARNING. Check IDs are
  fixed 0..14; bit positions in masks equal the check ID.
- Build digest kind: 0 FIRMWARE, 1 CORE_PARAMETERS, 2 PROFILE, 3 ABI.
  Each digest is a complete 32-byte SHA-256 represented as 64 lowercase hex
  characters. Source commit is 20 bytes represented as 40 hex characters
  only when `sourceCommitValid` is true.

## Validity masks

The raw mask is always exposed; consumers must gate individual measurements
with the corresponding bit rather than interpreting a zero as a valid value.

- Link validity: bit 8 RSSI valid, bit 9 link-quality valid, bit 10 packet-loss
  valid (same definitions as `ContractHeader.validity_flags`).
- Storage validity: bit 0 SD, bit 1 NOR, bit 2 free-space value.
- Resource validity: bit 0 WCET, bit 1 stack, bit 2 CPU, bit 3 AXI.
- RC valid axes: bit 0 roll, bit 1 pitch, bit 2 yaw, bit 3 throttle.
- Control DOF masks: bit 0 total thrust, bit 1 roll torque, bit 2 pitch torque,
  bit 3 yaw torque.
- Navigation capability bits are the Flight navigation capability mask; do
  not infer position/velocity capability from numeric accuracy fields alone.

## Deliberate limitations

- `FCU_EVENT` currently exposes the latest event plus an incrementing
  `eventSequence`; a persistent event-history ListModel and acknowledgement
  workflow are a separate UI task.
- `FCU_BUILD_INFO` exposes each digest chunk separately. A release-verification
  workflow must compare these values with a trusted build manifest; a telemetry
  hash alone is not a signature or proof of provenance.
- The group exposes numeric enum values and raw masks. Human-readable labels
  and dashboard severity styling belong in the QML presentation layer.
- The group does not issue commands or modify any Flight state.
