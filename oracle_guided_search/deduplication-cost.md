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
`canonisations` is what counts now.

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
