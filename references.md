# References

Every algorithm in this repository comes from a published paper. This file is
where they are named; nothing else restates a citation, and code cites a key and
a numbered result, `[hastad1990, Lemma 2]`, so that the claim can be checked
against the source rather than against the code that implements it.

Papers are cited, never redistributed. [NOTICE](NOTICE) says what the licence
does and does not cover.

This file is longer than the eighty lines the conventions ask of a markdown
document, deliberately. It is a bibliography, which is a data table: one entry
per paper, no prose to factor out, and splitting it across files would make a
citation harder to find rather than easier, which is the opposite of what the
length rule is for.

## What the problem is, and how hard

**`brockett1978`**: R. W. Brockett, D. Dobkin. *On the optimal evaluation of a
set of bilinear forms.* Linear Algebra and its Applications (1978). Defines the
rank of a set of bilinear forms as the smallest number of rank-one matrices
whose span contains the set. **That definition is what this repository's whole
rank strand computes.** A map is held as its slices, their span is
[`span_of`](linear_algebra/span_basis.h), and every search here is a search for
rank-one matrices spanning it, which is why
[`rank_one_basis_of`](exhaustive_search/rank_one_basis.h) is the question at
every leaf rather than a step on the way to one.

**`grigoriev1978`**: D. Grigoriev. *Multiplicative complexity of a pair of
bilinear forms and of the polynomial multiplication.* MFCS 1978, LNCS **64**.
The same definition, reached independently, and with it the polynomial-time
solution for two slices. Polynomial multiplication is what every `.tensor`
fixture here is; the two-slice case is the one shape of it that never needed a
search, and nothing here special-cases it.

**Theorem 1 is the formula, and it was read**, from the author's own copy at
[logic.pdmi.ras.ru/~grigorev/pub/pair.pdf](https://logic.pdmi.ras.ru/~grigorev/pub/pair.pdf),
the LNCS volume being paywalled. The report's own summary calls it *"the explicit
formula for the rang of a pair of matrices over an algebraically-closed field
(theorem I)"*, and the statement counts, over the Weierstrass-Kronecker form,
`Σ(a_i + 1)` over the `L` blocks, `Σ(β_j + 1)` over the transposed ones, `p` the
size of the regular part, and `d` the number of blocks of dimension at least
2 × 2. That is
[`kronecker_structure.h`](pencil_rank/kronecker_structure.h)'s
`Σ(eps_i + 1) + Σ(eta_j + 1) + regular_size + delta` term for term. **The
algebraically-closed hypothesis is the theorem's own**, which is why the module
reports that count as a lower bound over `GF(p)` and not as the rank.

**`jaja1979`**: J. Ja'Ja'. *Optimal evaluation of pairs of bilinear forms.* SIAM
Journal on Computing **8** (1979), no. 3, 443-462; STOC 1978. Determines the
rank of any `p x q x 2` tensor in polynomial time, through the Kronecker theory
of matrix pencils. It marks where the exhaustive machinery here starts being
necessary: at two slices the answer is a canonical form, and the searches in
this repository are for what lies past that.

**Its theorem numbers are named through `[sumi2009]`, the way `[gabriel1972]` is
named through `[brion2008]`.** The paper itself is behind SIAM's paywall and was
not read; what was checked is that `[sumi2009]`, whose reference [6] is this
paper, cites *"[6, Theorem 2.1]"* for `rank((0, E_k); (E_k, 0)) = k + 1`, the
singular-block count, *"[6, Theorem 3.3 and proof of Theorem 3.1]"* for the
regular part, and *"[6, Theorem 3.6]"* for the reverse inequality under
`p_1(A)` splitting. Those three are the only numbers this repository quotes for
it, in [`kronecker_structure.h`](pencil_rank/kronecker_structure.h); a
`[jaja1979, Thm. n]` for any other `n` would be an invention.

**`sumi2009`**: T. Sumi, M. Miyazaki, T. Sakata. *Rank of 3-tensors with 2 slices
and Kronecker canonical forms.* Linear Algebra and its Applications (2009),
[arXiv:0808.1167](https://arxiv.org/abs/0808.1167). The modern treatment of the
two-slice case, and the warning that comes with it: **the rank of a Kronecker
canonical form is not the sum of the ranks of its direct summands**, so a
decomposition into summands does not license adding their ranks up.

Two of its numbered results are what [`pencil_rank/`](pencil_rank/README.md)
actually rests on, and neither is about `ℝ` or `ℂ` despite the paper's own
setting. **Lemma 2.1** is the Kronecker canonical form, quoted there from
`gantmacher1959`. **Theorem 3.3** carries Ja'Ja's count only under
`Card(K) >= deg p_1(A)`, and **Proposition 3.4** is the counterexample that
shows the condition cannot be dropped: `(E_3, A)` over `GF(2)` with `A` the
companion matrix of `x^3 + x + 1` has rank at least 5, which is the pencil this
repository's own table settles at exactly 5 against a `GF(q)` count of 4.

**Theorem 3.5 is the one this module should be computing and is not.** With `k`
the number of invariant polynomials `p_i(A)` that do not factor into *distinct*
linear factors over `K`, Ja'Ja's Theorem 3.3 gives `rank_K(E^n; A) <= n + k`
under the cardinality hypothesis, and Sumi, Miyazaki and Sakata prove
`rank_K(E^n; A) >= n + k` **with no hypothesis at all**, where Ja'Ja' had the
reverse inequality only when `p_1(A)` splits into linear factors. So `n + k` is a
proved lower bound over every field, and it is *exact* whenever
`Card(K) >= deg p_1(A)`, which is a condition a program can check.

**`gantmacher1959`**: F. R. Gantmacher. *The Theory of Matrices*, Vols. 1 and 2.
Translated by K. A. Hirsch. Chelsea, New York, 1959. Where Kronecker's 1890
theory of pencils is stated and proved, and the source `sumi2009` quotes for it.
Volume 2, **Chapter XII §4, equation (30)** is the canonical form of a pencil in
the most general case, `{0; L_ε…; L_ηᵀ…; N…; J + λE}`. **Chapter XII §5, Theorem
5 (Kronecker)** is the classification: two pencils of the same shape are
strictly equivalent *if and only if* they have the same minimal indices and the
same finite and infinite elementary divisors. That is what licenses
[`kronecker_structure.h`](pencil_rank/kronecker_structure.h) computing four
counts and never assembling the form.

**§5 also settles the one thing that would otherwise be an assumption**: *"L_ε
has no elementary divisors, since among its minors of maximal order ε there is
one equal to 1 and one equal to λ^ε"*, the same holding for its transpose, so
the elementary divisors of a pencil are exactly those of its regular kernel.
[`pencil_divisors.h`](pencil_rank/pencil_divisors.h) reads the regular part
without isolating it on the strength of that sentence.

Volume 1, **Chapter VI**, *Equivalent Transformations of Polynomial Matrices.
Analytic Theory of Elementary Divisors*, is where the diagonalisation
`pencil_divisors.cpp` runs half of is proved: **§3, Theorem 3**, p. 141, that a
rectangular polynomial matrix is always equivalent to a canonical diagonal one
whose entries are its invariant polynomials. **Not §2 and not Theorem 1**, whose
Theorems 1 and 2 are about one-sided operations and a triangular form.
**Gantmacher never calls it the Smith form**; the name is `smith1861`'s and
attaching it is ours.

**`smith1861`**: H. J. S. Smith. *On Systems of Linear Indeterminate Equations
and Congruences.* Philosophical Transactions of the Royal Society of London
**151** (1861), item XV, 293-326. Where the diagonal normal form is first
obtained, **over the integers**. Named here for the name and not cited in code,
because what [`pencil_divisors.h`](pencil_rank/pencil_divisors.h) needs is the
form over `GF(p)[x]` and that is `gantmacher1959`'s Volume 1.

**It has no numbered theorems.** The paper runs to twenty numbered **Articles**
and sets its theorems in quotation marks without numbering them, so a citation
of the shape `[smith1861, Thm. n]` would be to something that does not exist.
The transformation is **Art. 12** (p. 311) and **Art. 14**, equation (67)
(p. 314); **Art. 16** is what makes the diagonal entries a divisibility chain,
each being itself a greatest common divisor of quotients of minors.

**`byrne2021`**: E. Byrne, G. Cotardo. *Bilinear Complexity of 3-Tensors Linked
to Coding Theory.* [arXiv:2103.08544](https://arxiv.org/abs/2103.08544), 2021.
Uses the same characterisation as `brockett1978`, under names worth knowing
because they are the ones a search finds: `span(T)` is the **first slice space**,
a matrix space generated by rank-one matrices is **perfect**, and a set of
independent rank-one matrices whose span contains the first slice space is a
**perfect base**. Their Lemma 2.7, that a tensor has rank at most `R` exactly
when an `R`-base exists, is the equivalence
[`canonical_factorisation/`](canonical_factorisation/README.md) is built on,
credited there to Bürgisser-Clausen-Shokrollahi Proposition 14.45.

**Theorem 5.4 is the other numbered result cited to it, and it was read.**
*"(Tensor-rank bound). We have `trk(C) ≥ dim_{F_q}(C) + d(C) − 1`"*, over `F_q`
throughout section 5. Its two symbols are the paper's own: the code is a slice
space in the sense of **Definition 2.1**, *"the first slice space of `X` ... the
span of `X_1, …, X_k`"*, and `d` is **Definition 5.3**, *"the minimum (rank)
distance ... `min{rk(X) : X ∈ C, X ≠ 0}`"*. So `k` and `d` mean in
[`rank_metric_bound/`](rank_metric_bound/README.md) what they mean here. Byrne
and Cotardo attribute the inequality itself to Kruskal (1977); **that paper was
not read**, and the module's header says so rather than passing the attribution
on as checked.

**`bnrs2019`**: E. Byrne, A. Neri, A. Ravagnani, J. Sheekey. *Tensor
Representation of Rank-Metric Codes.* SIAM Journal on Applied Algebra and
Geometry **3** (2019), no. 4, 614-643,
[doi:10.1137/19M1253964](https://doi.org/10.1137/19M1253964),
[arXiv:1904.05227](https://arxiv.org/abs/1904.05227). Reads a matrix code as the
first slice space of a 3-tensor, the same object `byrne2021` and `brockett1978`
name, and is where `byrne2021` gets the coding-theoretic form of Kruskal's bound
from (their reference [7]).

**What was read.** Section 3, which opens "In this section, F denotes an
arbitrary field", so nothing in it asks for characteristic zero. **Corollary
4.14(2)**, that pulling a code back along a rank-`R` decomposition gives an
`[R, k, >= d]` block code, which is the hypothesis
[`rank_metric_bound.h`](rank_metric_bound/rank_metric_bound.h) hands to Griesmer.
**Corollary 4.15**, their own proof of `trk >= dim + d - 1` that never leaves a
finite field, so the Kruskal form here does not rest on reading a 1977 paper
about the reals correctly. And their **Theorem 3.6**, which is Kruskal's bound
and cites Kruskal 1977 **Corollary 1** for it, the same number `byrne2021` gives
it: two papers, one attribution, neither of them the original.

**What was not read, and what is therefore not claimed.** The numbering was
checked in **arXiv:1904.05227v1**; the SIAM version is paywalled and was not
opened, so `[bnrs2019, Cor. 4.14(2)]` cites the preprint's numbering and may not
be the journal's. Corollary 4.14 is stated for `V` and `W` of full rank, that is
for a concise tensor, and **that hypothesis is not what the code relies on**:
`rank_metric_bound.h` derives the same block code by restricting to a complement
of the contraction kernel, which needs no conciseness, so the paper is cited for
the code's parameters and not for the route to them. **Griesmer's name appears
nowhere in this paper**: 4.14(2) produces the block code and stops, and applying
the Griesmer bound to it is ours.

**`hastad1990`**: J. Håstad. *Tensor rank is NP-complete.* Journal of
Algorithms **11** (1990), no. 4, 644-654.
[doi:10.1016/0196-6774(90)90014-6](https://doi.org/10.1016/0196-6774(90)90014-6),
author's copy at <https://www.csc.kth.se/~johanh/tensorrank.pdf>.
Deciding tensor rank is NP-complete over every finite field and NP-hard over the
rationals. Lemma 2 is the reduction from 3SAT implemented in
[`satisfiability/formula_to_tensor.h`](satisfiability/formula_to_tensor.h): a
formula of `n` variables and `m` clauses becomes a tensor of rank `4n + 2m`
exactly when it is satisfiable.

**`swernofsky2018`**: J. Swernofsky. *Tensor Rank is Hard to Approximate.*
APPROX/RANDOM 2018, LIPIcs **116**, 26:1-26:9,
[doi:10.4230/LIPIcs.APPROX-RANDOM.2018.26](https://doi.org/10.4230/LIPIcs.APPROX-RANDOM.2018.26).
Approximating the rank of a 3-tensor within `1 + 1/1852 - delta` is NP-hard **over
any field**, so it holds over `GF(p)` and bites here rather than being a fact about
the reals.

**This is what the descent's missing guarantee is made of.**
[`descent_search/correctness.md`](descent_search/correctness.md) says there is no
approximation ratio and nothing here checks one, which reads as an admission and
is a citable structural fact: no polynomial-time heuristic can promise better than
that factor unless P = NP. It bounds every heuristic in this repository at once,
including any future one.

**`kyrillidis2025`**: Y. Cai, Z. Zhang, et al. *Massively Parallel Continuous
Local Search for Hybrid SAT Solving on GPUs.* AAAI 2025,
[arXiv:2308.15020](https://arxiv.org/abs/2308.15020). FastFourierSAT, and the
reason a GPU does not help this repository: it accelerates **continuous local
search**, which finds satisfying assignments and cannot refute, while the
expensive half here is the refutation. The author list is from the arXiv record
and the first name may not be the one to cite by.

**The key is not used anywhere.**
[`positioning/hardware-and-parallelism.md`](positioning/hardware-and-parallelism.md)
carries the argument, and records that GPU CDCL is reported as slower than CPU
CDCL, but it names the system in prose as FastFourierSAT and cites no key, so a
reader there has nothing to look up. This entry said it was named there, which
was the wrong half of that sentence to state.

**`delaurentis2026`**: G. De Laurentis, J. Franklin. *Linac: linear algebra with
CUDA over finite fields.* [arXiv:2605.25863](https://arxiv.org/abs/2605.25863),
2026. A high-performance, open-source, parallel implementation of Gaussian
elimination over finite fields and over floating point on GPUs, written for the
analytic reconstruction of scattering amplitudes in quantum field theory, on the
argument that Gaussian elimination is cubic, is a bottleneck, and is inherently
parallel.

**This is the honest prior art for this repository's GPU claim**, and it narrows
it rather than sinking it. [`gpu_leaf/README.md`](gpu_leaf/README.md) measures a
leaf kernel at 3962x one core, and
[`positioning/hardware-and-parallelism.md`](positioning/hardware-and-parallelism.md)
closes with *"Nothing in this literature reports an exact finite-field rank
search on a GPU"*. That sentence stands as written, because a rank search is not
Gaussian elimination; what it must not be read as saying is that finite-field
linear algebra on a card is unexplored ground. It is published, packaged and
maintained by other people, and only the search on top of it is ours.

**It is not a drop-in.** Its fields are prime fields, integers modulo a prime,
where [`pool_scan.cu`](gpu_leaf/pool_scan.cu) and
[`subspace_walk.cu`](gpu_leaf/subspace_walk.cu) are built out of GF(2) as 16-bit
masks and exclusive or, which is the whole reason nothing is transferred per
element. **Only the abstract was read; the code was not**, so what it would be
worth for the `GF(p)` half of this repository, which is the half with no kernel,
is not something this entry settles.

**`schaefer2018`**: M. Schaefer, D. Štefankovič. *The Complexity of Tensor
Rank.* Theory of Computing Systems **62** (2018), 1161-1174.
[preprint](https://www.cs.rochester.edu/~stefanko/Publications-new/J36.pdf).
Tensor rank over a field `F` is polynomial-time equivalent to the existential
theory of `F`, which gives NP-complete over finite fields, `∃ℝ`-complete over
the reals and `∃ℚ`-complete over the rationals, where decidability is open. The
reason [`satisfiability/complexity.md`](satisfiability/complexity.md) exists.

**`hillar2013`**: C. J. Hillar, L.-H. Lim. *Most Tensor Problems are NP-hard.*
Journal of the ACM **60** (2013), no. 6, article 45,
[arXiv:0911.1393](https://arxiv.org/abs/0911.1393). Extends Håstad's hardness to
`ℝ` and `ℂ`, and the source of the shorthand that flattens the per-field
picture.

**`buss1999`**: S. Buss, G. Frandsen, J. Shallit. *The computational complexity
of some problems of linear algebra.* 1999. **MinRank**: given matrices over a
finite field, is some combination of them of rank at most `r`? It is
NP-complete, and it is the inner question a rank search asks over and over.
Every leaf of the searches here is MinRank at `r = 1` over the span in hand,
which is why the leaf and not the branching is where the time goes:
[`rank_one_basis.h`](exhaustive_search/rank_one_basis.h) chooses between two
ways of asking it per call because neither is cheap.

**`huang2023`**: H. Huang, J. M. Landsberg. *On linear spaces of matrices of
bounded rank.* [arXiv:2306.14428](https://arxiv.org/abs/2306.14428), 2023.
Spaces of bounded rank three were classified in 1983 and the rank-four case was
open; this gives the classification of the **basic** spaces of bounded rank
four, of which there are **exactly four up to isomorphism**, and exhibits the
first non-classical examples of such a space.

**The object it classifies is the object the search carries.** The subspace `V`
held at every node of
[`expand_subspace`](exhaustive_search/exhaustive_search.h) is a linear space of
matrices, and the leaf test is a question about which of its elements have rank
one:
[`generating-candidates-from-the-span.md`](exhaustive_search/generating-candidates-from-the-span.md)
writes that as `deficit(V) == 0`. **Only the abstract was read**, and the base
field the classification is stated over was not checked, so nothing here quotes
a result of it; what the paper supplies is the name of the object and the
reason to expect its structure to be worth knowing, not a bound. The record
carries no journal-ref. Its own title field reads *"On Linear spaces of of
matrices bounded rank"*, a typo in the arXiv metadata; the form quoted above is
the one the abstract uses.

**`faugere2013`**: J.-C. Faugère, M. Safey El Din, P.-J. Spaenlehauer. *On the
complexity of the generalized MinRank problem.* Journal of Symbolic Computation
**55** (2013), 30-58, [arXiv:1112.4411](https://arxiv.org/abs/1112.4411).
The algorithmic half of `[buss1999]`. The points where a polynomial matrix has
rank at most `r` are the zeroes of a **determinantal ideal**, the one generated
by all `(r+1)`-minors of the matrix, and this gives complexity bounds for
computing a Gröbner basis of it, along with the families of generalized MinRank
problems where the arithmetic cost of solving is polynomial in the number of
solutions.

**`bardet2025`**: M. Bardet, A. Gilard. *Computation of the Hilbert Series for
the Support-Minors Modeling of the MinRank Problem.*
[arXiv:2502.12721](https://arxiv.org/abs/2502.12721), 2025. MinRank has three
main algebraic modellings, which this names as Kipnis-Shamir, Minors and
Support-Minors, and the Hilbert series of a modelling is what prices a Gröbner
basis over it. Faugère et al. did the Minors modelling in 2010; Bardet et al.
gave the first terms of the Support-Minors series in 2020 heuristically and
experimentally; this proves the **complete** series for generic instances.

**Both of those hold for generic instances, and that caveat is the load-bearing
one here.** A matrix multiplication tensor is about the least generic object in
this subject: its slices are sparse matrices of zeroes and ones, and it carries
a symmetry group large enough that
[`orbit_reduction/`](orbit_reduction/README.md) exists to quotient by it, which
is exactly the hypothesis both complexity statements exclude. They are a
reference point for what solving MinRank algebraically is known to cost, not a
forecast of what it would cost on [`fixtures/`](fixtures/README.md). **Neither
was read past its abstract**, and no Hilbert series has been computed here for
any shape in this repository. Where all three of these entries land, and what
the leaf test is called once it is named properly:
[`state-of-the-art/rank-one-elements-of-a-subspace.md`](state-of-the-art/rank-one-elements-of-a-subspace.md).

## The exact search

**`bdez2012`**: R. Barbulescu, J. Detrey, N. Estibals, P. Zimmermann.
*Finding Optimal Formulae for Bilinear Maps.* WAIFI 2012, Bochum.
[doi:10.1007/978-3-642-31662-3_12](https://doi.org/10.1007/978-3-642-31662-3_12),
[hal-00640165v2](https://inria.hal.science/hal-00640165v2).
Algorithm 1 is the search over subspaces rather than subsets, which
[`exhaustive_search/exhaustive_search.h`](exhaustive_search/exhaustive_search.h)
implements. Its Tables 1-4 are the published ranks the fixtures are checked
against. §4.5 says only that *"duplicates have to be detected and removed"*, plain
duplicates and not duplicates up to the group, which this entry claimed for a
while; it is the paper's **Conclusion** that names using the symmetries of the
problem, and neither place says how. `[mckay1998]` is how.

**`mckay1998`**: B. D. McKay. *Isomorph-free exhaustive generation.* J. Algorithms
26(2):306-324, 1998.
[doi:10.1006/jagm.1997.0898](https://doi.org/10.1006/jagm.1997.0898). Canonical
augmentation: give each object a group-invariant canonical parent and accept an
extension only from that parent's class, so every isomorphism class is generated
exactly once with **no memory of what has been generated**. Implemented in
[`oracle_guided_search/canonical_parent.h`](oracle_guided_search/canonical_parent.h).
The `nauty` lineage. The refinement-based canonical labelling that makes the
invariant cheap was long the part not implemented here, which
[`deduplication-cost.md`](oracle_guided_search/deduplication-cost.md) measures the
absence of; it now exists as
[`pool_set_canon.h`](oracle_guided_search/pool_set_canon.h), a reduction to
`[linton2004]` through `[permlib]` rather than an implementation, and is not yet
the parent test's canonical form.

**`linton2004`**: S. A. Linton. *Finding the smallest image of a set.* ISSAC 2004,
229-234, [doi:10.1145/1005285.1005319](https://doi.org/10.1145/1005285.1005319).
The canonical image of a set under a **prescribed** permutation group, which is
the primitive [`pool_set_canon.h`](oracle_guided_search/pool_set_canon.h) reduces
to, and the name the field gives the problem `mckay1998`'s canonical parent needs
solved cheaply.

**Not read.** It is named through `[permlib]`, whose
`include/permlib/search/orbit_lex_min_search.h` cites it by title and year as what
that file implements, and through Jefferson et al.'s account of it, the same way
`[gabriel1972]` is named through `[brion2008]`. No numbered result of it is quoted
anywhere here, and none should be until somebody has the paper. What matters for
this repository is settled without it: the group is prescribed rather than
discovered, which is why `nauty` is the wrong instrument and not merely a slower
one, since it would canonise under `Sym(left) x Sym(right)`, merge orbits `G`
separates and yield a floor nothing downstream could catch.

**`permlib`**: T. Rehn. *PermLib*, a C++ library for permutation group
computations, [github.com/tremlin/PermLib](https://github.com/tremlin/PermLib),
BSD 3-clause. Vendored at commit `2b4e468`, the `include/` tree unmodified, in
[`vendor/permlib/`](vendor/permlib/README.md) with its licence and authors. Read:
`permlib_api.h`'s `smallestSetImage` and the `OrbitLexMinSearch` it calls, and
enough of `permutation.h` and `transversal/` to know that the transversal is a
Schreier tree rather than explicit, which is what makes a degree of 261 121
affordable. Not read: everything else.

**`covanov2019`**: S. Covanov. *Improved Method for Finding Optimal Formulae
for Bilinear Maps in a Finite Field.*
[arXiv:1705.07728v3](https://arxiv.org/abs/1705.07728), 2018.
Definition 7 and Definition 13 are the automorphism action and the setwise
stabiliser; Algorithm 3 is `BDEZStab`; Definitions 20 and 22 and Algorithm 4 are
the covering-sets method; Propositions 28 and 29 are the stems for the short
product and the matrix product.

**Theorem 17 and Corollary 18 are the two `orbit_reduction/` leans on hardest,
and they say different things.** Read from v3. **Theorem 17** is the closed-form
stabiliser, *"for the group action `M · (X, Y) ↦ X^T M Y`, the subgroup
stabilizing the vector space `T_{p,q,r}` can be described as the group given by
the pairs `(P ⊗ R^T, Q ⊗ R^{-1})` for `P ∈ GL_p`, `R ∈ GL_q`, `Q ∈ GL_r`"*, which
is the group [`pool_orbits.h`](orbit_reduction/pool_orbits.h) acts by.
**Corollary 18** is *"the elements of `T_{p,q,r}` of a given rank lie in the same
orbit under the action of `Stab(T_{p,q,r})`"*, which is a statement about
elements **of the target subspace**, and the rank-one pool is not inside it. The same header
records that the pool orbits were once attributed to it and are not its.

**`covanov2018`**: S. Covanov. *Algorithmes de Multiplication: Complexité
Bilinéaire et Méthodes Asymptotiquement Rapides.* Thèse, Université de Lorraine,
2018. NNT 2018LORR0057, [tel-01825744](https://theses.hal.science/tel-01825744v1).
The long form of `covanov2019`, in French, open access. **It has now been read**,
and the correspondence is: §1.3 *RP-automorphisms* is the paper's §3.1, with
Def. 1.16 = Def. 7, Prop. 1.17 = Prop. 8, Prop. 1.18 = Prop. 9 and
Rem. 1.20 = Not. 11; §2.2.4 is the paper's §3.3, with Prop. 2.6 = Prop. 14 and
Alg. 6 = Alg. 3. Where they agree, cite the paper. Three places where they do
not:

- **`[covanov2018, Def. 1.16]` defines a larger group than the paper does.**
  `RPA_{m,n}` there is *"the smallest group containing `GL(K^m) × GL(K^n)` and,
  if `m = n`, the transposition `τ`"*, where `[covanov2019, Def. 7]` is the pair
  alone and `[covanov2019, Rem. 10]` says *"for simplicity, we do not take into
  account the possible transposition τ"*. The code implements the paper's group,
  so the name `RPA` should not be used for it.
- **`[covanov2018, Prop. 1.19]` has no counterpart in the paper**: the elements
  of `RPA_{m,n}` are the *only* rank-preserving automorphisms. It is a statement
  about the group **with** `τ` and is false of the pair group when `m = n`. The
  thesis does not prove it; its proof is a pointer to `[burichenko2014]`.
- **`[covanov2018, Alg. 6]` line 11 recurses on `V` and not on
  `V ⊕ Span({φ})`**, so the subspace never grows and the base case's
  `dim V = r` cannot be reached. `[covanov2019, Alg. 3]` line 11 has the same
  slip. It is typographical, and
  [`orbit_plan/the-algorithm.md`](orbit_reduction/orbit_plan/the-algorithm.md)
  records that the pseudocode there departs from both.

**`burichenko2014`**: V. P. Burichenko. *On symmetries of the Strassen
algorithm.* [arXiv:1408.6273](https://arxiv.org/abs/1408.6273), 2014. Cited here
for one thing only: it is where `[covanov2018, Prop. 1.19]` sends the reader for
its proof, in the formalism of order-3 tensors. **Not read.** Nothing in this
repository rests on it; it is named so that the pointer does not dead-end.

**`gabriel1972`**: P. Gabriel. *Unzerlegbare Darstellungen I.* Manuscripta
Mathematica **6** (1972), 71-103, with a *Berichtigung* at **6**, 309-310. A
quiver has finitely many indecomposable representations exactly when its
underlying graph is a union of simply-laced Dynkin diagrams, and then the
indecomposables correspond to the positive roots. **Not read**: it is in German
and no reachable scan was found. What was read is `[brion2008]`, and that is
what the code cites.

**`brion2008`**: M. Brion. *Representations of quivers.* Notes of the summer
school *Geometric Methods in Representation Theory*, Grenoble 2008,
[www-fourier.univ-grenoble-alpes.fr/~mbrion/notes_quivers_rev.pdf](https://www-fourier.univ-grenoble-alpes.fr/~mbrion/notes_quivers_rev.pdf).
**Theorem 2.4.3** is `[gabriel1972]` in a form that can be cited: (ii)
indecomposables correspond to the positive roots, (iii) *"every indecomposable
representation is uniquely determined by its dimension vector, up to
isomorphism"*, (iv) there are finitely many. The remark after it names the
positive roots of type `A_r` as the intervals `Σ_{ℓ=i}^{j} ε_ℓ`, `1 ≤ i ≤ j ≤ r`,
which is what makes `A_3` have six indecomposables and is what
[`pool_orbits.h`](orbit_reduction/pool_orbits.h) counts.

**The field is the one thing this does not settle.** Brion works over an
algebraically closed field throughout and says on his first page that *"Gabriel's
theorem holds over an arbitrary field"*, pointing at Benson, *Representations
and Cohomology I*, Cambridge Studies in Advanced Math. 30, 1991, §4.7. **Benson
has not been read.** That one sentence is what the `GF(p)` case here rests on,
and it is the only unchecked link in that argument.

**`buchfulton1999`**: A. S. Buch, W. Fulton. *Chern class formulas for quiver
varieties.* Inventiones Mathematicae **135** (1999), 665-687,
[arXiv:math/9804041](https://arxiv.org/abs/math/9804041). Their subject is
degeneracy loci, not orbits, but their **condition (1.2)** on a rank array
`(r_ij)` for an equioriented `A_n` quiver, `r_ij ≤ r_{i,j-1}`, `r_ij ≤ r_{i+1,j}`
and `r_{i+1,j-1} - r_{i,j-1} - r_{i+1,j} + r_ij ≥ 0`, is at `n = 2` exactly the
range `max(0, rU+rV-m) ≤ rank UV ≤ min(rU, rV)` that
[`pool_orbits.h`](orbit_reduction/pool_orbits.h) loops over, arrived at
independently. They add that rank arrays satisfying (1.2) *"are the only
conditions that can actually occur"*, which is the existence half.

**`nakatsukasa2017`**: Y. Nakatsukasa, T. Soma, A. Uschmajew. *Finding a
low-rank basis in a matrix subspace.* Mathematical Programming **162** (2017),
no. 1, 325-361, [arXiv:1503.08601](https://arxiv.org/abs/1503.08601). The
closest existing work to the search this repository runs at every leaf, stated
as its own problem: given a matrix subspace, find a basis of it made of low-rank
matrices.

**Section 2 of it is step 1 of the descent, and this repository called that step
new for a while.** Their problem is *"minimize rank(X_1)+...+rank(X_d) subject to
span{X_1,...,X_d} = M"*, which is step 1's objective; their Algorithm 1 is the
greedy; and the matroid observation is theirs, in the sentence *"this algorithm
can be understood as a greedy algorithm for an infinite matroid of finite rank"*.
Theorem 2.1 is the exactness that
[`article/bilinear-rank.pdf`](article/bilinear-rank.pdf) states as Theorem 3.1,
and **Corollary 1 is stronger than what the article claims**: not merely that the
optimal value is tie-break independent but that *"any other basis of lowest rank
takes the same ranks up to permutation"*, so the whole multiset of ranks is an
invariant of the subspace. They also state the caveat this repository does not,
that the greedy's oracle is itself NP-hard, so the theorem makes the *choice*
exact and not the problem tractable.
Their route is a nuclear-norm relaxation, soft singular-value thresholding and
alternating projections, over the reals throughout. **None of that machinery has
a finite-field analogue**, since a nuclear norm needs singular values and a
threshold needs an ordering and `GF(p)` supplies neither, which is why this line
of work has no `GF(p)` descendant and why
[`rank_one_basis_of`](exhaustive_search/rank_one_basis.h) answers the same
question by enumeration instead. Worth naming for what it rules out: the
continuous method is not waiting to be ported.

**`oxley`**: J. Oxley. *Matroid Theory*, 2nd edition. Oxford Graduate Texts in
Mathematics 21, Oxford University Press, 2011. The general form of what
`nakatsukasa2017` states for this problem in particular, and the book
[`article/bilinear-rank.pdf`](article/bilinear-rank.pdf) already carried in its
bibliography while this file did not. **Proposition 1.1.1** is that linear
independence of the columns of a matrix over a field is a matroid, the vector
matroid `M[A]`. **Lemma 1.8.3** is that the greedy returns a maximal independent
set of maximum weight, and the remark after **1.8.2** is that solving for `−w`
instead gives one of minimum weight, which is what makes ascending rank the
right order in
[`minimum_weight_basis.h`](descent_search/minimum_weight_basis.h).
**Theorem 1.8.5** is the converse, that the greedy works for matroids and for
nothing else.

**The name is not Oxley's.** §1.8 never says "Rado-Edmonds"; it credits the
first published proof of Theorem 1.8.5 to Borůvka (1926). The name this
repository uses comes from the optimisation literature, for R. Rado, *Note on
independence functions*, Proc. LMS (3) **7** (1957), 300-320, and J. Edmonds,
*Matroids and the greedy algorithm*, Mathematical Programming **1** (1971),
127-136. Those two are named here and cited nowhere, because the numbered result
the code needs was read in Oxley and not in them.

**`knuth2011`**: D. E. Knuth. *The Art of Computer Programming*, Volume 4A:
*Combinatorial Algorithms, Part 1*. Addison-Wesley, 2011. **§7.2.1.1, Algorithm
H** is loop-free reflected mixed-radix Gray generation: it visits every
mixed-radix digit string exactly once, changing one digit by `+1` or `-1` per
step, and names the digit to change in constant time through an array of focus
pointers rather than by scanning. Implemented at one radix, a field's
characteristic, in
[`reflected_gray_walk.h`](exhaustive_search/reflected_gray_walk.h).

**The order is why the general leaf costs one row addition an element.** Every
element of a `dim`-dimensional subspace over `GF(p)` is a digit string read as
coefficients on a basis; counting in base `p` moves several digits at a step and
forces the combination to be rebuilt from the basis, whereas this order moves
one, so the update is a row added or subtracted with no field multiplication in
it. **Table 1 of §7.2.1.1** is the reflected code itself and the ± property is
its defining one; the loop-free part is Algorithm H's, and
[`tests/test_reflected_gray_walk.cpp`](exhaustive_search/tests/test_reflected_gray_walk.cpp)
asserts both against the implementation.

## Deciding rank with a solver

**`heule2021`**: M. J. H. Heule, M. Kauers, M. Seidl. *New ways to multiply
3 × 3-matrices.* Journal of Symbolic Computation **104** (2021), 899-916.
The SAT encoding of tensor decomposition over `Z/2Z`, and the method that
actually produced new schemes at that size.

**`heule2019`**: M. J. H. Heule, M. Kauers, M. Seidl. *Local search for fast
matrix multiplication.* SAT 2019, [arXiv:1903.11391](https://arxiv.org/abs/1903.11391).

**`matrixchallenges`**: M. J. H. Heule. *Challenging SAT benchmarks for matrix
multiplication.*
[github.com/marijnheule/matrix-challenges](https://github.com/marijnheule/matrix-challenges).
The instances behind `[heule2019]`, which the front page names as the article
the details are in. Four challenges at `⟨3,3,3⟩`: ten satisfiable formulas for
local search without streamlining constraints, ten for proving subproblems at 23
multiplications unsatisfiable, one blocking type-3 terms from the last summand,
and one asking for 22.

**This is the standard [`satisfiability/`](satisfiability/README.md) should be
tested against, and it has not been.**
[`satisfiability/measurements.md`](satisfiability/measurements.md) times this
repository's encoders on this repository's own fixtures against this
repository's own exhaustive search, which is a consistency check between two
things written here and not a comparison with anyone. A published benchmark
suite, whose instances someone else generated and whose difficulty someone else
calibrated, is the missing half of that. **Read from the front page only**: no
instance was downloaded, none was run, and the front page states no licence.

**`ozdemir2023`**: A. Ozdemir, G. Kremer, C. Tinelli, C. Barrett.
*Satisfiability Modulo Finite Fields.* CAV 2023,
[eprint.iacr.org/2023/091](https://eprint.iacr.org/2023/091). The decision
procedure for prime fields implemented in cvc5, which is how `GF(p)` for `p > 2`
is decided here without hand-writing field arithmetic into clauses. The theory's
SMT-LIB surface is [arXiv:2407.21169](https://arxiv.org/abs/2407.21169).

**`morgado2013`**: A. Morgado, F. Heras, M. Liffiton, J. Planes, J. Marques-Silva.
*Iterative and core-guided MaxSAT solving: a survey and assessment.*
Constraints **18** (2013), 478-534. The survey that names the search this module
does: finding an optimum by a sequence of decision queries, as linear UNSAT-SAT,
linear SAT-UNSAT and binary search. It is cited here for that taxonomy, for the
call-count pricing this module borrows at **Table 6 p. 498** and §4.3 p. 497, and
for recording that linear UNSAT-SAT has no known MaxSAT implementation while being
this module's default. **Not for a verdict against binary search:** its own
assessment at §7 p. 520 puts BIN ahead of linear UNSAT-SAT, 261 solved against
185. The review positioning the strand against it is
[`satisfiability/search-in-the-literature/`](satisfiability/search-in-the-literature/README.md).

**`heras2011`**: F. Heras, A. Morgado, J. Marques-Silva. *Core-guided binary
search algorithms for maximum satisfiability.* AAAI 2011, 36-41. Where the verdict
quoted in
[`satisfiability/search-in-the-literature/what-the-survey-says.md`](satisfiability/search-in-the-literature/what-the-survey-says.md)
originates: binary search "is optimal in terms of the number of calls to a SAT
oracle" yet "has seldom been used in practical MaxSAT solvers", because relaxing
every clause makes the cardinality constraints complex enough to hurt the solver
on exactly the unsatisfiable calls it needs. **`morgado2013` does not restate it**,
though three of its five authors are shared: neither "seldom" nor "rarely" occurs
in the survey at all, so the verdict must be cited here or not at all. This paper
is also about *core-guided* binary search, BIN-C and BIN-C-D; plain binary search
is Fu and Malik, SAT 2006, LNCS 4121:252-265, credited at `morgado2013` §1.3
p. 482. Its Theta(log W) call count is the bound the measurement in
[`satisfiability/search/`](satisfiability/search/README.md) does not contradict and
does not benefit from.

## Proving that no smaller decomposition exists

The other direction from a search for schemes, and the direction
[`satisfiability/`](satisfiability/) exists for. A refutation here is measured
against these.

**`wang2026`**: C. Wang. *Automated Lower Bounds for Bilinear Complexity over
Finite Fields.* [arXiv:2603.07280](https://arxiv.org/abs/2603.07280), first
posted March 2026 and **cited here at v10, 30 July 2026**.
Classifies the orbits of constraint subspaces under a group of rank-preserving
symmetries acting on one argument, runs a dynamic program over the orbits
combining flattening, degenerate reduction, forced product and substitution with
backtracking, and emits a certificate a separate verifier rechecks. **Raises
`⟨3,3,3⟩` over F₂ from 19 to 20**, plus `⟨2,3,4⟩` to 19, `⟨3,3,4⟩` to 25 and
`⟨3,4,4⟩` to 29, and eighteen new bounds for polynomial multiplication over F₂
and F₃, cyclic convolution of length 7 over F₂ among them at 13, which meets the
best known upper bound and so settles that one outright. Implemented and public:
MIT-licensed C++ at
[github.com/wcgbg/tensor-rank-lower-bound](https://github.com/wcgbg/tensor-rank-lower-bound).

**The version is part of the citation here, unusually for this file**, because
this preprint's table grew across ten revisions rather than being corrected in
place: v1 carries the matrix multiplication formats alone, with neither `⟨3,3,4⟩`
nor `⟨3,4,4⟩` in it, and the polynomial and cyclic convolution bounds arrive
later. Four of the six fixtures in
[`fixtures/published-targets.md`](fixtures/published-targets.md) are aimed at
numbers that exist only from a later version, so quoting the bare arXiv
identifier would name a document that does not contain them.

**`blaser2003`**: M. Bläser. *On the complexity of the multiplication of matrices
of small formats.* Journal of Complexity **19** (2003), no. 1, 43-60. The source
of the `⟨3,3,3⟩` bound of 19 that stood for twenty-three years, and of the
`⟨3,3,4⟩` bound `wang2026` improves.

## Searching for decompositions, which is the other direction

Where this repository stands against all of these:
[`positioning/`](positioning/README.md).

**`alphatensor2022`**: A. Fawzi et al. *Discovering faster matrix multiplication
algorithms with reinforcement learning.* Nature **610** (2022), 47-53.
AlphaZero applied to decomposition as a single-player game; 14 236 non-equivalent
schemes for `⟨4,4,4⟩`.

**`kauers2023`**: M. Kauers, J. Moosbauer. *Flip Graphs for Matrix
Multiplication.* Proc. ISSAC 2023, 381-388,
[arXiv:2212.01175](https://arxiv.org/abs/2212.01175).
Rewriting a working decomposition rather than searching for one: a random walk
on a graph whose vertices are decompositions, where a flip preserves the rank
and a reduction lowers it, which is what
[`flip_graph.h`](flip_graph/flip_graph.h) implements. `⟨5,5,5⟩` in 95 with no
machine learning.

**Definition 2 and Proposition 3 are the reduction.** Read: a scheme is
*reducible* when some nonempty `I` has `dim⟨A^(i)⟩_{i∈I} = 1` and
`{B^(i)}_{i∈I}` linearly dependent, or the same with any other pair of the three
modes, and **Proposition 3** is that a reducible scheme of rank `r` yields one of
rank `r - 1`. `flip_graph.cpp` detects the free case of it, two terms agreeing on
two modes, widened to agreement up to a scalar in each.

**`moosbauer2025`**: J. Moosbauer, M. Poole. *Flip Graphs with Symmetry and New
Matrix Multiplication Schemes.* ISSAC 2025,
[arXiv:2502.04514](https://arxiv.org/abs/2502.04514). `⟨5,5,5⟩` in 93 and
`⟨6,6,6⟩` in 153, by taking the tensor's symmetries into the walk.

**`chen2025`**: S. Chen, M. Kauers. *Flip Graphs for Polynomial Multiplication.*
[arXiv:2502.06264](https://arxiv.org/abs/2502.06264), 2025. The flip graph
applied to **polynomial multiplication**, which is this repository's own
subject: every `.tensor` fixture here is a polynomial product. Their walk finds the schemes and **a SAT solver proves them optimal** (their
Theorem 7), which is exactly the division of labour between the two strands
here. **Their `(n,m)` are degrees, not term counts**: Toom-Cook gives rank
`n+m+1`, so their `(n,m)` is this repository's `(n+1)x(m+1)`. Their proven
list, translated, is 2x2, 2x3, 2x4, 2x5, 2x6, 3x3, 3x4, 3x5 and 4x4 over `Z2`.

**Their closing question is this repository's opening, and so is the obstacle
they name for it.** Their conclusion asks for polynomial multiplication over
`Z3`, `Z5` and `Z7`, and says why the walk works best over `Z2`: for a larger
field a constant factor moves freely between the three components,
`(αu)⊗v⊗w = u⊗(αv)⊗w = u⊗v⊗(αw)`, and *"a search engine that follows a random
path in the flip graph would somehow have to cope with this freedom, and it is
unclear what is the best way of doing this"*.

**`kauers2025`**: M. Kauers, I. Wood. *Exploring the Meta Flip Graph for Matrix
Multiplication.* [arXiv:2510.19787](https://arxiv.org/abs/2510.19787), 2025.

**`ikenmeyer2025`**: C. Ikenmeyer, J. Moosbauer. *Strassen's Algorithm via Orbit
Flip Graphs.* [arXiv:2503.05467](https://arxiv.org/abs/2503.05467), 2025.
Strassen's 7 reproved from an order-6 group action, with no calculation and no
pattern matching.

**`arai2024`**: Y. Arai, Y. Ichikawa, K. Hukushima. *Adaptive Flip Graph
Algorithm for Matrix Multiplication.* Proc. ISSAC'24, 292-298,
[arXiv:2312.16960](https://arxiv.org/abs/2312.16960).
Transitions that do not strictly reduce the count, and a constrained search range.

**`perminov2026`**: A. I. Perminov. *Fast Matrix Multiplication in Small Formats:
Discovering New Schemes with an Open-Source Flip Graph Framework.*
[arXiv:2603.02398](https://arxiv.org/abs/2603.02398), code at
[github.com/dronperminov/FastMatrixMultiplication](https://github.com/dronperminov/FastMatrixMultiplication),
MIT. Bit-level encoding, OpenMP, 680 formats from `(2,2,2)` to `(16,16,16)`, and
a GPU variant. **The baseline for any flip graph number produced here.**

**`sedoglavic2024`**: A. Sedoglavic. *Yet Another Catalogue of Fast Matrix
Multiplication Algorithms.* [fmm.univ-lille.fr](https://fmm.univ-lille.fr/).
The field's running record of best known upper bounds.

**`deza2023`**: A. Deza, C. Liu, E. B. Khalil, P. Vaezipoor. *Fast Matrix
Multiplication Without Tears: A Constraint Programming Approach.* Proc. CP 2023,
LIPIcs vol. 280, [arXiv:2306.01097](https://arxiv.org/abs/2306.01097).
The Brent equations solved by constraint programming. The 2x2 and 3x3 cases are
MIPLIB 2017 benchmarks, so the formulation is standard and nothing here is new.
This repository stated the same equations for a MILP solver, measured them
against the SAT strand and the tree search, and retired the encoding:
[`state-of-the-art/rank-as-a-milp.md`](state-of-the-art/rank-as-a-milp.md).

**`alphaevolve2025`**: Google DeepMind. *AlphaEvolve: A Coding Agent for
Scientific and Algorithmic Discovery.* 2025. `⟨4,4,4⟩` in 48 multiplications over
`ℂ`, the first improvement on 49 in fifty-six years.

**`dumas2026`**: *Complex to Rational Fast Matrix Multiplication.*
[arXiv:2602.13171](https://arxiv.org/abs/2602.13171), 2026. Converts a complex
scheme to a rational one or proves none exists, generalising Dumas, Pernet and
Sedoglavic (2025).

**`yang2024`**: J. Yang. *Fixed-parameter tractability of canonical polyadic
decomposition over finite fields.*
[arXiv:2405.11699](https://arxiv.org/abs/2405.11699), 2024. Finding a rank-`R`
CPD of a 3-dimensional tensor over a finite field `F` is **fixed-parameter
tractable in `R` and `|F|`**. Theorem 1 is
`O(f(|F|, R) + poly(n₀, n₁, n₂, R))` time and `O(poly(n₀, n₁, n₂, R))` space, so
the shape of the tensor enters the cost only polynomially and everything that
explodes sits in the two parameters. **That is the complexity-theoretic
statement about exactly the question every search in this repository asks**, and
it is the one `[buss1999]`'s NP-completeness leaves open: hard in general says
nothing about hard at fixed `R` over a fixed field, which is the only regime
`decide-rank` is ever run in.

**The concrete exponent is in the body, not the abstract**, which promises *"a
nontrivial upper bound on the time complexity"* and does not state it. §3.2.3 of
the **v2** HTML reduces to a tensor of shape at most `R x R x R` and reaches
`F(2R² − 3R + 4, 2R)` for `R ≥ 4`, with
`F(n, k) := O(|F|^n / (|F|−1)^k · R^O(1))` defined in §1.1. That was read from
the HTML rendering and not the PDF, and the v1 record's comments field says some
of its proofs are copied from arXiv:2401.06857, which was not read at all.
**How that bound compares with `[yang2025]`'s at any shape here was not worked
out**, the two being stated over different reductions, and neither is
implemented in this repository.

**`yang2025`**: J. Yang. *Faster search for tensor decomposition over finite
fields.* ISSAC 2025, 132-139, [doi:10.1145/3747199.3747555](https://doi.org/10.1145/3747199.3747555),
[arXiv:2502.12390](https://arxiv.org/abs/2502.12390). Exact decision in
`O*(|F|^(min{R, Σ_{d≥2} n_d} + (R−n₀)(Σ_{d≠0} n_d)))` and polynomial space. The
venue is not on the arXiv record, which carries no journal-ref, so it was looked
up rather than copied.

**Theorem 1 is subspace extension, not deflation**, and the distinction decides
what it costs. Algorithm 1 enumerates `Y_d` for `1 ≤ d < D` only, recovering axis
0 by inverting a matrix, so a candidate is a rank-one *matrix* and the pool at
`⟨2,2,2⟩` is `(2^4−1)^2 = 225`, not `15^3 = 3375`. The paper's own state count
`Σ_{k≤2} C(225, k) = 25426` is the same tree
[`expand_subspace`](exhaustive_search/exhaustive_search.h) walks in 25399 states,
which is why the two agree to a fraction of a percent: both are `[bdez2012]`
Algorithm 1. **Deflation is the predecessor and the border case**, arXiv:2411.14676
and `yang2025thesis` Theorem 2, where the thesis explains that no border `rref`
exists so the subspace argument does not transfer.

**`yang2025thesis`**: J. Yang. *New results in canonical polyadic decomposition
over finite fields.* [arXiv:2505.09824](https://arxiv.org/abs/2505.09824), 2025.
The long form of `yang2025`, carrying the border-CPD search and the pruners. Both
of its rank-sum bounds are
[`linear_algebra/tensor_rank_sum.h`](linear_algebra/tensor_rank_sum.h): its
`ranksum` as `line_rank_sum_lower_bound_on_axis`, and the bound its Java calls
`lask`, which is **Laskowski's, Theorem 3 of this thesis**, as
`total_rank_sum_lower_bound_on_axis`: it sums the same counting inequality over
every projective point instead of along one line, and costs `|F|^n_d` against the
line bound's `|F|^(2 n_d)`.

**`rref` is not ported, and it is not what its name suggests.** Read from the
author's repository, `coolcomputery/tensor-cpd-search`, notebook `other/k-th order
rref pruning.ipynb`, because the paper's abstract does not mention it. It is a
**refutation test** rather than any reduction of the rank-one pool: contract the
tensor with `k`-tuples of vectors, wedge them, and refuse the rank when the
resulting `k`-planes fail to span enough of the Grassmannian. Its cost there is
`C(|F|^n_0, k)` tuples, so it is not a free bound, and the repository pairs it
with `ranksum` when reporting its Strassen refutation, so whether it refuses rank
6 alone is not settled by reading that README. Where it would land, and why that
place matters, is in
[`state-of-the-art/lower-bounds.md`](state-of-the-art/lower-bounds.md).

**`heule2024`**: *Ruling Out Low-rank Matrix Multiplication Tensor
Decompositions with Symmetries via SAT.*
[arXiv:2402.01011](https://arxiv.org/abs/2402.01011), 2024.

**`alman2025`**: J. Alman, R. Duan, V. Vassilevska Williams, Y. Xu, Z. Xu,
R. Zhou. *More Asymmetry Yields Faster Matrix Multiplication.* SODA 2025,
[arXiv:2404.16349](https://arxiv.org/abs/2404.16349). `ω < 2.371339`. The laser
method, which shares no machinery with anything here.

## Sparsifying the operators

**`beniamini2020`**: G. Beniamini, N. Cheng, O. Holtz, E. Karstadt, O. Schwartz.
*Sparsifying the Operators of Fast Matrix Multiplication Algorithms.*
[arXiv:2008.03759](https://arxiv.org/abs/2008.03759), 2020.
Definition 3.2 is the Ω-valid set; Algorithms 3 and 4 are the two exact oracles
in [`matrix_sparsification/oracle_sparsifier.h`](matrix_sparsification/oracle_sparsifier.h);
Algorithm 2 is the driver they feed, from `gottlieb2010`; Algorithm 6 is the
greedy in `greedy_sparsifier.h`.

**This entry claimed Claim 2.11 and the paper has no such claim.** Its numbered
claims run 2.12, 3.10, 3.11 and 3.18, and 2.12 is the complexity of a recursively
applied linear map, not an additive complexity. The additive complexity
`algorithm_cost.h` implements is `[beniamini2019, Claim 2.11]`, which is what the
header has always cited; only this file was pointing at the wrong paper.

**`beniamini2019`**: G. Beniamini, O. Schwartz. *Faster Matrix Multiplication
Via Sparse Decomposition.* SPAA 2019, pp. 11-22.
Definition 2.8 is the trilinear identity `algorithm_check.h` verifies; Claim 3.9
and Corollary 3.10 are the arithmetic complexity and its leading coefficient;
Definition 3.5 and Algorithm 2 are the decomposed recursive-bilinear algorithm.

**Claim 2.11 is here and not in `beniamini2020`**, read from the SPAA proceedings
copy: it gives the additive complexities of the encoding and decoding matrices as
`q_u = nnz(U) + nns(U) - rows(U)`, the same for `V`, and
`q_w = nnz(W) + nns(W) - cols(W)`, which is
[`algorithm_cost.h`](matrix_sparsification/algorithm_cost.h) term for term. The
paper credits the claim to `[karstadt2017]`; it is cited to here because here is
where it was read.

**`karstadt2017`**: E. Karstadt, O. Schwartz. *Matrix Multiplication, A Little
Faster.* SPAA 2017. Also JACM 67(1), 2020. The alternative-basis technique both
papers above build on, and the source of Strassen's leading coefficient dropping
from 7 to 5, that is of `⟨2,2,2;7⟩` needing 12 additions rather than 15.

**`probert1976`**: R. L. Probert. *On the additive complexity of matrix
multiplication.* SIAM J. Comput. 5(2):187-203, 1976. Fifteen additions are
necessary for any `⟨2,2,2;7⟩` algorithm in the standard basis. A bound over every
rank-7 decomposition, so it closes the standard-basis question rather than
constraining one orbit.

**`bshouty1995`**: N. H. Bshouty. *On the additive complexity of 2 × 2 matrix
multiplication.* Inf. Process. Lett. 56(6):329-335, 1995. The same bound over an
arbitrary ring, by a different method. `[karstadt2017]` gets past both by changing
basis, which is the assumption they share.

**`martensson2026`**: E. Mårtensson, P. Stankovski Wagner, J. Stapleton. *A Rank 23
Algorithm for Multiplying 3 × 3 Matrices with an Arithmetic Complexity of 59.*
[arXiv:2601.05272](https://arxiv.org/abs/2601.05272), 2025. States that both the
multiplication and the addition counts are optimal for the 2 × 2 case, and gives the
`⟨3,3,3⟩` addition record as it stood at 59.

**`karunaratne2026`**: S. Karunaratne, A. Idamekorala. *55 Additions Suffice for
3 × 3 Matrix Multiplication at Rank 23.*
[arXiv:2607.28676](https://arxiv.org/abs/2607.28676), 2026. The current record, and
provably optimal for its fixed tensor orientation. The baseline any joint search over
rank and sparsity has to name before it starts.

**`colemanpothen1986`**: T. F. Coleman, A. Pothen. *The null space problem I.
Complexity.* SIAM Journal on Algebraic and Discrete Methods **7** (1986), no. 4,
527-537, [doi:10.1137/0607059](https://doi.org/10.1137/0607059). Part II is
**8** (1987), 544-563, [doi:10.1137/0608045](https://doi.org/10.1137/0608045).
The ancestor of step 1 with Hamming weight where it has rank: a greedy
characterisation of sparsest null bases, and NP-hardness of finding one **even
when the optimal value is given**. Named here because the repository cited the
weight version of this lineage in one strand while calling the rank version its
own in another.

**`gottlieb2010`**: L.-A. Gottlieb, T. Neylon. *Matrix Sparsification and the
Sparse Null Space Problem.* APPROX/RANDOM 2010. The greedy driver that the
sparsest-independent-vector oracles are oracles for.

**`dumas2024cex`**: J-G. Dumas. *Cex_Poldet*, Maple worksheet, 27 May 2024,
unpublished; supplied directly. The determinant-polynomial
feasibility test in `matrix_sparsification/pattern_feasibility.h`, and the
counterexample fixture `fixtures/dumas_counterexample_l.matrix`.

## Finite field extensions and curves

**`rambaud2014`**: M. Rambaud. *Finding Optimal Chudnovsky-Chudnovsky
Multiplication Algorithms.* WAIFI 2014.
Its four-step roadmap in §1; Theorem 2 is the bound an interpolation system
gives; Algorithm 3 is `bdez2012`'s search restricted to symmetric forms; Tables
1 and 2 are the published bounds on `µ_sym`.

**`ballet2021`**: S. Ballet, J. Chaumine, J. Pieltant, M. Rambaud,
H. Randriambololona, R. Rolland. *On the Tensor Rank of Multiplication in Finite
Extensions of Finite Fields and Related Issues in Algebraic Geometry.* Russian
Mathematical Surveys **76** (2021), no. 1, 29-89,
[arXiv:1906.07456](https://arxiv.org/abs/1906.07456). The survey these bounds
sit in.

**Not the source of the table here**, which this entry used to say it was.
[`symmetric_bound_table.h`](curve_bounds/symmetric_bound_table.h) is
`[rambaud2014, Table 1]` transcribed, and a 2021 survey cannot be where a 2014
table came from. What this is cited for is Theorem 2.1, below.

Its §2, *Old classical results*, is also the small-field story for polynomial
multiplication, and its **Theorem 2.1** restates the field-size statement: if
`Card(F) < 2n-2`, every algorithm for `R(u)S(u) mod P(u)` has bilinear
complexity `> 2n-1`. It is `[lsw1983]` p. 287 item (iii), and `[lsw1983]` is the
better citation for it because Winograd is one of its authors.

**`winograd1977`**: S. Winograd. *Some bilinear forms whose multiplicative
complexity depends on the field of constants.* Mathematical Systems Theory
**10** (1976/77), 169-180,
[doi:10.1007/BF01683270](https://doi.org/10.1007/BF01683270). Where the
small-field obstruction to attaining `2n-1` products comes from. **Not read**:
it is behind Springer's paywall, no open copy was found, and its internal
theorem numbering is therefore unknown here. **Nothing cites it directly**; it
is reached through `[lsw1983]`, exactly as `[gabriel1972]` is reached through
`[brion2008]`, and a `[winograd1977, Thm. n]` anywhere in this repository would
be an invention.

**`lsw1983`**: A. Lempel, G. Seroussi, S. Winograd. *On the complexity of
multiplication in finite fields.* Theoretical Computer Science **22** (1983),
285-296. Read. **This is the authority on what `[winograd1977]` proved, because
Winograd co-wrote it.** Page 287 summarises the large-field results as three
unnumbered items:

- **(i)**, credited to Fiduccia-Zalcstein, Toom and `[winograd1977]`: over any
  field `μ(r) ⩾ 2n-1` and `μ(r_P) ⩾ 2n-1`, and *"if |F| ⩾ 2n − 2, these lower
  bounds are tight"*.
- **(ii)**, credited to `[winograd1977]` **alone**: every length-`2n-1`
  algorithm for the plain product multiplies `(b_i x(a_i))·(c_i y(a_i))` at
  distinct `a_i`, so *"at least 2n − 2 distinct elements from F (namely, the
  a_i) are needed ... and if |F| < 2n − 2 then μ(r) ⩾ 2n"*. **The cardinality
  consequence is drawn inside the item credited to the 1977 paper**, which is
  why calling the effect Winograd's is fair.
- **(iii)**, credited to Winograd's 1979 *On multiplication in algebraic
  extension fields*: the same for the product **modulo `P`**.

`r` is the product of two polynomials of `n` coefficients and `r_P` is that
product in `F[u]/(P)` with `deg P = n`. **Neither is a pencil**, which is the
whole of why
[`pencil_rank/the-measured-gap.md`](pencil_rank/the-measured-gap.md) cites
`[sumi2009, Thm. 3.3]` and not this.

**`kaminskibshouty1989`**: M. Kaminski, N. H. Bshouty. *Multiplicative
complexity of polynomial multiplication over finite fields.* Journal of the ACM
**36** (1989), no. 1, 150-170. Read for its introduction only, p. 150, which
states the mechanism in the plainest form found anywhere and attributes it to
`[winograd1977]`: an optimal algorithm *"must evaluate the multiplicands at a
minimum of 2n distinct points, multiply the samples, and interpolate the result.
However, in finite fields, this method fails if 2n exceeds the number of field
elements."* A second witness to (ii), independent of `[lsw1983]`.

**`rousseau2021`**: É. Rousseau. *Arithmétique Efficace des Extensions de Corps
Finis.* Thèse, Institut Polytechnique de Paris, 2021. NNT 2021IPPAT013,
[tel-03299466](https://theses.hal.science/tel-03299466). Context, not implemented
here.

**`akleylek2014`**: S. Akleylek, F. Özbudak, C. Özel. *On the Arithmetic
Operations Over Finite Fields of Characteristic Three with Low Complexity.*
Journal of Computational and Applied Mathematics **259** (2014), 546-554.
Context, not implemented here.

## The algorithms everything is measured against

**`strassen1969`**: V. Strassen. *Gaussian elimination is not optimal.*
Numerische Mathematik **13** (1969), no. 4, 354-356.
`fixtures/strassen_u.matrix` and `strassen_v.matrix`.

## Linear and integer programming

What [`integer_programme/`](integer_programme/README.md) is, all of it somebody
else's and most of it older than this problem area.

**`dantzig1951`**: G. B. Dantzig. *Maximization of a Linear Function of
Variables Subject to Linear Inequalities.* In T. C. Koopmans (ed.), *Activity
Analysis of Production and Allocation*, Cowles Commission Monograph 13, Wiley,
New York, 1951, **Chapter XXI, pp. 339-347**. The simplex method. Chapter,
pages, editor, series and year were read off the volume rather than copied:
two corruptions are in wide circulation, "pp. 359-373" and the year 1947, the
second being when the work was done and not when it appeared.

**Its two stages are not yet the two-phase method.** The chapter's own summary
is that the technique *"consists in constructing first a feasible, and then a
maximum feasible, solution"*, and its two sections are exactly those. But it
reaches feasibility from a fixed reference point rather than from artificial
variables: the words *artificial* and *phase* do not occur in it at all.

**`dantzig1955`**: G. B. Dantzig, A. Orden, P. Wolfe. *The generalized simplex
method for minimizing a linear form under linear inequality restraints.* Pacific
Journal of Mathematics **5** (1955), no. 2, 183-195,
[open access](https://msp.org/pjm/1955/5-2/pjm-v5-n2-p04-s.pdf). Where the two
phases are named as such, **p. 193**: *"the first phase is to apply the
generalized simplex procedure to maximize the variable x_{n+1}"*, and when it
becomes positive *"the second phase, which is the search for an optimal
solution, begins"*. That is the shape
[`simplex.h`](integer_programme/simplex.h) has.

**`bland1977`**: R. G. Bland. *New Finite Pivoting Rules for the Simplex
Method.* Mathematics of Operations Research **2** (1977), no. 2, 103-107.
**Theorem 1.1**, and the only numbered theorem in the paper: *"The simplex
method under Rule I cannot cycle, hence it is finite."* Rule I is the
smallest-subscript choice on **both** the entering and the leaving variable,
which is what `simplex.cpp` does and the whole of why it terminates. There is no
Theorem 2 here; Rule II's finiteness is deferred to a separate paper.

**`landdoig1960`**: A. H. Land, A. G. Doig. *An Automatic Method of Solving
Discrete Programming Problems.* Econometrica **28** (1960), no. 3, 497-520. The
first branch and bound over a linear relaxation. **It carries no numbered
results at all**: no theorem, lemma or proposition occurs in it, only numbered
equations and the steps of a procedure, so **§3**, *Description of the Method*,
is the finest pinpoint there is.

**Their branching is by equality and this repository's is not.** At p. 504 the
children of a variable left at `x_r⁰` are the two problems with `x_r = [x_r⁰]`
and `x_r = [x_r⁰] + 1`, and the method then steps outward to further integers,
so the tree is not binary. The dichotomy in
[`branch_and_bound.h`](integer_programme/branch_and_bound.h) is `dakin1965`.

**`dakin1965`**: R. J. Dakin. *A tree-search algorithm for mixed integer
programming problems.* The Computer Journal **8** (1965), no. 3, 250-255,
[doi:10.1093/comjnl/8.3.250](https://doi.org/10.1093/comjnl/8.3.250). Where
splitting a fractional `x` into `x ≤ ⌊v⌋` and `x ≥ ⌈v⌉` comes from, and now read
in full rather than from the abstract.

**The dichotomy is (6) and (7)**, p. 250: having found a continuous solution in
which an integer variable `x_j` takes a non-integral `b_j`, *"we may divide all
solutions to the constraints (3) to (5) into two non-overlapping groups, viz.:
(i) solutions in which `x_j ≤ k`, (ii) solutions in which `x_j ≥ k + 1`"*, with
`k = [b_j]` the integral part, that being (8) and (9). That is exactly what
[`branch_and_bound.cpp`](integer_programme/branch_and_bound.cpp) does, and the
paper's own §"Comparison with Land and Doig method", p. 252, is what rules the
predecessor out: *"the Land and Doig algorithm forces variables to take exact
integral values rather than applying bounds"*, searching over the range a
variable may take, where this branches on one inequality and its complement.

Two further things it contains that this repository does independently, worth
recording since neither was taken from it: the depth-first walk with a list and a
"list marker" saying which of a node's two subtrees has been explored, §"
Computational search procedure" and Fig. 2, which is `Node`'s stack here; and
Dakin's own reason for it, that recording the tree *"could involve excessive
storage requirements"*, which he credits to Little et al.'s "Throw away the
tree".

**`mps360`**: IBM. *Mathematical Programming System/360, Linear and Separable
Programming - User's Manual*, order number **GH20-0476**, program number
360A-CO-14X, 224 pages. The origin of the fixed-column MPS format, whose eighty
columns are a punched card.

**The form number is now from IBM and not from a secondhand list.** IBM's own
*System/360 and System/370 Bibliography*, GA22-6822-16, July 1971, is readable
on bitsavers and describes it as *"the information required to prepare input
data and control cards and to interpret the system's output"*, alongside its
siblings GH20-0136, GH20-0290, GH20-0291 and GH20-0505. The prefix is `G`,
IBM's distribution letter; `H20-0476` is the same document.

**The manual itself still has not been read, and this is now a checked negative
rather than an impression.** Where it was looked for: bitsavers' complete file
index, `pdf/IndexByDate.txt`, **93 664 files, newest entry 2026-08-17**, has
zero hits for the form number, and `pdf/ibm/360/` has no `mps` directory; the
Internet Archive's search returns nothing for `H20-0476` or `GH20-0476`; the
Computer History Museum holds only a *different* MPS/360 manual, Y20-0065-0
(1967, 502 pp), and holds it as an unscanned artefact. **Two places were not
checked and are where a falsification should start**: HathiTrust, which answers
a scripted request with a Cloudflare challenge, and Google Books, which answered
with a quota error. The edition suffix and year remain unsettled, secondhand
sources giving 1968, 1969 and 1976, so none is stated here.

**It is also the wrong manual for half of what was cited to it.** MPS/360 was
*linear and separable* programming, and separable is not integer: a system with
no integer variables has no convention for marking them. The GA22-6822-16
bibliography bears that out, since **it contains no occurrence of "integer
programming" or "mixed integer" anywhere in it**, MPS/360 document set included.
IBM's own `[oslmps]` refers the reader to the *Mathematical Programming System
Extended/370 (MPSX/370) Program Reference Manual*, SH19-1095, and MPSX/370's
mixed integer option MIP/370, whose Program Reference Manual is **SH19-1099**
(SH19-1099-1, November 1975, per the Centre for Computing History's catalogue
record), is the lineage the markers belong to. No scan of either exists either:
the same bitsavers index has no `H19-1xxx` file at all. So `[mps360]` stands for
the fixed-column card layout and nothing more, and
[`mps_format.h`](integer_programme/mps_format.h) cites `[oslmps]` for the
markers.

**`oslmps`**: IBM. *Mathematical Programming System (MPS) Format*, a chapter of
the *Optimization Subroutine Library Guide and Reference*, SC23-0519. Read at
the mirror
[cenapad.unicamp.br/parque/manuais/OSL/oslweb/features/featur11.htm](https://www.cenapad.unicamp.br/parque/manuais/OSL/oslweb/features/featur11.htm),
IBM's own text and not a third-party account of it. It gives the six fields as
columns **2-3, 5-12, 15-22, 25-36, 40-47 and 50-61**, and tabulates the marker
record as field 1 blank, field 2 the marker name, field 3 `'MARKER'`, **field 4
blank**, field 5 the keyword, in the words *"field 5 must contain the value
'INTORG' (including the quotation marks) in the record that denotes the start of
integer variables"*. That is this repository's claim, from IBM, and *"field 4,
although ignored, must be blank"* is the sentence that rules the other reading
out.

**`lpsolve_mps`**: lp_solve reference guide, *MPS file format*,
[lpsolve.sourceforge.net/5.5/mps-format.htm](https://lpsolve.sourceforge.net/5.5/mps-format.htm).
The same six column ranges and, in as many words, *"the start marker has its
name in field 2, 'MARKER' in field 3, and 'INTORG' in field 5"*. Read; a second
witness rather than a source.

**`cplex_mps`**: ILOG CPLEX, *MPS File Format*, read at
[plato.asu.edu/cplex_mps.pdf](https://plato.asu.edu/cplex_mps.pdf). **Its prose
says field 4 and its own example says field 5, and the example is right.** The
prose is *"Field 4: Keyword 'INTORG' and 'INTEND'"* with *"fields 5 and 6 are
ignored"*; but its Table 2 of the six fields gives no column positions at all,
and it requires only that fields *"must be separated by white space"*. CPLEX is
counting tokens, and a marker line has three, so its fourth field is the
fixed-format fifth. Its worked example settles it: `'INTORG'` and `'INTEND'` sit
at the same offset as the field-5 row names on the `COLUMNS` lines above and
below them, not at the field-4 values. **This is not a disagreement about the
format**, which is what this file used to record; it is two ways of numbering
the same characters.

**`murtagh1981`**: B. A. Murtagh. *Advanced Linear Programming: Computation and
Practice.* McGraw-Hill, New York, 1981, ISBN 0-07-044095-6. The description of
MPS most bibliographies point at, this one included.

**It has now been read for the claim that matters, and the page range everyone
quotes is the wrong one.** Every copy is closed: the Internet Archive scan
`advancedlinearpr0000murt` is `access-restricted-item`, lending-only, with the
page images not viewable; HathiTrust's `mdp.39015000495419` is rights code `ic`,
*"Limited (search-only)"*; Google Books offers no preview. But the Archive's
search-inside index over that scan returns the paragraphs themselves, and two of
them settle it. On **leaf 191, printed page 175**:

> *"Marker cards can be used to specify the start and end of a group of integer
> variables in the same manner as GUB sets, discussed in Sec. 9-6-1. The start
> marker has the marker name in field 2, the keyword 'MARKER' in field 3, and
> the keyword 'INTORG' in field 5. The end marker has the marker name in field
> 2, the keyword 'MARKER' in field 3, and the keyword 'INTEND' in field 5."*

and on **leaf 189, printed page 173**, the GUB marker table it refers back to,
carrying the column ranges: *"Field 1 Field 2 Field 3 Field 4 Field 5 Field 6
(2-3) (5-12) (15-22) (25-36) (40-47) (50-61)"*, with the start marker's keyword
`'GUBORG'` in field 5 and **field 4 blank**.

So Murtagh says field 5, in the same words as `[oslmps]`, and **pp. 163-166 do
not contain it**: the marker passage is p. 175 and the field table p. 173. The
printed page numbers are the scan's own leaf-to-page table, not an arithmetic
guess. What was read is the full-text index, not the page images, which is a
weaker thing than holding the book and is said here so that it can be checked.

## Software

**`givaro`**: Exact Arithmetic Over GF(p) and Over the Rationals, from the CASYS
team at the Laboratoire Jean Kuntzmann.
[casys.gricad-pages.univ-grenoble-alpes.fr/givaro](https://casys.gricad-pages.univ-grenoble-alpes.fr/givaro/).
Used, not vendored. **GMP** underneath it. The only build dependency.

**`cryptominisat`**: M. Soos et al. Found on `PATH` at run time, never linked.
Chosen for native XOR clauses, which is exactly the shape of a GF(2) tensor
equation.

**`cvc5`**: The SMT solver implementing `ozdemir2023`'s finite-field theory.
Also found at run time. Its finite-field solver requires a CoCoALib build.

**`plinopt`**: J-G. Dumas, B. Grenet, C. Pernet, A. Sedoglavic. *PLinOpt: C++
Routines for Linear, Bilinear & Trilinear Straight-line Programs.*
[github.com/jgdumas/plinopt](https://github.com/jgdumas/plinopt), CeCILL-B.
The reference implementation for this problem area, and the one to check
against: `bin/sparsifier`, `bin/factorizer`,
`bin/orbiter`. It reaches sparsity by a different route from `beniamini2020`;
sparse QLUP elimination and bounded coefficient search rather than the Ω-valid
oracles; so the two are worth comparing rather than one replacing the other.
Not a dependency here: it needs LinBox, which this repository does not.

**`cbc`**, **`glpsol`**, **`lp_solve`**, **`gurobi_cl`**: The integer programming
backends of [`integer_programme/`](integer_programme/README.md), which is what
[`curve_bounds/`](curve_bounds/README.md)'s step 3 is handed to. Ranked and found
on `PATH` at run time, never linked. CBC (COIN-OR, EPL), GLPK (GNU, GPL) and lp_solve
(LGPL) are in the Ubuntu archive and are the three verified on this machine;
Gurobi is proprietary, free to academics, and its recipe here is unverified for
want of a licence. None is a dependency: absent all four, the built-in exact
simplex and branch and bound answers.
