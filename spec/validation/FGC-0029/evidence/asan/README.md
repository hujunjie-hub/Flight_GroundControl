# ASan evidence

| Artefact | Location |
|---|---|
| Configure log | `work/validation/fgc0029-r17/asan-configure.log` |
| Full build log | `work/validation/fgc0029-r17/asan-build.log` |
| Instrumentation proof | `build-win-asan-geotag/compile_commands.json` (2221 of 2222 compile units carry `/fsanitize=address`; the only exception is `QGroundControl_resource.rc`) |
| Pre-fix runs | `work/validation/fgc0029-r17/asan-run1/` (0/120), `asan-run2/` (2/240) |
| **Double-free report #1** | `work/validation/fgc0029-r17/asan-run2/r11-i15.err` |
| **Double-free report #2** | `work/validation/fgc0029-r17/asan-run2/r13-i10.err` |
| Post-fix run (same matrix) | `work/validation/fgc0029-r17/asan-postfix/` (0/240) |
| Differential probe | `tools/validation/fgc0029-asan-probe/`, runs in `asan-runs-qt/`, `asan-probe-prelim.log` |
| Interception calibration | `work/bench/asan-interpose/` (uninstrumented DLL double free is caught) |

ASan runtime used: `clang_rt.asan_dynamic-x86_64.dll` from MSVC 14.44
(`VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64`).

Coverage limits (documented honestly): Qt 6.11.1 ships as pre-built DLLs and is **not** instrumented,
so a buffer overflow *inside* Qt would not be detected by instrumentation; however, experiment E-06
proves the ASan runtime still intercepts `malloc`/`free` for uninstrumented modules, so
double free / invalid free inside Qt **are** detected. libexif and all QGC sources of interest
(`ExifParser.cc`, `ExifUtility.cc`, `GeoTagController.cc`, `GeoTagControllerTest.cc`) are instrumented.
