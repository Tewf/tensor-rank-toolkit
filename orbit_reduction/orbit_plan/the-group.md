# The group, exactly

`σ = (X, Y) ∈ GL_n(K) × GL_m(K)` acting on an `n × m` map by `M ↦ Xᵀ M Y`.
Call it `RPA`. Two facts make everything else work:

- **Prop. 1.18**: `RPA` preserves rank, of a single form and of a subspace.
- **Prop. 1.19**: `RPA` is *all* of the rank-preserving automorphisms. There
  is no larger group to wish for later.

**The transposition `τ` is not in it, and this page planned for it.** When
`n = m` there is a further rank-preserving map, `Φ ↦ Φ ∘ τ` with
`τ(a, b) = (b, a)`, and an earlier draft of this plan put it in the group and
gave `Symmetry` a `transposed` flag. It was never built:
[`automorphism.h`](../automorphism.h) carries `{left, right}` and nothing else.

That is deliberate and it follows the source. `[covanov2019, Rem. 10]` says in
as many words that *"for simplicity, we do not take into account the possible
transposition τ"*, so the paper's `RPA` is the pair alone, which is what the code
implements. Leaving the claim here would have been worse than an untidy document:
a group element that is not really a symmetry of the span is the one way
[`orbit_search.h`](../orbit_search.h) can report a false `NO`, and a reader
adding `τ` on this page's authority would have introduced exactly that.

Adding `τ` is sound and available, since it does preserve rank; it would need
`stabiliser_of` to keep only the elements that fix the span, as it already does,
and the pool action to be closed under it, which it is. Nobody has measured what
it would be worth.

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
