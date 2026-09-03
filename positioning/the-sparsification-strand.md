# The other strand, where the finding is an absence

[`../matrix_sparsification/`](../matrix_sparsification/README.md) implements
`[karstadt2017]`'s alternative-basis construction, SPAA 2017 and JACM 67(1) 2020,
[doi:10.1145/3364504](https://doi.org/10.1145/3364504), and `[beniamini2020]`'s
sparsification built on top of it,
[arXiv:2008.03759](https://arxiv.org/abs/2008.03759). A search for anybody
else's implementation of either found none: `gh search code` over the
identifiers and the papers' own vocabulary returned bibliography files and paper
metadata, and no implementing code. **So this may be the only public
implementation of that strand.**

**That is a search that found nothing, which is weaker than proof of absence**,
and it is written here as the weaker thing rather than the stronger one. Code a
code search cannot reach still exists: a supplement, a private group repository,
a thesis appendix. Read the claim narrowly in the other direction too. What was
not found is an implementation of *these two papers' construction*, not an
absence of sparsification code in general; `[plinopt]` is public, is the near
neighbour in this problem area, and is already recorded in
[`references.md`](../references.md) as reaching sparsity by a different route.

One distinction is worth a sentence, because the names sound adjacent and the
adjacency is false. Those papers optimise **additions**, the leading coefficient
of the arithmetic complexity, over a change of basis, with **the rank held
fixed**. They do not optimise rank. Strassen stays at seven multiplications and
goes from 15 additions to 12; nothing in that is a rank result, and nothing in
this strand moves a bound the rest of this file is about.

## What this strand does, and where it stops

Written 2026-08-22 after three literature reads, because a sparsification count
means nothing until the scope it is claimed in is pinned down.

**This is one stage and not a pipeline.** The repository searches for the
decomposition and then minimises `nnz` over a change of basis, with the rank held
fixed. It finds no common subexpressions, does no in-place accumulation, no
Tellegen transposition and no compaction, and it emits no straight-line program.
A tool that produces one is downstream of a decomposition and is a different job;
the ones that do are in [`references.md`](../references.md).

**What this strand does promise is the minimum.** On `Grey-221`'s three operators
the answer is **128** nonzeros, and 128 is the minimum over every invertible `V`
rather than the best found, because Rado-Edmonds says so. It holds over the finite
fields as well as over `Q`, the greedy running on `q^k`, which is the half that
matters here: every operator [the rank search](../methods/bilinear_rank/descent_search/README.md) emits
is over a finite field.

## Is any of it new? Four candidates, and the algorithm is not one of them

**The exact method is not new and this file will not pretend otherwise.** It is
`[beniamini2020]`'s Algorithm 2 with an oracle for its own Problem 2.15, and the
bottom-up oracle **this repository implements** is its Algorithm 3, proved
optimal by its Theorem 3.22. That one lives on the `rejected-experiments`
branch (`retired/dominated_sparsifiers/`) since 2026-08-22, so read the claim
above as "in this repository" rather than "in this working tree". What was added is speed and a written proof, not a result.

What is *not* found in the literature, in descending order of confidence:

1. **The bridge.** `[sanjose2025]`'s relative generalized Hamming weight at
   `r = 1` is `[beniamini2020]`'s Sparsest Independent Vector, exactly. One
   community states the problem and enumerates column subsets for it; the other
   has had a Brouwer-Zimmermann-pruned algorithm for it in ACM TOMS since 2025.
   **Neither cites the other, in either direction.**
2. **The greedy on a pruned oracle.** Nobody has put the Rado-Edmonds driver on
   top of that pruned oracle and summed the successive minima into a
   minimum-weight basis. Both halves exist, in separate codebases, and never call
   each other.
3. **Any of it over `Q`.** The whole Brouwer-Zimmermann / GHW / RGHW corpus is
   finite-field; the sparsification corpus over `Q` is floating-point and
   heuristic. That the bound is characteristic-free is written down nowhere.
4. **A measurement rather than a method**: that minimising nonzeros can *cost*
   additions once common subexpressions are on the table, which
   [`../matrix_sparsification/measured-with-other-tools/before-a-subexpression-pass.md`](../matrix_sparsification/measured-with-other-tools/before-a-subexpression-pass.md)
   shows going the wrong way on one operator of three schemes.

**Read all four narrowly.** They rest on searches that found nothing, which is
weaker than proof of absence, and on three reads by delegated agents whose
findings were checked but whose coverage was not exhaustive.
