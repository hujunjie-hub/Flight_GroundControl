# FGC-0043 — EXIF typed-reader malformed-entry bounds validation

| Item | Value |
|---|---|
| GitHub Issue | [#43](https://github.com/hujunjie-hub/Flight_GroundControl/issues/43) |
| Severity | S2-MEDIUM / SECURITY |
| Component | `src/Utilities/Parsing/Exif/ExifUtility.cc` |
| Current branch | `main` |
| Current source baseline | `41d8f6d3d89814225a826c9c511d7f715d2c52b6` |
| Issue status | OPEN — implementation present; exact-head MSVC ASan regression pending |

## Finding

Typed EXIF readers must not trust metadata supplied by a malformed `ExifEntry`. In particular, GPS coordinate conversion reads three 8-byte rationals and therefore must reject a wrong format, fewer than three components, a null data pointer, or a declared payload smaller than three rationals before touching the payload.

Scalar SHORT and RATIONAL readers must likewise validate format, component count, backing data, and minimum byte size. Missing IFD content must be treated as malformed input rather than dereferenced.

This finding is a defensive parser/API hardening issue. It is **not** a demonstrated root cause for FGC-0029's historical heap corruption.

## Implementation already present on main

The defensive implementation and regression fixture were committed before the current test takeover:

- `f1c709edf89d140c0f9480c8c4d34fc72e2e9eb5`: typed-reader format/component/data/size validation.
- `1d11678c51efa9a5ae3af51ff5d3233cfb093cab` and `0bf2d9723b5a48029e1143306596f12295808a2f`: null payload and missing IFD guards/fixtures.
- `2ae61c96d076b2be4037f4917413e927b1d15543` and `d7bce312981bdae9bec949d1d98aab31b5cad7e5`: MSVC test-fixture correction and formatting.

The regression slot `ExifUtilityTest::_testTypedReadersRejectMalformedEntries` covers:

- a 3-component SHORT entry presented to the GPS RATIONAL reader;
- a declared undersized RATIONAL payload;
- a non-null entry with null backing data;
- a missing GPS IFD;
- wrong-format SHORT and RATIONAL scalar entries.

## Existing Windows Release evidence

Windows Release run [43567619588](https://github.com/hujunjie-hub/Flight_GroundControl/actions/runs/43567619588) passed on exact HEAD `37035a38d8fcdecca6412b3696704d6416aa3600`. Its Unit JUnit report includes `ExifUtilityTest` as passed. The source blobs for both `ExifUtility.cc` and `ExifUtilityTest.cc` are identical between that validated HEAD and current main `41d8f6d3d89814225a826c9c511d7f715d2c52b6`.

This is Windows Release/Unit evidence, not sanitizer evidence.

## MSVC ASan acceptance

The manual workflow `.github/workflows/fgc0037-windows-asan.yml` is extended to:

1. require `test/Utilities/Parsing/Exif/ExifUtilityTest.cc` in the ASan compile database;
2. confirm CTest registers `ExifUtilityTest` under the Unit label and dispatches `--unittest:ExifUtilityTest`;
3. execute `ExifUtilityTest` 20 times with the MSVC `/fsanitize=address` instrumented binary;
4. parse each fresh QtTest XML report, reject failures/errors/skips, and require `_testTypedReadersRejectMalformedEntries` to have actually executed;
5. preserve per-run logs and XML, and reject AddressSanitizer diagnostics.

The ASan run is manual and must be launched against the exact main HEAD after this workflow change is committed. Until that exact-head run succeeds, the ASan acceptance item remains NOT_RUN and this issue remains OPEN.

## Closure criteria

- [x] Defensive checks for type, component count, data pointer, and byte size are present.
- [x] Missing IFD and malformed-entry regression cases are present.
- [x] Exact-head Windows Release/Unit evidence exists and the relevant source/test files are unchanged since that evidence.
- [ ] Exact-head MSVC ASan build includes `ExifUtility.cc` and `ExifUtilityTest.cc`.
- [ ] `_testTypedReadersRejectMalformedEntries` executes in all 20 ASan repetitions with zero skipped/failures/errors.
- [ ] ASan logs contain no memory-safety diagnostics.
- [ ] Independent review of exact-head sanitizer evidence.

Do not close this issue by reusing FGC-0037 evidence or by treating the normal Unit result as an ASan pass.
