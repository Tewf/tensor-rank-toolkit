# Symmetry, which is where the saving is

A change of coordinates on each operand preserves rank, so if it also fixes the
target subspace it maps solutions to solutions and only one member of each orbit
need be visited. Six modules, because the group, the orbits and each consumer of
them are separate jobs, and the header is the authority in every case: this is a
map, not a second copy of one.

## Whose work this is

Almost all of it is `[covanov2019]`'s. `[bdez2012]` is where the want of it
shows twice and neither time as a method: **§4.5** ends by saying only that
duplicate solution subspaces *"have to be detected and removed"*, and the
**Conclusion** lists *"using the symmetries of the problem"* among the things
someone wanting to go further would have to do. Keys are
[`../references.md`](../references.md).

| Result | What rests on it |
|---|---|
| `[covanov2019, Def. 7]` | the action `(a, b) ↦ Φ(μa, νb)`, which is `Automorphism` |
| `[covanov2019, Prop. 8]` | that it is a group action and every element invertible |
| `[covanov2019, Prop. 9]` | that it preserves the rank of a form and of a subspace, which is why a quotient is lossless |
| `[covanov2019, Def. 13]` | the setwise stabiliser `Stab(T)`, which is the group actually used |
| `[covanov2019, Prop. 14]` | that a decomposition meeting an orbit is equivalent to one containing its representative: the cubes |
| `[covanov2019, Alg. 3]` | `BDEZStab`, which [`orbit_search.h`](orbit_search.h) implements |
| `[covanov2019, Thm. 17]` | the closed-form stabiliser of a matrix multiplication tensor |

**One thing here is not published and is marked as such.** The closed-form
orbits of the rank-one pool in [`pool_orbits.h`](pool_orbits.h), the triple loop
over `rank U`, `rank V` and `rank UV`, are this repository's own; the header
carries the derivation and says where it stops. They used to be attributed to
`[covanov2019, Cor. 18]`, which is a statement about elements of the target
subspace and not about the pool.

| File | Role |
|---|---|
| [`automorphism.h`](automorphism.h) | The group itself: the rank-preserving action, and the stabiliser of a subspace |
| [`group_construction.h`](group_construction.h) | Where the groups come from: by brute force, by closed form, and by closing a generating set, the last two pinned against each other |
| [`pool_orbits.h`](pool_orbits.h) | The orbits of the rank-one pool, found on the operand vectors rather than on their products |
| [`orbit_search.h`](orbit_search.h) | The exact search of [`../exhaustive_search/`](../exhaustive_search/) with its tree quotiented: one branch per orbit |
| [`orbit_heuristic.h`](orbit_heuristic.h) | Steps 2 and 3 of [`../descent_search/`](../descent_search/) against a quotiented pool |
| [`orbit_cubes.h`](orbit_cubes.h) | The first term fixed to one representative per orbit, for a solver to split on |

Two documents rather than a third table. Why the family is arranged this way:
[`orbit_plan/`](orbit_plan/README.md). What the cubes promise the SAT encoder,
stated as a contract because the two sides are compiled apart:
[`orbit_cube_boundary/`](orbit_cube_boundary/README.md).

`requested_group.h` sits here too but outside the library target: it turns a
`--symmetry` word typed at a shell into a group, and the library has no business
knowing a command line exists.

## What it is worth, measured

Refuting `⟨2,2,2⟩` at k = 6 costs the tree search **0.56 s plain and 0.02 s
quotiented**, twenty-eight times. Finding the 7 goes 0.36 s to 0.15 s. The
saving is largest on a refusal, which is the case that costs most, because a
refusal has to visit the whole tree and the quotient is what shrinks it.

## Where it stops, and why that is not a defect

The closed form exists only for matrix multiplication shapes. Asked for
`--symmetry auto` on F2 5×5 the group construction **refuses**, declining to
build 9.99872×10¹³ automorphisms and saying so. That is the right answer: the
orbit reduction is not free, and a wait long enough to be indistinguishable from
a hang is worse than a refusal that names the alternative.

## The one mistake nothing downstream catches

A group that does not stabilise the span it is handed makes
`expand_subspace_up_to_symmetry` report a false `NO`, which is a false lower bound.
Nothing further down the pipeline can tell. So `test_symmetry_agreement` asserts
that the quotiented search and the unquotiented one answer the same question the
same way, on every fixture where both terminate, and it is the test to look at
first if a bound here is ever doubted.
