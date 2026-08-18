# Exact oracle, top-down

The same, walking column subsets from the largest downwards and taking the
first validator found. A vector forced to zero on more columns cannot be beaten
by one forced on fewer, so the first hit is the best and there is nothing to
gain by looking further.

```
sparsify_by_descending_support(rows):
    settled ← ∅
    repeat a times:
        for size s = b−1 down to a−1:
            for each S ⊆ [b] with |S| = s:
                if find_validator(rows, S, settled) gives a vector with a zero:
                    take it, settle that row, next round
        if nothing was taken: stop
    return rows
```

| | |
|---|---|
| Time | O( a · 2^b · (a⁴ + a·b) ) worst case, Ω( a·b·a⁴ ) when the first size hits |
| Space | Θ( C(b, ⌊b/2⌋)·b ) at the widest subset size |

Its advantage is the early exit; its exposure is that when large subsets yield
nothing it walks `Σ_s C(b,s)` of them on the way down. The top-down variant
walks column subsets from largest down and takes the first valid solution without
exploring smaller subsets.
