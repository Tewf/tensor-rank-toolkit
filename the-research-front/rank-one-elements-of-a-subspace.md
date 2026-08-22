# The rank-one elements of a subspace, and what they are called

The leaf test asks which elements of the subspace `V` have rank one. That set
has a name in algebraic geometry and a literature attached to the name, and an
enumerator for it was built here without reaching either.

## The dictionary

Over `F_q`, the rank-one matrices of `M_{m x n}` up to scalar are the points of
the **Segre variety**, the image of `PG(m−1, q) x PG(n−1, q)` in `PG(mn−1, q)`
under `(u, v) ↦ u ⊗ v`: `PG(n²−1, q)` for `n x n` matrices, `PG(n⁴−1, q)` for
the `n² x n²` slices of `⟨n,n,n⟩`. It is cut out by the `2 x 2` minors, and the
matrices of rank at most `i` are its `(i−1)`-st **secant variety**, cut out by
the `(i+1) x (i+1)` minors, so the rank stratification of a matrix space is a
chain of secant varieties and nothing else.

`ones(V)` in
[`../exhaustive_search/generating-candidates-from-the-span.md`](../exhaustive_search/generating-candidates-from-the-span.md)
is therefore the affine cone over `P(V) ∩ Segre`, a **linear section** of the
Segre variety, and `deficit(V) == 0` asks whether that section spans `V`.

**The pool is the point set, exactly.** `normalised_vectors` in
[`../descent_search/candidate_pool.cpp`](../descent_search/candidate_pool.cpp)
builds vectors with leading entry 1, one per point of `PG(m−1, q)`, and
`RankOnePool::at` takes outer products, so the pool holds one matrix per point
of the Segre variety and `|pool| = ((q^m−1)/(q−1)) · ((q^n−1)/(q−1))`, which is
the Segre point count over `F_q`. Over GF(2) that is `(2^k−1)²` at `k x k`:
**225, 261 121 and 4 294 836 225** at 4x4, 9x9 and 16x16, the three numbers
`generating-candidates-from-the-span.md` opens with. The enumeration was already
walking the right object; it did not know the name.

## What the name buys, and what it does not

The Segre variety of `M_{m x n}` has dimension `m + n − 2` and degree
`C(m+n−2, m−1)`, so a **general** linear subspace of complementary codimension
`m + n − 2` meets it in `C(m+n−2, m−1)` points. At `4 x 4` that is a general `V`
of dimension `mn − m − n + 2 = 10` meeting it in `C(6, 3) = 20`.

**That count is not about the subspaces this search visits, and the gap is the
part worth having.** The expected dimension of `P(V) ∩ Segre` is
`(dim V − 1) + (m + n − 2) − (mn − 1)`, which at `4 x 4` with `dim V = 7`, the
width the `⟨2,2,2⟩` search carries at its deepest, is `−3`. **A general `V` of
that dimension meets the Segre variety in nothing at all.** Every node the
search does not discard is special in the strongest sense the word has here, and
a leaf that succeeds is a subspace spanned by its own rank-one points.

Two caveats, neither small. Degree counts are statements over an algebraically
closed field about a general subspace, where this repository counts
`F_q`-rational points of a very particular one. And `[huang2023]`, which is the
front of classifying spaces of matrices of *bounded* rank (rank three in 1983,
rank four there, exactly four basic spaces up to isomorphism), **was read only
in its abstract**, over a base field this page did not check.

## The leaf is MinRank at `r = 1`, the easy end of a studied problem

`[buss1999]` already says the leaf is MinRank and that MinRank is NP-complete.
What this repository did not have is the rest of it. MinRank has three standard
algebraic modellings, Kipnis-Shamir, Minors and Support-Minors, named as such in
`[bardet2025]`. The Minors modelling is the determinantal ideal of the
`(r+1)`-minors, here the `2 x 2` minors, and its Gröbner complexity is
`[faugere2013]`. The Hilbert series that prices a Gröbner basis over the
Support-Minors modelling is now known in closed form, `[bardet2025]` again. And
`[yang2024]` puts the surrounding problem, finding a rank-`R` decomposition over
a finite field, in FPT with respect to `R` and `|F|`.

**This repository reaches the leaf by enumeration**, scanning the pool or
walking the subspace, whichever
[`../exhaustive_search/rank_one_basis.h`](../exhaustive_search/rank_one_basis.h)
prices cheaper per call. No ideal is ever formed:
[`../satisfiability/`](../satisfiability/README.md) hands a solver the *whole*
rank question in one encoding, never the leaf.

**Whether solving would beat enumerating at these shapes is not settled, and
nothing above is a verdict.** What would settle it is the Hilbert series of
`[bardet2025]` evaluated at the parameters these leaves actually have, `r = 1`
and the dimensions in [`../fixtures/README.md`](../fixtures/README.md), and
nobody has computed it. Against any hope from that direction sits the hypothesis
both complexity papers state for themselves: their bounds are for **generic**
instances, and a matrix multiplication tensor is the least generic object in the
subject. The enumerator has never been raced against a solver here, and the race
is available to run.
