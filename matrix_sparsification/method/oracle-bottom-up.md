# Exact oracle, bottom-up

Column subsets one smaller than the row count: any larger and the only vector
orthogonal to all of them is zero.

```
sparsify_by_best_corank_one(rows):
    viable ← { S ⊆ [b] : |S| = a−1, rank(rows[:,S]) = a−1 }
    settled ← ∅
    repeat a times:
        best ← the validator over all S ∈ viable whose vector has the most zeros
        if none: stop
        rows[best.i] ← best.vector ;  settled ← settled ∪ {best.i}
    return rows
```

| | |
|---|---|
| Time | Θ( a · C(b, a−1) · (a⁴ + a·b) ) |
| Space | Θ( C(b, a−1)·a ) |

**Not the minimum, and the reason is the `rank = a−1` filter.** A column subset
whose vanishing space has dimension two or more is dropped, so a codeword whose
zero set admits no corank-one subset is invisible here. `[beniamini2020]`'s
Algorithm 2 wants a *maximal* Ω-valid set and this fixes the size at `a−1`.
[`exact-over-q.md`](exact-over-q.md) is that algorithm without the restriction;
no operator has yet separated the two, but only one of them can say so.
