# Application Verifier - NOT EXECUTED

Status: **blocked by environment**, recorded instead of silently skipped.

- This session runs non-elevated (`elevated=False`) and approval prompts are disabled.
- `appverif.exe` applies checks through `Image File Execution Options` (HKLM), which requires
  administrator rights.
- Not on the critical path any more: experiment E-10 (ASan) already produced the first illegal
  operation with allocation / first-free / second-free stacks and thread identities.

If a future round needs AppVerifier (Heap, Handles, Locks), run it elevated against the minimal
target `QGroundControl.exe --unittest:GeoTagControllerTest` only, not the whole environment.
