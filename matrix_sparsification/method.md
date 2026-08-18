# The method, exactly

Notation: [`../linear_algebra/README.md`](../linear_algebra/README.md). The cost
of each primitive: [`../linear_algebra/costs.md`](../linear_algebra/costs.md).
Here `U` is the operator, `r × c`, over `Q`. The oracles work on `Uᵀ`, written
`a × b` with `a = c` and `b = r`, because on that side the question becomes
"replace a row by a sparser vector spanning the same space".

## The problem

> Given `U ∈ Q^{r×c}`, find invertible `V ∈ Q^{c×c}` minimising `nnz(U·V)`.

`V` invertible is what makes it the same operator: `U·V` computes the same
algorithm in a different basis, and fewer nonzeros means fewer additions. Every
result is checked with `same_row_space(Uᵀ, (U·V)ᵀ)`, because sparsity is
trivial to improve by returning a different matrix.

## Row-basis heuristic

Take a square block of rows of `U` that is invertible, and invert it. Whichever
`c` rows are chosen become singletons in the result, so at most `r − c` rows can
carry more than one nonzero.

```
row_basis_sparsifier(U):
    best ← I_c ;  fewest ← nnz(U)
    for each S ⊆ rows(U) with |S| = c:          # C(r, c) subsets
        R ← U[S, :]                              # c × c
        if R is invertible:
            V ← R⁻¹
            if nnz(U·V) < fewest: fewest, best ← nnz(U·V), V
    return best
```

| | |
|---|---|
| Time | Θ( C(r,c) · (c⁴ + r·c²) ), where the `c⁴` is `invert`; see the exact layer's note |
| Space | Θ( C(r,c)·c + r·c ) |

For the 7×4 operators here that is 35 subsets, and it finishes in under a
millisecond.

## The validator both oracles share

A vector in the row space of `rows`, zero on every column of `S`, whose support
avoids the rows already settled. It exists exactly when some unsettled row is,
restricted to `S`, in the span of the others; the coefficients that say so are
the validator, with `−1` in that row's place.

```
find_validator(rows, S, settled):
    R ← rows[:, S]
    for each candidate i ∉ settled:
        if R[i] ∈ span(R[j] : j ≠ i):
            λ ← those coefficients, with λᵢ = −1
            return (λ, i)                        # λᵀ·rows is 0 on every column of S
    return none
```

Cost: Θ(a³·|S|), up to `a` candidates, each a solve with `a−1` unknowns over
`|S|` equations.

## Exact oracle, bottom-up

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

## Exact oracle, top-down

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

## The caveat

These count **field operations**. Over `Q` those are not constant time; see
[the exact layer](../linear_algebra/costs.md#the-caveat-that-matters-not-all-field-operations-cost-the-same).
Numerators and denominators grow through elimination, so wall-clock grows faster
than the operation counts above. It has not bitten yet at 7×4 with entries in
ninths; it would on anything substantial.

Nothing here is proved optimal either. Both oracles are exact for the
sparsest-independent-vector subproblem, but they assemble the answer greedily,
one row at a time.
