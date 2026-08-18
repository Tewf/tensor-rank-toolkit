# Symmetry, which is where the saving is

A change of coordinates on each operand preserves rank, so if it also fixes the
target subspace it maps solutions to solutions and only one member of each orbit
need be visited. Six modules, because the group, the orbits and each consumer of
them are separate jobs, and the header is the authority in every case: this is a
map, not a second copy of one.

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
