# Exchanging operators with PLinOpt

`[plinopt]` is the reference toolchain for exactly these operators, so SMS is
not a convenience here. It is the interface to the established implementation,
and every disagreement about it is a disagreement about whether the two sides can
exchange anything at all. Both directions have been run against its binaries,
built from source and installed nowhere.

What the type letter means, and what is not known about it, is
[`sms_file.h`](../sms_file.h). The `stem_{L,R,P}.sms` naming is his checkers'
calling convention and is explained where the files are written,
[`../../descent_search/commands/minimise_rank_main.cpp`](../../descent_search/commands/minimise_rank_main.cpp).

| | |
|---|---|
| [`ours-through-his-checker.md`](ours-through-his-checker.md) | our operators read by `PMchecker`, which confirms two published counts independently |
| [`his-file-through-ours.md`](his-file-through-ours.md) | his shipped Winograd operator read and sparsified here |
| [`four-false-failures.md`](four-false-failures.md) | four ways to make this layer look broken when it is not |
| [`what-is-checked-automatically.md`](what-is-checked-automatically.md) | the part of the above the test suite carries, and the part it cannot |
