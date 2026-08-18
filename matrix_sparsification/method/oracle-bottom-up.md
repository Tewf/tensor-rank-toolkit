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
