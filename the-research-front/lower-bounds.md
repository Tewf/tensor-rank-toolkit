# Lower bounds: still the hard direction, and where this repository lives

Nothing above proves anything is optimal. That is the other half, it is where
this repository is, and the front is much closer to us:

- `[bdez2012]` searching subspaces rather than subsets, which
  [`exhaustive_search/`](../exhaustive_search/README.md) implements.
- `[covanov2019]` adding the automorphism group, which the orbit work implements.
- `[heule2021]` encoding the question for a SAT solver, which
  [`satisfiability/`](../satisfiability/README.md) implements, and `[yang2024sat]` using SAT
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

  **`rref` was ported to nothing, because it was measured first.** What
  `--pruners rref` runs is `prune_rref0` in `cpd/original/cpd_search.py`, and not
  the wedge-and-Grassmannian test in `other/k-th order rref pruning.ipynb` that
  an earlier version of this page described. The pruner argues from the weight of
  an rref row: a rank-`R` CPD forces `n_0` independent vectors `v` with
  `rk(v ._0 T) <= R - n_0 + 1` to exist, and refuses the rank when they do not.

  **It never beats `rank_lower_bound` on any fixture here**, ties on five and
  loses on fifteen, including 11 against our 14 on `⟨3,3,3⟩`. The whole table,
  what the two names mean, and the one thing it does that no bound here does:
  [`what-rref-is-worth.md`](what-rref-is-worth.md).

  It leaves `⟨2,2,2⟩` where it was. Our `rank_lower_bound` is the maximum over the
  flattening rank, both rank sums and Griesmer, and on `⟨2,2,2⟩` that maximum is
  **6**; ruling out 6 then costs `decide-rank` **25 399 nodes and 0.65 s** of
  exhaustion. That shape is still one where every bound we have stops one short of
  the answer and a search pays for the last step, and the literature's one
  candidate aimed at that gap has now been tried.
