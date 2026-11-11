# ETW - NOT EXECUTED

Status: **blocked by environment**. `wpr` / `wpr -profiles` requires administrator rights; round 16
already confirmed this and this round does not claim ETW was completed.

Justification for not retrying: the ASan evidence (E-10) already answers the question ETW was meant
to answer (which operation first corrupted the heap, and for which object), and in more detail:
it provides the allocation stack, the first-free stack with the **thread that performed it**, and the
second-free stack, plus the exact 20-byte object identity.

If ETW is required later, start from a real `wpr -profiles` listing on the target machine and record
ETL + CSV + summary + lost-event counts, and only then mark it complete.
