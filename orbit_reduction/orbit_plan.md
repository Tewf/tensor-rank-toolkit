# Plan: orbit methods, and where they belong in this repository

**This plan has been carried out, under different names.** It was written on a
branch that never held the code, so read it for the argument and the sources, not
as work outstanding. Its four items landed here as:

| planned | built as |
|---|---|
| 1. `symmetry_group` and `stabilises` | [`automorphism.h`](automorphism.h) |
| 2. orbits of the pool by union-find | [`pool_orbits.h`](pool_orbits.h) |
| 3. generators, each verified before use | [`group_construction.h`](group_construction.h) |
| 4. wired into `expand_subspace`, same answers | [`orbit_search.h`](orbit_search.h), proved by [`tests/test_symmetry_agreement.cpp`](tests/test_symmetry_agreement.cpp) |

What it says about the heuristic being a separate question, and about the pool
having to be closed under the action, still governs.

The published continuation of the algorithm in
[`exhaustive_search.h`](../exhaustive_search/exhaustive_search.h). Source: Covanov, *Multiplication
algorithms: bilinear complexity and fast asymptotic methods*, thesis 2018,
§1.3 and §2.2.4, Algorithm 6 (`BDEZStab`), attributed there as an unpublished
improvement to BDEZ by its own authors. The BDEZ paper's conclusion names
"using the symmetries of the problem" as the thing it did not do.

## The group, exactly

`σ = (X, Y) ∈ GL_n(K) × GL_m(K)` acting on an `n × m` map by `M ↦ Xᵀ M Y`,
plus the transposition `τ` when `n = m` (Covanov Def. 1.16, Remark 1.20).
Call it `RPA`. Two facts make everything else work:

- **Prop. 1.18**: `RPA` preserves rank, of a single form and of a subspace.
- **Prop. 1.19**: `RPA` is *all* of the rank-preserving automorphisms. There
  is no larger group to wish for later.

The relevant subgroup is `Stab(T) = { σ : span(T) ∘ σ = span(T) }`. Note this
is the **setwise** stabiliser of the span, not of the slice tuple: the search
only ever reads `span(T)`, so any change of basis among the slices is free.

## Why it is sound, and why `one_per_row_space` is not

For `σ ∈ Stab(T)`, `S_r(T) ∘ σ = S_r(T)`: the solution set is closed under the
group, so enumerating one representative per orbit loses nothing (Covanov
Prop. 2.6). [`candidate_pool.h`](../descent_search/candidate_pool.h) already says its
`one_per_row_space` must not be wired in, and it is right, because that quotients by
row space alone, which fixes almost no span. It is not a weaker version of this;
it is a different equivalence.

## The algorithm

`BDEZStab` prunes at **every** node, not only the widest one:

```
expand(V, H, U, d, r):            # H remaining pool, U ⊆ Stab(T) stabilising V
    if d == r: succeed iff V has a rank-one basis
    O ← orbits of H under U
    for i, orbit in enumerate(O):
        φ ← representative(orbit)
        expand(V ⊕ ⟨φ⟩, ∪_{j≥i} O_j, Stab(V ⊕ ⟨φ⟩) ∩ U, d+1, r)
```

Two details carry the correctness. The `from`-index pruning already in
`expand_subspace` survives, but it now applies **to orbits, not to elements**
(`∪_{j≥i} O_j`). And `U` shrinks by intersection as the subspace grows, which is
why deeper levels prune less and why this is not just a level-1 trick.

## What to build, in order

1. `symmetry_group.h/.cpp`: `Symmetry {left, right, transposed}`, the action,
   and `stabilises(field, σ, span)`. Verification is cheap and exact: `X`, `Y`
   invertible, and every basis slice's image back inside the span.
2. Orbits of the pool under an explicit element list, by union-find. `U` is
   carried as a list, so `Stab(V ⊕ ⟨φ⟩) ∩ U` is a filter, not group theory.
3. Sources of generators (below), each **verified before use**.
4. Wire into `expand_subspace` behind a parameter, and prove it by re-running
   every existing exact-search assertion with orbits on: same answers, fewer
   nodes. That test is the deliverable, not the speedup.

## Where `Stab(T)` comes from, which is the honest hard part

Computing it in general is tensor isomorphism, harder than graph isomorphism.
Do not attempt it. Take generators from three places and verify all of them:

- **Substitution, for polynomial multiplication.** `g ∈ GL₂(K)` acts on binary
  forms by substitution, and substitution is multiplicative, so
  `A_gᵀ T_i B_g ∈ span(T)`. This gives `PGL₂(K)`: order 6 over `F₂`, 24 over
  `F₃`. `X ↦ 1/X` is the reversal; the slides give `X ↦ X+1` as the other
  generator. Derived rather than guessed, but assert it in a test.
- **Monomial pairs**, general and cheap: sweep permutation-and-scaling pairs,
  keep what stabilises. Budgeted, works on any tensor, finds order 2 on the
  Hankel fixtures.
- **A file**, for a group the caller knows and the tool cannot derive.

**The safety property that makes this worth doing:** a wrong or incomplete
group costs speed and never correctness. Fewer verified elements means more
orbits means a bigger search, still exhaustive and still sound. Only an
*unverified* element could corrupt an answer, so nothing may skip the check.

**One guard:** orbit pruning requires the pool to be closed under the action.
`all_rank_one_maps` is; `rank_one_candidates` is not. Refuse rather than assume.

## What it is worth, and where to point it

On the polynomial fixtures, `|Stab(T)|` is about 6 over `F₂`, a single-digit
constant at the top of the tree and less below. That turns the seven-hour
`--target 12` into one or two hours. Useful; not a change of kind. And per
[`known_ranks.md`](../descent_search/known_ranks.md), that run now reproduces a published
exclusion rather than settling anything.

**Point it at matrix multiplication instead.** ⟨2,2,2⟩ has the sandwich
symmetries and the cyclic one: order in the hundreds over `F₂`, against a
225-element pool. That is where orbits collapse a search rather than trim it,
and [`famous_tensors.md`](../famous_tensors.md) is where the open questions are.

## The heuristic is a separate question

Reducing step 3's pool to orbit representatives is one line once the machinery
exists, but it is **not** answer-preserving: `minimise_rank` is
first-improvement with irreversible pruning, so a different pool is a different
walk. It cannot produce a *false* claim, since every result is rebuilt and checked
with `spans_all`, so it is the safe place to experiment. It is not the place
the proof lives. Do the exact search first.
