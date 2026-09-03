# The group, exactly

`σ = (X, Y) ∈ GL_n(K) × GL_m(K)` acting on an `n × m` map by `M ↦ Xᵀ M Y`.
The action is `[covanov2019, Def. 7]` and the matrix form is
`[covanov2019, Not. 11]`; in the thesis the same two are `[covanov2018,
Def. 1.16]` and `[covanov2018, Rem. 1.20]`. Two facts make everything else work:

- `[covanov2019, Prop. 9]`: the action preserves rank, of a single form and of
  a subspace, stated as the two lines `rk(Φ ∘ σ) = rk(Φ)` and
  `rk(T ∘ σ) = rk(T)`. `[covanov2018, Prop. 1.18]` is the same two lines with
  the same proof, for the larger group described below.
- `[covanov2018, Prop. 1.19]`: those are *all* the rank-preserving
  automorphisms. **The paper has no counterpart**, so this claim is the thesis
  only, and it does not say what this page used to say. See below.

## The transposition, and the name `RPA`

**This page used to call the pair group `RPA` and say the transposition `τ` was
not in it. Both halves were wrong about the thesis.** When `n = m` there is a
further rank-preserving map, `Φ ↦ Φ ∘ τ` with `τ(a, b) = (b, a)`, and
`[covanov2018, Def. 1.16]` defines `RPA_{m,n}` as **the smallest group
containing `GL(K^m) × GL(K^n)` and, when `m = n`, that `τ`**. So the thesis's
`RPA` is strictly larger than the pair group whenever the map is square, which
is every fixture here.

That matters for the second bullet above. `[covanov2018, Prop. 1.19]` says the
elements of `RPA_{m,n}` are the only rank-preserving automorphisms, and it is
true of the group *with* `τ`. Asserted of the pair group alone it is false, and
false in exactly the square case, because `τ` is the witness against it. The
thesis does not prove it either: its proof is one line pointing at
`[burichenko2014]`, which is where anyone wanting the argument has to go.

The code is right and only the page was wrong.
[`automorphism.h`](../automorphism.h) carries `{left, right}` and no
`transposed` flag, and that follows the *paper*, whose `[covanov2019, Def. 7]`
is the pair alone and whose `[covanov2019,
Rem. 10]` says in as many words that *"for simplicity, we do not take into
account the possible transposition τ"*. An earlier draft did plan a `transposed`
flag; leaving that here would have been worse than an untidy document, since a
group element that is not really a symmetry of the span is the one way
[`orbit_search.h`](../orbit_search.h) can report a false `NO`.

Adding `τ` is sound and available, since it does preserve rank; it would need
`stabiliser_of` to keep only the elements that fix the span, as it already does,
and the pool action to be closed under it, which it is. Nobody has measured what
it would be worth. What is now clear is that it is not a free extra: it is the
difference between the paper's group and the thesis's maximal one.

The relevant subgroup is `Stab(T) = { σ : span(T) ∘ σ = span(T) }`,
`[covanov2019, Def. 13]`. Note this is the **setwise** stabiliser of the span,
not of the slice tuple: the search only ever reads `span(T)`, so any change of
basis among the slices is free.

At `⟨2,2,2⟩` over GF(2) this `Stab(T)` has 216 elements, reached by
`decide-rank fixtures/matmul_2x2x2.tensor --target 6 -s matmul 2 2 2`
([`../group_construction.h`](../group_construction.h)).

## Why it is sound, and why `row_space_representatives` is not

Two separate results, and this page used to run them together under one number.

For `σ ∈ Stab(T)`, `S_r(T) ∘ σ = S_r(T)`: the solution set is closed under the
group. That is the displayed equation opening `[covanov2018]` §2.2.4, and the
thesis derives it from `[covanov2018, Prop. 1.18]`, the rank-preservation above,
with the words *"because σ preserves the rank"*. It is **not** Prop. 2.6.

What licenses keeping one representative per orbit is the separate
`[covanov2019, Prop. 14]`, the thesis's `[covanov2018, Prop. 2.6]`: if a
`V ∈ S_r(T)` meets the `Stab(T)`-orbit of a rank-one `φ`, then some `V'` in
`V`'s class contains `φ` itself. Closure says the quotient is well defined;
Prop. 14 says a representative may be substituted at a node. The search needs
both, so citing one for the other left half the argument unsourced.

[`candidate_pool.h`](../../greedy_heuristic/candidate_pool.h) already says its
`row_space_representatives` must not be wired in, and it is right, because that
quotients by row space alone, which fixes almost no span. It is not a weaker
version of this; it is a different equivalence.
