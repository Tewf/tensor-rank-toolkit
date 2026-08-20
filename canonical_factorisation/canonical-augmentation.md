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
| `canonical`, from generators | 6 generators | **72** | **0.263 s** |

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

So the honest account has no single per-node number in it. **What survives is the
ratio this page already measured: 0.263 s against 0.0102 s, 25.8x**, and the node
saving of 53x that fails to pay for it. The "299x a node" that used to appear
here was the target-6 rate against a plain node, quoted as if it were the whole
sweep's.

**The gap widened while both halves improved.** This route is 10.5x faster than
the file used to record and lost anyway, because the packed GF(2) leaf made a
plain node 18x cheaper meanwhile. **An optimisation to the common path raises the
bar for every quotient competing with it**, which is the reusable lesson.

## Where it could still win

The node saving grows with the group and the group grows fast with the shape, so
299x is not out of reach the way it was behind a `|G|` walk. What stopped bigger
shapes being tried was this file's own choice to enumerate, since
`matrix_multiplication_symmetries` refuses above a list it can hold. From
generators there is no refusal, and `<3,3,3>`'s 4.7 million elements have nine.
Whether the crossing arrives before its pool of 261 121 becomes the binding cost
is unmeasured, and is the open question this route now poses.

So it ships behind a flag, never by default, with a slow test asserting it still
reaches 7, still engages rather than falling back, and still visits strictly
fewer nodes than the plain route. **A wired route known to lose is worth more
than an unwired one somebody will propose again**, and it is worth more still
when the reason it loses is a number rather than an impression.
