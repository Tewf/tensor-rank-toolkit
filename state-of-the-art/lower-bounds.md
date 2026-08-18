# Lower bounds: still the hard direction, and where this repository lives

Nothing above proves anything is optimal. That is the other half, it is where
this repository is, and the front is much closer to us:

- `[bdez2012]` searching subspaces rather than subsets, which
  [`exhaustive_search/`](../exhaustive_search/) implements.
- `[covanov2019]` adding the automorphism group, which the orbit work implements.
- `[heule2021]` encoding the question for a SAT solver, which
  [`satisfiability/`](../satisfiability/) implements, and `[heule2024]` using SAT
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
  [`expand_subspace`](../exhaustive_search/exhaustive_search.h) on the same
  question. **Agreement to a tenth of a percent on three questions is empirical
  proof the two walk the same tree**, and they do, because both are `[bdez2012]`
  Algorithm 1. Wall-clock comparisons between the two cross a JVM boundary and
  are **not quoted here**: where the node counts match, a time difference
  measures the runtime, not the algorithm.

  **Polynomial space is real, and it is narrower than it sounds.** The *search*
  state here is already polynomial, a basis plus an index. What is not is the
  *pool*: `all_rank_one_maps` materialises it, which
  [`descent_search/method/descent-cost.md`](../descent_search/method/descent-cost.md)
  already
  diagnoses and which is 4.3e9 maps at `⟨4,4,4⟩`. Yang walks it with an in-place
  odometer instead. So the whole difference is the pool, and an iterator is the
  whole fix.

  **What was genuinely missing is the pruners, which shrink the tree rather than
  re-deriving it.** Both rank-sum bounds are now
  [`linear_algebra/tensor_rank_sum.h`](../linear_algebra/tensor_rank_sum.h):
  `ranksum` and `lask`, the latter being Laskowski's bound, Theorem 3 of the
  thesis. Note that neither is in the Theorem-1 implementation either, which
  contains no pruner at all, so the paper's own timings are unpruned and no
  implementation anywhere combines the two halves.

  **`rref` is not ported, and it is the one worth porting.** Read from the
  author's own repository (`coolcomputery/tensor-cpd-search`, the notebook
  `other/k-th order rref pruning.ipynb`) rather than from the paper, whose
  abstract does not mention it. It is **not** a reduction of the rank-one pool,
  which is what the name suggests and what two readers here guessed: it is a
  **higher-order lower bound**. Contract the tensor with `k`-tuples of vectors,
  wedge them, and if the resulting `k`-planes fail to span enough of the
  Grassmannian then no CPD of that rank exists. The author's own headline use of
  it is proving the Strassen tensor has **no rank-6 CPD over GF(2)**, with no
  search.

  That is precisely where this repository still pays a search.
  `rank_lower_bound` gives `⟨2,2,2⟩` a floor of **6**, and ruling out 6 costs
  `decide-rank` **25 399 nodes and 0.65 s** of exhaustion. A ported `rref` would
  settle the same step in polynomial time, and 6 is not a number any bound here
  reaches by another route. It belongs beside the rank sums in
  [`rank_lower_bound.h`](../linear_algebra/rank_lower_bound.h), whose docstring
  already takes the maximum over every bound it has.
