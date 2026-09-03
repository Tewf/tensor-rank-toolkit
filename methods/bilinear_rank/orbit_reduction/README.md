# Symmetry, which is where the saving is

A change of coordinates on each operand preserves rank, so if it also fixes the
target subspace it maps solutions to solutions and only one member of each orbit
need be visited. Eight modules, because the group, the orbits, the rule that
rejects a repeat and each consumer of them are separate jobs, and the header is
the authority in every case: this is a map, not a second copy of one.

## Whose work this is

Almost all of it is `[covanov2019]`'s. `[bdez2012]` is where the want of it
shows twice and neither time as a method: **§4.5** ends by saying only that
duplicate solution subspaces *"have to be detected and removed"*, and the
**Conclusion** lists *"using the symmetries of the problem"* among the things
someone wanting to go further would have to do. Keys are
[`../references.md`](../../../references.md).

| Result | What rests on it |
|---|---|
| `[covanov2019, Def. 7]` | the action `(a, b) ↦ Φ(μa, νb)`, which is `Automorphism` |
| `[covanov2019, Prop. 8]` | that it is a group action and every element invertible |
| `[covanov2019, Prop. 9]` | that it preserves the rank of a form and of a subspace, which is why a quotient is lossless |
| `[covanov2019, Def. 13]` | the setwise stabiliser `Stab(T)`, which is the group actually used |
| `[covanov2019, Prop. 14]` | that a decomposition meeting an orbit is equivalent to one containing its representative: the cubes |
| `[covanov2019, Alg. 3]` | `BDEZStab`, which [`orbit_search.h`](orbit_search.h) implements |
| `[covanov2019, Thm. 17]` | the closed-form stabiliser of a matrix multiplication tensor |

**The closed-form pool orbits were marked unpublished and are not.** The triple
loop over `rank U`, `rank V` and `rank UV` in [`pool_orbits.h`](pool_orbits.h)
is the orbit classification of the `A_3` quiver: `[brion2008, Thm. 2.4.3]`, an
account of `[gabriel1972]`, and `[buchfulton1999]`'s condition (1.2) is the same
range of `rank UV` written independently. The header carries the reduction and
the one step still owed over `GF(p)`. The orbits were also once attributed to
`[covanov2019, Cor. 18]`, which is a statement about elements of the target
subspace and not about the pool.

| File | Role |
|---|---|
| [`automorphism.h`](automorphism.h) | The group itself: the rank-preserving action, and the stabiliser of a subspace |
| [`group_construction.h`](group_construction.h) | Where the groups come from: by brute force, by closed form, and by closing a generating set, the last two pinned against each other |
| [`pool_orbits.h`](pool_orbits.h) | The orbits of the rank-one pool, found on the operand vectors rather than on their products |
| [`isomorph_rejection.h`](isomorph_rejection.h) | Which of a node's candidates open a branch: exact rejection, or the cheap partial rule `--orbit-test generators` selects |
| [`orbit_search.h`](orbit_search.h) | The exact search of [`../exhaustive_search/`](../exhaustive/README.md) with its tree quotiented: one branch per orbit |
| [`orbit_heuristic.h`](orbit_heuristic.h) | Steps 2 and 3 of [`../descent_search/`](../greedy_heuristic/README.md) against a quotiented pool |
| [`orbit_cubes.h`](orbit_cubes.h) | The first term fixed to one representative per orbit, for a solver to split on |
| [`subspace_canon.h`](subspace_canon.h) | Naming a subspace so two of them can be compared: moved here from `oracle_guided_search` on 2026-08-23 so a second consumer could reach it |

Two documents rather than a third table. Why the family is arranged this way:
[`orbit_plan/`](orbit_plan/README.md). What the cubes promise the SAT encoder,
stated as a contract because the two sides are compiled apart:
[`orbit_cube_boundary/`](orbit_cube_boundary/README.md).

`requested_group.h` sits here too but outside the library target: it turns a
`--symmetry` word typed at a shell into a group, and the library has no business
knowing a command line exists.

## What it is worth, measured

The quotient removes 39.2x the nodes on a `⟨2,2,2⟩` refutation
(`decide-rank fixtures/matmul_2x2x2.tensor --target 6 -s matmul 2 2 2`) and
2.35x on a satisfiable question (the same command at `--target 7`), and since
`least_in_orbit` charges only about 1.2x a node,
the time follows: **about 30x** and **about 2.1x**, quoted that way because the
quotiented runs are under a millisecond. The saving is largest on a refusal,
which is the case that costs most, because a refusal has to visit the whole tree
and the quotient is what shrinks it.
[`what-the-quotient-costs.md`](what-the-quotient-costs.md) has the table, and
the reason the surcharge used to be much larger than 1.2x. Cheapening the
rejection rule instead was tried and **does not pay**, costing 5.10x and 17.96x
the nodes on the two refutations measured:
[`what-partial-rejection-leaves.md`](what-partial-rejection-leaves.md).

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
