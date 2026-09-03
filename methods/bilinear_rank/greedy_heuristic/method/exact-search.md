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
| F2 5×5 | is there a 12? **no** | 146 402 553 | 535.59 s |

`decide-rank fixtures/f2_2x2.tensor` reproduces the first row; this run's own
printed output was

    fixtures/f2_2x2.tensor
      rank bound: rank is at least 3
      pool: 9 rank-one maps of shape 2x2
      leaf: GF(2), one bit per entry
      plan:
        pool: materialised (9 maps at 88 B each is 792 B, inside the 4.00 GiB budget)
        leaf route: auto (a sweep tests leaves of many dimensions, so each takes the cheaper by size)
        device: cpu (9 elements at the deepest leaf, under the 8192 launch floor)
        threads: 1
        quotient: none
        orbit test: full
        anchor: map
      1 nodes in 1.7802e-05 s
      FOUND: 3 products, rank bound 3, gap 0
      verified: they compute the map

which agrees on the node count and the answer; the microseconds are this run's
own, not the table's 7 µs, since two different runs measure two different
moments rather than one number twice.

The last row was an extrapolation, `C(961,3)` = 1.47×10⁸ nodes priced at seven
hours from the k = 11 rate, until the run itself on 2026-08-19: the predicted
node count was right to within half a percent, and the hours were wrong by an
order of magnitude because the GF(2) leaf did not exist when the rate was
taken. The full account, including the retraction it settles, is
[`../what-the-exact-search-decides.md`](../what-the-exact-search-decides.md).
[`[bdez2012]`](../../../../references.md) had already decided F2 5×5 in 2012 over a
complete run of 9.65×10⁹ tests, and its rank is **13**, so the run reproduces
their exclusion as a check and settles nothing open. The genuinely open fixture is **F2 4×7**, at `15 ≤ rank ≤ 16`, where
closing the gap means deciding `k` = 15. Their 7×4 row reads by the stated
convention for the `k` column and carries no timing, so verify it against the
paper before treating the 15 as established.

Every row but the last is asserted and run in CI: the two Karatsuba answers and
both exclusions in
[`methods/bilinear_rank/exhaustive/tests/test_exhaustive_search.cpp`](../../exhaustive/tests/test_exhaustive_search.cpp), with the
11 as its own `slow`-labelled test, and GF(8)'s 6 in
[`methods/bilinear_rank/map_construction/tests/test_map_construction.cpp`](../../map_construction/tests/test_map_construction.cpp) beside the
tensor it is built from.
