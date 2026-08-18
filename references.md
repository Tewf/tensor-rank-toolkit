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

**`jaja1979`**: J. Ja'Ja'. *Optimal evaluation of pairs of bilinear forms.* SIAM
Journal on Computing **8** (1979), no. 3, 443-462; STOC 1978. Determines the
rank of any `p x q x 2` tensor in polynomial time, through the Kronecker theory
of matrix pencils. It marks where the exhaustive machinery here starts being
necessary: at two slices the answer is a canonical form, and the searches in
this repository are for what lies past that.

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
expensive half here is the refutation. Named in
[`positioning/hardware-and-parallelism.md`](positioning/hardware-and-parallelism.md),
which also records that GPU CDCL is reported as slower than CPU CDCL. The author
list is from the arXiv record and the first name may not be the one to cite by.

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
The `nauty` lineage; the refinement-based canonical labelling that makes the
invariant cheap is the part not implemented here, and is what
[`deduplication-cost.md`](oracle_guided_search/deduplication-cost.md) measures the
absence of.

**`covanov2019`**: S. Covanov. *Improved Method for Finding Optimal Formulae
for Bilinear Maps in a Finite Field.*
[arXiv:1705.07728v3](https://arxiv.org/abs/1705.07728), 2018.
Definition 7 and Definition 13 are the automorphism action and the setwise
stabiliser; Algorithm 3 is `BDEZStab`; Definitions 20 and 22 and Algorithm 4 are
the covering-sets method; Propositions 28 and 29 are the stems for the short
product and the matrix product.

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

## Deciding rank with a solver

**`heule2021`**: M. J. H. Heule, M. Kauers, M. Seidl. *New ways to multiply
3 × 3-matrices.* Journal of Symbolic Computation **104** (2021), 899-916.
The SAT encoding of tensor decomposition over `Z/2Z`, and the method that
actually produced new schemes at that size.

**`heule2019`**: M. J. H. Heule, M. Kauers, M. Seidl. *Local search for fast
matrix multiplication.* SAT 2019, [arXiv:1903.11391](https://arxiv.org/abs/1903.11391).

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
Finite Fields.* [arXiv:2603.07280](https://arxiv.org/abs/2603.07280), March 2026.
Classifies the orbits of constraint subspaces under a group of rank-preserving
symmetries acting on one argument, runs a dynamic program over the orbits
combining flattening, degenerate reduction, forced product and substitution with
backtracking, and emits a certificate a separate verifier rechecks. **Raises
`⟨3,3,3⟩` over F₂ from 19 to 20**, plus `⟨2,3,4⟩` to 19, `⟨3,3,4⟩` to 25 and
`⟨3,4,4⟩` to 29, and eighteen new bounds for polynomial multiplication over F₂
and F₃. Implemented and public: MIT-licensed C++ at
[github.com/wcgbg/tensor-rank-lower-bound](https://github.com/wcgbg/tensor-rank-lower-bound).

**`blaser2003`**: M. Bläser. *On the complexity of the multiplication of matrices
of small formats.* Journal of Complexity **19** (2003), no. 1, 43-60. The source
of the `⟨3,3,3⟩` bound of 19 that stood for twenty-three years, and of the
`⟨3,3,4⟩` bound `wang2026` improves.

## Searching for decompositions, which is the other direction

**`alphatensor2022`**: A. Fawzi et al. *Discovering faster matrix multiplication
algorithms with reinforcement learning.* Nature **610** (2022), 47-53.
AlphaZero applied to decomposition as a single-player game; 14 236 non-equivalent
schemes for `⟨4,4,4⟩`.

**`kauers2023`**: M. Kauers, J. Moosbauer. *Flip Graphs for Matrix
Multiplication.* ISSAC 2023, [arXiv:2212.01175](https://arxiv.org/abs/2212.01175).
Rewriting a working decomposition rather than searching for one. `⟨5,5,5⟩` in 95
with no machine learning.

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
greedy in `greedy_sparsifier.h`; Claim 2.11 is the additive complexity in
`algorithm_cost.h`.

**`beniamini2019`**: G. Beniamini, O. Schwartz. *Faster Matrix Multiplication
Via Sparse Decomposition.* SPAA 2019, pp. 11-22.
Definition 2.8 is the trilinear identity `algorithm_check.h` verifies; Claim 3.9
and Corollary 3.10 are the arithmetic complexity and its leading coefficient;
Definition 3.5 and Algorithm 2 are the decomposed recursive-bilinear algorithm.

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

**`ballet2021`**: S. Ballet et al. *On the Tensor Rank of Multiplication in
Finite Extensions of Finite Fields and Related Issues in Algebraic Geometry.*
Russian Mathematical Surveys **76** (2021), no. 1, 29-89. The survey the bound
tables are taken from.

**`rousseau2021`**: É. Rousseau. *Arithmétique Efficace des Extensions de Corps
Finis.* Thèse, Institut Polytechnique de Paris, 2021. NNT 2021IPPAT013,
[tel-03299466](https://theses.hal.science/tel-03299466). Context, not implemented
here.

**`akleylek2014`**: S. Akleylek, F. Özbudak, C. Özel. *On the Arithmetic
Operations Over Finite Fields of Characteristic Three with Low Complexity.*
Journal of Computational and Applied Mathematics **259** (2014), 546-554.
Context, not implemented here.

## Walking a decomposition

Where this repository stands against all of these:
[`positioning/`](positioning/README.md).

**`kauers2023`**: M. Kauers, J. Moosbauer. *Flip graphs for matrix
multiplication.* Proc. ISSAC'23, 381-388. arXiv:2212.01175. The method
[`flip_graph.h`](flip_graph/flip_graph.h) implements: random walks on a graph
whose vertices are decompositions, where a flip preserves the rank and a
reduction lowers it.

**`chen2025`**: S. Chen, M. Kauers. *Flip graphs for polynomial multiplication.*
arXiv:2502.06264. The same walk on this repository's own subject, over `Z2`, with
optimality proved by SAT for every degree pair up to `(3,3)`. Their closing
question, polynomial multiplication over `Z3`, `Z5` and `Z7`, and the obstacle
they name for it, are the one opening this repository has.

**`moosbauer2025`**: J. Moosbauer, M. Poole. *Flip graphs with symmetry and new
matrix multiplication schemes.* arXiv:2502.04514. The walk restricted to schemes
admitting a group action: `5x5` in 93 multiplications, `6x6` in 153.

**`ikenmeyer2025`**: C. Ikenmeyer, J. Moosbauer. *Strassen's algorithm via orbit
flip graphs.* arXiv:2503.05467. Strassen's 7 reproved from an order-6 group
action, with no calculation and no pattern matching.

**`arai2024`**: Y. Arai, Y. Ichikawa, K. Hukushima. *Adaptive flip graph
algorithm for matrix multiplication.* Proc. ISSAC'24, 292-298. arXiv:2312.16960.
Transitions that do not strictly reduce the count, and a constrained search range.

**`kauers2025meta`**: M. Kauers, I. Wood. *Exploring the meta flip graph for
matrix multiplication.* arXiv:2510.19787.

**`perminov2026`**: A. I. Perminov. *Fast matrix multiplication in small formats:
discovering new schemes with an open-source flip graph framework.*
arXiv:2603.02398, code at
[github.com/dronperminov/FastMatrixMultiplication](https://github.com/dronperminov/FastMatrixMultiplication),
MIT. Bit-level encoding, OpenMP, 680 formats from `(2,2,2)` to `(16,16,16)`, and
a GPU variant. **The baseline for any flip graph number produced here.**

**`sedoglavic2024`**: A. Sedoglavic. *Yet another catalogue of fast matrix
multiplication algorithms.* [fmm.univ-lille.fr](https://fmm.univ-lille.fr/). The
field's running record of best known upper bounds.

**`deza2023`**: A. Deza, C. Liu, E. B. Khalil, P. Vaezipoor. *Fast matrix
multiplication without tears: a constraint programming approach.* Proc. CP 2023,
LIPIcs vol. 280. arXiv:2306.01097. The Brent equations solved by constraint
programming. The 2x2 and 3x3 cases are MIPLIB 2017 benchmarks, so the formulation
is standard and nothing here is new. This repository stated the same equations for
a MILP solver, measured them against the SAT strand and the tree search, and
retired the encoding: [`state-of-the-art/rank-as-a-milp.md`](state-of-the-art/rank-as-a-milp.md).

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
Programming - User's Manual*, form **H20-0476**. The origin of the fixed-column
MPS format, whose eighty columns are a punched card. The form number is IBM's
own, from *Catalog of Programs for IBM System/360, Models 25 and above*,
GC20-1619-8, January 1971, which lists it under the MPS/360 documentation.
**The manual itself was not read and no scan of it appears to exist**; the
edition suffix and year are unsettled, secondhand sources giving 1968, 1969 and
1976 for the same form number, so none is stated here.

**What was checked instead is the format, against three descriptions that
agree**: the fields begin at columns 2, 5, 15, 25, 40 and 50, and an integrality
marker puts its own name in field 2, `'MARKER'` in field 3, and `'INTORG'` or
`'INTEND'` in field 5. CPLEX's file-format manual says field 4, counting
whitespace-separated tokens in a format it no longer parses by column, which is
a disagreement worth knowing about rather than a second opinion.

**`murtagh1981`**: B. A. Murtagh. *Advanced Linear Programming: Computation and
Practice.* McGraw-Hill, New York, 1981, ISBN 0-07-044095-6, xii + 202 pp. The
description of MPS most bibliographies point at, this one included. **The pages
usually quoted, 163-166, could not be verified**: every copy reachable is
lending-restricted. The book exists and the range fits inside it, which is not
the same as having read it.

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
