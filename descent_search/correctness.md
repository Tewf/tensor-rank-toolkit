# What the descent guarantees, and what it does not

Proved, not measured. Measurements are in [`README.md`](README.md); the
algorithms and their cost are in [`method.md`](method.md), whose notation this
follows: `T` the map, `S` the current spanning set, `cost(X) = Σᵢ rank(Xᵢ)`, and
`G` the candidate pool.

Throughout, `span(T) \ {0}` under linear independence is a **vector matroid**
and `w(M) = rank(M)` is a non-negative integer weight.

This file is past the eighty lines the conventions ask, deliberately. It is a
chain of proofs, and Lemma 4 is a hypothesis of Theorem 5, so splitting it would
put a lemma in a different file from the theorem consuming it.

## Theorem 1, step 1 is exact

*`minimum_weight_basis` returns a basis of `span(T)` of minimum total rank.*

Sorting by ascending weight and keeping whatever preserves independence is the
matroid greedy, which by **Rado-Edmonds** returns a minimum-weight basis of any
matroid under any non-negative weight. ∎

The **value** is therefore tie-break independent, all minimum-weight bases of a
matroid having equal weight. The **basis** need not be, which is what the pinned
enumeration order buys: reproducibility, not optimality.

## Theorem 2, soundness

*`span(S) ⊇ span(T)` always, so every `S` the algorithm holds computes `T`.*

Induction. `span(S₀) = span(T)`. A step replaces `S` by a basis of
`span(S ∪ {φ}) ⊇ span(S) ⊇ span(T)`. ∎

A result is therefore never wrong, only possibly large. It also justifies the
relaxation: `S` need only *generate* `T`, so admitting spanning sets that are not
bases enlarges the feasible set and cannot raise the optimum. That is what lets
`|S|` exceed `|T|`, as in Karatsuba's five products for four coefficients.

## Theorem 3, termination with a bound

*At most `cost(S₀) − rank(T)` adoptions.*

A candidate is adopted only when `cost` strictly decreases. `cost` is a
non-negative integer bounded below by `rank(T)`, since any spanning set of
`span(T)` yields an algorithm for `T`, and a strictly decreasing integer sequence
bounded below is finite. ∎

## Lemma 4, the pruning is sound

*If `φ ∈ span(S)` it cannot improve `S`, nor any later `S′`.*

`S` is always a minimum-weight basis of its own span, by Theorem 1 applied to
that span. If `φ ∈ span(S)` then `span(S ∪ {φ}) = span(S)`, whose minimum-weight
basis has weight `cost(S)`: no decrease. By Theorem 2 the span only grows, so
`φ ∈ span(S′)` thereafter and the argument repeats. ∎

This is what lets `survivors_after` discard candidates permanently, and it is
the hypothesis Theorem 5 needs.

## Theorem 5, the fixed point is 1-opt over the whole pool

*On return no `φ ∈ G` lowers the cost, including candidates never tested.*

Every `φ ∈ G` was either tested and failed, or discarded by Lemma 4 and
therefore provably could not improve. ∎

Stronger than the loop's stopping condition, which speaks only of the survivors:
the guarantee is local optimality against single additions from all of `G`.

## Theorem 6, the orbit quotient loses nothing

*Let `σ = (μ, ν)` act by `M ↦ μᵀMν` with `μ, ν` invertible and `σ(span S) =
span S`. Then `φ` improves `S` if and only if `σ(φ)` does.*

Invertibility gives `rank(μᵀMν) = rank(M)`, so `σ` preserves every weight and
`cost(X) = cost(σ(X))`. Being a linear isomorphism fixing `span(S)`, it carries
`span(S ∪ {φ})` onto `span(S ∪ {σ(φ)})`. Minimum weight is a function of the span
alone, by Theorem 1, so the two additions cost the same. ∎

One representative per orbit of `Stab(span S)` therefore suffices, which is what
[`../orbit_reduction/orbit_heuristic.h`](../orbit_reduction/orbit_heuristic.h)
rests on. Note the hypothesis is the stabiliser of the **current** span, not of
`T`: a quotient taken once and reused as the map moves is unsound, not merely
weaker.

## What is not proved, and is not true

**There is no approximation ratio.** Theorem 5 gives local optimality under
single additions and nothing more. A 2-opt move, exchanging two elements at once,
lies outside the neighbourhood and can pay: `⟨2,2,2⟩` sits at a 1-opt fixed point
of cost 8 while its rank is 7, which is why crossing plateaus is a separate
method rather than a tuning of this one.

**Step 1's optimality does not survive step 2.** Theorem 1 is about bases of
`span(T)`; once the relaxation admits spanning sets the matroid is gone, and with
it the guarantee. The steps are filed apart for that reason.
