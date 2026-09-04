# How the exact search works, in one page

`decide-rank` answers "is there an algorithm with exactly `k` products". It is
[`[bdez2012]`](../../references.md) Algorithm 1 with [`[covanov2019]`](../../references.md)'s quotient, an addressed pool, and
a bit-packed leaf. Five pieces are usually named together when this is discussed
and **only two of them are wired into this command unconditionally**, so the
table says which, and on what condition the third one is.

| piece | in `decide-rank` | where it is |
|---|---|---|
| **orbit quotient** | yes, on `-s` | [`../../methods/bilinear_rank/orbit_reduction/orbit_search.h`](../../methods/bilinear_rank/orbit_reduction/orbit_search.h) |
| **odometer / addressed pool** | yes, automatic | [`../../methods/bilinear_rank/candidate_pool.h`](../../methods/bilinear_rank/candidate_pool.h) |
| **McKay canonical augmentation** | **no** | [`../../methods/bilinear_rank/canonical_augmentation/`](../../methods/bilinear_rank/canonical_augmentation/), reached by `enumerate-subspaces` and `factor-over-canonical-basis --route canonical` |
| **`SortedSpan`** | **no**, and it belongs in the descent rather than here | [`../../methods/bilinear_rank/greedy_heuristic/sorted_span.h`](../../methods/bilinear_rank/greedy_heuristic/sorted_span.h) |
| **GPU leaf** | **only where `CUDAToolkit_FOUND`**: there `decide-rank` links the registration and `--device auto` sends a leaf past the 8 192 floor to the card; in a build without the toolkit the seam is null | [`../../infrastructure/gpu_leaf/`](../../infrastructure/gpu_leaf/) |

Three of the five are outside the default build's production path, and each is
now a decision with a number behind it rather than a gap: [`what-to-wire.md`](what-to-wire.md). All
five composed into one algorithm, with the rule that picks a device:
[`the-whole-algorithm.md`](the-whole-algorithm.md).

## The search that runs

```
decide(T, k):
    V0    <- span(T)                      the subspace every algorithm contains
    floor <- rank_lower_bound(T)          flattening, rank sums, Griesmer
    if k < floor: return NO, proved without opening a node

    P <- rank-one maps of the shape       materialised, or addressed by index
    G <- requested_group(T)               empty unless -s was given

    return descend(V0, from = 0)

descend(V, from):
    if dim V > k: return NO                     no room left
    if dim V == k: return leaf(V)               the whole question, at depth
    for i in from .. |P| - 1:                   a contiguous suffix, not a set
        if G is non-empty and not least_in_orbit(G, i, from): continue
        W <- V + P[i]
        if dim W == dim V: continue             P[i] already inside
        if descend(W, i): return YES            or i+1; the child skips it either way
    return NO

leaf(V):                                        has V a rank-one basis?
    n <- p^dim V, counted only up to |P|
    if n != 0 and n < |P|: walk the p^dim elements of V, testing rank one
    else:                  scan P, testing membership of V
```

Two facts do the work. `least_in_orbit` makes the quotient need **no array over
`P`**: it is a breadth-first walk of one orbit under the generators, early-exiting
the moment it meets a smaller index still in `[from, |P|)`. And `P[i]` on an
addressed pool is arithmetic rather than stored ([`the-whole-algorithm.md`](the-whole-algorithm.md)
has the formula), so 4 294 836 225 candidates cost no memory at all.

## What it prints

Run against a fixture this repository ships, read-only from `build/`:

```
$ decide-rank evidence/fixtures/matmul_2x2x2.tensor --target 7
evidence/fixtures/matmul_2x2x2.tensor
  rank bound: rank is at least 6
  pool: 225 rank-one maps of shape 4x4
  leaf: GF(2), one bit per entry
  plan:
    pool: materialised (225 maps at 184 B each is 40 KiB, inside the 4.00 GiB budget)
    leaf route: walk (128 subspace elements at dimension 7 against 225 pool maps)
    device: cpu (128 elements at the deepest leaf, under the 8192 launch floor)
    threads: 1
    quotient: none
    orbit test: full
    anchor: map
  7436 nodes in 0.014831 s
  FOUND: 7 products, rank bound 6, gap 1
  verified: they compute the map
```

Every line the plan prints is one of the five pieces answering for itself before
`descend` opens a node: which pool, which leaf route, which device, how many
workers, which quotient. The node count is `nodes_visited` in `SearchBudget`,
the counter [`what-the-rewrites-were-worth.md`](what-the-rewrites-were-worth.md)
times whole questions by.

The leaf is where the run lives. Which route it takes is decided by count and
[that rule is measured](../../methods/bilinear_rank/exhaustive/which-leaf-route-is-cheaper.md).
Both routes were rewritten on 2026-08-20 and neither forms an element any more:
the walk moves in reflected Gray order, over GF(2) and over GF(p) alike, so a
step is one row added; and the scan carries a residual through the pool, so a
step is one exclusive or and a test for zero. Same verdicts, same counts, and
[`the-whole-algorithm.md`](the-whole-algorithm.md) has both in place.

## What it costs

| | |
|---|---|
| Time | `O(C(\|pool\|, k − dim T) · \|pool\| · d · w)`, one pool scan per leaf |
| Space | `Θ((k − dim T) · d · w)`, the recursion depth times a basis |

**Where the cost actually is.** Essentially every node is a leaf, and every leaf
scans the whole pool testing membership at `Θ(d·w)` each. For F2 5×5 at `k = 11`
that is 459 239 leaves × ~950 tests × ~275 field operations ≈ 10¹¹, and it
measured 77 seconds, which agrees.

Six questions, measured, anchored at the map throughout, the true minimum
([`parameters.md`](parameters.md) says why):

| Map | Question | Nodes | Time |
|---|---|---|---|
| F2 2×2 | fewest | 1 | 7 µs |
| F2 2×3 | fewest | 3 | 20 µs |
| GF(8) | fewest | 1 606 | 4.7 ms |
| F2 5×5 | is there a 10? **no** | 959 | 0.17 s |
| F2 5×5 | is there an 11? **no** | 459 239 | 77 s |
| F2 5×5 | is there a 12? **no** | 146 402 553 | 535.59 s |

**The 10 and 11 rows are the tree's own cost, not what `decide-rank` prints on this
build today.** The polynomial floor above already proves `f2_5x5`'s rank is at
least 12, so a plain `--target 10` or `--target 11` run stops at `rank bound: rank
is at least 12` and the line straight after it, no node opened. Only the 12 row
still needs the tree, since 12 meets the floor rather than clearing it. The 959
and 459 239 counts are `expand_subspace` invoked directly, which is what the tree
still costs and what the wall clock comparisons elsewhere in this repository
measure it by.

The `F2 2×3` and `GF(8)` fewest rows drift the same way, only smaller: the
search now reaches the same product count in fewer nodes than the row states,
with every verdict unchanged. This table is not the source of record for
either count; the corrected ones, dated and against a commit, are in
[`results.json`](../../methods/bilinear_rank/greedy_heuristic/results.json).

`decide-rank evidence/fixtures/f2_2x2.tensor` reproduces the first row; this run's own
printed output was

    evidence/fixtures/f2_2x2.tensor
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
[`../../methods/bilinear_rank/exhaustive/what-it-decides.md`](../../methods/bilinear_rank/exhaustive/what-it-decides.md).

Every row but the last is asserted and run in CI: the two Karatsuba answers and
both exclusions in
[`../../methods/bilinear_rank/exhaustive/tests/test_exhaustive_search.cpp`](../../methods/bilinear_rank/exhaustive/tests/test_exhaustive_search.cpp), with the
11 as its own `slow`-labelled test, and GF(8)'s 6 in
[`../../methods/bilinear_rank/map_construction/tests/test_map_construction.cpp`](../../methods/bilinear_rank/map_construction/tests/test_map_construction.cpp) beside the
tensor it is built from.

## The pieces that are not in it

**McKay** replaces "have I seen this subspace" with "is this subspace's canonical
parent the one I came from", so duplicates are never generated and nothing is
remembered. It is measured at 22 778x fewer nodes while *counting* subspaces and
at 53x fewer nodes for 5.1x the wall clock while *deciding*, which is why
deciding does not use it: [the cost model](../../methods/canonical_factorisation/canonical-augmentation.md).

**`SortedSpan`** holds `V` as a rank filtration instead of a sorted list
([`parameters.md`](parameters.md) has the members); wired as a leaf test it
would just replay the walk route without its early exit, which is why it stays
at the descent's cost query instead ([`what-to-wire.md`](what-to-wire.md)).

**The GPU** does one whole `⟨4,4,4⟩` leaf in 1.019 s, and answers no question
this repository can currently pose, because the tree above that leaf has a node
count nothing here bounds. **The host figure it was set against is stale**: the
9.2 minutes this said was 4 294 836 225 elements at the scan rate of the day, and
the scan has since carried a residual instead of reducing each element. The
verdict on the card is suspended for that reason and for one more in
[`what-to-wire.md`](what-to-wire.md).

Every parameter each of these takes: [`parameters.md`](parameters.md).
What the 2026-08-20 rewrites came to on whole questions:
[`what-the-rewrites-were-worth.md`](what-the-rewrites-were-worth.md).
