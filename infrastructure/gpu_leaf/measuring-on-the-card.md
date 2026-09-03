# Measuring a number taken on the card

[`../MEASURING.md`](../../MEASURING.md) is the protocol for every timing in this
repository, and none of it transfers to a kernel unexamined. Four things differ
and each is a rule, with a fifth that is not about timing at all.

**The baseline is the path the kernel would replace, on this machine, today.**
Not a figure from an older document and not the general Givaro path when the
packed one is what ships. Timing the leaf kernel against 785 ns rather than the
940.2 ns the leaf actually cost would have flattered the card by up to 23x,
which is recorded in [`what-the-card-did.md`](what-the-card-did.md)
because it was nearly done.

**A host baseline is quoted at one core and at twelve, both.** One core is the
comparison the rest of this file makes; twelve is the comparison a reader with
the same laptop will make anyway, and a speedup that survives only against one
core is a thread count reported as an architecture.

**The card is warmed and the clock is the kernel's.** The first launch pays
context creation and JIT, so it is discarded. What is timed is the kernel, with
allocation and any transfer stated separately, because a kernel that needs its
input shipped is a different claim from one that derives it from an index.

**A GPU number is not reproducible by the Containerfile and does not pretend to
be.** The image has no CUDA and skips `infrastructure/gpu_leaf/` entirely, so the 95 tests it
runs are the ones a machine without a card has. A card number is reproduced on
comparable hardware or not at all, and the model is published beside it: an RTX
4060 Laptop, 8 GB.

**Correctness is checked against the CPU, never assumed from the timing.** The
survivor sets are compared element for element with the shipped leaf's on every
question measured. A kernel that is fast and wrong is the failure this guards,
and it does not announce itself in a table of seconds.

