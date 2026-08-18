# Where the research front is, and where this repository sits on it

Three questions get called "fast matrix multiplication" and they have almost
nothing to do with each other. Keeping them apart is the first thing, because a
result in one says nothing about the others.

| | The question | Who is winning it |
|---|---|---|
| **Upper bounds** | find a decomposition with fewer products | search, and since 2022 machine learning |
| **Lower bounds** | prove no smaller one exists | orbit classification with certificates, and SAT; still the hard side |
| **The exponent** | how does the cost scale asymptotically | the laser method, a separate field entirely |

## Upper bounds: the front moved twice, and neither time by exhaustive search

**`[alphatensor2022]`** put reinforcement learning on it. AlphaZero treated
decomposition as a single-player game and produced 14 236 non-equivalent schemes
for `⟨4,4,4⟩` alone, the first time a learned system improved on human schemes.

**`[kauers2023]`** then matched much of that with no learning at all. The **flip
graph** starts from a decomposition that works and rewrites it: a *flip* swaps a
shared factor between two terms and keeps the sum, so rank is unchanged and the
walk can move sideways for ever, and a *reduction* fires when two terms come to
share two factors, dropping the rank by one. A random walk on that graph found
`⟨5,5,5⟩` in 95.

**`[moosbauer2025]`** added the tensor's own symmetries to the walk and reached
**`⟨5,5,5⟩` in 93 and `⟨6,6,6⟩` in 153**, and `[kauers2025]` generalised the
construction again.

**`[alphaevolve2025]`** then found **`⟨4,4,4⟩` in 48 multiplications over `ℂ`**,
the first improvement on Strassen applied twice, 49, in fifty-six years. That
one needed complex coefficients, which cost arithmetic, so the follow-up work is
about removing them: `[dumas2026]` gives a systematic method that either
converts a complex scheme to a rational one or proves no rational equivalent
exists, generalising Dumas, Pernet and Sedoglavic's earlier ad hoc results.

**The pattern is that every recent record came from walking or evolving a
decomposition that already worked, not from searching a space from nothing.**

## Lower bounds: still the hard direction, and where this repository lives

Nothing above proves anything is optimal. That is the other half, it is where
this repository is, and the front is much closer to us:

- `[bdez2012]` searching subspaces rather than subsets, which
  [`exhaustive_search/`](exhaustive_search/) implements.
- `[covanov2019]` adding the automorphism group, which the orbit work implements.
- `[heule2021]` encoding the question for a SAT solver, which
  [`satisfiability/`](satisfiability/) implements, and `[heule2024]` using SAT
  specifically to rule decompositions out under assumed symmetries.
- `[yang2025]`, and this entry has now been wrong twice, so it is written out at
  length. Exact decision over finite fields in
  `O*(|F|^(min{R, Σ_{d≥2} n_d} + (R−n₀)(Σ_{d≠0} n_d)))` and **polynomial space**.
  **It is implemented and public**, Java and Python, MIT, at
  `github.com/coolcomputery/tensor-cpd-search`, with a border-CPD search and a Z3
  baseline of the same shape as this repository's SAT encoding.

  **Its recursion is not something we lack. It is `expand_subspace`.** Theorem 1
  is subspace extension, not deflation: Algorithm 1 enumerates the factor columns
  for `1 ≤ d < D` only and recovers axis 0 by inverting a matrix, so a candidate
  is a rank-one *matrix* and the pool at `⟨2,2,2⟩` is `(2^4−1)^2 = 225`. The
  paper's own state count for ruling out rank 6 there is
  `Σ_{k≤2} C(225, k) = 25426`, against 25399 for
  [`expand_subspace`](exhaustive_search/exhaustive_search.h) on the same
  question. **Agreement to a tenth of a percent on three questions is empirical
  proof the two walk the same tree**, and they do, because both are `[bdez2012]`
  Algorithm 1. Wall-clock comparisons between the two cross a JVM boundary and
  are **not quoted here**: where the node counts match, a time difference
  measures the runtime, not the algorithm.

  **Polynomial space is real, and it is narrower than it sounds.** The *search*
  state here is already polynomial, a basis plus an index. What is not is the
  *pool*: `all_rank_one_maps` materialises it, which
  [`descent_search/method.md`](descent_search/method.md) already
  diagnoses and which is 4.3e9 maps at `⟨4,4,4⟩`. Yang walks it with an in-place
  odometer instead. So the whole difference is the pool, and an iterator is the
  whole fix.

  **What was genuinely missing is the pruners, which shrink the tree rather than
  re-deriving it.** Both rank-sum bounds are now
  [`linear_algebra/tensor_rank_sum.h`](linear_algebra/tensor_rank_sum.h):
  `ranksum` and `lask`, the latter being Laskowski's bound, Theorem 3 of the
  thesis. `rref` is not ported. Note that none of the three is in the Theorem-1
  implementation either, which contains no pruner at all, so the paper's own
  timings are unpruned and no implementation anywhere combines the two halves.

## An instrument built here, measured, and retired: rank as a MILP

Brent's equations were also written here as a mixed integer programme, so a third
instrument would answer the same question as the SAT strand and the tree search on
identical instances. `[deza2023]` solves those equations by constraint programming
and the 2x2 and 3x3 cases are MIPLIB 2017 benchmarks, so the formulation was
standard; the open part was whether a MILP solver could compete on this question.
**It cannot.** Fastest of three runs on a quiet machine, seconds:

| question | answer | tree search | MILP | SAT |
|---|---|---|---|---|
| `f2_2x2` k=3 | yes | 0.00 | 0.27 | 0.01 |
| `f2_2x2` k=2 | no | 0.00 | 3.34 | 0.01 |
| `f2_2x3` k=5 | yes | 0.00 | 2.77 | 0.01 |
| `f2_2x3` k=4 | no | 0.00 | **no answer in 45 s** | 0.01 |
| `gf4_multiplication` k=2 | no | 0.00 | 1.37 | 0.01 |
| `gf8_multiplication` k=6 | yes | 0.00 | 26.76 | 0.02 |

Two to three orders of magnitude behind throughout, and it fails in 45 s a
question the tree search settles in under 0.01 s. The automorphism quotient is
worth 1.9x to 9x to it and rescues nothing. It also leaked its `cbc` badly enough
to defeat three measurement runs, so the comparison table was finally taken with
the MILP row excluded. **A MILP is the wrong instrument for deciding tensor rank**,
and the encoding is retired rather than left as a fourth column nobody would run.

The solver chain underneath it stays, because it has a consumer that suits it:
[`curve_bounds/`](curve_bounds/README.md) step 3 is a genuine integer programme of
25 variables, and there the same machinery wins. The retired work is in the
history: `git log --diff-filter=D --stat -- integer_programme/` names its files.

## The baseline a refutation here is measured against

**`[wang2026]`**, March 2026, and it is the front on the infeasible side. Chengu
Wang classifies the orbits of constraint subspaces under a group of
rank-preserving symmetries acting on one argument, runs a dynamic program over
the orbits combining four lower-bound techniques, and emits a certificate a
separate verifier rechecks. **It raises `⟨3,3,3⟩` over F₂ from 19 to 20**,
retiring `[blaser2003]`'s bound after twenty-three years, improves three more
small formats, and adds eighteen bounds for **polynomial multiplication**, which
is what every fixture here is.

**It is implemented and public**, MIT-licensed C++ under the author's own name at
`github.com/wcgbg/tensor-rank-lower-bound`, so nothing here is unimplemented
ground. The paper reports the `⟨3,3,3⟩` proof found "in about 40 minutes on a
laptop" with the certificate verifying "in seconds"; **it publishes no
per-instance timing table, so any sharper verification figure is not a quotable
number** and this file does not invent one.

**Where that leaves this strand, stated plainly.** The shape is the same and the
reach is not. Both search and then hand a refusal to an independent checker, and
that discipline is the one place the two are level: a DRAT proof rechecked by
`drat-trim` is exactly Wang's certificate argument in a different notation. But
Wang settles `⟨3,3,3⟩`, and the largest thing this encoding refutes is far
smaller: `f3_3x6` does not answer at ten in 300 s, though the exhaustive search
settles that map at nine in under eight seconds, and `f2_5x5` is only bracketed
at 12 ≤ rank ≤ 14, where `[bdez2012]` settled 13 by exhaustive search in 2012.
**The gap is not the certificate, it is the orbit
classification and the dynamic program in front of it**: a monolithic CNF asks
one enormous question where Wang asks many small ones and combines them. That is
the same lesson the cube work reaches from the other end.

## The exponent, for completeness

`ω < 2.371339` (`[alman2025]`), improving Duan, Wu and Zhou and then Vassilevska
Williams, Xu, Xu and Zhou. This is the laser method on border rank and shares no
machinery with anything here. It is also famously not implementable: the schemes
behind it are galactic.

## So where are we

**On the lower-bound side we are close to the front and on it in one place.**
The SAT strand is the same technique as `[heule2021]`, and the measurement that
Kissat beats CryptoMiniSat five times on these instances while native XOR is
worth nothing is not in any paper I could find.

**The sharpest positioning available is `[chen2025]`**, and it is uncomfortable
in the useful way. Chen and Kauers apply the flip graph to *polynomial
multiplication*, which is what every `.tensor` fixture here is, and prove the
schemes optimal **with a SAT solver** (Theorem 7). That is precisely the
division of labour between these two strands, published February 2025. This
repository did not invent the pairing; it rebuilt it, on the same problem
class, without knowing.

**How much of our ground they already cover**, once their degrees are
translated into term counts (`n+m+1` is Toom-Cook, so their `(n,m)` is our
`(n+1)x(m+1)`): their proven-optimal list is 2x2, 2x3, 2x4, 2x5, 2x6, 3x3, 3x4,
3x5 and 4x4 over `Z2`. So **`f2_2x2` and `f2_2x3` are theirs already**, and
`f2_3x8`, `f2_4x7` and `f2_5x5` are not. Their rank is not therefore unknown:
`[bdez2012]` settled `f2_5x5` at 13 and `f3_3x6` at 10 in 2012, and this
repository brackets `f2_5x5` at 12 ≤ rank ≤ 14 by its own searches, which is
narrower than nothing and wider than the published answer. **`f2_4x7` is the one
genuinely open map here**, at 15 ≤ rank ≤ 16, their lower bound against our
upper one; deciding 15 would close it and neither side has.

They also state the asymmetry this repository is built around, in their own
words: "Flip graphs are useful for finding low-rank tensor representations, but
it is not clear how to use the technique for checking whether an optimum has
been reached."

And their open question is the symmetry both strands attacked today: "constant
factors can be freely moved between the components of a rank-one tensor ... it
is unclear what is the best way of doing this. This may be an explanation why an
automated search in the flip graph works best for `K = Z2`." Two independent
answers were measured here on 2026-08-16 and both are negative: quotienting by
that freedom makes the flip graph run over `GF(3)` without making it
competitive, and breaking the same symmetry in the SAT encoding is sound and
does not rescue `f3_3x6`.

**On the upper-bound side we were a decade behind until today**, when the flip
graph landed on the orbit branch and recovered Strassen by walking. That is
`[kauers2023]`, the 2023 method, and reaching `[moosbauer2025]` means adding
symmetry to the walk, which is exactly the group the orbit work already computes.

**What is missing from this repository as a whole**: `[yang2025]`'s algorithm,
symmetry-aware flip graphs, and any evolutionary or learned search. The last is
not a weekend's work and needs hardware this laptop does not have.

**What is missing from the solver strand specifically is a shorter and
different list**, and it is worth separating, because a feature another design
needs is not automatically a gap in this one:

- **Proof logging is no longer on this list**, and it was the first item on it.
  `--proof` writes kissat's DRAT refutation and `drat-trim` rechecks it, so a
  lower bound from this strand now rests on two programs sharing no code instead
  of on one. What each verdict rests on is
  [`satisfiability/correctness.md`](satisfiability/correctness.md).
- **Incremental solving.** A sweep re-encodes and re-solves from scratch at
  every `k`, so nothing learned at `k` is reused at `k+1`. The clauses differ
  only in the number of terms.
- **The instances that do not answer**: `f2_5x5` at twelve. `f3_3x6` was on this
  list and is off it, and how it came off is the lesson. Nobody asked it the
  cheap question. The solver was asked `--target 10`, which is a *find*, and it
  is `--target 9`, a *refutation*, that settles the rank: `decide-rank` returns
  NO exhaustively in **7.65 s over 4729 nodes**, so no ten-product algorithm is
  beaten and `rank(f3_3x6) = 10`. The instance was never out of reach; the
  question was being asked the expensive way round.

**Conciseness reduction is not on that list, and an earlier version of this
file wrongly implied it was.** It is an internal step of a *recursive* search,
which re-compresses the residual tensor at every node; a single monolithic
encoding has no nodes to do it at. Applied once at the top it would help only a
tensor that is not concise, and **every fixture in this repository is concise**,
measured: the flattening ranks equal the shape on all twelve. It would buy
nothing here. Flip graphs are likewise not a gap in this strand, since they
produce upper bounds only and this strand exists for the other direction.

**And a correction about how this file was written.** The first version said
`[yang2025]` was "not implemented here", which was true of this repository and
read as though no implementation existed. One does, it is public, and I had
read the paper's abstract before writing that sentence without looking for its
code. The search that would have found it is `gh search repos "tensor CPD"`,
where it is the first hit; the searches I actually ran were for "tensor rank
SAT" and "matrix multiplication SAT solver", which return nothing at all. The
lesson: search the problem's own vocabulary, not the vocabulary of the method
you already chose.

Full citations, with what each contributes: [`references.md`](references.md).
