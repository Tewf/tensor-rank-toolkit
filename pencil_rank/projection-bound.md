# The projection bound, and why it loses here

`projections_refute` is `[yang2025thesis]`'s `rref` pruner at `k = 2`. It was
implemented to test a prediction, and the prediction held: **the bounds this
repository already has beat it on every fixture where it applies.**

## What it does

Suppose `T` has `n0` slices and a rank-`R` decomposition whose first-axis vectors
span `GF(p)^n0`. Project that axis onto a plane, a `2 x n0` matrix `M` applied to
the slices. The projection removes `n0 - 2` independent directions, so `M T` has a
decomposition with at most `R - n0 + 2` terms for enough planes that their wedge
products span the whole second exterior power. Find that the planes which could
possibly qualify wedge to something smaller, and no rank-`R` decomposition exists.

**What makes it affordable here is that `M T` is a pencil.** Yang's notebook runs
a CPD search per plane. Two slices is exactly what
[`kronecker_structure`](kronecker_structure.h) settles by exact linear algebra, so
the inner question costs a canonical form rather than a search: 105 planes on
`⟨2,2,2⟩` in **0.94 ms**.

## Measured against what we have

The least `R` each argument fails to refute, on one core:

| fixture | slices | `rank_lower_bound` | projections | |
|---|---|---|---|---|
| `f2_2x2` | 3 | 3 | 3 | equal |
| `f2_2x3` | 4 | 5 | 5 | equal |
| `matmul_2x2x2` | 4 | **6** | 6 | equal |
| `gf8_multiplication` | 3 | **6** | 4 | weaker |
| `cyclic_f2_5` | 5 | **9** | 8 | weaker |
| `gf4_multiplication`, `w_state` | 2 | 3 | n/a | needs three slices |

It never wins. The hope was `⟨2,2,2⟩`, where every bound here stops at 6 while the
rank is 7 and `decide-rank` pays 25 399 nodes to close the gap. It stops at 6 too.

## Why, precisely

**The inner test is a bound and not a rank.** `pencil_rank_of` returns the rank
over the algebraic closure, exact only where the pencil is diagonalisable over
`GF(p)`, and a plane is admitted whenever that bound fails to rule it out. The
admitted set is therefore a **superset** of the planes that truly qualify, which
is what keeps a refutation sound and is also what makes it weak: too many planes
are admitted, their wedges span the whole exterior square, and the argument
declines to refute.

Yang's version does not have this problem because its inner test is an exact CPD
search. Ours is fast for the same reason it is blunt.

**So the way to sharpen it is known and priced.** Settle the inner pencil exactly
rather than bounding it, by handing the undecided ones to `decide-rank`; on
`⟨2,2,2⟩` that is up to 105 exhaustive searches of a 2x4x4 tensor to save one
search of a 4x4x4. Whether that pays is not obvious and is not measured. What is
measured is that the cheap version does not.

## What it is kept for

It is sound, it is a millisecond, and it is a second opinion of a different
family: a Grassmannian span condition rather than a rank inequality. The test
asserts only soundness, never that it is weaker, because a test pinning it as
weaker would fail the day somebody sharpens it.
