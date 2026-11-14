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


## R31 PageHeap Release diagnostic result — 2026-11-13

The exact historical production reproducer `c746af2608de6c51e8632dd75775fef4dac6d97d` was rebuilt as MSVC x64 Release with Qt 6.11.1 and PDB symbols. The historical EXIF `QDateTime::fromString()` call was preserved; only the current `_concurrentControllersExifStressTest` was injected into the historical test source from `119edd4af97af4e159050650317fd945e4e03ecc`.

GFlags confirmed `QGroundControl.exe: page heap enabled with flags (full size traces)` for the configured 16–32-byte allocation range, which covers the historical 20-byte block. UST was enabled and WER full dumps were configured. Run `43444888358` completed 12 isolated processes × 3 synchronized rounds: 36/36 PASS, all 36 XML reports had 19 tests with the required concurrency case executed, zero failures/errors/skips/timeouts, zero `0xC0000374` exits, and zero WER dumps. Artifact `14049871972` was inspected, including all process CSV rows and XML reports.

**Interpretation:** the historical Release/PageHeap workload did not reproduce. This is a valid non-reproduction under one CI timing window, not proof that the defect is absent and not first-invalid-operation evidence. FGC-0029 remains **OPEN / S1 / BLOCKED**.

A follow-up workflow revision on main `1d117516f0744ec26126b2e23138c238caf1d101` and diagnostic branch `cf676641b062a062e1d68e48646d79b7accb49ea` adds a Windows Desktop Debuggers installation fallback and validates the actual `full size traces` GFlags output. Exact-head run `43451171747` ended **CANCELLED**; the added installation fallback and explicit GFlags output assertion have no successful exact-head evidence. Do not treat cancellation as PASS or FAIL.


## R32 状态更正 — 2026-11-14

- main 审查基线：`77563b118f8757c12e091ce9427ea369e0b9638d`。
- 历史 Release PageHeap run `43444888358` 已成功执行 36/36，0 个 `0xC0000374`，但属于非复现结果，不是首次非法操作证据。
- 后续 PageHeap workflow run `43451171747` 的最终状态为 **CANCELLED**。此前的 pending 表述已过期；该轮新增安装兜底与 GFlags 输出断言没有取得 exact-head 通过证据。
- 当前 main 完整 Windows Release run `43571983530` 在本轮检查时仍为 IN_PROGRESS；上一完整成功 run `43567619588` 只适用于 HEAD `37035a3`。
- Issue #29 保持 OPEN / S1 / BLOCKED。ASan 证明双重释放，不证明首次破坏引用计数的操作。
