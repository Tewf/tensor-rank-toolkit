# The parent test was bound by a pool scan, not by the group

**The first item at the bottom of this page has since been done**, and the shares
below are the ones that asked for it. What it was worth: `<2,3,3>` at 8 from 98.0 s
to **13.1 s**, `<2,2,4>` at 10 from 0.500 s to 0.164 s, `<3,3,3>` at 10 from 3.70 s
to 2.41 s and from a 1.33x win to a **2.17x** one — with every node count and every
canonical-image count unchanged, which is the check that it was a cost and not an
answer that moved. [`../pool_cosets.h`](../pool_cosets.h) is the mechanism.

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
2. **Still open: give what is left of the pool pass the packed GF(2) leaf.**
   `PoolCosets` and `independent_rank_one_maps_in` both walk the pool through the
   general path, one reduction at a time, where
   `exhaustive_search/gf2_leaf.h` already does that walk packed for the other
   route. That pass is now most of what a canonical node spends.
3. **Only then the group work**, where `[linton2004]` §4's advanced algorithm and a
   factored stabiliser sit. Priced against the shares above they were worth 1% to
   62% depending on the shape, and the shapes where they were worth most are the
   small ones nobody needs. Section 4 reports 67x to 100x on its own benchmark, in
   a regime that matches ours; 100x on 1.1% of a node is 1.1% of a node.
