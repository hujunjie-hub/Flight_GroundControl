# FGC-0029 — Qt 6.11.1 source-path audit and pre-fix ASan plan

Date: 2026-11-13

## Executive conclusion

The previous working hypothesis that the EXIF timestamp parser itself was querying `QLocale::system()` concurrently is **not established** by the Qt 6.11.1 source path used by this project.

The original FGC-0029 source at reproducer commit `c746af2608de6c51e8632dd75775fef4dac6d97d` calls:

```cpp
QDateTime::fromString(dateTimeDigitized, QStringLiteral("yyyy:MM:dd HH:mm:ss"))
```

The project ASan artifact's `CMakeCache.txt` identifies Qt 6.11.1 (`Qt6_DIR=.../Qt/6.11.1/msvc2022_64/lib/cmake/Qt6`). In Qt 6.11.1:

1. The `QDateTimeParser` constructor initially sets `defaultLocale(QLocale::system())` ([qdatetimeparser_p.h, constructor](https://github.com/qt/qtbase/blob/v6.11.1/src/corelib/time/qdatetimeparser_p.h#L45-L55)).
2. The explicit-format `QDateTime::fromString(const QString&, QStringView, int, QCalendar)` implementation then calls `dt.setDefaultLocale(QLocale::c())` before parsing ([qdatetime.cpp](https://github.com/qt/qtbase/blob/v6.11.1/src/corelib/time/qdatetime.cpp#L6033-L6052)).
3. The two-argument `QStringLiteral` call resolves through the explicit-format overload with the default base year ([qdatetime.h](https://github.com/qt/qtbase/blob/v6.11.1/src/corelib/time/qdatetime.h#L498-L510)).

Therefore the parser's numeric `NumericData` path is switched to the C locale and short-circuits the `ZeroDigit` query that would otherwise occur during numeric parsing. This does **not** mean the overall `QDateTime::fromString()` operation cannot reach system-locale formatting code through another internal path. R19 runtime QueryType instrumentation measured 60 `ZeroDigit` queries on the first `QDateTime::fromString()` call and about 20 on each subsequent call; the ASan stack separately identifies `QLocale::toString(qlonglong)` → `longLongToString` → `zeroDigit` → `systemLocaleString` → `query(ZeroDigit)` above the `QDateTime::fromString` frames. The exact internal caller and the `QLocale` instance carrying system data remain unresolved. The Qt Windows `QSystemLocalePrivate::zero` unsynchronized lazy-cache race therefore remains a strong candidate mechanism, but it must not be confused with the C-locale `NumericData` parsing path or presented as a fully proven first-invalid-write cause.

## What remains valid

- The historical crash was a real `0xC0000374` heap-corruption event.
- The failure was observed during concurrent GeoTagController / EXIF workloads.
- The old call path used `ExifUtility::readString()` and `QDateTime::fromString()`.
- The fixed implementation validates the EXIF entry bounds and parses the fixed ASCII timestamp without the intermediate QString conversion or Qt date-time parser.
- The R26 exact-head MSVC ASan and Windows Release gates passed on code HEAD `d38eaeb6c449ca887fa34bb2eb7ffbddab3b91bb`.
- None of those facts identifies the historical first invalid write/free. FGC-0029 remains **OPEN / S1 / BLOCKED**.

## New diagnostic experiment

A dedicated diagnostic workflow checks out the original reproducer commit `c746af2608de6c51e8632dd75775fef4dac6d97d` and builds that historical source with MSVC `/fsanitize=address` using the same Qt 6.11.1 Windows toolchain.

It will:

1. Verify that the checked-out source is the exact historical reproducer and still contains the old EXIF `QDateTime::fromString` path.
2. Verify ASan instrumentation on all C/C++ translation units and the `QGroundControl.exe` link command.
3. Run `GeoTagControllerTest` 20 times sequentially under ASan.
4. Run the original 12-process × 3-round concurrent controller workload with isolated per-process APPDATA/LOCALAPPDATA/TEMP/TMP directories and a synchronized start barrier.
5. Preserve process exit codes, XML, stdout/stderr and ASan diagnostics in a downloadable workflow artifact.

The historical test class predates `_concurrentControllersExifStressTest`; the stress runner therefore has an explicit `RequireStressCase=false` mode for this baseline only. The default remains `true` for the post-fix workflow.

## Interpretation rules

- An ASan report on the historical baseline is actionable first-fault evidence; record the exact invalid access/free, allocation stack, freeing stack, source location and thread/process.
- A clean ASan baseline is **not** root-cause closure. Sanitizer instrumentation can change timing and suppress the original race.
- Do not infer that the old locale hypothesis is proven by a passing or failing baseline run.
- If ASan does not expose the first fault, the next step is an external low-perturbation capture (UST/ETW or exception-time dump). PageHeap/Application Verifier still requires an administrator-authorized Windows session; do not claim it was run without that authorization.

## Branch and scope

This diagnostic-only branch is based on the validated FGC-0037 code/doc baseline. It changes only the stress runner's optional test-slot requirement and adds this workflow/audit record. It does not change product source and is not intended for merge into the product branch unless the diagnostic tooling is independently useful.
