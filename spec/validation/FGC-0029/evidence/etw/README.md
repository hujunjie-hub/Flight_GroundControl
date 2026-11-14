# ETW - NOT EXECUTED

Status: **blocked by environment**. `wpr` / `wpr -profiles` requires administrator rights; round 16
already confirmed this and this round does not claim ETW was completed.

ASan E-10 provides allocation, first-free and second-free stacks, the participating thread IDs and the 20-byte object identity. It confirms a double-free, but does **not** answer which earlier operation first corrupted the heap or the reference count. ETW remains NOT_RUN; the root-cause question is still open.

If ETW is required later, start from a real `wpr -profiles` listing on the target machine and record
ETL + CSV + summary + lost-event counts, and only then mark it complete.


Current status remains NOT_RUN. Do not describe ASan as having answered the first-corruption question. If ETW is later run, retain the ETL, provider/session configuration, lost-event counts and correlation with the exact tested binary/HEAD.
