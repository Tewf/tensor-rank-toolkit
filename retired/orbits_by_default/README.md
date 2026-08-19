# Quotienting a product shape by default

`decide-rank` defaults to no symmetry. This made the default *infer* `<n,m,k>`
from the three dimensions and use the closed-form group where they come out
whole, since the quotient is what shrinks the branching factor and the branching
factor is the base of the exponent.

It is here because it is dominated, not because it is wrong.

## What retired it, measured

`matmul_2x2x2`, one core, best of three:

| question | quotiented | `-s none` | |
|---|---|---|---|
| `--target 6`, refute | 648 nodes, 0.0161 s | 25 399 nodes, 0.0368 s | 39x nodes, **2.3x faster** |
| `--target 7`, find | 3 167 nodes, 0.1545 s | 7 436 nodes, 0.0210 s | 2.3x nodes, **7.4x slower** |

The quotient costs about 17 us a node against 2.8 us, spent striking orbits and
narrowing the residual subgroup. A refutation visits the whole tree, so 39x fewer
nodes pays for it. A find stops at the first witness in index order, so the plain
search gets there before the quotient has earned its per-node cost back.

**This is the same asymmetry `orbit_cubes` measures on the solver**, where cubes
are worth 8.2x on a refutation and 250x worse on a find. Same cause.

Both rows are milliseconds, so they fix the direction of the effect and not its
size. A question with real weight was never measured either way.

## Two further reasons it is dominated rather than merely uneven

**The sweep is untouched.** `fewest_products_by_sweep` has no quotiented form, so
`decide-rank` with no `--target` ran exactly as it did. The change reached only
`--target k`, which is the half where it loses on finds.

**The fixed target is going away.** The branch-and-bound search that replaces the
sweep carries a running incumbent instead of asking one `k` at a time, so
"quotient or not, at this target" stops being the question this flag answers.

## What survived onto `main`

The refactor did, separately: `inferred_matmul_shape` belongs beside the groups it
feeds rather than file-local to `canonical_factorisation`, and it now has two
callers.

Two regressions this change exposed are worth keeping visible, because both were
the same mistake and neither was caused by the flag itself. Code read
`kind != None` as "the user asked for symmetry", which stops being true the moment
the default is not `None`: `decide-rank` refused a request nobody made, and
`decide-rank-by-sat` indexed an empty `shape` vector and **segfaulted**. The fix
was one predicate, `cli::was_requested`.

## The change itself

[`the-change.patch`](the-change.patch), against `main` at the time it was taken.
