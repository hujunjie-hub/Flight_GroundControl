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

## Ground station page

The Flight Configure sidebar includes **飞控诊断** (`FlightTelemetry.qml`). It is a read-only
view of operation, navigation, sensor health, control, actuator, link, storage,
resource, time, event, pre-arm, RC input and build information, with a bounded recent-event history.
Sensor health is shown per sensor instance, communication state per RC/GCS/Companion
link instance, and resource measurements per task. Each telemetry card shows its own
message availability and local ground-station receive age. The page honors the
link/storage/resource validity masks and the RC valid-axis mask; it does not send commands
or grant control authority.

## QML access

The per-vehicle FactGroup is exposed as:

```qml
vehicle.flightTelemetry
```

The 64-bit `opBootId` is also exposed as `opBootIdHex` (16-digit,
zero-padded hexadecimal) for display. QML/JavaScript numbers cannot exactly
represent every 64-bit integer, so do not format boot IDs through numeric
conversion.

Read a value using:

```qml
vehicle.flightTelemetry.getFact("solverStatus").rawValue
vehicle.flightTelemetry.sensorHealthStatuses
vehicle.flightTelemetry.linkStatuses
vehicle.flightTelemetry.resourceStatuses
vehicle.flightTelemetry.prearmChecks
vehicle.flightTelemetry.recentEvents
```

The list properties return the latest accepted record for each instance/check,
ordered by their numeric key. Facts are intentionally flat within this group. Each message family has an
`<family>Available` boolean and its own `<family>TimeBootUs`,
`<family>BootId`, and `<family>ProfileId` facts where present. Consumers
must check the availability flag before displaying a value. Availability means
that at least one valid message in that family has been received during the
current boot; it is not a freshness watchdog. A zero value is not a substitute
for checking validity. For periodic data, compare each family timestamp with
the latest `opTimeBootUs` (same boot); apply a UI-side age threshold only after
the corresponding publisher frequency has been defined and measured. For link
and navigation measurements, also honor their transmitted `linkAgeUs` and
`navigationAgeUs` fields. The `familyReceiveAgeMs` property separately reports
monotonic ground-station receive age for all 13 message families (`-1` means no
valid message received in the current boot). The QML page refreshes this display
twice per second. This is host-side packet receipt age, not FCU sample age, and
does not apply a hard-coded stale/healthy threshold. The Flight ICD currently
does not assign publish frequencies to these 13 private telemetry families;
a stale alarm threshold must be configured only after the firmware publisher
schedule is defined and measured. Until then the page reports age without
claiming that a family is stale or healthy.

The group accepts only messages from the connected vehicle's system and
autopilot component IDs. The GCS dialect currently targets FCU schema 6.0 and
QUAD_X4 profile 0x00010001. The diagnostics page labels exact 6.0 as compatible,
a different major version as incompatible, and a different minor version as
unverified; Profile is cross-checked between operation status and build info.
These labels are informational UI diagnostics only and do not authorize,
inhibit, or alter flight operations. Until build info arrives, compatibility is
unknown.

A bounded set of recently retired boot IDs is retained for the lifetime of the
Vehicle object. This prevents a delayed OPERATION_STATUS packet from a prior
boot from rolling the telemetry cache back to an old session. No ordering is
assumed for the random 64-bit boot ID.

All 13 telemetry families must carry a nonzero `profile_id`. The first
accepted message in a boot session establishes the active profile; subsequent
messages with a different profile are rejected before their timestamps or
cached values are updated. A newly accepted boot session clears that profile
baseline and establishes a new one. This prevents navigation, control, actuator
and diagnostic snapshots from different product profiles being mixed in one
Vehicle view.

For single-instance periodic families it rejects duplicate or older
`time_boot_us` samples. Sensor-health, link-status and resource-status use
independent timestamp watermarks keyed by `sensor_instance_id`,
`link_instance_id` and `task_id`; different instances may legitimately publish
at the same timestamp. Sensor IDs are 1 IMU, 2 GNSS, 3 barometer and 4 magnetometer.
Link IDs are 1 RC, 2 GCS, 3 Companion and 4 other (the first-version contract
requires `link_instance_id == link_kind`). Resource records are bounded to 64
task IDs. It tracks one active `boot_id`; a new operation-status boot ID resets
the family availability flags, per-instance records, digest values and timestamp
watermarks. The QML page gates every displayed fact on its family availability flag,
so values left in the cache from the previous boot appear as 未上报 until a new family sample arrives.
Event messages reject older publication timestamps and exact duplicate
`(event_id, time_boot_us)` pairs. Repeated updates to the same event ID with a
newer timestamp remain valid, and distinct event IDs may share one publication
timestamp. Build-info digest chunks are keyed by `digest_kind`; duplicate chunks
at the same publication timestamp are rejected independently, so all four
hashes can be retained even when they share one snapshot timestamp. All chunks
in a snapshot must agree on profile, product/port revision, schema and source
commit. Mixed metadata is rejected. A newer snapshot timestamp clears every
old digest and source-commit value before accepting the new snapshot, preventing
hashes from different builds being mixed. Reserved enum values and invalid
boolean/mask ranges are rejected rather than interpreted as healthy.

## Message coverage

| ID | Message | Availability fact | Main fields |
|---:|---|---|---|
| 43000 | `FCU_OPERATION_STATUS` | `operationAvailable` | arm/mode/authority/failsafe/mission; authority/directive/permit epochs |
| 43001 | `FCU_NAV_STATUS` | `navigationAvailable` | state/origin, capabilities, position/velocity/heading accuracy, age |
| 43002 | `FCU_SENSOR_HEALTH` | `sensorHealthAvailable` | sensor/policy IDs, health/action, reason, gap/reject counts |
| 43003 | `FCU_CONTROL_STATUS` | `controlAvailable` | control/reference IDs, DOF masks, solve time/status, reason |
| 43004 | `FCU_ACTUATOR_STATUS` | `actuatorAvailable` | control/permit epochs, availability/saturation masks, policy, feedback |
| 43005 | `FCU_LINK_STATUS` | `linkAvailable` | link kind/state, validity, loss, RSSI/LQ, age, reason |
| 43006 | `FCU_STORAGE_STATUS` | `storageAvailable` | validity, settings generation, event/log drops, free space, SD/NOR |
| 43007 | `FCU_RESOURCE_STATUS` | `resourceAvailable` | validity, task, WCET, stack, deadline misses, AXI, CPU |
| 43008 | `FCU_TIME_STATUS` | `timeAvailable` | time/source epochs, UTC, uncertainty, quality, discipline, validity |
| 43009 | `FCU_EVENT` | `eventAvailable` | event/source IDs, severity, reason, authority/directive/permit epochs |
| 43010 | `FCU_PREARM_STATUS` | `prearmAvailable` | check ID/state, mode mask, evidence time, reason, recovery |
| 43011 | `FCU_RC_INPUT_STATUS` | `rcInputAvailable` | axis mask/values, sample age, receiver failsafe, link state |
| 43012 | `FCU_BUILD_INFO` | `buildInfoAvailable` | product/port/schema revisions, digest type/hash, source commit |

## Fixed enum meanings

The values below follow the Flight ICD and safety contract. Unknown/reserved
values should be displayed as unknown, not silently mapped to healthy.

- Arm state: 0 DISARMED, 1 ARMING, 2 ARMED, 3 DISARMING.
- Flight mode: 0 NONE, 1 MANUAL_RATE, 2 MANUAL_ATTITUDE, 3 ALT_HOLD,
  4 POS_HOLD, 5 AUTO_MISSION, 6 AUTO_RETURN, 7 AUTO_LAND, 8 EMERGENCY_LAND.
- Authority: 0 NONE, 1 PILOT, 2 MISSION, 3 GCS_SUPERVISED.
- Failsafe level: 0 NORMAL, 1 DEGRADED, 2 ACTION_REQUIRED, 3 TERMINAL.
- Mission state: 0 IDLE, 1 LOADED, 2 RUNNING, 3 PAUSED, 4 ABORTED, 5 COMPLETED.

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
  0 common, 1 Guidance, 2 Control, 3 SensorConsistency. Reason-code labels follow
  the Flight ICD: common codes 0–25, Guidance 0x0601–0x060F, Control
  0x0701–0x0705, and SensorConsistency 0–8. The UI displays unknown numeric
  codes as unknown rather than guessing their meaning.
- Pre-arm check state: 0 UNKNOWN, 1 PASS, 2 FAIL, 3 WARNING. Check IDs are
  fixed 0..14; bit positions in masks equal the check ID. Check epochs start at
  1; epoch 0 is reserved and rejected.
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
- RC valid axes: bit 0 roll, bit 1 pitch, bit 2 yaw, bit 3 throttle. Valid
  roll/pitch/yaw values must be finite and in [-1, 1]; throttle must be finite
  and in [0, 1].
- Control DOF masks: bit 0 total thrust, bit 1 roll torque, bit 2 pitch torque,
  bit 3 yaw torque.
- Navigation capability bits are the Flight navigation capability mask; do
  not infer position/velocity capability from numeric accuracy fields alone.

## Deliberate limitations

- Decoder coverage does not imply that the firmware currently publishes
  these messages. At the Flight `main` revision checked for this GCS change,
  `spec/contracts/FCU.xml` and the generated wire-layout header exist, but the
  runtime telemetry-link and MAVLink frame modules contain no send call sites
  for FCU message IDs 43000-43012. Until the firmware publisher is integrated,
  the diagnostic page is expected to show "waiting telemetry". Do not interpret
  silence or default zero values as healthy measurements.
- `FCU_EVENT` exposes the latest event plus a per-boot, bounded history of the
  most recent 64 accepted records. The UI displays the latest eight. Exact
  duplicate event IDs at the same publication timestamp are suppressed;
  repeated updates with a newer timestamp remain visible. Acknowledgement and
  persistent cross-session history require a separate protocol/storage design.
- `FCU_PREARM_STATUS` carries one check per message. The ground station
  aggregates the latest result for each check ID within `check_epoch`, permits
  distinct check IDs to share one publication timestamp, and rejects duplicate
  or older samples per check ID. It clears the set on a newer epoch or FCU reboot
  and rejects delayed older epochs. The UI supports the registered check IDs;
  actual check coverage depends on the firmware publisher implementation.
- Sensor/link/resource records are retained per instance. Resource task records
  have a fixed 64-task cache limit; additional unseen task IDs are ignored until
  the next FCU boot rather than allowing unbounded ground-station memory growth.
- `FCU_BUILD_INFO` exposes each digest chunk separately. A release-verification
  workflow must compare these values with a trusted build manifest; a telemetry
  hash alone is not a signature or proof of provenance.
- The group exposes numeric enum values and raw masks. Human-readable labels
  and dashboard severity styling belong in the QML presentation layer.
- The group does not issue commands or modify any Flight state.
