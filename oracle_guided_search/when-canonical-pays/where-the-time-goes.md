# The parent test was bound by a pool scan, not by the group

**The first two items at the bottom of this page have since been done**, and the
shares below are the ones that asked for them. What they were worth: `<2,3,3>` at 8
from 98.0 s to **11.4 s**, `<2,2,4>` at 10 from 0.500 s to 0.144 s, `<3,3,3>` at 10
from a 1.33x win to a **2.07x** one — with every node count unchanged, which is the
check that it was a cost and not an answer that moved.
[`../pool_cosets.h`](../pool_cosets.h) is the first mechanism and the parent test's
early exit is the second.

Take each canonical node of [against-the-sweeps.md](against-the-sweeps.md) apart,
using the canonical-image count the run reports and the per-operation prices in
[what-it-costs-here.md](what-it-costs-here.md). Everything not a canonical image or
a setwise stabiliser is a pool scan.

| shape | levels | pool scans | canonical images | setwise stabiliser | scans a node |
|---|---|---|---|---|---|
| `<2,2,2>` | 2 | 28% | **62%** | 10% | 8.6 |
| `<2,2,3>` | 2 | 48% | 44% | 8% | 6.9 |
| `<2,2,4>` | 2 | **82%** | 13% | 4% | 7.3 |
| `<2,3,3>` | 2 | **98%** | 1.1% | 1.0% | 13.9 |
| `<3,3,3>` | 1 | **97%** | 0.1% | 3.3% | 2.4 |

**The two operations the whole apparatus exists for are 2% of the cost at the
largest shape swept.** The setwise stabiliser — GI-hard, no proven subexponential
bound, the term this model was warned would be the hard one — is 1.0% at `<2,3,3>`
and 4% at `<2,2,4>`. The canonical image, which the factored presentation and
`[linton2004]` were brought in to make cheap, is 1.1%. They were made cheap, the
measurement says so, and what is left is not them.

**What is left is `ReducedBasis::contains`, called `|P|` times, seven to fourteen
times a node.** Reading `descend`: two scans of its own, in `augmentations` for the
elements outside the span and in `pool_inside` for those within, and then one more
inside `is_canonical_augmentation` **per candidate child**, since that function
opens with `pool_inside(field, pool, child)`. A plain node scans a *suffix* of the
pool, rejects most of it through `opens_a_branch` before any containment test, and
finishes the leaf through the packed GF(2) leaf. **The canonical route has none of
those three.** `canonical-augmentation.md` records the packed leaf making a plain
node 18x cheaper and the canonical route losing ground because of it; this is where
that 18x went, and it is still available.

## The row where canonical wins, and why it is not a node saving

`<3,3,3>` at 10 is 3.70 s against 4.92 s with **both routes visiting 14 nodes**.
There is no duplication removed and no orbit merged: the two trees are the same
tree. What differs is how each route decides which children to open. The baseline
asks `opens_a_branch` of every one of 261 121 pool elements, once per residual
generator; the canonical route asks `orbit_representatives` once, over the whole
pool, from the stabiliser's generators. One orbit computation beats a quarter of a
million per-element tests, and that is the entire margin.

It is worth naming because it is a *different mechanism* from the one this folder
prices, it appears only at pool sizes where the baseline's per-element work
dominates, and a model of node savings cannot see it. The predicate misses this row
and `../tests/test_route_price.cpp` records the miss rather than fitting it away.

## What this changed about where to spend effort

1. **Done: stop scanning the pool once per candidate child.**
   `is_canonical_augmentation` rescanned for `content(child)` when the caller had
   just computed `content(current)`, and the two differ only by the pool elements
   whose residue modulo the current span spans the same line as the added map's.
   [`../pool_cosets.h`](../pool_cosets.h) gets every child's content from one pass
   a node; the seven to fourteen above are now **one**. Worth 7.5x at `<2,3,3>`,
   and the win grows with the pool, which is what a removed `|P|`-proportional term
   does.
2. **Done: ask the parent test for an exit, not for a minimum.** The condition is
   that the parent's class is least among the candidate parents', and one candidate
   below it settles that; the minimum was never wanted. The parent's own name is
   also the same for every child of a node, so it is computed once a node instead
   of once a test. Canonical images fell **31% to 45%** and the node counts did not
   move.
3. **Still open: give what is left of the pool pass the packed GF(2) leaf.**
   `PoolCosets` and `independent_rank_one_maps_in` both walk the pool through the
   general path, one reduction at a time, where `exhaustive_search/gf2_leaf.h`
   already does that walk packed for the other route and measures 6x to 40x for it.
   That pass is now most of what a canonical node spends at the large shapes. It
   needs `Gf2Leaf` to expose the packed pool and the packed span it already holds
   privately, which is a change in `exhaustive_search/` rather than here.
4. **Only then the rest of the group work**, where `[linton2004]` §4's advanced
   algorithm and a factored stabiliser sit. §4 loops over orbit representatives of
   a subgroup `H` of the setwise stabiliser **acting on the set itself**, and
   reports 67x to 100x where that induced group is large — his benchmark is 20
   points with a stabiliser of order 2 880 acting faithfully on them. Ours are sets
   of three to ten cells whose stabilisers induce very little on so few points, so
   the regime that produced his figure is not the one here, and the figure should
   not be carried across. It is worth measuring before it is worth building.
