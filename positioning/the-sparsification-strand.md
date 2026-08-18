# The other strand, where the finding is an absence

[`../matrix_sparsification/`](../matrix_sparsification/) implements
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
