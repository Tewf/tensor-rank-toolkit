# Bringing an algorithm in, and sending one out

**Nothing in this folder is a dependency.** No tool named here is required to
build, test or run anything in this repository: there is no entry for one in any
`CMakeLists.txt`, nothing links against one, and the whole suite passes on a
machine where none of them is installed. What is here is one job, in one
direction each way: **given a file somebody else published, produce the file this
repository reads**, and given ours, produce one they can read back.

**SMS is the format the field publishes a bilinear algorithm in.** A `⟨L, R, P⟩`
triple in SMS is what catalogues and libraries hand out, so it is the way in and
the way out. Two places publish it in quantity: the
[FMM catalogue](https://fmm.univ-lille.fr/), which lists thousands of
decompositions by rank, and [PLinOpt](https://github.com/jgdumas/plinopt), the
C++ library for linear and bilinear straight-line programs by Dumas, Grenet,
Pernet and Sedoglavic, whose `data/` ships operators for Strassen, Winograd,
Karatsuba, Toom-3 and matrix multiplication up to 32x32x32.

Reading either is a test rather than a claim: an operator triple published
elsewhere rebuilds the fixture this repository writes from the definition of the
map, entry for entry, and a disagreement would be ours to explain.

What the type letter means, and what is measured about it, is
[`sms_file.h`](../sms_file.h). The `stem_{L,R,P}.sms` naming is the convention
the external checkers expect, and it is explained where the files are written,
[`../../descent_search/commands/minimise_rank_main.cpp`](../../methods/bilinear_rank/greedy_heuristic/commands/minimise_rank_main.cpp),
and read, [`../../descent_search/commands/operators_to_tensor_main.cpp`](../../methods/bilinear_rank/commands/operators_to_tensor_main.cpp).

| | |
|---|---|
| [**`exchanging-files.md`**](exchanging-files.md) | **start here**: what to install, both directions in four lines, the command that checks our output with an external tool, and the four differences that bite |
| [`where-the-conventions-differ.md`](where-the-conventions-differ.md) | the field-by-field comparison, with the file and line on both sides, and what round-tripped across all 153 published matrices |
| [`checking-ours-with-another-tool.md`](checking-ours-with-another-tool.md) | our operators read by `PMchecker`, which confirms two published counts independently |
| [`bringing-an-algorithm-in.md`](bringing-an-algorithm-in.md) | a published Winograd operator read and sparsified here |
| [`four-false-failures.md`](four-false-failures.md) | four ways to make this layer look broken when it is not |
| [`what-is-checked-automatically.md`](what-is-checked-automatically.md) | the part of the above the test suite carries, and the part it cannot |
