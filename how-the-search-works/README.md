# How the exact search works, in one page

`decide-rank` answers "is there an algorithm with exactly `k` products". It is
`[bdez2012]` Algorithm 1 with `[covanov2019]`'s quotient, an addressed pool, and
a bit-packed leaf. Five pieces are usually named together when this is discussed
and **only two of them are wired into this command**, so the table says which.

| piece | in `decide-rank` | where it is |
|---|---|---|
| **orbit quotient** | yes, on `-s` | [`../orbit_reduction/orbit_search.h`](../orbit_reduction/orbit_search.h) |
| **odometer / addressed pool** | yes, automatic | [`../descent_search/candidate_pool.h`](../descent_search/candidate_pool.h) |
| **McKay canonical augmentation** | **no** | [`../oracle_guided_search/`](../oracle_guided_search/), reached by `enumerate-subspaces` and `factor-over-canonical-basis --route canonical` |
| **`SortedSpan`** | **no**, and it belongs in the descent rather than here | [`../descent_search/sorted_span.h`](../descent_search/sorted_span.h) |
| **GPU leaf** | **no**, a proof of concept wired to nothing | [`../gpu_leaf/`](../gpu_leaf/) |

Three of the five are not in the production path, and each is now a decision with
a number behind it rather than a gap: [`what-to-wire.md`](what-to-wire.md). All
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

The leaf is where the run lives. Which route it takes is decided by count and
[that rule is measured](../exhaustive_search/which-leaf-route-is-cheaper.md).

## The pieces that are not in it

**McKay** replaces "have I seen this subspace" with "is this subspace's canonical
parent the one I came from", so duplicates are never generated and nothing is
remembered. It is measured at 22 779x fewer nodes while *counting* subspaces and
at 53x fewer nodes for 25.8x the wall clock while *deciding*, which is why
deciding does not use it: [the cost model](../canonical_factorisation/canonical-augmentation.md).

**`SortedSpan`** holds `V` as its rank filtration `R[1] ⊆ … ⊆ R[16]` instead of a
sorted list, which makes the minimum-weight cost a closed form. It reads as a
leaf test too, `dim R[1] == dim V`, and that reading is the trap: building the
filtration walks the same `p^dim` elements the walk route walks, without its
early exit and with a Gaussian rank where a rank-one test would do.

**The GPU** does one whole `⟨4,4,4⟩` leaf in 1.019 s against 9.2 minutes of one
core, and answers no question this repository can currently pose, because the
tree above that leaf has a node count nothing here bounds.

Every parameter each of these takes: [`parameters.md`](parameters.md).
