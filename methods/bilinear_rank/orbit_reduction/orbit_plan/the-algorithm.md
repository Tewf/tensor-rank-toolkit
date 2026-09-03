# The algorithm

`BDEZStab` prunes at **every** node, not only the widest one. It is
`[covanov2019, Alg. 3]`, printed again as `[covanov2018, Alg. 6]`:

```
expand(V, H, U, d, r):            # H remaining pool, U ⊆ Stab(T) stabilising V
    if d == r: succeed iff dim V == r and V has a rank-one basis
    O ← orbits of H under U
    for i, orbit in enumerate(O):
        φ ← representative(orbit)
        expand(V ⊕ ⟨φ⟩, ∪_{j≥i} O_j, Stab(V ⊕ ⟨φ⟩) ∩ U, d+1, r)
```

Two details carry the correctness. The `from`-index pruning already in
`expand_subspace` survives, but it now applies **to orbits, not to elements**
(`∪_{j≥i} O_j`). And `U` shrinks by intersection as the subspace grows, which is
why deeper levels prune less and why this is not just a level-1 trick.

**One line above is not what the source prints, and the deviation is a fix.**
Line 11 of both `[covanov2019, Alg. 3]` and `[covanov2018, Alg. 6]` reads
`ExpandSubspace(V, H′, U′, d + 1, r)`, recursing on `V` rather than on
`V ⊕ Span({φ})`. As printed the subspace never grows, so the base case's
`dim V = r` could only ever hold at `r = dim T`; the same line already computes
`Stab(V ⊕ Span({φ}))`, which is what the recursion was meant to carry. It is a
typographical slip, identical in the paper and the thesis. The pseudocode above
has always corrected it; what is new is that it now says so.
`orbit_search.h` implements the corrected form, which is the only one that
grows a subspace at all.

The initial call is `ExpandSubspace(T, G, Stab(T), ℓ, r)` with `ℓ = dim T` and
`G` the whole rank-one pool up to scalars, and the source's orbits on line 6 are
of `H` under `U` **after reduction by a basis of `V`**, not of `H` alone: the
thesis spells the four-step reduction out just under the algorithm.

At `⟨2,2,2⟩` over GF(2), `Stab(T)` has 216 elements
([`../group_construction.h`](../group_construction.h)), and running
`decide-rank evidence/fixtures/matmul_2x2x2.tensor --target 6 -s matmul 2 2 2` walks 648
nodes where the unquotiented call walks 25 399
([`../what-the-quotient-costs.md`](../what-the-quotient-costs.md)).

## What to build, in order

1. `symmetry_group.h/.cpp`: `Symmetry {left, right, transposed}`, the action,
   and `stabilises(field, σ, span)`. Verification is cheap and exact: `X`, `Y`
   invertible, and every basis slice's image back inside the span.
   **Built as [`automorphism.h`](../automorphism.h), without `transposed`**, and
   named `Automorphism` rather than `Symmetry`. Why the transposition is absent
   is in [`the-group.md`](the-group.md); it follows the *paper*, and the thesis
   does differ.
2. Orbits of the pool under an explicit element list, by union-find. `U` is
   carried as a list, so `Stab(V ⊕ ⟨φ⟩) ∩ U` is a filter, not group theory.
3. Sources of generators (below), each **verified before use**.
4. Wire into `expand_subspace` behind a parameter, and prove it by re-running
   every existing exact-search assertion with orbits on: same answers, fewer
   nodes. That test is the deliverable, not the speedup.
