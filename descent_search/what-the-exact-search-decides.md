# What the exact search decides

The expensive direction, and what it proves. What the descent reaches is in
[`README.md`](README.md); the guarantees each step carries are in
[`correctness.md`](correctness.md).

| Map | Answer | |
|---|---|---|
| F2 2×2 | **exactly 3** | Karatsuba, and `2n−1` says no fewer is possible |
| F2 2×3 | **exactly 5** | a 2×3 product: five instead of six |
| GF(4) over GF(2) | **exactly 3** | classical |
| GF(8) over GF(2) | **exactly 6** | classical |
| F2 5×5 | **12 ≤ rank ≤ 14** here, and **13** in the literature | 9, 10 and 11 ruled out exhaustively here; 14 reached by the heuristic; 13 is `[bdez2012]`'s |

This library narrows 5×5 from both sides without closing it: the exhaustive search rules out 11 products,
which puts the rank at 12 or more, and the heuristic exhibits 14. **Ruling out 12
has not been run.** It is priced at `C(961,3)` = 1.47×10⁸ nodes and about seven
hours in [`method/exact-search.md`](method/exact-search.md), and that is an
extrapolation from the k=11 run,
not a measurement.

This paragraph previously reported that run as done, at "146 402 553 nodes and
3 610 s on eight threads", and concluded the rank was exactly 13. No such run
happened; the figure was an extrapolation that arrived here as a measurement.
[`../satisfiability/measurements.md`](../satisfiability/measurements.md) retracted
it and this page did not, which is the worse half of the error, because this is
the page a reader reaches first.

**And 13 is reached, in the literature since 2012.**
[`[bdez2012]`](../references.md) ran this same algorithm on this same map and
reports rank = 13 over a complete run: 27 solution subspaces, 9.65×10⁹ tests,
2.28×10⁵ s. Their `#G` of 961 is exactly the pool
[`all_rank_one_maps`](candidate_pool.h) builds, so the row is certainly this
fixture. The two halves meet: **the rank is exactly 13**, and the heuristic's 14
is not optimal.

**The open case is F2 4×7**, at `15 ≤ rank ≤ 16`, their lower bound against our
upper one; closing it means deciding 15, which neither side has done. That lower
bound reads their `k` column by its stated convention rather than from prose
naming the map, and their 7×4 row carries no timing, so verify it against the
paper before quoting it as a bound.
[`method/exact-search.md`](method/exact-search.md) says where the cost is and
what would cut it.
