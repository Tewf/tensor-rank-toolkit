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

## The `rank = a−1` filter loses nothing, and this page said it did

The filter drops every column subset whose vanishing space has dimension two or
more, which looks like it could hide a light vector. **It cannot**, and the two
steps are short enough to give here.

**A minimum-weight vector of the row space outside a settled subspace `C₂` has
minimal support.** Suppose `supp(v′) ⊊ supp(v)` for some nonzero `v′`. Pick `λ`
killing one coordinate of `v`; then `v − λv′` is strictly lighter. If it is
outside `C₂` it beats `v`. If it is inside `C₂`, then `v′` is outside `C₂` (else
`v` would be inside), and `v′` is lighter. Either way `v` was not minimal.

**A minimal-support vector's zero set has a one-dimensional vanishing space.** If
`u` also vanishes on `Z(v)` and is independent of `v`, then `supp(u) ⊆ supp(v)`,
so some `j ∈ supp(v)` has `u_j ≠ 0`, and `v − (v_j/u_j)u` vanishes on `Z(v)` and
at `j` as well: a smaller support, or zero, and both are contradictions.

So `rank(rows[:, Z(v)]) = a − 1`, `Z(v)` contains `a − 1` independent columns,
that subset is corank one, and its unique vanishing vector is `v`. **The filter is
the article's own and Theorem 3.22 proves this method optimal.**

**What this page claimed until 2026-08-22 was the opposite**, and the evidence
against it was already here: over 400 random operators and 203 with a sparse
basis hidden behind a change of basis, this method never once came back heavier
than the proved minimum. That was read as "no operator has separated them yet"
when the reason is that none can.

So there are **two** exact methods here and not one, and
[`exact-over-q.md`](exact-over-q.md) is a third that is faster rather than
righter. What separates them is cost: this one materialises `C(b, a−1)` subsets
before looking at any of them.
