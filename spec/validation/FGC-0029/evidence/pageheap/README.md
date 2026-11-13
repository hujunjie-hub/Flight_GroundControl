# GFlags PageHeap - NOT EXECUTED

Status: **blocked by environment** (requires administrator rights for the IFEO keys).

Historical note (round 10): PageHeap changed the race timing, so even when it is available it must be
compared against a non-PageHeap baseline. This round used ASan instead, with the interception
capability explicitly calibrated in experiment E-06 (an uninstrumented DLL performing a double free
is still reported by the ASan runtime).
