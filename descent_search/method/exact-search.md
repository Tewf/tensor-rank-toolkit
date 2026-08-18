# The exact search

A different question: not "can this be improved" but "is there one with exactly
`k` products". Complete, and exponential.

```
expand_subspace(W, pool, from, k):
    if dim W > k: fail
    if dim W == k:
        within <- the rank-one maps of pool inside span(W), taken independent
        succeed with `within` if there are k of them, else fail
    for each p in pool[from...] not already in span(W):
        expand_subspace(W + {p}, pool, index+1, k)
```

Success means `span(W)` has a basis made entirely of rank-one maps, and a
rank-one basis **is** an algorithm. Sweeping `k` upward gives the fewest
products; bisecting gives the same answer under the assumption that a
`k`-product solution implies a `k+1`-product one, which is why both are kept and
tested against each other.

| | |
|---|---|
| Time | `O(C(\|pool\|, k − dim T) · \|pool\| · d · w)`, one pool scan per leaf |
| Space | `Θ((k − dim T) · d · w)`, the recursion depth times a basis |

**Where the cost actually is.** Essentially every node is a leaf, and every leaf
scans the whole pool testing membership at `Θ(d·w)` each. For F2 5×5 at `k = 11`
that is 459 239 leaves × ~950 tests × ~275 field operations ≈ 10¹¹, and it
measured 77 seconds, which agrees.

Carrying each pool element's reduction down the tree instead of recomputing it
at every leaf would cut this by roughly the depth. Two cheaper guesses were
tried first (hoisting the span rebuild, and removing an allocation per test)
and bought 8% between them. The measurement is the reason to believe the third
idea and not the first two.

## What `W` is decides what the answer means

The search never leaves `span(W)` behind: it only ever *adds* to it. So the
answer is the fewest products **among decompositions containing `W`**, and the
starting subspace is part of the claim rather than a detail of the run.

| `--anchor` | `W` starts as | The answer means |
|---|---|---|
| `map` (default) | the map's own slices | the true minimum, since every algorithm for `T` generates `T` |
| `heuristic` | the heuristic's result | the minimum among algorithms containing *that* subspace |

The algorithm can anchor at either the map itself or a heuristic's result.
Anchoring at the map gives the true minimum; anchoring at a heuristic result
gives a conditional minimum relative to that subspace. All results below were
run anchoring at the map, which costs `C(|pool|, k − dim T)` nodes.

| Map | Question | Nodes | Time |
|---|---|---|---|
| F2 2×2 | fewest | 1 | 7 µs |
| F2 2×3 | fewest | 3 | 20 µs |
| GF(8) | fewest | 1 606 | 4.7 ms |
| F2 5×5 | is there a 10? **no** | 959 | 0.17 s |
| F2 5×5 | is there an 11? **no** | 459 239 | 77 s |
| F2 5×5 | is there a 12? | `C(961,3)` = 1.47×10⁸ | ~7 h, extrapolated |

The last row is an extrapolation from the row above it, at 5 900 nodes per
second, rather than a statement that it cannot be done. It is not, however, the
thing to spend the time on. [`[bdez2012]`](../../references.md) decided F2 5×5 in
2012 and its rank is **13**, over a complete run of 9.65×10⁹ tests, so seven
hours at `k` = 12 would reproduce their exclusion as a check and settle nothing
open. The genuinely open fixture is **F2 4×7**, at `15 ≤ rank ≤ 16`, where
closing the gap means deciding `k` = 15. Their 7×4 row reads by the stated
convention for the `k` column and carries no timing, so verify it against the
paper before treating the 15 as established.

Every row but the last is asserted and run in CI: the two Karatsuba answers and
both exclusions in
[`exhaustive_search/tests/test_exhaustive_search.cpp`](../../exhaustive_search/tests/test_exhaustive_search.cpp), with the
11 as its own `slow`-labelled test, and GF(8)'s 6 in
[`map_construction/tests/test_map_construction.cpp`](../../map_construction/tests/test_map_construction.cpp) beside the
tensor it is built from.
