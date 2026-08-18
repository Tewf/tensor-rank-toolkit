# The method, exactly

Notation: [`../linear_algebra/README.md`](../linear_algebra/README.md). The cost
of each primitive: [`../linear_algebra/costs.md`](../linear_algebra/costs.md).
Here `p` is the characteristic, `k` the number of slices, `n × m` their shape,
`w = n·m`, and `d` the dimension of the span.

## The problem

Given a bilinear map `T = (T₁, …, T_k)`, slices of shape `n × m` over `GF(p)`,
find `S = (S₁, …, S_j)` with

> `span(S) ⊇ span(T)`, minimising `Σᵢ rank(Sᵢ)`.

`Σ rank(Sᵢ)` is the number of multiplications, because a rank-one bilinear form
is exactly one product. **`j` may exceed `k`**: `S` only has to *generate* `T`,
so a larger spanning set of lower total rank is a better answer. That is what
Karatsuba's five products for a four-coefficient product is.

## Step 1 is exact, and it is the only step that is

Choosing a basis of `span(T)` minimising `Σ rank` is a matroid problem:
independence of vectors forms a matroid, and the greedy (sort by weight
ascending, keep whatever stays independent) yields a **minimum-weight basis**
(Rado-Edmonds). So step 1 does not approximate anything, and its result is
tie-break independent. Everything after it relaxes the constraint that the
answer be a basis of `span(T)`, and that is where the guarantee goes.

## Step 1: greedy smallest basis

```
minimum_weight_basis(T):
    C ← [ Σᵢ cᵢ·Tᵢ  for c ∈ GF(p)^k, c ≠ 0 ]        # p^k − 1 candidates
    sort C by (rank, enumeration index)
    B ← ∅
    for M in C:
        if B is full (|B| = dim span T): break
        if M ∉ span(B): B ← B ∪ {M}
    return B
```

Ties break on enumeration order, with the first coordinate varying fastest,
pinning this choice to ensure reproducible results.

| | |
|---|---|
| Time | Θ(p^k · (k + d)·w), building and ranking every element of the span, then Θ(p^k·k·log p) to sort |
| Space | **Θ(p^k · w)**: it materialises the whole span |

## Steps 2 and 3: minimise over a candidate pool

Identical code; they differ only in the pool `G` they are handed.

```
minimise_rank(T, G):
    loop:
        span ← basis(T)
        for i in 0 … |G|−1:
            if G[i] ∈ span: continue
            V ← minimum_weight_basis(T ∪ {G[i]})
            if cost(V) < cost(T):
                T ← V ; span ← basis(T) ; continue      # keep going down this G
            else:
                G ← [ g ∈ G[i+1…] : g ∉ span(V) ]        # drop what V already reaches
                restart loop
        return T                                          # a full pass changed nothing
```

`improving_candidates` has the same shape but never updates `T`; it just
collects the candidates that would individually pay, as a pre-filter.

Every restart replaces `G` with a strict suffix of itself, so there are at most
`|G|` restarts and at most `|G|` candidates examined per pass.

| | |
|---|---|
| Time | O(\|G\|² · p^(k+1) · (k+d)·w) worst case |
| Space | Θ(p^(k+1)·w + \|G\|·w) |

The worst case is very loose, because the pruning step is doing the real work.
The shortlists `improving_candidates` actually returned on the four fixtures
were **0, 1, 0 and 6** out of pools of 961 to 4732, so almost every candidate
is discarded before `minimum_weight_basis` is ever called on it.

**Step 2's pool** is the rank-one maps already inside `T`: `rank_one_candidates`
decomposes each slice, giving `Σ rank(Tᵢ)` candidates, which is `cost(T)`.

**Step 3's pool** is every rank-one map of the shape, one per scalar class:

> `|G| = (p^n − 1)(p^m − 1) / (p − 1)²`

built as outer products of vectors normalised to leading entry 1, in
Θ(\|G\|·w) time and space. That gives 961, 1785, 1905 and 4732 for the four
fixtures, which is what the tool reports.

## Where the cost actually is

`p^k` dominates everything, and **`k` grows during the search**, which is the
point of the method, and also its wall. Peak memory is essentially the largest
`minimum_weight_basis` call:

> peak ≈ `p^(k+1) · (8w + 88)` bytes

| Fixture | Largest call | Predicted | Measured peak RSS |
|---|---|---|---|
| F2 5×5 | 2¹¹ | 5.2 MB | 5.4 MB |
| F2 3×8 | 2¹⁴ | 9.2 MB | 9.4 MB |
| F2 4×7 | 2¹⁴ | 9.7 MB | 10.0 MB |
| F3 3×6 | 3¹¹ | 45.7 MB | 42.3 MB |

(including a 4.6 MB baseline process, measured with `--steps 1`.)

So the scaling limit is not time, it is memory, and it is exponential in a
quantity the search deliberately increases. Enumerating the span is the honest
first thing to replace: nothing in the method requires *materialising* it, only
visiting it in nondecreasing rank order.

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
thing to spend the time on. [`[bdez2012]`](../references.md) decided F2 5×5 in
2012 and its rank is **13**, over a complete run of 9.65×10⁹ tests, so seven
hours at `k` = 12 would reproduce their exclusion as a check and settle nothing
open. The genuinely open fixture is **F2 4×7**, at `15 ≤ rank ≤ 16`, where
closing the gap means deciding `k` = 15. Their 7×4 row reads by the stated
convention for the `k` column and carries no timing, so verify it against the
paper before treating the 15 as established.

Every row but the last is asserted and run in CI: the two Karatsuba answers and
both exclusions in
[`exhaustive_search/tests/test_exhaustive_search.cpp`](../exhaustive_search/tests/test_exhaustive_search.cpp), with the
11 as its own `slow`-labelled test, and GF(8)'s 6 in
[`map_construction/tests/test_map_construction.cpp`](../map_construction/tests/test_map_construction.cpp) beside the
tensor it is built from.
