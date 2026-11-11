# Symbol evidence

- Product PDB: `build-win-release/Release/QGroundControl.pdb`
  (pre-fix SHA256 recorded in `work/validation/fgc0029-r17/baseline-binaries/baseline-sha256.txt`).
- Qt 6.11.1 ships **no private PDB** (only export symbols). Qt-internal frames in the ASan report
  and in the dumps are therefore resolved to the nearest preceding exported symbol; every report in
  this round states explicitly which offsets are internal functions rather than pretending they are
  the named export.
- Public symbol cache used by cdb: `work/setup/symbols` (Microsoft public symbol server),
  which is what made `!heap -i` work this round (round 16 failed only because ntdll symbols were
  missing, not because of Segment Heap).

Nearest-symbol mapping used in `ROOT_CAUSE_ANALYSIS.md`:

| Qt6Core RVA | Nearest export | Interpretation |
|---|---|---|
| `+0x1c236d` | `QArrayData::allocate` | heap allocation of the QString payload |
| `+0x17f3cd` | `QString::QString` | QString construction |
| `+0x26c89e` / `+0x26c8f8` | `QSystemLocale::query` neighbourhood | system locale query / release |
| `+0x2686a2` | `QSystemLocale::query+0x492` | system locale query |
| `+0x16d7b1` | **`QLocale::system`** | cached system locale update |
| `+0x1707a8` | `QDateTimeParser::locale` neighbourhood | parser default locale |
