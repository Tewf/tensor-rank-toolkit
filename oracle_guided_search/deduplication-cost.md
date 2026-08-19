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

So the honest reading is that the deduplication is **correct and worth having, and
currently bottlenecked by its own invariant** rather than by the enumeration. The fix is
not more pruning; it is a real canonical labelling with refinement, `nauty`'s actual
contribution rather than the augmentation scheme built on top of it.

**That piece now exists**, as a reduction rather than an algorithm:
[`pool_set_canon.h`](pool_set_canon.h) names an orbit from generators by canonical
image of a set under a prescribed permutation group, `[linton2004]` through
`[permlib]`, and agrees with the walk on every one of the 225 depth-one children
of `⟨2,2,2⟩`. It is not yet wired into `is_canonical_augmentation`, which would
change that function's signature from a group list to this object and make the
`group visits` column above mean something else, so the numbers here still
describe the walk.

**The 3000x this used to hope for is not supported by anything published.** No
source found states a proven complexity bound for canonical image: Linton, Leon
and Jefferson et al. offer correctness proofs and experiments, and the problem
subsumes setwise stabiliser, so it is Graph-Isomorphism-hard and no polynomial
algorithm is on offer. Jefferson et al.'s own measurements against Linton report
*instances solved inside a timeout* rather than a ratio, so even the published
improvement does not divide into a speedup. Expect a measurement, not a factor.

## Where it does not apply

`canonical_subspace` needs the group as a list, and
`matrix_multiplication_symmetries` refuses above a million elements. `⟨3,3,3⟩` is
4 741 632, so the canonical route degenerates there to the plain one: an empty group
makes every object its own orbit. That is reported rather than refused, because it is
what a caller at that shape actually gets.

## Spreading it over cores

A separate question with a separate answer, and the short version is that no
number in this file changes: [`enumerating-on-every-core.md`](enumerating-on-every-core.md).
