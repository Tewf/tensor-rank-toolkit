# When the matroid is regular, this stops being a search

The scan in [`exact-over-q.md`](exact-over-q.md) does not finish on
`4x4x4_49_156_L`; [`where-the-scan-stops.md`](where-the-scan-stops.md) says why.
This page is a way past it that is not a faster search, and it applies to exactly
the operator that needed one.

## The theorem, and the duality that makes it ours

`[tillmann2019, Thm. 5]`: for a **unimodular** matrix the spark is computable in
polynomial time, because minimising `ℓ0` collapses to minimising `ℓ1` on basic
solutions. He adds the matroid statement: the vector matroids of totally
unimodular matrices are the **regular** ones, so their girth is polynomial too.

Spark is the *girth* of the column matroid and the quantity here is the
**cogirth** — a minimum-weight vector of the row space has minimal support, and
minimal supports of the row space are the cocircuits of `M[G]`, which are the
circuits of its dual. **Regular matroids are closed under duality**, so if `M[G]`
is regular the theorem applies to `M[G]*`, and

> minimum weight = `spark(H)`, for any `H` whose row space is the null space of `G`

is an LP rather than a walk:

```
spark(H) = min over j of   min ‖x‖₁   subject to   H x = 0,  x_j = 1
```

`n` small linear programmes, one per coordinate.

## Which of our operators are regular, and it is the opposite of convenient

Tested two ways, because refuting is cheap and confirming is not: random basis
determinants (one `|det| ≥ 2` refutes unimodularity), and Tutte's criterion, that
a matroid is regular exactly when it is binary and ternary, so the `Q`, GF(2) and
GF(3) ranks must agree on every subset.

| operator | basis determinants | `Q` / GF(2) / GF(3) ranks | regular? |
|---|---|---|---|
| `Grey-221_L` 9×23 | `|det| = 2` after 614 samples | — | **no** |
| `Grey-221_R` 9×23 | `|det| = 2` after 145 samples | — | **no** |
| `4x4x4_49_156_L` 16×49 | 200 000 bases, all in `{0, 1}` | 20 000 subsets, all agree | **strong evidence yes** |

**The operators the scan finishes are not regular, and the one it cannot is.**
That is the useful way round and it was not the expected one.

## What it reaches, measured

`4x4x4_49_156_L`, 49 linear programmes, **0.08 s**:

| | weights | total |
|---|---|---|
| LP candidates through each coordinate, greedily assembled | 4×9, 8×6, 16 | **100** |
| a cheap coefficient box, for comparison | 4×8, 8×7, 16 | 104 |
| the exact scan | 4×9, then it stops | no answer |

**Checked outside the solver, in exact rational arithmetic**, because an LP runs
in floating point and a basis spanning a slightly different space is worthless
however light it is. The 16 vectors come back to exact rationals, and: rank of
the original row space 16, rank of the answer 16, rank of the two stacked 16, and
every vector individually in the row space. It is the same operator.

**The first nine weights are proved optimal and the LP finds exactly those nine.**
The scan established that nine vectors of weight 4 exist and that nothing weighs
5; the LP returns nine 4s. Its minimum, 4, is the exact minimum weight by the
theorem above.

## Read this narrowly

- **Regularity is sampled, not proved.** 200 000 bases and 20 000 rank triples
  are evidence, not a certificate. Deciding regularity is polynomial
  (Truemper's algorithm, implemented in CMR) and none of that is run here.
- **The theorem gives the *first* weight, not the basis.** Being outside the span
  of what is settled is not a convex constraint, so the later greedy steps do not
  follow from it. 100 is an **upper bound** on the minimum-weight basis, and the
  lower bound the scan licenses leaves a real gap for vectors ten onward.
- **This is a prototype in Python against HiGHS**, not a method in this
  repository. [`../../integer_programme/README.md`](../../integer_programme/README.md)
  already keeps a solver chain (`gurobi → cbc → glpk → lp_solve → built-in`) and
  is where an implementation belongs.

**What it changes today** is that an operator with no answer has one: a certified
basis at 100 nonzeros with its first nine vectors provably minimal, in under a
tenth of a second, where the search cannot finish at all.
