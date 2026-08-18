# The algorithm

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
