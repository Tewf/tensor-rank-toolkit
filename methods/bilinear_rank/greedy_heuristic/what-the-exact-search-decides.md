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
| F2 5×5 | **exactly 13**, both sides here | 9 to 12 ruled out exhaustively here; 13 exhibited by [`lower-the-bound`](../branch_and_bound/README.md) in 80 nodes. `[bdez2012]` report the same 13 |

This library closes 5×5 from both sides: the exhaustive search rules out 12
products, which puts the rank at 13 or more, and the incumbent search exhibits 13.
It was `13 ≤ rank ≤ 14` until 2026-08-21, the descent reaching only 14 and the 13
being `[bdez2012]`'s. **Nothing on this page moved**; what closed the map is a
construction, which this search cannot make and does not have to.

**Ruling out 12 has now been run**, 2026-08-19, on a quiet machine:
`decide-rank evidence/fixtures/f2_5x5.tensor --target 12 --node-limit 300000000 --threads 6`
returns **NO exhaustively in 146 402 553 nodes and 535.59 s**. A refutation's node
count does not depend on the thread count, measured in
[`../exhaustive_search/what-threads-change.md`](../exhaustive/what-threads-change.md),
so the node figure is the tree's and not this run's.

**What that costs is now a tenth of what this page priced it at, and the reason is
the leaf, not the tree.** Seven hours came from extrapolating the k=11 run on the
general field path, 77.88 s over 459 239 nodes, or 169 µs a node. The same run on
the GF(2) leaf is 7.69 s, or **16.7 µs a node**, and 1.47×10⁸ nodes at that rate is
41 minutes on one core. Six cores took 8 m 56 s.

**The retracted figure's node count was exactly right.** This paragraph previously
reported this run as done, at "146 402 553 nodes and 3 610 s on eight threads", and
[`../satisfiability/measurements.md`](../../satisfiability/measurements.md) retracted
it because no such run had happened. The measured node count is **146 402 553**, the
same number to the digit, which says the count was derived correctly and only the
seconds were invented. The measured 535.59 s is 6.7x faster than the invented
3 610 s, because the GF(2) leaf did not exist when that number was written. A
derivation presented as a measurement is still not a measurement, and it stays
retracted; it was simply a good derivation.

**And 13 is reached, in the literature since 2012.**
[`[bdez2012]`](../../../references.md) ran this same algorithm on this same map and
reports rank = 13 over a complete run: 27 solution subspaces, 9.65×10⁹ tests,
2.28×10⁵ s. Their `#G` of 961 is exactly the pool
[`all_rank_one_maps`](../candidate_pool.h) builds, so the row is certainly this
fixture. The two halves meet: **the rank is exactly 13**, and the heuristic's 14
is not optimal.

**The open case is F2 4×7**, at `15 ≤ rank ≤ 16`, their lower bound against our
upper one; closing it means deciding 15, which neither side has done. That lower
bound reads their `k` column by its stated convention rather than from prose
naming the map, and their 7×4 row carries no timing, so verify it against the
paper before quoting it as a bound.
[`method/exact-search.md`](method/exact-search.md) says where the cost is and
what would cut it.
