# The group, exactly

`σ = (X, Y) ∈ GL_n(K) × GL_m(K)` acting on an `n × m` map by `M ↦ Xᵀ M Y`,
plus the transposition `τ` when `n = m` (Covanov Def. 1.16, Remark 1.20).
Call it `RPA`. Two facts make everything else work:

- **Prop. 1.18**: `RPA` preserves rank, of a single form and of a subspace.
- **Prop. 1.19**: `RPA` is *all* of the rank-preserving automorphisms. There
  is no larger group to wish for later.

The relevant subgroup is `Stab(T) = { σ : span(T) ∘ σ = span(T) }`. Note this
is the **setwise** stabiliser of the span, not of the slice tuple: the search
only ever reads `span(T)`, so any change of basis among the slices is free.

## Why it is sound, and why `row_space_representatives` is not

For `σ ∈ Stab(T)`, `S_r(T) ∘ σ = S_r(T)`: the solution set is closed under the
group, so enumerating one representative per orbit loses nothing (Covanov
Prop. 2.6). [`candidate_pool.h`](../../descent_search/candidate_pool.h) already says its
`row_space_representatives` must not be wired in, and it is right, because that quotients by
row space alone, which fixes almost no span. It is not a weaker version of this;
it is a different equivalence.
