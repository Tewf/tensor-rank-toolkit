# Canonical augmentation, wired and measured

The third route, why it exists, and why it is never the default. Every other way
of narrowing this search, the one that works and the two that cannot, is
[`narrowing-the-search.md`](narrowing-the-search.md).

`--route canonical` is McKay canonical augmentation `[mckay1998]`: give every
subspace a group-invariant canonical parent and accept an augmentation only from
that parent's class, so each class is reached exactly once with **no memory** of
what was generated. It is the standard fix for the duplication the plain tree
leaves, and `oracle_guided_search/` measures it at **22 779x fewer nodes** when
*counting* solution subspaces.

Deciding is not counting, and the first wiring of it lost by 129x. Two fixes
since have taken most of that back. On `matmul_2x2x2`, over the whole sweep from
the floor:

| route | group given | nodes | time |
|---|---|---|---|
| `exhaustive` | 6 generators, the stabiliser | 3815 | **0.0102 s** |
| `canonical`, as first wired | 216 elements, enumerated | 1057 | 24.62 s |
| `canonical`, with an early exit | 216 elements | 235 | 2.76 s |
| `canonical`, from generators | 6 generators | **72** | 0.263 s |
| `canonical`, one pool pass a node | 6 generators | **72** | 0.0458 s |
| `canonical`, and a parent test that exits | 6 generators | **72** | **0.0381 s** |

The last row is [`../oracle_guided_search/pool_cosets.h`](../oracle_guided_search/pool_cosets.h),
and the plain route moved under it too: 0.0102 s to 0.00742 s on the same sweep,
because the two rows were not measured on the same afternoon. **The ratio that
stands today is 5.1x against, from 25.8x.**

The early exit was the first 9x. The enumerator was written to *count* solution
subspaces, so it finishes every level; a rank search only asks whether the level
is empty, and the level that answers does not have to be finished. It is an
argument now, off by default, because a count that stopped early is not a count.

**The second 10.5x was a comment that outlived its reason.** This route handed
the parent test the whole 216-element group, deliberately, because the test named
an orbit by walking every element and generators would have made it wrong rather
than slow. `PoolSetCanon` replaced that walk with a base and strong generating
set and the comment stayed, so the route kept paying to enumerate a group in
order to hand it to something that immediately rebuilt six generators from it.

## What it costs now, and the line that was fitted through the wrong points

**There is no 0.196 s presentation fee. That number was an artefact of the fit
and it stood here until 2026-08-20.**

The line was drawn through two points, `--floor 7` at 14 nodes and 0.2068 s and
`--floor 6` at 72 nodes and 0.2530 s, and read as an intercept plus 0.795 ms a
node. But the 58 extra nodes are all at **target 6**, and the original 14 are all
at **target 7**, where a node costs far more: a target-7 subspace is a dimension
larger, so its pool scan, its stabiliser and its canonisations all cost more. The
fit mistook a per-node cost that varies with the target for a fixed entry fee.

Measured directly instead of inferred: building `PoolSetCanon` on the 225 points
of `⟨2,2,2⟩` takes **0.013 s**, three runs, and that was taken on a loaded
machine, which can only make it slower. Subtracting it leaves about **13.8 ms** a
node at target 7 against **0.79 ms** at target 6 — a factor of seventeen between
two levels of the same sweep.

**That 0.013 s was the grid presentation and it is 98 us now.** The number stood
here after the presentation moved onto the two axes, which took `⟨2,2,2⟩`'s domain
from 225 points to 30; re-measured by `price-canonical-route` it is **133x less**,
and the entry fee stopped being a term in any account of this route. What the two
levels cost apart is unchanged and is the real content of the paragraph above.

So the honest account has no single per-node number in it. **What survives is the
ratio: 0.263 s against 0.0102 s when this paragraph was written, 25.8x, and
0.0381 s against 0.00748 s now, 5.1x**, against a node saving of 53x that still
fails to pay for it. The "299x a node" that used to appear here was the target-6
rate against a plain node, quoted as if it were the whole sweep's.

**The gap widened while both halves improved.** This route is 10.5x faster than
the file used to record and lost anyway, because the packed GF(2) leaf made a
plain node 18x cheaper meanwhile. **An optimisation to the common path raises the
bar for every quotient competing with it**, which is the reusable lesson.

## Where it could still win, now measured rather than posed

That open question — whether the crossing arrives before the pool becomes the
binding cost — has been swept at five shapes, and the answer is that **the pool
became the binding cost first, and it is not the pool's size that does it**. Both
routes at `⟨2,2,2⟩`, `⟨2,2,3⟩`, `⟨2,2,4⟩`, `⟨2,3,3⟩` and `⟨3,3,3⟩`, level by level:
[`../oracle_guided_search/when-canonical-pays/`](../oracle_guided_search/when-canonical-pays/README.md).

Three results from it bear on this page. The node saving is **nothing at all** at
one level of augmentation, because the baseline's `least_in_orbit` is the exact
rule and has already taken that quotient — both routes emit one child per pool
orbit, and `orbits + 1` is the node count of either. It is 11x to 226x at two
levels, which is 0.017% to 5% of `|G|` and nowhere near the orbit-counting bound.
And at `⟨3,3,3⟩` the canonical route is **2.15x faster** with both routes visiting
the same 14 nodes, so its first win here is not a node saving at all but one
orbit computation replacing 261 121 per-element orbit walks.

**That last one has since been taken apart, and it is not this route's win.**
`least_in_orbit` costs `Θ(Σ|Oᵢ|²)` and `orbit_representatives` costs `Θ(|P|)` for
the identical children: 5.05 s against 51.2 ms at the `⟨3,3,3⟩` root, which is
the whole of the plain route's 4.87 s run. The margin is a quadratic in the
baseline, on the one level a real sweep never reaches, and the predicate that
finds it is deliberately wired to nothing:
[`../oracle_guided_search/when-canonical-pays/why-nothing-consults-it.md`](../oracle_guided_search/when-canonical-pays/why-nothing-consults-it.md).

What decided the rest was a pool scan and not the group: seven to fourteen of them
per canonical node, 98% of the cost at `⟨2,3,3⟩`, where the setwise stabiliser was
1.0% and the canonical image 1.1%. Most of those were one per candidate child, and
`pool_cosets.h` replaced the lot with one pass a node. Asking the parent test for
an exit rather than for a minimum took another 31% to 45% of its canonical images.
Together: **98.0 s to 11.4 s at `⟨2,3,3⟩`**, with every node count unchanged. The
18x this page credits the packed GF(2) leaf with is still sitting on the table for
the pass that is left.

So it ships behind a flag, never by default, with a slow test asserting it still
reaches 7, still engages rather than falling back, and still visits strictly
fewer nodes than the plain route. **A wired route known to lose is worth more
than an unwired one somebody will propose again**, and it is worth more still
when the reason it loses is a number rather than an impression.
