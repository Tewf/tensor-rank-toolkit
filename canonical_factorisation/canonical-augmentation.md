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

## What it costs now, which is a different shape of answer

`--floor 7` visits 14 nodes and `--floor 6` visits 72, for 0.2068 s and 0.2530 s.
Two points, one line: **0.196 s to build the presentation, then 0.795 ms a node.**

A plain node is 2.66 us. So a canonical node costs **299x** a plain one, and the
route must remove more than 299x the nodes to break even. It removes 53x. That is
the whole account, and the gap is 5.6x plus an entry fee.

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
