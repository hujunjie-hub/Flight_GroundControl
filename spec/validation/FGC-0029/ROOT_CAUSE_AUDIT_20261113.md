# FGC-0029 — Qt 6.11.1 source-path clarification

Date: 2026-11-13

## Confirmed facts

- The historical reproducer commit is `c746af2608de6c51e8632dd75775fef4dac6d97d`.
- The original EXIF code calls `QDateTime::fromString(dateTimeDigitized, QStringLiteral("yyyy:MM:dd HH:mm:ss"))`.
- The R26 ASan artifact's `CMakeCache.txt` confirms Qt 6.11.1.
- Qt 6.11.1's explicit-format overload sets `QDateTimeParser`'s default locale to `QLocale::c()` before parsing. Therefore the `NumericData` parse-side `ZeroDigit` query is short-circuited for this call.
- This does **not** mean the overall `QDateTime::fromString()` operation cannot reach a separate system-locale formatting path. R19 QueryType instrumentation recorded 60 `ZeroDigit` queries on the first call and about 20 on subsequent calls. The ASan stack identifies the formatting path `QLocale::toString(qlonglong)` → `QLocaleData::longLongToString` → `zeroDigit` → `systemLocaleString` → `QSystemLocale::query(ZeroDigit)`, with `QDateTime::fromString` higher in the stack.
- Qt 6.11.1 Windows source shows the shared `QSystemLocalePrivate::zero` cache is lazily read/written without a common synchronization mechanism. This remains a strong candidate mechanism.

## What is not proven

The exact internal caller and the `QLocale` instance carrying system locale data that causes the formatting path during `QDateTime::fromString()` remain unresolved. The first operation that corrupts the QString reference count and the precise worker interleaving are also not directly captured. Do not describe the Qt cache race as a proven first-invalid-write root cause.

The correct distinction is:

| Path | Evidence | Interpretation |
|---|---|---|
| `NumericData` parse path | Qt source | C-locale short-circuit; does not explain the observed system-locale query count |
| `QLocale::toString(qlonglong)` formatting path | ASan stack + Qt source | Directly observed route into unsynchronized `zeroDigit` cache |
| Internal caller/locale instance connecting `fromString()` to formatting path | Not resolved | Remains an open source/stack attribution question |

## Mitigation and current diagnostic

The bounded EXIF timestamp parser removes the `QDateTime::fromString()` call path from QtConcurrent workers and validates the libexif-owned ASCII buffer before reading it. Exact-head R26 validation passed ExifParserTest 20/20, GeoTagControllerTest 20/20, 12-process × 3-round stress 36/36, and Windows Release on code HEAD `d38eaeb6c449ca887fa34bb2eb7ffbddab3b91bb`. This is mitigation/regression evidence, not proof of the historical first invalid operation.

A separate diagnostic branch `diag/fgc0029-pre-fix-asan` rebuilds the historical reproducer commit `c746af2` with MSVC ASan and reruns the 20-run controller suite plus 12-process × 3-round matrix: [workflow 43364448297](https://github.com/hujunjie-hub/Flight_GroundControl/actions/runs/43364448297). A report may provide first-fault evidence; a clean result does not close the issue because sanitizer instrumentation can perturb timing.

**Status: FGC-0029 remains OPEN / S1 / BLOCKED.**


## R30 historical pre-fix differential result — 2026-11-13

The first corrected historical-source differential run [43376096363](https://github.com/hujunjie-hub/Flight_GroundControl/actions/runs/43376096363) completed successfully on diagnostic branch HEAD `26c744a6078d88fbb6e39987a191f8560ab6ab01`. It checked out historical source `c746af2608de6c51e8632dd75775fef4dac6d97d`, injected only the current concurrent-controller test, and verified that the legacy `QDateTime::fromString(dateTimeDigitized, ...)` path remained unchanged.

- Historical `GeoTagControllerTest`: 20/20 sequential runs passed.
- Historical 12-process × 3-round matrix: 36/36 process runs passed.
- All process exit codes were 0; no timeouts, skipped cases or ASan diagnostics.
- Artifact `14015786417` was inspected; every process result was PASS with `asan_finding_count=0`.

**Interpretation:** the historical defect did not reproduce in this ASan-instrumented CI sample. This does not disprove the prior local Release reproductions or the six earlier ASan reports. The sanitizer and test harness alter timing, and a non-reproduction cannot establish the absence of a race.

A higher-sample run [43385432403](https://github.com/hujunjie-hub/Flight_GroundControl/actions/runs/43385432403) is running on diagnostic HEAD `bc0348f5d40024ab2f17caa23e7c34ad5ca8d706`, increasing the historical synchronized matrix from 36 to 120 process runs (12 × 10). Its result is pending.

## R30 current mitigation exact-head acceptance — 2026-11-13

On `fix/fgc-0037-clean` HEAD `73404fc90bbde92ae111d1f5706d7a7f7ef33985`:

- Dedicated MSVC ASan [43380452713](https://github.com/hujunjie-hub/Flight_GroundControl/actions/runs/43380452713): SUCCESS; full compile/link instrumentation; ExifParserTest 20/20; GeoTagControllerTest 20/20; 12 × 3 process matrix 36/36; no ASan finding.
- Windows x64 MSVC Release [43380455799](https://github.com/hujunjie-hub/Flight_GroundControl/actions/runs/43380455799): SUCCESS; Unit 198/198; Integration 70/70; installer creation and installation; installed executable verification; artifact upload.

This validates the current mitigation and regression gates on the exact branch HEAD. It does not directly capture the historical first invalid write/free or establish the precise Qt-side reference-count corruption.

**Status remains OPEN / S1 / BLOCKED.**
