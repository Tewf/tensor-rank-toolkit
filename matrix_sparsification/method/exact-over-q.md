# Exact over `Q`, by the matroid greedy

The one method here proved to return the minimum. It is not a new composition:
it is `[gottlieb2010]`'s driver, which `[beniamini2020]` reproduces as its
Algorithm 2, with an oracle under it that solves the oracle's actual problem
(their Problem 2.15: sparsest vector in the row space *not in the span of the
rows already settled*).

```
sparsest_basis_over_the_rationals(rows):
    space ← reduced row echelon form of rows        # a × b, rank r, pivots P
    held  ← ∅
    for w = 1 … b − r + 1:
        for each T ⊆ [b] with |T| = w:
            for each codeword v with supp(v) ⊆ T:
                if v is outside span(held): held ← held ∪ {v}
                if |held| = r: return held
```

## Why it is the minimum and not a good answer

`nnz(U·V)` over invertible `V` is the total weight of a basis of `U`'s column
space, and every basis arises from some `V`, so the problem *is* "choose a basis
of least total weight". Linear independence is a matroid `[oxley, Prop. 1.1.1]`
and the greedy returns a minimum-weight basis of any matroid under any weight
`[oxley, Lem. 1.8.3]`. Scanning in ascending weight and keeping whatever is not
already spanned is therefore the optimum, whatever breaks the ties.

**The same theorem [`../finite_field_sparsifier.h`](../finite_field_sparsifier.h)
rests on**, over a field where the space has `q^k` elements and can be listed.
Over `Q` it cannot, which is what forces the scan.

## Why the scan is by support, and why it ends

A codeword of weight `w` is supported on `w` columns, so ascending subset size is
ascending weight. Restricted to a support `T` the codewords are one small
homogeneous solve: in reduced echelon form a codeword's entry on a pivot column
*is* that echelon row's coefficient, so the unknowns are the echelon rows whose
pivot lies inside `T`, never more than `|T|`, and the equations are the columns
that are neither a pivot nor in `T`.

**The scan ends at `w = b − r + 1`, proved rather than hoped.** Pick any
information set; the systematic basis it gives has every vector of weight at most
`b − r + 1`. At each step the span of the fewer than `r` vectors held cannot
contain all `r` of them, so one is always available at that weight or below. **No
greedy weight ever exceeds `b − r + 1`**, which is the Singleton ceiling
extended to every step of the greedy and not only the first.

**That retires the widening this method was expected to need.** Reading the older
oracles as "column subsets of size `a − 1`" invites the worry that once some
vectors are settled the lightest vector outside their span has *fewer* than
`a − 1` zeros and is invisible. The bound says it cannot. On `Grey-221`, where
`b` is 23 and `r` is 9, the largest weight taken was 6, 6 and 7 against a ceiling
of 15.

| | |
|---|---|
| Time | O( Σ_{w ≤ W} C(b, w) · (w³ + w·b) ), `W` the largest greedy weight, `W ≤ b − r + 1` |
| Space | Θ( r·b ): the echelon form and what is held. The subsets are walked, not listed |

Counts and timings: [`../README.md`](../README.md).

**Where this scan stops, why, and who already has the algorithm that gets past
it**: [`where-the-scan-stops.md`](where-the-scan-stops.md).
