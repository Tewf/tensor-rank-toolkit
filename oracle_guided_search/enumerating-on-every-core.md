# Enumerating on every core

`enumerate_solution_subspaces` is the safest thing in this repository to spread
over cores, and the reason is that **it counts rather than stops**. There is no
witness to race to, no early exit and no shared budget, so every subtree is
visited whatever the thread count. Every number `EnumerationReport` carries is
then a sum of the branches, or for `distinct` the size of their union, so
`emitted`, `distinct`, `nodes` and `group_visits` are the same at one worker and at
twelve. All four are asserted equal at 1, 2, 4, 6 and 12 workers in
[`tests/test_canonical_augmentation.cpp`](tests/test_canonical_augmentation.cpp),
which also pins the 1 890 601 and 954 node totals of
[`deduplication-cost.md`](deduplication-cost.md) so neither can move unnoticed.
Contrast
[`../exhaustive_search/what-threads-change.md`](../exhaustive_search/what-threads-change.md),
where a shared budget makes a thread count visible in a verdict.

**The split is not at the root**, because the canonical route's root is five
orbits wide and twelve cores would leave seven idle. The frontier is widened a
node at a time until it is at least as wide as the worker count, and the prefix
above it is walked sequentially and counted exactly as the recursion counts it.
`stop_at_first` stays sequential: `canonical_factorisation` uses it to decide
rather than to count, and a walk that stops early has no invariant total to
preserve.

## What it is worth, and the disappointment in it

`enumerate-subspaces fixtures/matmul_2x2x2.tensor --target 7 -s matmul 2 2 2`,
best of three under [`../MEASURING.md`](../MEASURING.md), no throttle events on
either route:

| threads | 1 | 2 | 4 | 6 |
|---|---|---|---|---|
| `--plain`, 1 890 601 nodes | 35.52 s | | 9.51 s | **6.22 s, 5.7x** |
| `--canonical`, 954 nodes | 20.42 s | 14.73 s | 11.19 s | **11.23 s, 1.82x** |

**The plain route scales and the canonical route does not**, and the reason is
the thing this module exists for: canonical augmentation has already removed the
work threads would have spread. 954 nodes against 1 890 601 is 1975x less tree,
and what is left of it sits in few, expensive nodes near the root, above the
frontier, where the walk is sequential. Widening the frontier further would not
help, because there are only five orbits at the first level to widen from.

So the two speed-ups do not compose, which is the same shape as
[`../orbit_reduction/orbit_plan/what-it-is-worth.md`](../orbit_reduction/orbit_plan/what-it-is-worth.md)
finding that a quotient removes exactly the parallelism the plain search uses.
The canonical route is still 3.3x faster than the plain one at one thread and
slower than it at six, which is worth knowing before reaching for both at once.
