# Which leaf route is cheaper, asked rather than assumed

A leaf asks whether a subspace has a rank-one basis, and there are two ways to
ask: scan the pool for members, or walk the subspace testing each element for
rank one. [`rank_one_basis.h`](rank_one_basis.h) picks per call, by counting:
walk when `p^dim` is under the pool size, scan otherwise.

**That rule assumes the two elements cost the same**, and nothing here had ever
checked. It is the whole of the leaf, and the leaf is where the search lives, so
an unchecked constant sat under every published second in this repository.

## Making it answerable

Neither route can be priced against the other while the rule picks one, and
pricing them on two questions prices two questions. So `--leaf-route
auto|scan|walk` forces one, in the spirit of `--general-leaf`.

Forcing the walk needed one change beyond the flag. `elements_of` stops counting
once `p^dim` passes a ceiling it is handed, and the ceiling handed to it was the
pool size, so **the count and the comparison were the same act**: a non-zero
answer already meant the walk was smaller. A forced walk is counted against
`SIZE_MAX` instead. Without that the flag was accepted, changed nothing, and
said nothing, which is worse than not having it.

## What it says

One core, fastest of three, quiet machine, `flock` held, node counts identical
across each pair because only the leaf test differs.

| question | pool | rule picks | scan | walk |
|---|---|---|---|---|
| `f2_3x8 --target 15`, 100 000 nodes | 1785 | scan | **5.734 s** | 151.59 s |
| `f2_5x5 --target 12`, 100 000 nodes | 961 | scan | **1.793 s** | 16.916 s |
| `gf16 --target 9`, 100 000 nodes | 225 | scan | **0.355 s** | 1.153 s |
| `matmul_2x2x2 --target 7`, 7 436 nodes | 225 | walk | 0.0229 s | **0.0189 s** |

**The rule is right on all four, and the last row is why that is worth saying.**
Three of them it sends to the pool and the pool wins by 3.3x, 9.4x and 26.4x.
The fourth it sends to the walk, against a pool of the same size, and the walk
wins. A rule that only ever picked one route would be indistinguishable from
this one on three questions out of four.

The bottom row is milliseconds, so by [`../MEASURING.md`](../../../MEASURING.md) it is
a correctness control and not speed evidence. It is used here for its direction
only, and the direction is the claim.

## The correction that was going to be made, and was not

The plan was to weight the comparison by a measured cost ratio, since equal
counts do not mean equal work. The ratio is not a constant. On `f2_5x5` the walk
element is dearer than the scan element and on `matmul_2x2x2` it is cheaper,
because a rank test and a membership test scale differently in the shape.

Weighting by any single ratio therefore breaks a row that currently works: a
ratio below one, which `f2_5x5` argues for, moves `matmul_2x2x2` onto the pool
and costs it 1.21x. **So the count rule stands, and now it stands because it was
measured rather than because nobody looked.** A shape-dependent weight is a
different piece of work and would have to beat this table to be worth it.
