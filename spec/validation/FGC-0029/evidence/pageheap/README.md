# GFlags PageHeap — historical diagnostic evidence

Status: **PARTIALLY EXECUTED**. Do not label PageHeap globally as NOT_RUN.

- Historical Release PageHeap run [43444888358](https://github.com/hujunjie-hub/Flight_GroundControl/actions/runs/43444888358) completed the 12-process × 3-round matrix: **36/36 PASS**, zero timeouts, zero `0xC0000374`, zero WER full dumps. Full-size traces for 16–32 byte allocations and UST were verified in the artifact.
- This is a non-reproduction under that historical timing window. It did not capture the first invalid write/free and does not close FGC-0029.
- Follow-up exact-head workflow [43451171747](https://github.com/hujunjie-hub/Flight_GroundControl/actions/runs/43451171747) ended **CANCELLED**. The added Debuggers installation fallback and explicit GFlags output assertion therefore have no successful exact-head evidence.
- Application Verifier and ETW remain separate NOT_RUN diagnostics; their status is not changed by the PageHeap run.

If PageHeap is repeated, record the exact source HEAD, binary hash, GFlags output proving full-size traces, UST state, WER configuration, process matrix, per-process XML/stdout/stderr and archive SHA-256. Compare with a non-PageHeap baseline because instrumentation may change race timing.
