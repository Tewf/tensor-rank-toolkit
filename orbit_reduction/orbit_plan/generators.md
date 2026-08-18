# Where `Stab(T)` comes from, which is the honest hard part

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
