# libexif concurrency probe (FGC-0029)

Standalone Windows x64 Release console program that links **only libexif** (no Qt, no
QGroundControl) and hammers the same parse path `ExifUtility` uses:

```
exif_data_new_from_data -> exif_content_get_entry -> exif_entry_get_value -> exif_data_unref
```

It exists to separate "libexif is not thread-safe" from "the Qt + libexif combination
corrupts memory" when diagnosing [FGC-0029](https://github.com/hujunjie-hub/Flight_GroundControl/issues/29).

## Usage

```powershell
cmake -S tools/validation/libexif-concurrency-probe -B build-probe -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-probe
.\build-probe\exif_probe.exe test/AnalyzeView/GeoTag/DSCN0010.jpg 12 20000 shared
.\build-probe\exif_probe.exe test/AnalyzeView/GeoTag/DSCN0010.jpg 12 20000 perthread
```

`shared` gives every thread the same read-only byte buffer (what QGC does); `perthread`
gives each thread its own copy. Exit code 0 means every parse succeeded and
`HeapValidate(GetProcessHeap())` reported the heap intact; a heap block corruption aborts
the process with 0xC0000374, exactly like the QGC crash.

## Result (2026-11-11, MSVC 19.44, Windows 11 10.0.26300)

12 threads x 20,000 iterations x 2 modes x 3 rounds = **1,440,000 parses, 0 failures,
heap valid**. So libexif on its own did not corrupt the heap; the defect needs the Qt
side (`QDateTime::fromString`) in the same critical section. The probe is kept in the tree
so that conclusion can be re-checked after a libexif upgrade.