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
