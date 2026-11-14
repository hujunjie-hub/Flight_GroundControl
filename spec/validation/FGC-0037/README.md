# FGC-0037 — GeoTag GPS Version ID malformed-entry write guard

> Archived validation record: the write guard and its regression fixtures are part of `main`.
> GitHub Issue #37 is CLOSED / VERIFIED; this file preserves branch-scoped evidence history.
> The old R27 FGC-0029 ASan shutdown observation below is historical and does not reopen FGC-0037.

| Item | Value |
|---|---|
| GitHub Issue | [#37](https://github.com/hujunjie-hub/Flight_GroundControl/issues/37) |
| Severity | S2-HIGH / SECURITY |
| Component | **ExifParser::write()** |
| PR | [#39](https://github.com/hujunjie-hub/Flight_GroundControl/pull/39), superseded by local delivery on `main` |
| Branch | **fix/fgc-0037-clean** (historical) |
| Current validated code HEAD | **9b22bab756139bd46174515dfb724c0e0120bfdd** |
| Issue status | **VERIFIED** (archived record) |

## Current mainline status (2026-11-14)

The GPS Version ID write guard remains in `main`; GitHub Issue #37 is CLOSED. Its scope is independent from FGC-0029. Historical R27 ASan process-shutdown evidence below is retained as-is and must not be read as the current status of Issue #37.

## Finding and implementation

The prior writer obtained an existing GPSVersionID EXIF entry and wrote bytes 0–3 after checking only that the entry and its data pointer were non-null. An existing malformed entry could have an incompatible format, component count, or allocation size.

The current implementation validates the GPS IFD entry before writing:

- format must be EXIF_FORMAT_BYTE;
- component count must be exactly four;
- data must be non-null;
- allocated size must be at least four bytes.

Malformed existing entries are rejected before the caller's JPEG buffer is modified. Regression fixtures cover a valid four-byte entry, short 0–3 component entries, an oversized component count, and an invalid format.

## Exact-head Windows Release evidence

Run [43275165598](https://github.com/hujunjie-hub/Flight_GroundControl/actions/runs/43275165598) validated exact HEAD **9b22bab756139bd46174515dfb724c0e0120bfdd**:

- Windows x64 MSVC 2022 Release build: PASS.
- Unit: 198/198 passed.
- Integration: 70/70 passed.
- Executable verification: PASS.
- Installer creation, clean installation, and installed executable verification: PASS.

This is historical R24 evidence. R26 exact-head Windows Release evidence is recorded below; it complements, but does not replace, the separate MSVC AddressSanitizer diagnostic gate.

R26 validated code HEAD `d38eaeb6c449ca887fa34bb2eb7ffbddab3b91bb`: Windows run [43348185004](https://github.com/hujunjie-hub/Flight_GroundControl/actions/runs/43348185004) completed SUCCESS — Configure, Build, Unit, Integration, installer creation, clean installation, installed executable verification, attestation and artifact upload all passed.

## ASan workflow defect and correction

Run [43275152519](https://github.com/hujunjie-hub/Flight_GroundControl/actions/runs/43275152519) stopped after the first GeoTagControllerTest CTest execution because the workflow required the XML to contain **_concurrentControllersExifStressTest**. That test slot is not present in the current clean FGC-0037 branch. The CTest target itself passed 1/1 before the stale assertion failed. The preceding ExifParserTest repeated matrix passed 20/20. No AddressSanitizer memory diagnostic was emitted before the workflow stopped, but this is **not** an ASan pass: the 20-run controller loop did not complete.

The corrected workflow (R26 exact-head run [43348178961](https://github.com/hujunjie-hub/Flight_GroundControl/actions/runs/43348178961) completed SUCCESS):

1. validates that each QtTest XML contains executed test functions and no failed/fatal incidents, without requiring a test slot that is absent from this branch;
2. removes stale XML before each repetition so an earlier report cannot satisfy a later run;
3. retains 20 sequential GeoTagControllerTest repetitions;
4. runs the separate FGC-0029 reproduction shape: 12 QGroundControl processes concurrently for 3 rounds, with a ready/start barrier and unique APPDATA, LOCALAPPDATA, TEMP, TMP, stdout, stderr, and QtTest XML for every process;
5. records per-process PID, start/end time, exit code, timeout, XML execution/failure counts, ASan matches, and round barrier status.

The R26 run completed the instrumented build, ExifParserTest 20/20, GeoTagControllerTest 20/20, and the 12-process × 3-round FGC-0029 matrix 36/36 with no ASan diagnostics. The stress matrix is regression/diagnostic evidence; a pass does not prove the historical FGC-0029 first-invalid-operation/root cause.

## Remaining acceptance

- [x] GPS Version ID destination validation and malformed-entry regression fixtures committed.
- [x] Exact-head Windows Release build, Unit, Integration, executable, and installer gates passed.
- [x] Corrected exact-head MSVC ASan run [43348178961](https://github.com/hujunjie-hub/Flight_GroundControl/actions/runs/43348178961) passed the instrumented build and both 20-run focused suites.
- [x] Corrected 12-process × 3-round FGC-0029 stress matrix completed 36/36 with no ASan findings in run 43348178961.
- [ ] Independent review of the final exact-head evidence.

Issue #37 is VERIFIED and closed on validated code HEAD `d38eaeb6c449ca887fa34bb2eb7ffbddab3b91bb` after the exact-head ASan and Windows Release gates passed. PR #39 remains Open / Draft; FGC-0029 remains a separate OPEN / S1 root-cause investigation. No PR merge is performed by this validation record.


## R27 — 2026-11-13 ASan retest boundary

The exact-head ASan run [43365258406](https://github.com/hujunjie-hub/Flight_GroundControl/actions/runs/43365258406) did not pass: `ExifParserTest` passed 20/20 and `GeoTagControllerTest` passed its first 18 repetitions, but repetition 19 wrote a complete 19-case QtTest XML report (0 failures/errors/skips; 0.952 s) and then the process failed to exit before CTest's 60 s timeout. No ASan memory error signature was found in the captured logs. This is a post-test process-shutdown hang, not a failed QtTest assertion. It resembles FGC-0028; the precise blocking call remains unproven. FGC-0029 remains **OPEN / S1 / BLOCKED**.

The follow-up diagnostic adds QtTest per-function verbosity, map-engine shutdown begin/end and worker stop/wait markers, and preserves the XML report on CTest failure. Its exact-head retest is pending; do not infer a pass from the earlier R26 run.
