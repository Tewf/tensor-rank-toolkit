# The baseline prunes nothing, so its tree is a formula

Which is why the comparison could be taken past `⟨2,2,2⟩` at all: three of the
six questions are trees the baseline cannot walk to the end, and it did not have
to. Where the numbers sit beside this search's:
[`comparing-against-the-baseline.md`](comparing-against-the-baseline.md).

**`dfs` prunes nothing in its recursion.** It recurses on every strictly
increasing tuple, and the only pruning lives in `dfs_base`, which is called *at* a
node and never suppresses one. So a refutation's work array is exactly

    work[i] = C(P, i)   for i = 0 .. R − n0,   P = (2^n1 − 1)(2^n2 − 1)

with the axis lengths taken after `search()`'s own `reduce()` and longest-first
sort. Confirmed against four measured refutations, exactly, including the
`⟨2,2,2⟩` row above: `[1, 225, 25200]`, `[1, 961, 461280]` twice, and
`[1, 225, 25200, 1873200]`.

**That is what made the comparison extendable at all**, because three of the six
questions below the baseline cannot finish. Given twenty minutes it reached 1.7%,
10.0% and 2.9% of the three largest trees.

| question | `k` | baseline | `decide-rank` | baseline is |
|---|---|---|---|---|
| `matmul_2x2x2` | 6 | 25 426 | 25 399 | +0.11% |
| `gf16_multiplication` | 7 | 1 898 626 | 1 897 576 | +0.06% |
| `cyclic_f2_5` | 7 | 462 242 | 461 251 | +0.21% |
| `gf16_multiplication` | 8 | 105 861 226 | 105 600 301 | +0.25% |
| `f2_5x5` | 11 | 462 242 | 459 239 | +0.65% |
| `f2_5x5` | 12 | 147 918 082 | 146 402 553 | **+1.04%** |

**Six questions, one direction, and the same line accounts for all of it.** The
baseline is above this search every time, by 0.06% to 1.04%, and the gap is
`if (span.contains(map)) continue`: a candidate already inside the span cannot
raise the dimension, so no node is spent on it, and the Java loop has no such
test. The agreement first seen at `⟨2,2,2⟩` was not a coincidence of one shape.

`cyclic_f2_5` at 8 is the odd row and is left out above: this search answers it
from the flattening floor without opening a node, where the baseline has no such
bound and would walk 147 918 082. Its rank is unsettled at `9 ≤ rank ≤ 10`, so
`k = 9` is the question where both would walk a tree, and it is out of reach for
both.
