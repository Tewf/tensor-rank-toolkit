# What deduplicating up to the group is worth

Measured 2026-08-17, one build, the lock held for the whole run. `enumerate-subspaces`
runs the same enumeration twice: `plain` deduplicates orderings only, which is what
`expand_subspace`'s `from` index already did, and `canonical` adds McKay's parent test
(`[mckay1998]`).

## It is right, and the target says so independently

`⟨2,2,2⟩` at 7 has **36** solution subspaces in **1** orbit under its 216-element
group. That was computed twice by separate means before this was written, so it is a
target and not a self-check.

| pass | distinct subspaces | paths | nodes |
|---|---|---|---|
| plain | **36** | 720 | 1 890 601 |
| canonical | **1** | 1 | 954 |

Both numbers come out. The single orbit is de Groote's uniqueness theorem for `⟨2,2,2⟩`
recovered from the tensor. Every emitted basis was multiplied out against the map by
`recovers_map` before being counted.

**720 paths to 36 subspaces is the size of the problem.** The ordering constraint stops
a *sequence* of pool elements repeating, not a *set*: each solution subspace contains
seven pool elements spanning a three-dimensional quotient, and it is rebuilt once per
basis of that quotient. Getting 36 out of the plain pass at all needs one code stored
per solution found, which is the O(objects) memory the parent test replaces with
nothing.

## What it costs, which was the open question

| case | pool | group | plain nodes | plain | canonical nodes | canonical | group visits |
|---|---|---|---|---|---|---|---|
| `⟨2,2,2⟩` at 6 | 225 | 216 | 25 399 | 0.80 s | **103** | **0.65 s** | 169 344 |
| `⟨2,2,2⟩` at 7 | 225 | 216 | 1 890 601 | 95.3 s | **954** | **60.4 s** | 10 873 008 |
| `⟨2,2,3⟩` at 8 | 945 | 6048 | 446 923 | 133.5 s | **145** | **63.5 s** | 6 598 368 |

**No crossover in the wrong direction.** The canonical pass wins on wall clock in all
three, and the margin grows with the group: 1.2x, 1.6x, 2.1x. On nodes it wins by
247x, 1982x and 3082x, and that margin also grows with the group, which is what a
quotient should do.

**But the two margins are three orders of magnitude apart, and that gap is the result.**
The parent test spends about 99.9% of the node saving on itself. The reason is visible
in the last column: `canonical_subspace` finds a canonical code by walking the whole
group and taking the least, so one test costs `|G|` reductions per candidate parent.
At `⟨2,2,3⟩` that is 6048 elements looked at per parent, seven parents per node.

So the honest reading **was** that the deduplication is correct and worth having,
and bottlenecked by its own invariant rather than by the enumeration: the fix was
never more pruning, it was a real canonical labelling, `nauty`'s actual
contribution rather than the augmentation scheme built on top of it.

## That change is made, and the route stopped losing

`canonical_subspace` named an orbit by walking every element of the group and
taking the least code. [`pool_set_canon.h`](pool_set_canon.h) names the same
orbits by least image under a prescribed permutation group, `[linton2004]` through
`[permlib]`, and the parent test asks it instead. Measured on
`enumerate-subspaces fixtures/matmul_2x2x2.tensor --target 7 -s matmul 2 2 2`,
both routes back to back on an idle machine:

| route | distinct | nodes | wall |
|---|---|---|---|
| plain | 36 | 1 890 601 | 35.89 s |
| canonical, walking the group | 1 | 954 | 21.9 s |
| canonical, canonical image | 1 | **83** | **3.04 s** |

**22 779x fewer nodes than the plain route and 11.8x faster**, against 1 982x and
1.7x before. The group is no longer walked at all: `group_visits` is 0 and
`canonisations` is what counts now. Handing the caller's group as **generators**
rather than as a list took the same run from 3.04 s to **1.12 s**, because the
presentation is built from six of them instead of 216.

**The last `|G|` dependency is gone too, and it was the one that decided which
shapes this route could reach.** `stabiliser_of` filtered a group held as a list,
so the canonical route refused `⟨3,3,3⟩` outright at 4 741 632 elements and
6.2 GiB before opening a node. The subgroup fixing a subspace now comes from a
backtrack over generators, `[permlib]`'s `setStabilizer`, which is sound here
because the enumerator descends from `span(T)`: every subspace it reaches is
`span(T) + span(pool ∩ U)`, so fixing the subspace and fixing its pool content are
the same condition. Node counts did not move, which is the check that matters:
83 and one distinct subspace, before and after.

**What that bought.** The route now reaches `⟨3,3,3⟩`, whose pool is 261 121 and
which refused outright this morning: 14 nodes and 101 s at target 10, correctly
finding nothing. Getting there needed one thing that was not obvious.
`[permlib]`'s point type is `unsigned short` unless `PERMLIB_DOMAIN_INT` is
defined, so **a degree past 65 535 wraps and writes out of bounds**. It does not
refuse and it does not slow down, it segfaults, inside
`Transversal::foundOrbitElement`, with a healthy stack. Verified by bracketing:
225, 945 and 32 193 worked and 261 121 crashed, and 65 535 lies between them. With
the define it presents 261 121 points in 29.8 s and 97 MB. The library scaled all
along; its default type did not.

**The node count moved, and that is expected rather than alarming.** A canonical
form is not unique; any function constant on orbits and separating them will do,
and a different one accepts a different representative per class, so it walks a
different tree. What may not move is the answer, and it does not: 36 subspaces in
one orbit by the plain route, one found here, none at target 6, and
`factor-over-canonical-basis` still returns a factorisation that multiplies out.
Losing solutions would also shrink a node count, which is why `distinct` is
asserted beside it rather than the count being trusted alone.

## Where it does not apply

`canonical_subspace` needs the group as a list, and
`matrix_multiplication_symmetries` refuses above a million elements. `⟨3,3,3⟩` is
4 741 632, so the canonical route degenerates there to the plain one: an empty group
makes every object its own orbit. That is reported rather than refused, because it is
what a caller at that shape actually gets.

## Spreading it over cores

A separate question with a separate answer, and the short version is that no
number in this file changes: [`enumerating-on-every-core.md`](enumerating-on-every-core.md).
