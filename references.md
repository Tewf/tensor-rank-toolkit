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

Two of its numbered results are what [`pencil_rank/`](pencil_rank/)
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
[`canonical_factorisation/`](canonical_factorisation/) is built on,
credited there to Bürgisser-Clausen-Shokrollahi Proposition 14.45.

**Theorem 5.4 is the other numbered result cited to it, and it was read.**
*"(Tensor-rank bound). We have `trk(C) ≥ dim_{F_q}(C) + d(C) − 1`"*, over `F_q`
throughout section 5. Its two symbols are the paper's own: the code is a slice
space in the sense of **Definition 2.1**, *"the first slice space of `X` ... the
span of `X_1, …, X_k`"*, and `d` is **Definition 5.3**, *"the minimum (rank)
distance ... `min{rk(X) : X ∈ C, X ≠ 0}`"*. So `k` and `d` mean in
[`rank_metric_bound/`](rank_metric_bound/) what they mean here. Byrne
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
it rather than sinking it. [`gpu_leaf/README.md`](gpu_leaf/) measures a
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
[`orbit_reduction/`](orbit_reduction/) exists to quotient by it, which
is exactly the hypothesis both complexity statements exclude. They are a
reference point for what solving MinRank algebraically is known to cost, not a
forecast of what it would cost on [`fixtures/`](fixtures/). **Neither
was read past its abstract**, and no Hilbert series has been computed here for
any shape in this repository. Where all three of these entries land, and what
the leaf test is called once it is named properly:
[`the-research-front/rank-one-elements-of-a-subspace.md`](the-research-front/rank-one-elements-of-a-subspace.md).

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

**`junttila2020`**: T. Junttila, M. Karppa, P. Kaski, J. Kohonen. *An adaptive
prefix-assignment technique for symmetry reduction.* Journal of Symbolic
Computation **99** (2020), 21-49,
[doi:10.1016/j.jsc.2019.03.002](https://doi.org/10.1016/j.jsc.2019.03.002),
[arXiv:1706.08325](https://arxiv.org/abs/1706.08325); SAT 2017, LNCS, 101-118.
`[mckay1998]`'s canonical extension framework, which they cite by that name,
turned into something a constraint system can use: assign a **prefix** of the
variables, keep one prefix-assignment per orbit, and hand the rest to a solver.
Four properties they claim for it, and each is a property this repository wants:
the prefix is user-prescribed and truncatable, so it stops where the group stops
being cheap; the prefix-assignments are pairwise non-isomorphic and independent,
so they **parallelise**, including across nodes by MPI; the group need only be
expressible as the automorphism group of a vertex-coloured graph; and canonical
labelling is the only nontrivial subroutine.

**This is what [`orbit_cubes.h`](orbit_reduction/orbit_cubes.h) is a one-term
special case of.** A cube here fixes the *first* term to one representative per
orbit; a prefix-assignment fixes as many variables as the group can still afford
and is the general form of the same trick, with the parallel split as its point
rather than as a side effect. Implemented and public: `reduce`, MIT-licensed C at
[github.com/pkaski/reduce](https://github.com/pkaski/reduce). **Read from the
abstract and the repository's licence only**; no numbered result is quoted here.

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
[`vendor/permlib/`](vendor/permlib/) with its licence and authors. Read:
`permlib_api.h`'s `smallestSetImage` and the `OrbitLexMinSearch` it calls, and
enough of `permutation.h` and `transversal/` to know that the transversal is a
Schreier tree rather than explicit, which is what makes a degree of 261 121
affordable. Not read: everything else.

**`degroote1978`**: H. F. de Groote. *On varieties of optimal algorithms for the
computation of bilinear mappings. I. The isotropy group of a bilinear mapping.*
Theoretical Computer Science **7** (1978), 1-24,
[doi:10.1016/0304-3975(78)90038-5](https://doi.org/10.1016/0304-3975(78)90038-5);
and *II. Optimal algorithms for 2 × 2-matrix multiplication.* Theoretical
Computer Science **7** (1978), 127-148,
[doi:10.1016/0304-3975(78)90045-2](https://doi.org/10.1016/0304-3975(78)90045-2).
**This is where the group is named.** The title of part I is the definition
every symmetry claim in this repository is a claim about: a bilinear mapping
carries a group acting on the variety of its optimal algorithms, and that group
is its **isotropy group**. Part II is the consequence at the smallest
interesting size, which `[berger2019]`'s abstract states as: de Groote showed
that for 2 × 2 matrix multiplication with 7 active multiplications, *"all
algorithms are essentially equivalent to Strassen's algorithm"*.

**Nothing in this file said the word "isotropy" until 2026-08-20**, which left
the whole symmetry strand here resting on unnamed ground.
`[covanov2019, Def. 7]`'s rank-preserving action is this group written for a
finite field, `[burichenko2014]` is about the isotropy group of Strassen's
algorithm, `[berger2019]` decides equivalence under it, and
[`orbit_reduction/`](orbit_reduction/) quotients a search by it.
**Not read**: both parts are behind Elsevier's paywall and no reachable scan was
found, so no numbered result of either is quoted anywhere here, and the two
sentences above are the title of part I and `[berger2019]`'s account of part II.

**`berger2019`**: G. O. Berger, P.-A. Absil, L. De Lathauwer, R. M. Jungers,
M. Van Barel. *Equivalent Polyadic Decompositions of Matrix Multiplication
Tensors.* Journal of Computational and Applied Mathematics **406** (2022),
113941, [arXiv:1902.03950](https://arxiv.org/abs/1902.03950).
**An algorithm that decides whether two decompositions of a given matrix
multiplication tensor are equivalent** under the invariance transformations —
`[degroote1978]`'s group — and with it a count of the equivalence classes at
several formats. The answer is not the one the 2 × 2 case suggests: at larger
formats, *"e.g., 2 × 3 by 3 × 2 or 3 × 3 by 3 × 3"*, two decompositions *"are
very likely to be essentially different"*. They also give a necessary criterion
for a decomposition to be equivalent to one with integer entries, which is what
makes a scheme cheap and stable to run.

**This is the test this repository does not have and would need before calling
anything new.** [`descent_search/`](descent_search/) and
[`flip_graph/`](flip_graph/) produce decompositions; whether one of them
is a rediscovery of a published scheme or a genuinely different point of the
variety is exactly the question this decides, and
[`positioning/already-published.md`](positioning/already-published.md) currently
answers it by format and count alone. **Read from the abstract only.**

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
[`reflected_gray_walk.h`](descent_search/reflected_gray_walk.h).

**The order is why the general leaf costs one row addition an element.** Every
element of a `dim`-dimensional subspace over `GF(p)` is a digit string read as
coefficients on a basis; counting in base `p` moves several digits at a step and
forces the combination to be rebuilt from the basis, whereas this order moves
one, so the update is a row added or subtracted with no field multiplication in
it. **Table 1 of §7.2.1.1** is the reflected code itself and the ± property is
its defining one; the loop-free part is Algorithm H's, and
[`tests/test_reflected_gray_walk.cpp`](descent_search/tests/test_reflected_gray_walk.cpp)
asserts both against the implementation.

## Deciding rank with a solver

**`courtois2011`**: N. T. Courtois, G. V. Bard, D. Hulme. *A New General-Purpose
Method to Multiply 3x3 Matrices Using Only 23 Multiplications.*
[arXiv:1108.2830](https://arxiv.org/abs/1108.2830), 2011. **Where this method
starts.** They state the problem as Brent's equations, convert it to SAT, and
throw *"our portfolio of some 500 SAT solvers"* at it; out comes a new
23-multiplication scheme for `⟨3,3,3⟩` which they check is **not** an equivalent
variant of Laderman's, so the solution space is larger than had been assumed and
22 becomes more plausible rather than less. That is the same conversion the three
encoders in [`satisfiability/`](satisfiability/) do, ten years before
`[heule2021]` made it produce results at scale, and it is also the first
appearance here of the question `[berger2019]` later answers properly: whether a
scheme a search hands you is genuinely a new one.

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

**This is the standard [`satisfiability/`](satisfiability/) should be
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
[`satisfiability/search-in-the-literature/`](satisfiability/search-in-the-literature/).

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
[`satisfiability/search/`](satisfiability/search/) does not contradict and
does not benefit from.

## Breaking the solver's symmetries

`[covanov2019]`'s group is quotiented out of a search *tree*. This is the same
idea written as clauses, for a solver that has no tree to quotient, and it is
what [`satisfiability/symmetry_breaking.h`](satisfiability/symmetry_breaking.h)
implements. The field's words for it are **symmetry-breaking predicate** and
**lex-leader**; **none of them appeared in this file until 2026-08-20**, which is
why that header reads as though the technique were local to it.

**`crawford1996`**: J. M. Crawford, M. L. Ginsberg, E. M. Luks, A. Roy.
*Symmetry-breaking predicates for search problems.* Proc. of the Fifth
International Conference on Principles of Knowledge Representation and Reasoning
(KR'96), Cambridge, MA, Morgan Kaufmann, 148-159. The paper the technique is
from: add constraints
satisfied by exactly one member of each class of symmetrical points — the
lexicographic leader — so the solver is never offered the rest. **That is what
`symmetry_breaking.h` is doing, both halves of it**:
`order_lexicographically` is a lex-leader predicate for the `r!` orderings of the
terms, and `normalise_first_nonzero` is one for the scalings of a term. The same
paper is where **computing a predicate true of only the lex-leader is proved
NP-hard**, which is why the constraints here are partial and why partial is the
normal condition rather than a corner cut.

**Not read**, and the NP-hardness above is quoted through `[anders2024]` §1,
which attributes it to this paper, not from the paper itself: the copy on the
third author's page, `ix.cs.uoregon.edu/~luks/symmetrybreaking.pdf`, is a scan
whose text layer is unusable. Only its bibliographic details are first-hand.

**`luksroy2004`**: E. M. Luks, A. Roy. *The complexity of symmetry-breaking
formulas.* Annals of Mathematics and Artificial Intelligence **41** (2004),
19-45,
[doi:10.1023/B:AMAI.0000018578.92398.10](https://doi.org/10.1023/B:AMAI.0000018578.92398.10).
How much worse it is than NP-hard in the small print. **Read from the authors'
own copy** at
[ix.cs.uoregon.edu/~luks/symmetry.pdf](https://ix.cs.uoregon.edu/~luks/symmetry.pdf),
the journal being paywalled: even for **abelian** groups the number of essential
clauses in the *natural* lex-leader formula can be exponential; finding any
expression of lex-leadership **without reordering the variables** is NP-hard even
for elementary abelian groups with orbits of size 3; and with a reordering
obtained by computational group theory, small lex-leader formulas for abelian
groups can be built after all. The last clause is the one that matters here,
because a variable order is exactly what
[`order_lexicographically`](satisfiability/symmetry_breaking.h) is handed and
what nothing in this repository has ever chosen deliberately.

**`katsirelos2010`**: G. Katsirelos, N. Narodytska, T. Walsh. *On the complexity
and completeness of static constraints for breaking row and column symmetry.*
CP 2010, 305-320,
[doi:10.1007/978-3-642-15396-9_26](https://doi.org/10.1007/978-3-642-15396-9_26),
[arXiv:1007.0602](https://arxiv.org/abs/1007.0602). What a *partial* predicate
leaves behind, measured instead of assumed. DOUBLELEX and SNAKELEX on a matrix of
variables with interchangeable rows and columns are *"often effective in
practice"* and *"can leave a large number of symmetric solutions in the worst
case"*; propagating DOUBLELEX completely is NP-hard; a unique representative per
class is computable in polynomial time when the number of rows or of columns is
bounded; and the paper closes with the first experimental study of how much
symmetry is left over on benchmarks. It also **corrects a published claim about
when different symmetry-breaking constraints can safely be combined**, which is
the exact way a predicate here would become too strong and report a false lower
bound — the risk `symmetry_breaking.h`'s header calls the most dangerous thing in
its folder. **Read from the abstract only.**

**This is the measurement this repository owes for its own predicate.** Ordering
plus scaling is a partial break of the decomposition symmetries, the residual
duplication has never been counted here, and
[`satisfiability/choices/two-defaults-that-were-wrong.md`](satisfiability/choices/two-defaults-that-were-wrong.md)
reports the time the constraint saves, not the symmetry it leaves.

**`anders2024`**: M. Anders, S. Brenner, G. Rattan. *The Complexity of Symmetry
Breaking Beyond Lex-Leader.* CP 2024, LIPIcs vol. 307, 3:1-3:24,
[doi:10.4230/LIPIcs.CP.2024.3](https://doi.org/10.4230/LIPIcs.CP.2024.3),
[arXiv:2407.04419](https://arxiv.org/abs/2407.04419). Why nobody escapes partial
by being cleverer. **Theorem 1.1** in the arXiv numbering, **Theorem 1** in the
LIPIcs one: *"Suppose there exists a polynomial time algorithm for generating
complete symmetry breaking predicates for row-column symmetries. Then GI ∈ co-NP
holds"* — efficient complete symmetry breaking would hand graph
non-isomorphism a short certificate. The barrier is not an artefact of
lex-leader: it holds for predicates using any other order, for predicates given
as Boolean circuits, and for predicates allowed to introduce extra variables.
Read from the LIPIcs PDF.

**`sakallah2021`**: K. A. Sakallah. *Symmetry and Satisfiability.* Handbook of
Satisfiability, 2nd edition, Frontiers in Artificial Intelligence and
Applications **336**, IOS Press 2021, chapter 13, 509-570,
[doi:10.3233/FAIA200996](https://doi.org/10.3233/FAIA200996). The survey, and the
thing to read before a third constraint is added to `symmetry_breaking.h`.
**Not read**; it is named so that this strand has a map and not only its
ancestors, the same way `[morgado2013]` is the map for the search over `k`.

The measured baseline for all of this, on this repository's own instance, is
`[deza2023]` §4.1 and Table 2.

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
[`positioning/`](positioning/).

**`smirnov2013`**: A. V. Smirnov. *The bilinear complexity and practical
algorithms for matrix multiplication.* Computational Mathematics and
Mathematical Physics **53** (2013), no. 12, 1781-1795,
[doi:10.1134/S0965542513120129](https://doi.org/10.1134/S0965542513120129);
Russian original Zh. Vychisl. Mat. Mat. Fiz. **53** (2013), no. 12, 1970-1984.
A method for deriving bilinear algorithms, new estimates for the bilinear
complexity of exact and approximate multiplication of rectangular matrices, an
improved bound on the border rank of 3 × 3, and a practical `n x n` algorithm of
asymptotic arithmetic complexity `O(n^2.7743)`.

**It is what the search literature was measuring itself against before the
learning came.** `[deza2023]` §3.1 calls it *"the state-of-the-art local method"*
— alternating least squares with regularisation — *"the most successful in
finding fast algorithms whilst remaining computationally tractable"*, scaled to
`⟨4,4,4⟩` at rank 49, and names three of its limitations: local minima,
ill-conditioned least-squares, and solutions good only to machine precision.
**The first is shared and the other two are not.**
[`minimise_rank.h`](descent_search/minimise_rank.h) is first-improvement with
irreversible pruning and guarantees nothing, so it stops at local minima too;
conditioning and machine precision do not exist over `GF(p)`, and an answer here
is exact or it is not an answer. That is the one clean thing this repository can
say against a numerical method, and it has never said it. Read from the abstract,
and from `[deza2023]` §3.1 for the rest.

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
LIPIcs vol. 280, 26:1-26:15,
[doi:10.4230/LIPIcs.CP.2023.26](https://doi.org/10.4230/LIPIcs.CP.2023.26),
[arXiv:2306.01097](https://arxiv.org/abs/2306.01097), code at
[github.com/khalil-research/Matrix-Mult-CP](https://github.com/khalil-research/Matrix-Mult-CP).
The Brent equations solved by constraint programming. The 2x2 and 3x3 cases are
MIPLIB 2017 benchmarks, so the formulation is standard and nothing here is new.
This repository stated the same equations for a MILP solver, measured them
against the SAT strand and the tree search, and retired the encoding:
[`the-research-front/rank-as-a-milp.md`](the-research-front/rank-as-a-milp.md).

**That was the whole of this entry until 2026-08-20, and it named the wrong half
of the paper.** §4.1 and Table 2 are the published baseline for
[`satisfiability/symmetry_breaking.h`](satisfiability/symmetry_breaking.h),
measured on the instance this repository measures itself on. Their §4.1 breaks
the same two symmetries: **§4.1.1** posts `lexicographic-strict` on the concatenated
`r`-th columns of `U` and `V` against the permutation of the terms, which is
`order_lexicographically`; **§4.1.2** forces the first nonzero entry of each
column of `U` to be `−1` against the sign symmetry, which is
`normalise_first_nonzero` with the other representative chosen, over
`{−1, 0, 1}` where here the group is the `λμν = 1` scalings of `GF(p)`.

**Table 2 is the `⟨2,2,2⟩` rank-6 refutation, question for question.** Over ten
seeds with a two-hour limit, the base model `B` does not finish at all and `B+S`
takes 429.26 s as a shifted geometric mean, with the branch count falling from
`5.99×10⁹` to `3.28×10⁸`. Their own sentence: proving infeasibility for `R = 6`
*"is not even currently possible without symmetry-breaking constraints in 2 hours
whereas the CP model with symmetry-breaking constraints (B+S) requires around 7
minutes"*. `7200 / 429.26 ≈ 16.8` is therefore **a lower bound and not a ratio**,
the run without them having never finished, the same shape of number as this
repository's own "at least seventy-six times" under cryptominisat.
[`satisfiability/choices/two-defaults-that-were-wrong.md`](satisfiability/choices/two-defaults-that-were-wrong.md)
measures **24.7 s → 0.31 s, seventy-nine times**, on the same instance and the
same technique, and cites nothing for it.

**And they ship it off by default too**, which is the part worth having:
`src/main.py` has `parser.set_defaults(symmetry=False)`, and their conclusion
gives the reason — *"the base CP model outperforms the addition of symmetry
constraints and valid inequalities in the case of feasible solutions, likely due
to the latter's tendency to prune symmetric solutions early in the tree search"*.
The default here is off for a different reason, that an over-strong constraint
would turn a satisfiable instance into UNSAT and report a false lower bound. Two
projects arriving at the same default from two different arguments is worth more
than either argument.

**`alphaevolve2025`**: Google DeepMind. *AlphaEvolve: A Coding Agent for
Scientific and Algorithmic Discovery.* 2025. `⟨4,4,4⟩` in 48 multiplications over
`ℂ`, the first improvement on 49 in fifty-six years.

**`moran2026`**: Y. Moran, O. Schwartz, S. Yuan. *Complex to Rational Fast Matrix
Multiplication.* [arXiv:2602.13171](https://arxiv.org/abs/2602.13171), 2026.
Converts a complex scheme to a rational one or proves none exists, generalising
Dumas, Pernet and Sedoglavic (2025), whose ad hoc results it recovers and extends
to coefficients carrying square roots, `i = √−1` being the special case. The
machinery is *"basic linear-algebraic properties of similarity transformations of
complex matrices"* and nothing heavier. Two non-existence results come out of it:
**no rational scheme is equivalent to Smirnov's `⟨4,4,9,104⟩` `Q[√161]` algorithm
(2022), and no real scheme to Kaporin's complex `⟨4,4,4,48⟩` (2024)** — the
second at the format and count `[alphaevolve2025]` also reached over `ℂ`, though
the paper does not say the two schemes are the same one and it is not assumed
here.

**The key was `dumas2026` until 2026-08-20.** Dumas is who the paper
generalises, not who wrote it, and the entry carried no author list to catch it.

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
[`the-research-front/lower-bounds.md`](the-research-front/lower-bounds.md).

**`yang2024sat`**: J. Yang. *Ruling Out Low-rank Matrix Multiplication Tensor
Decompositions with Symmetries via SAT.*
[arXiv:2402.01011](https://arxiv.org/abs/2402.01011), 2024. Decompositions of
`⟨3,3,3⟩` over `Z/2Z` of rank at most 21, which the abstract gives as the range
where one would beat Strassen asymptotically, ruled out by a SAT solver — but
**only those carrying a prescribed symmetry**, which is what makes the space
small enough to exhaust and is therefore what the refutation is about. **The same
pairing exists here and is far more timid**:
[`orbit_cubes.h`](orbit_reduction/orbit_cubes.h) fixes the *first term* to one
representative per orbit for a solver to split on — a group cutting the space and
a solver refuting what is left, at one term rather than across the whole scheme.

**The key was `heule2024` until 2026-08-20.** The entry carried no author list,
the subject is SAT and matrix multiplication, and the guess was the wrong one:
the sole author is Jason Yang, of `[yang2024]`, `[yang2025]` and
`[yang2025thesis]`. Hence `yang2024sat`, `yang2024` being taken.

**`alman2025`**: J. Alman, R. Duan, V. Vassilevska Williams, Y. Xu, Z. Xu,
R. Zhou. *More Asymmetry Yields Faster Matrix Multiplication.* SODA 2025,
[arXiv:2404.16349](https://arxiv.org/abs/2404.16349). `ω < 2.371339`. The laser
method, which shares no machinery with anything here.

## Sparsifying the operators

**`beniamini2020`**: G. Beniamini, N. Cheng, O. Holtz, E. Karstadt, O. Schwartz.
*Sparsifying the Operators of Fast Matrix Multiplication Algorithms.*
[arXiv:2008.03759](https://arxiv.org/abs/2008.03759), 2020.
Definition 3.2 is the Ω-valid set; Algorithms 3 and 4 are the two exact oracles
archived on the `rejected-experiments` branch
(`retired/dominated_sparsifiers/`), with the measurement that moved them in
[`matrix_sparsification/dominated.md`](matrix_sparsification/dominated.md);
Algorithm 2 is the driver they feed, from `gottlieb2010`; Algorithm 6 is the
greedy in `greedy_sparsifier.h`.

**Problem 2.15 and Algorithm 2 together are the composition
[`matrix_sparsification/method/exact-over-q.md`](matrix_sparsification/method/exact-over-q.md)
implements, and the paper states it is exact.** Problem 2.15 asks for a vector in
the row space, *not in the span of the rows already settled*, with a minimal
number of nonzeros; Algorithm 2 builds the settled set from empty by calling it.
The paper's own words: given such a subroutine, "Algorithm 2 returns an exact
solution for MS". So the matroid greedy is not a new arrangement of their pieces,
it is their arrangement, and what was missing here was an oracle that answered
Problem 2.15 rather than a restricted version of it. **Read in the paper.**

**They solve Algorithm 6 with a solver, and it is Z3.** §5 encodes the objective
as a **MaxSAT** instance with two kinds of soft constraint, one penalising
nonzero entries and one penalising non-singleton entries, so the optimum
minimises `nnz + nns` directly. Reported costs on their corpus: Algorithms 3 and
4 within 40 minutes, Algorithm 6 under one minute. **Read in the paper**; not
reproduced here, and not comparable to anything measured on this machine.

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
[arXiv:2607.28676](https://arxiv.org/abs/2607.28676), submitted 2026-07-28. The
current record, and provably optimal for its fixed tensor orientation. The baseline
any joint search over rank and sparsity has to name before it starts.

**Read from the abstract, so that the number is not carried at third hand.** The
55 is **13 additions on the left input, 14 on the right and 28 at the output**, the
last obtained by transposing a 14-addition factor circuit, and with the 23 bilinear
products the circuit is 78 scalar operations. **The coefficient alphabet is
`{−1, 0, +1}`** and the order of every product is kept, so it holds over any
associative ring, commutative or not. It improves the **56** it attributes to Sun,
and it starts from `[perminov2026]`'s published **58**-addition realisation
`cr58_cn122`, which is
`schemes/results/addition_reduced_ZT/3x3x3_m23_cr58_cn122_ZT_reduced.json` in that
repository, checked there rather than assumed from the name. Four exact checks are
reported, including independent Python and Node.js implementations of all 729 Brent
identities over `Z`. **It is model (c) in the standard basis**, so what may and may
not be read against it is
[`matrix_sparsification/measured-with-other-tools/reading-the-program-length.md`](matrix_sparsification/measured-with-other-tools/reading-the-program-length.md).

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

**`lisonek2016`**: P. Lisoněk, L. Trummer. *Algorithms for the minimum weight of
linear codes.* Adv. Math. Commun. **10**(1):195-207, 2016. **The authoritative
statement of the bound, read in full.** Its equation (5), after the step that
enumerates information weight `w` in the first `j` of `D` systematic matrices, is

> `d ≥ Σ_{i≤j} max(0, w+1−k+r_i) + Σ_{i>j} max(0, w−k+r_i)`

with `r_i = |T_i \ (T_1 ∪ … ∪ T_{i-1})|` the relative rank, `Σ r_i = n`. For
disjoint information sets it collapses to `D·w + j`. Also Proposition 4, the
caveat that matters before anyone invests in this: **if `n ≤ 1.5k` the later
matrices never contribute and BZ degenerates to brute force.** And the
information sets are chosen by matroid partitioning in `O(n³k³)`, which is where
Edmonds enters this literature. Example 4.2: a `[1344,128]₂` code Magma estimated
at 10⁶ years, answered in 94 s once ten disjoint information sets were found.

**`zimmermann1996`**: K.-H. Zimmermann. *Integral Hecke Modules, Integral
Generalized Reed-Muller Codes, and Linear Codes.* Technical Report 3-96,
Technische Universität Hamburg-Harburg, 1996. With A. Brouwer's earlier work this
is the **Brouwer-Zimmermann algorithm**, the standard method for the minimum
weight of a linear code and what Magma and GAP/GUAVA ship. It enumerates over
several information sets, which buys a **lower** bound alongside the upper bound
and lets it stop before the enumeration is exhausted; the scan in
[`matrix_sparsification/method/exact-over-q.md`](matrix_sparsification/method/exact-over-q.md)
has the upper bound and no lower bound, which is where it stops being able to
finish. **Cited from the citing literature**, not read: the primary is an
unindexed 1996 technical report. What was read is the description in the two
entries below. Named here because our exact oracle solves *exactly* the
minimum-weight-codeword problem, under a different name, and this is that
problem's mature algorithm.

**`hernando2019`**: F. Hernando, F. D. Igual, G. Quintana-Ortí. *Algorithm 994:
Fast Implementations of the Brouwer-Zimmermann Algorithm for the Computation of
the Minimum Distance of a Random Linear Code.* ACM Trans. Math. Softw.
**45**(2), 2019, [doi:10.1145/3302389](https://doi.org/10.1145/3302389).
Sequential, vectorised and shared-memory implementations, over GF(2), reporting
speed-ups in **time** against Magma and Guava. **Abstract only; the full text is
paywalled and returned 403.**

**The codeword counts this entry first carried are not from this paper.** They
are Table 1 of `[bouyuklieva2021]`, comparing Magma V2.25-2 against the authors'
own QextNewEdition, and the entry said "Algorithm 994" because that is where a
search result put them. **The numbers are right and the paper was wrong**: see
[`matrix_sparsification/what-was-corrected.md`](matrix_sparsification/what-was-corrected.md).

**`quintanaorti2019`**: G. Quintana-Ortí, F. Hernando, F. D. Igual. *Parallel
Implementations for Computing the Minimum Distance of a Random Linear Code on
Multicomputers.* [arXiv:1911.08963](https://arxiv.org/abs/1911.08963), 2019.
The same algorithm on distributed memory, hundreds to thousands of cores, "up to
several orders of magnitude" faster than what is widely used. **This is where the
parallel work on this problem went, and it is not a card**: no GPU
implementation of Brouwer-Zimmermann was found. **Abstract read.**

**`sanjose2025`**: R. San-José. *An algorithm for computing generalized Hamming
weights and the Sage package GHWs.* ACM Trans. Math. Softw. **51**(4), 2025;
[arXiv:2503.17764](https://arxiv.org/abs/2503.17764). Generalises
Brouwer-Zimmermann from the minimum distance to the whole weight hierarchy **and
to the relative case**, and ships it as Sage at `github.com/RodrigoSanJose/GHWs`,
GPL-3.0-or-later, so the algorithm may be read and the code may not be lifted.
Full text read 2026-08-22.

**Its relative weight at `r = 1` is `[beniamini2020]`'s Sparsest Independent
Vector oracle, and neither paper knows about the other.** Definition 2.7, read
verbatim:

> `M_r(C₁, C₂) = min{ |supp(D)| : D a subcode of C₁ with dim D = r, D ∩ C₂ = {0} }`

At `r = 1` a subcode `D` is `⟨c⟩`, so `|supp(D)|` is `wt(c)` and `D ∩ C₂ = {0}`
says `c ∉ C₂`. So `M₁(C₁, C₂) = min{ wt(c) : c ∈ C₁ \ C₂ }`, which is Problem
2.15 word for word with `C₂` the span of the rows already settled. **The coding
side has had a Brouwer-Zimmermann-pruned algorithm for the sparsification side's
oracle since 2025, in ACM TOMS, shipping.** That is the same non-citation
[`matrix_sparsification/what-is-hard-about-it.md`](matrix_sparsification/what-is-hard-about-it.md)
records on the hardness side, showing up again on the algorithms side.

**`RGHW(C, C2, 1)` from that package is the baseline this module has to name**,
and the review is not finished until it has.

**But do not trust that baseline's answer without checking it.** Reading the
implementation on 2026-08-22 turned up an unsoundness in `core.py`, not in the
paper. Line 759 drops a generator matrix from a pass whenever its redundancy
exceeds the pass index, `red[j] <= w`; line 794 then credits that same matrix the
full `(w+1) − R_j` in the lower bound from `w = R_j` onward. The bound needs every
credited matrix to have had its message weights `1 … w` enumerated, and a
late-entering matrix has not, so the bound can overshoot and the search can stop
early with an answer that is **too high**.

Verified here from scratch, over GF(2), `n = 16`, `k = 8`, generator rows
`[2817, 17154, 6404, 27400, 27920, 41504, 53056, 9600]` read as bitmasks. Brute
force over all 256 codewords gives `d = 3` at support `{4,5,6}`. The information
sets come out `[[0..7], [0,4,8..13]]` with redundancies `[0,2]`, and that codeword
has message weight **1** in the second set and **3** in the first: the cheap route
to it is through the matrix that is skipped until `w = 2`. Algorithm 1 of the
paper enumerates with **all** `m` matrices every pass and is sound; the skip is an
"improvement" introduced only in §5.1.

**What was verified here and what was not.** The source lines, the arithmetic of
the counterexample, and the inference from the two: all checked directly. The
package itself was **not run** — Sage is not installed on this machine — so this
is a reading plus a proof, not an observed failure. Two consequences for this
module: implement `[lisonek2016]`'s per-`(w, j)` refinement, which `core.py` does
not do and which is strictly better; and keep the redundancy beside its matrix
rather than in a parallel array, because line 794 indexes an uncompressed `red`
with a compressed index and is only correct while `red` happens to be
non-decreasing. What nobody has done is put the
Rado-Edmonds driver on top of that oracle and sum the successive `M₁`, which is
the minimum-weight basis, and nobody has done any of it outside a finite field.

Three things still separate it from what this module needs:

- **A different invariant at `r > 1`.** `d_r` is the smallest *union of supports*
  over an `r`-dimensional subcode, not a sum of weights. Only `r = 1` is ours.
  The paper never mentions matroids, Rado or Edmonds: zero occurrences of all
  three.
- **The bound does not detach from the enumeration.** Everything structural is
  field-agnostic: Lemma 3.1, the information sets, and the bound itself,
  `Σ_j max{0, (w+1) − R_j}` with `R_j` the redundancy of the `j`-th information
  set, which is `m(w+1)` for `m` disjoint ones. But that bound is sound **only
  because** every message-space subspace of support `≤ w` was exhausted first,
  and that enumeration is `{v ∈ F_q^r : 1 ≤ wt(v) ≤ z}`, which over `Q` is
  infinite. There is no seam along which to keep the bound and drop the walk.
- **The scale is not there.** Its only length-49 measurement is at dimension 6
  and takes 517 s over GF(7). Ours is dimension 16.

**What survives for this module is the degenerate case**, `m = 1`, which the scan
here already licenses for nothing: having solved every support of size `≤ t`,
every remaining codeword weighs at least `t+1`. Measured against a cheap upper
bound, that rule fires on six of nine greedy steps for `Grey-221_L` and on none
at all for `4x4x4_49_156_L`, which is the operator it would need to rescue.

**`tillmann2019`**: A. M. Tillmann. *Computing the spark: mixed-integer
programming for the (vector) matroid girth problem.* Comput. Optim. Appl. **74**
(2019), 387-441,
[doi:10.1007/s10589-019-00114-9](https://doi.org/10.1007/s10589-019-00114-9).
Spark is NP-hard even for integer matrices, and its **Theorem 5** is the one that
matters here: for a **unimodular** matrix the spark is computable in *polynomial*
time, `ℓ0` collapsing to `ℓ1` on basic solutions, and the vector matroids of
totally unimodular matrices are the regular ones. Since regular matroids are
closed under duality and the quantity this module minimises is a **co**girth, the
theorem reaches it through the dual. **The hypothesis was tested properly and it fails on every operator here**,
including `4x4x4_49_156_L`, which sampling had suggested was regular: it has a
16-column minor of determinant −2, so it is not unimodular and its matroid is not
regular. The route is still what answers that operator, and now for no reason
this bibliography can supply:
[`matrix_sparsification/method/answering-without-searching.md`](matrix_sparsification/method/answering-without-searching.md). **Cited from the citing literature**, not read: the statement
and its matroid corollary are quoted at second hand and the theorem's proof was
not opened.

**`truemper1990`**: K. Truemper. *A decomposition theory for matroids. V.
Testing of matrix total unimodularity.* J. Combin. Theory Ser. B **49**(2):241-281,
1990, [doi:10.1016/0095-8956(90)90030-4](https://doi.org/10.1016/0095-8956(90)90030-4).
Decides total unimodularity in `O((m+n)³)`, on top of Seymour's decomposition of
regular matroids (JCTB **28**(3):305-359, 1980). **Cited from the citing
literature.**

**`cmr`**: M. Walter et al. *CMR: Combinatorial Matrix Recognition*,
[github.com/discopt/cmr](https://github.com/discopt/cmr), MIT. Implements the
simplified Walter-Truemper variant of the above, `O((m+n)⁵)`, and is the
successor of the TUtest library of Walter & Truemper, Math. Prog. Computation
**5**(1):57-73, 2013. **This is the instrument that refuted the regularity of
`4x4x4_49_156_L` in under three milliseconds**, where 200 000 random basis
determinants had found nothing: only about 0.8% of random 16-subsets are bases at
all. `cmr-equimodular M -u` is the exact hypothesis of `[tillmann2019, Thm. 5]`;
`cmr-tu --algo partition` is Ghouila-Houri's characterisation, an independent
code path at `O((m+n)·3^min(m,n))`, which is a quarter of a second at this size.
**Not vendored and not a dependency**: a witness it returns is checked here in
exact arithmetic, and a refutation needs no trust.

**`chenklove2001`, `chenklove2004`**: W. Chen, T. Kløve. *The weight hierarchies
of q-ary codes of dimension 4*, and *On the second greedy weight for linear codes
of dimension at least 4*, IEEE Trans. Inform. Theory **50**(2):354-356, 2004,
with companions in Discrete Math. 241 (2001) and AAECC 1999. **The name "greedy
weight" is theirs and it does not mean what this module means by it.** Chen and
Kløve's `g_r` is the *support* weight of a greedy `r`-dimensional subcode, a
union of supports like `d_r`; what the matroid greedy takes here is the weight of
one vector, and the module's objective is a *sum* of those. Named so the
collision is on the record rather than waiting to be discovered in a referee
report. The papers are structural, bounding `max(g₂ − d₂)` for small dimensions;
there is no algorithm and no code. **Cited from the citing literature.**

**`johnsenverdure2020`**: T. Johnsen, H. Verdure. *Greedy weights for matroids.*
[arXiv:2002.08824](https://arxiv.org/abs/2002.08824), 2020. Lifts Chen and
Kløve's greedy weights to matroids and proves a Wei duality for them. The one
place matroids and this weight literature meet, and it is a duality result rather
than an algorithm. **Abstract only.**

**`sparsevectorfocs2025`**: *Inapproximability of Finding Sparse Vectors in
Codes, Subspaces, and Lattices.* FOCS 2025,
[arXiv:2410.02636](https://arxiv.org/abs/2410.02636). NP-hard to approximate the
sparsest vector in a **real** subspace within any constant factor. Belongs beside
`[tillmann2014]` in
[`matrix_sparsification/what-is-hard-about-it.md`](matrix_sparsification/what-is-hard-about-it.md):
it is the sharpest statement over `R`, and the one that says the exact method
here is not one theorem away from being polynomial. **Abstract only.**

**`narisada2021`**: S. Narisada, K. Fukushima, S. Kiyomoto. *Fast GPU
Implementation of Dumer's Algorithm Solving the Syndrome Decoding Problem.*
IEEE, 2021. The one place low-weight-codeword search really is on a GPU, and it
is information-set decoding for code-based cryptanalysis rather than an exact
minimum. Named so that "put it on the card" has a citation attached to it and a
statement of what that citation actually did. **Abstract and summary only.**

**`bouyuklieva2021`**: S. Bouyuklieva, I. Bouyukliev. *An Extension of the
Brouwer-Zimmermann Algorithm for Calculating the Minimum Weight of a Linear
Code.* Mathematics **9**(19):2354, 2021,
[doi:10.3390/math9192354](https://doi.org/10.3390/math9192354). Uses the *short*
systematic matrix where classical BZ pads a partial information set back up to
size `k` by re-borrowing covered coordinates. **Read in full** (MDPI's own PDF
endpoint returns 403; reached through the Semantic Scholar mirror). **Its Table 1
is the source of the `[115, 60, 13]` codeword counts**: 198 461 377 against
Magma's 6 001 753 644, which is the lower bound doing the work rather than a
faster inner loop, and 28.56 s against 1.52 s on machines the authors say are not
comparable. Together with P. Lisoněk, L. Trummer,
*Algorithms for the minimum weight of linear codes*, Adv. Math. Commun.
**10**(1):195-207, 2016, and *Algorithm 994* (ACM TOMS **45**(2), 2019) for fast
implementations, these are the entry points to that literature.

**`qldpcsat2026`**: *SAT, MaxSAT, and SMT for QLDPC Distance Computation: A
Large-Scale Empirical Study.* [arXiv:2606.12445](https://arxiv.org/abs/2606.12445),
2026. The solver route to the same subproblem, benchmarked across Minisat,
**Glucose**, CaDiCaL, Lingeling and MapleSat. Named because this repository
already forks `kissat` and `cadical` for the rank strand, so the route is open
here at no new dependency. **Abstract and summary only.** Not attempted: at
`⟨3,3,3⟩` rank 23 the exact scan finishes in a third of a second and there is
nothing for a solver to improve.

**`dumas2024cex`**: J-G. Dumas. *Cex_Poldet*, Maple worksheet, 27 May 2024,
unpublished; supplied directly. The determinant-polynomial
feasibility test in `matrix_sparsification/pattern_feasibility.h`, and the
counterexample fixture `fixtures/dumas_counterexample_l.matrix`.

### How hard the sparsity problem is, and under which name

Surveyed 2026-08-22 for
[`matrix_sparsification/what-is-hard-about-it.md`](matrix_sparsification/what-is-hard-about-it.md),
which is where the four names for one problem are set out. **Provenance is marked
on every entry**, because most of this was reached through the citing literature
rather than through the paper, and a bibliography that hides that is worse than a
short one.

**`mccormick1983`**: S. T. McCormick. *A Combinatorial Approach to Some Sparse
Matrix Problems.* Technical report / PhD thesis, Stanford, 1983. The original
NP-hardness of the sparsest-basis problem. **Cited from the citing literature**;
`[gottlieb2010]`, `[tillmann2014]` and `[qu2020]` all name it for this.

**`tillmann2014`**: A. M. Tillmann, M. E. Pfetsch. *The Computational Complexity
of the Restricted Isometry Property, the Nullspace Property, and Related Concepts
in Compressed Sensing.* IEEE Trans. Inform. Theory **60**(2):1248-1259, 2014.
Deciding whether a rational matrix has a circuit of size at most `k` is **strongly**
NP-complete, by a reduction from `k`-CLIQUE adapting `[mccormick1983]`. The
sharpest published statement of spark hardness over `Q`. **Abstract only.**

**`vardy1997`**: A. Vardy. *The intractability of computing the minimum distance
of a code.* IEEE Trans. Inform. Theory **43**(6):1757-1766, 1997; STOC 1997.
NP-completeness of the minimum distance problem over `GF(2)`, by a deterministic
reduction. **Not read: IEEE elides the abstract on every route tried**, so the
problem it reduces from is unknown here and no claim above rests on it.

**`berlekamp1978`**: E. R. Berlekamp, R. J. McEliece, H. C. A. van Tilborg. *On
the inherent intractability of certain coding problems.* IEEE Trans. Inform.
Theory **24**(3):384-386, 1978. Proves **coset weight** and **subspace weight**
NP-complete, the latter being weight *exactly* `w`. Its own abstract says the
result "strongly suggests, but does not rigorously imply" the general case, so
**it is the wrong citation for minimum distance** and is frequently given as one.
**Abstract verbatim; body through a restatement.**

**`downey1999`**: R. G. Downey, M. R. Fellows, A. Vardy, G. Whittle. *The
parametrized complexity of some fundamental problems in coding theory.* SIAM J.
Comput. **29**(2):545-570, 1999. W[1]-hardness of **maximum-likelihood decoding**
and **weight distribution**, and **not** of minimum distance, which it leaves
open. **Paywalled; abstract from two independent renderings.**

**`dumer2003`**: I. Dumer, D. Micciancio, M. Sudan. *Hardness of approximating the
minimum distance of a linear code.* IEEE Trans. Inform. Theory **49**(1):22-37,
2003; FOCS 1999. **Theorem 22 read in full** from the authors' copy: over every
finite field, approximating within any constant is NP-hard under randomised
reductions, and within `2^(log^(1−ε) n)` under quasi-polynomial ones.

**`cheng2012`**: Q. Cheng, D. Wan. *A Deterministic Reduction for the Gap Minimum
Distance Problem.* IEEE Trans. Inform. Theory **58**(11):6935-6941, 2012; STOC
2009. **Theorem 1.4 read**: the same inapproximability without randomness, over
any `F_q`, by Weil character sums.

**`austrin2014`**: P. Austrin, S. Khot. *A Simple Deterministic Reduction for the
Gap Minimum Distance of Code Problem.* IEEE Trans. Inform. Theory
**60**(10):6636-6645, 2014; ICALP 2011. Hardness within `1+γ` **even on
asymptotically good codes**, which `[cheng2012]` left open. **Abstract only.**

**`bhattiprolu2025`**: V. Bhattiprolu, V. Guruswami, X. Ren. *Deterministic
hardness of the minimum distance and nearest codeword problems.*
[arXiv:2503.11131](https://arxiv.org/abs/2503.11131), 2025. Deterministic
hardness over any `F_q` reducing from homogeneous quadratic equations with no PCP
theorem. The cleanest proof to cite. **Abstract only.**

**`bhattacharyya2021`**: A. Bhattacharyya, É. Bonnet, L. Egri, S. Ghoshal,
Karthik C. S., B. Lin, P. Manurangsi, D. Marx. *Parameterized Intractability of
Even Set and Shortest Vector Problem.* J. ACM **68**(3):16, 2021. **Theorem 6.1
read**: `GapMDP_γ` is W[1]-hard under randomised reductions, over `GF(2)` only,
the field restriction explained in their §8. **Its dual formulation, read and
quoted in our page, is the identification of minimum distance with the shortest
circuit of a represented binary matroid.**

**`bennett2023`**: H. Bennett, M. Cheraghchi, V. Guruswami, J. Ribeiro.
*Parameterized Inapproximability of the Minimum Distance Problem over all Fields
and the Shortest Vector Problem in all `ℓ_p` Norms.* STOC 2023,
[arXiv:2211.07900](https://arxiv.org/abs/2211.07900). The same W[1]-hardness over
**any fixed finite field**, which is the citation for `GF(p)`, `p > 2`.
**Abstract read.**

**`stephensdavidowitz2019`**: N. Stephens-Davidowitz, V. Vaikuntanathan.
*SETH-hardness of Coding Problems.* FOCS 2019, pp. 287-301. **Abstract read
verbatim**: under SETH there is no `q^((1−ε)n)` algorithm for the minimum distance
problem over any finite field, for a code with `q^n` codewords. `n` is the
dimension, so **this is the lower bound matching the `q^k` walk in
[`matrix_sparsification/finite_field_sparsifier.h`](matrix_sparsification/finite_field_sparsifier.h)**.

**`qu2020`**: Q. Qu, Z. Zhu, X. Li, M. C. Tsakiris, J. Wright, R. Vidal. *Finding
the Sparsest Vectors in a Subspace: Theory, Algorithms, and Applications.*
[arXiv:2001.06970](https://arxiv.org/abs/2001.06970), 2020. The survey of the
real-field side. **Read in full, and what it does not contain is the point**: no
occurrence of "Vardy", "coding theory", "minimum distance", "Berlekamp", "finite
field" or "GF(2)". For hardness it cites `[mccormick1983]` and
`[colemanpothen1986]` and stops.

**`holtz2025`**: O. Holtz, J. Hsu, S. Moran, O. Schwartz, N. Wiernik.
*Alternative Bases for New Fast Matrix Multiplication Algorithms.* ACDA 2025.
Sparsifies the AlphaTensor and flip-graph algorithms, proves its method optimal
for the alternative-basis model, and gives a general lower bound of 5 on the
leading coefficient. Reproduces `[beniamini2020]`'s Table 2 on almost every row.
**Tables read; the bound's statement read.**

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

What [`integer_programme/`](integer_programme/) is, all of it somebody
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
The near neighbour in this problem area, whose binaries are
`bin/sparsifier`, `bin/factorizer`,
`bin/orbiter`, and for the exchange `bin/PMchecker`, `bin/MMchecker` and
`bin/sms2pretty`. It reaches sparsity by a different route from `beniamini2020`;
sparse QLUP elimination and bounded coefficient search rather than the Ω-valid
oracles.
Not a dependency here: it needs LinBox, which this repository does not.
Thirteen of its `data/` operators are vendored under
[`fixtures/plinopt/`](fixtures/plinopt/) with its licence; what was read
each way is [`formats/interchange/`](formats/interchange/).

**The three operators the sparsification strand reports on are its files too**,
`data/3x3x3_23_Grey-221_{L,R,P}.sms`, and they are not among the thirteen vendored
here. **Where that scheme itself comes from is recorded nowhere this repository can
reach.** Its `data/README.md` gives the filename grammar
`MxKxN_R_text_[L|R|P].sms` and leaves `text` a free label; no file in the checkout
mentions `Grey` in its contents; and the copy read here arrived as one squashed
snapshot, so there is no history to read either. `Grey-221` is therefore used as a
name for three files and is cited to nobody, which is why every surface here says
"a published rank-23 scheme" and none of them says whose.

**`fmm-catalogue`**: A. Sedoglavic. *Yet another catalogue of fast matrix
multiplication algorithms*, [fmm.univ-lille.fr](https://fmm.univ-lille.fr/).
Over five thousand formats with their best known rank, and the reason the
`.tensor` format here has no counterpart in the wild: each entry is published as
a *decomposition*, `_LRP.mpl.bz2` and `_tensor.mpl.bz2`, both of them the same
⟨L,R,P⟩ triple written as Maple matrices, never as the tensor. Not read here;
`operators-to-tensor` reads the SMS spelling of the same object, which PLinOpt
uses and which needs no Maple parser.

**`cbc`**, **`glpsol`**, **`lp_solve`**, **`gurobi_cl`**: The integer programming
backends of [`integer_programme/`](integer_programme/), which is what
[`curve_bounds/`](curve_bounds/)'s step 3 is handed to. Ranked and found
on `PATH` at run time, never linked. CBC (COIN-OR, EPL), GLPK (GNU, GPL) and lp_solve
(LGPL) are in the Ubuntu archive and are the three verified on this machine;
Gurobi is proprietary, free to academics, and its recipe here is unverified for
want of a licence. None is a dependency: absent all four, the built-in exact
simplex and branch and bound answers.
