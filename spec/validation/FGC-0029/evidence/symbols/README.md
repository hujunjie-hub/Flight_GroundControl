# FGC-0029 — Qt symbol and function-boundary evidence

- Qt 6.11.1 Windows kit does not include private `Qt6Core.pdb`; public exports alone are insufficient to name every internal frame.
- The R20 analysis parses the PE `.pdata` exception directory to identify function ranges (13,642 functions; 7,290 exported starts). An internal function is named only when its range and disassembly/source mapping support the name; otherwise retain the RVA/range as an unnamed internal function.
- The analysis explicitly rejects the earlier “nearest preceding export” shortcut for internal frames.

| Qt6Core RVA / range | Current interpretation | Confidence / boundary |
|---|---|---|
| `[0x26C840,0x26C94B)` | `QSystemLocalePrivate::zeroDigit()` candidate | High source/disassembly correspondence; candidate mechanism, not proof of first invalid operation |
| `[0x16D700,0x16D8FB)` | `systemLocaleString()` candidate | High source/disassembly correspondence |
| `[0x268210,0x268D18)` | `QSystemLocale::query` | Exported function range |
| `[0x170770,0x1707B2)` | `QLocale::toString(qlonglong)` | Exported function start matches range |
| `0x111F12` | `QVariant::~QVariant+0x42` | Exported destructor frame |

The ASan reports establish the double-free of a 20-byte QString block and identify the allocation/first-free/second-free stacks. They do not establish the first reference-count corruption operation or exact thread interleaving. Do not revert to the old “nearest export” labels in superseded R17/R18 notes.
