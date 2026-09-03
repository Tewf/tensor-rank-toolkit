# Do the two counters count the same event?

They do. This is the reading that established it, the figure it corrected on
[`comparing-against-the-baseline.md`](comparing-against-the-baseline.md), and the
27 nodes that survive.

Taken by compiling the baseline clone and calling `CPD_DFS.search` directly, and
by `decide-rank fixtures/matmul_2x2x2.tensor --target 6` on this side. Neither
number is a timing, so neither depends on the machine.

**They count the same event.** `work[partial_tups.size()]++` is the
first statement of `dfs` in `CPD_DFS.java`, once per invocation, bucketed by
depth and never touched again; `try_consume_node` is the first budget action of
`expand_subspace_impl` here, behind only the shared-witness early return. Both count the root, both exclude the linear algebra
inside a node, and both walk strictly increasing subsets of the same 225 rank-one
4x4 maps over F2. The depths coincide structurally rather than by luck: theirs is
`R - n0` and ours is `target - dim span(T)`, the same number for a concise tensor.

**Closing it corrected a figure.** `work` is an array, and the 25 200 published
for R = 6 was its **deepest element**, not its sum: it is
exactly `C(225, 2)`, the level-2 count, dropping the root and the 225 nodes above
it. The R = 7 row was the sum. Re-run, the arrays are
`[1, 225, 25200]` and `[1, 1, 54, 10536]`, so the totals are **25 426** and
10 592, and only one of the two rows had ever been a total.

**What is left is 27 nodes, and they are one line.** 25 426 against 25 399 is
0.106%, and the whole of the difference is
[`exhaustive_search.cpp`](../methods/bilinear_rank/exhaustive/exhaustive_search.cpp)'s
`if (span.contains(map)) continue`, which declines to spend a node on a candidate
already inside the span. The Java loop has no such test and recurses on every
candidate, so at level 2 it visits all `C(225, 2)` pairs where this visits
25 173.

**The find rows do not compare**, and should not be read as though they did:
10 592 against 7 436 is two searches stopping in different places in different
orders, and `dfs` runs its base case at *every* node where the leaf test here
runs only at full dimension.
