# Answering without searching, and the theorem that turned out not to apply

The scan in [`exact-over-q.md`](exact-over-q.md) enumerates column supports and
does not finish on `4x4x4_49_156_L`;
[`where-the-scan-stops.md`](where-the-scan-stops.md) says why. This page is a
route past it that searches nothing, the reason it was tried, and the fact that
the reason was wrong.

## Why it was tried

`[tillmann2019, Thm. 5]`: for a **unimodular** matrix the spark is computable in
polynomial time, because minimising `ℓ0` collapses to minimising `ℓ1` on basic
solutions. Spark is the *girth* of the column matroid and the quantity here is
the **cogirth**, but a minimum-weight vector of the row space has minimal support
and those supports are the cocircuits, so the theorem reaches this through the
dual, regular matroids being closed under duality:

> minimum weight = `spark(H)`, for any `H` whose row space is the null space of `G`
>
> `spark(H) = min over j of   min ‖x‖₁   subject to   H x = 0,  x_j = 1`

`n` small linear programmes, one per coordinate.

## The theorem does not apply, and finding that out took a real test

**Sampling said it did and sampling was wrong.** 200 000 random 16-subsets of
`4x4x4_49_156_L` all had determinant in `{0, ±1}`, and 20 000 random column
subsets had equal rank over `Q`, GF(2) and GF(3). That looked like Tutte's
criterion holding. It is not evidence: only about 0.8% of random 16-subsets are
even bases, and the witnesses are rarer still.

**The right instrument is a decision procedure, not a sample.** Truemper's
algorithm decides total unimodularity in polynomial time
`[truemper1990]`, and `[cmr]` implements the Walter-Truemper variant. It refuted
this matrix in **2.8 milliseconds** and handed back a witness:

| | |
|---|---|
| columns of `G` (0-indexed) | `0 1 5 10 12 15 16 19 21 22 25 28 31 39 44 48` |
| the same as rows of the `.sms` (1-indexed) | `1 2 6 11 13 16 17 20 22 23 26 29 32 40 45 49` |
| determinant | **−2** |
| rank of those columns | `Q` 16, GF(3) 16, **GF(2) 15** |

Checked here in exact arithmetic and sharing no code with that library: integer
Bareiss, exact rational elimination, and determinants modulo 1000003, 999983 and
32749, all agreeing on −2. **So the matrix is not unimodular, its matroid is not
regular, and Tillmann's theorem says nothing about it.** Nor is it equimodular
for any other `k`, so there is no `Δ = 2` consolation either.

## What survives, and it is the more interesting half

**The measurement stands and the explanation does not.**

| operator | matroid regular? | search | simplex |
|---|---|---|---|
| `Grey-221_L` 23×9 | **no**, a basis has determinant 2 | 43 in 0.34 s | **43 in 0.025 s** |
| `Grey-221_R` 23×9 | **no** | 42 in 0.33 s | **42 in 0.022 s** |
| `Grey-221_P` 9×23 | **no** | 43 in 0.44 s | **43 in 0.032 s** |
| `4x4x4_49_156_L` 16×49 | **no**, determinant −2 above | **cannot finish** | **100 in 0.34 s** |

On the three `Grey-221` operators the linear programme reaches the **proved
minimum**, about fourteen times faster than the search that proves it. On the
fourth it reaches a basis at 100 nonzeros, verified in exact rationals to span
the same space, where the search reaches nothing at all.

**Four operators out of four, and not one of them is regular.** The `4x4x4` case
was the only apparent confirmation that the theorem was why any of this worked,
and it was not one. So the standing question is no longer "does the theorem
apply here" but **"why is the `ℓ1` relaxation tight on operators that are not
unimodular"**, and nothing cited here answers it: recognising `Δ`-modularity is
itself open for `Δ ≥ 2`, and the girth literature and the `Δ`-modular literature
do not cite each other at all.

## Read this narrowly

- **The 100 is an upper bound.** With regularity gone, nothing here proves it
  minimal. What is proved is that it is a basis of the same space, checked in
  exact rational arithmetic independently of any solver.
- **The `Grey-221` numbers are minimal**, but because
  [`exact-over-q.md`](exact-over-q.md) proved them, not because of anything on
  this page.
- **`--simplex` says which it is holding** on every run, and always did.
- **Orientation trap, if you re-run the unimodularity test.** Handed the stored
  49×16 file, `cmr-equimodular` answers *unimodular*, because at that shape the
  rank-16 condition collapses to a gcd over a 16-element matroid and is vacuous.
  Feed it the 16×49 orientation, which is the one the oracles take.
