# Row-basis heuristic

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
