# How the exact search works, in one page

`decide-rank` answers "is there an algorithm with exactly `k` products". It is
`[bdez2012]` Algorithm 1 with `[covanov2019]`'s quotient, an addressed pool, and
a bit-packed leaf. Five pieces are usually named together when this is discussed
and **only two of them are wired into this command unconditionally**, so the
table says which, and on what condition the third one is.

| piece | in `decide-rank` | where it is |
|---|---|---|
| **orbit quotient** | yes, on `-s` | [`../orbit_reduction/orbit_search.h`](../methods/bilinear_rank/orbit_reduction/orbit_search.h) |
| **odometer / addressed pool** | yes, automatic | [`../descent_search/candidate_pool.h`](../methods/bilinear_rank/greedy_heuristic/candidate_pool.h) |
| **McKay canonical augmentation** | **no** | [`../oracle_guided_search/`](../methods/bilinear_rank/canonical_augmentation/README.md), reached by `enumerate-subspaces` and `factor-over-canonical-basis --route canonical` |
| **`SortedSpan`** | **no**, and it belongs in the descent rather than here | [`../descent_search/sorted_span.h`](../methods/bilinear_rank/greedy_heuristic/sorted_span.h) |
| **GPU leaf** | **only where `CUDAToolkit_FOUND`**: there `decide-rank` links the registration and `--device auto` sends a leaf past the 8 192 floor to the card; in a build without the toolkit the seam is null | [`../gpu_leaf/`](../infrastructure/gpu_leaf/README.md) |

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
addressed pool is arithmetic, `lefts[i / |rights|] ⊗ rights[i % |rights|]`, so
4 294 836 225 candidates cost no memory at all.

## What it prints

Run against a fixture this repository ships, read-only from `build/`:

```
$ decide-rank fixtures/matmul_2x2x2.tensor --target 7
fixtures/matmul_2x2x2.tensor
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
[that rule is measured](../methods/bilinear_rank/exhaustive/which-leaf-route-is-cheaper.md).
Both routes were rewritten on 2026-08-20 and neither forms an element any more:
the walk moves in reflected Gray order, over GF(2) and over GF(p) alike, so a
step is one row added; and the scan carries a residual through the pool, so a
step is one exclusive or and a test for zero. Same verdicts, same counts, and
[`the-whole-algorithm.md`](the-whole-algorithm.md) has both in place.

## The pieces that are not in it

**McKay** replaces "have I seen this subspace" with "is this subspace's canonical
parent the one I came from", so duplicates are never generated and nothing is
remembered. It is measured at 22 778x fewer nodes while *counting* subspaces and
at 53x fewer nodes for 5.1x the wall clock while *deciding*, which is why
deciding does not use it: [the cost model](../canonical_factorisation/canonical-augmentation.md).

**`SortedSpan`** holds `V` as its rank filtration `R[1] ⊆ … ⊆ R[16]` instead of a
sorted list, which makes the minimum-weight cost a closed form. It reads as a
leaf test too, `dim R[1] == dim V`, and that reading is the trap: building the
filtration walks the same `p^dim` elements the walk route walks, without its
early exit and with a Gaussian rank where a rank-one test would do.

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
