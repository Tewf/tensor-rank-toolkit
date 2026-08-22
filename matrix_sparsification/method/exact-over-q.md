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

## Where it stops, and it is the combinatorics

The scan has an upper bound and **no lower bound**, so it stops only on
collecting `r` vectors. Measured: `4x4x4_49_156_L`, a 16-dimensional space in
`Q^49`, holds 9 of its 16 vectors at weight 4 within 5 s and finds nothing new at
weight 5. The remaining 7 need weight 6 or more, where the scan is `C(49,6)`, 14
million subsets, reaching 451 million at weight 8. Neither the C++ nor an
independent reference finished it in 30 minutes. **That is a fact about the
combinatorics, not about the code, so running it longer buys nothing.**

The problem this reduces to is the one coding theory has computed for forty
years, minimum-weight codewords of a linear code, and its standard algorithm
**Brouwer-Zimmermann** `[zimmermann1996]` carries a lower bound from several
disjoint information sets and prunes on it. That is the first thing to try here,
because it prunes this same enumeration rather than replacing it. What that
bound is worth is measured rather than hoped: `[hernando2019]` reports 198
million codewords generated against Magma's 6 001 million on one code, so the
saving is the pruning and not a faster inner loop.

**The obvious generalisation was read and it does not transfer.**
`[sanjose2025]` extends Brouwer-Zimmermann from the minimum distance to the whole
weight hierarchy, which looks like the same step from "the first weight" to "a
sequence of weights" that this file takes. It is not. Its bound is sound only
because every message-space subspace of support `≤ w` was exhausted first, and
that enumeration is over `{v ∈ F_q^r : 1 ≤ wt(v) ≤ z}`, infinite over `Q`. The
bound is field-agnostic and the walk it is attached to is not, and they do not
come apart. **So the direction this page named as "the first thing to try" is not
available here in the form that would help**, and that is recorded rather than
quietly dropped.

What does survive is the degenerate case, one information set, which this scan
already licenses for nothing: having solved every support of size `≤ t`, whatever
is left weighs at least `t+1`. Paired with a cheap upper bound that is a
stopping rule. Measured: it would fire on six of nine greedy steps for
`Grey-221_L` and on **none** for `4x4x4_49_156_L`, so it does not rescue the
operator it would have to.

The minimum-weight basis is also **not unique**, and which one comes back is
decided by the order the subsets are walked in. That does not change the count,
which is what this method promises. It does change what a downstream common
subexpression pass makes of the result, measured in
[`../in-front-of-plinopt.md`](../in-front-of-plinopt.md).
