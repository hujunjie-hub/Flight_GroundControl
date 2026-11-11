# Crash dump evidence

All dumps live in `work/validation/dumps/` (not committed to Git; shipped in the ZIP).

| Dump | Origin |
|---|---|
| `QGroundControl.exe.12912.dmp` | **R17** pre-fix reproduction, round 2 instance 1 |
| `QGroundControl.exe.27796.dmp` | **R17** pre-fix reproduction, round 3 instance 1 |
| `QGroundControl.exe.32020.dmp` | R16 (analysed there, `!heap -x` was blocked by missing ntdll symbols) |
| `QGroundControl.exe.23088.dmp`, `32356`, `13660` | R8-R15 historical dumps |

Analysis logs produced this round:

- `work/validation/fgc0029-r17/dump-12912-analysis.log`
- `work/validation/fgc0029-r17/dump-27796-analysis.log`
- `work/validation/fgc0029-r17/dump32020-heapx2.log`  (`!heap -i` with public symbols, shows the
  failing block is a well-formed **LFH free block**: `Block flags: 0x0 LFH (free)`, 0x40 bytes)
- `work/validation/fgc0029-r17/threads-*.log`  (`~*k` for every thread of every dump)

All dumps share the same signature:

```
EXCEPTION_CODE_STR:  c0000374
SYMBOL_NAME:         ucrtbase!free_base+0
FAILURE_BUCKET_ID:   HEAP_CORRUPTION_ACTIONABLE_BlockNotBusy_DOUBLE_FREE_c0000374_ucrtbase.dll!free_base
```

and the faulting thread is always a `QtConcurrent` pool thread inside Qt date/time parsing,
reached from `ExifParser::readTime`.
