# Application Verifier - NOT EXECUTED

Status: **blocked by environment**, recorded instead of silently skipped.

- This session runs non-elevated (`elevated=False`) and approval prompts are disabled.
- `appverif.exe` applies checks through `Image File Execution Options` (HKLM), which requires
  administrator rights.
- ASan experiment E-10 confirms a double-free and records allocation / first-free / second-free stacks and thread identities. It does **not** identify the first invalid operation or the operation that first corrupted the reference count. Application Verifier remains NOT_RUN and may provide independent evidence if required.

If a future round needs AppVerifier (Heap, Handles, Locks), run it elevated against the minimal
target `QGroundControl.exe --unittest:GeoTagControllerTest` only, not the whole environment.


Current FGC-0029 interpretation: ASan confirms a historical double-free, not the first invalid operation. Keep Application Verifier status NOT_RUN until an elevated run produces and archives actual evidence.
