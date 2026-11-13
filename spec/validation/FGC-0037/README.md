# FGC-0037 — GeoTag GPS Version ID malformed-entry write guard

| Item | Value |
|---|---|
| GitHub Issue | [#37](https://github.com/hujunjie-hub/Flight_GroundControl/issues/37) |
| Severity | S2-HIGH / SECURITY |
| Component | **ExifParser::write()** |
| Active PR | [#39](https://github.com/hujunjie-hub/Flight_GroundControl/pull/39), Open / Draft |
| Branch | **fix/fgc-0037-clean** |
| Current validated code HEAD | **9b22bab756139bd46174515dfb724c0e0120bfdd** |
| Issue status | **FIXED_WAITING_RETEST** |

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

This is Windows Release evidence only; it does not replace the separate MSVC AddressSanitizer diagnostic gate.

## ASan workflow defect and correction

Run [43275152519](https://github.com/hujunjie-hub/Flight_GroundControl/actions/runs/43275152519) stopped after the first GeoTagControllerTest CTest execution because the workflow required the XML to contain **_concurrentControllersExifStressTest**. That test slot is not present in the current clean FGC-0037 branch. The CTest target itself passed 1/1 before the stale assertion failed. The preceding ExifParserTest repeated matrix passed 20/20. No AddressSanitizer memory diagnostic was emitted before the workflow stopped, but this is **not** an ASan pass: the 20-run controller loop did not complete.

The corrected workflow:

1. validates that each QtTest XML contains executed test functions and no failed/fatal incidents, without requiring a test slot that is absent from this branch;
2. removes stale XML before each repetition so an earlier report cannot satisfy a later run;
3. retains 20 sequential GeoTagControllerTest repetitions;
4. runs the separate FGC-0029 reproduction shape: 12 QGroundControl processes concurrently for 3 rounds, with a ready/start barrier and unique APPDATA, LOCALAPPDATA, TEMP, TMP, stdout, stderr, and QtTest XML for every process;
5. records per-process PID, start/end time, exit code, timeout, XML execution/failure counts, ASan matches, and round barrier status.

The stress matrix is regression/diagnostic evidence. A pass does not prove the historical FGC-0029 first-invalid-operation/root cause.

## Remaining acceptance

- [x] GPS Version ID destination validation and malformed-entry regression fixtures committed.
- [x] Exact-head Windows Release build, Unit, Integration, executable, and installer gates passed.
- [ ] Corrected exact-head MSVC ASan run passes the instrumented build and both 20-run focused suites.
- [ ] Corrected 12-process × 3-round FGC-0029 stress matrix completes with all processes passing and no ASan findings.
- [ ] Independent review of the final exact-head evidence.

Keep Issue #37 Open / FIXED_WAITING_RETEST until the new exact-head ASan run completes successfully. Keep PR #39 Open / Draft; do not merge it as part of this validation work.
