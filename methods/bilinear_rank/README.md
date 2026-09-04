# methods/bilinear_rank/

The search core, and the home of the one namespace its members share:
everything under this directory declares `bilinear_rank`. What sits at this
root is what the import-proximity rule sends to the common ancestor, the
vocabulary several members need and none owns.

In this folder:

- [`candidate_pool.h`](candidate_pool.h): the rank-one pool, materialised
  where it fits and addressed by [`[yang2025]`](../../references.md)'s
  odometer where it does not.
- [`reflected_gray_walk.h`](reflected_gray_walk.h): the step order both
  leaves walk in, one digit changed per step over GF(2) and GF(p) alike.
- [`algorithm_recovery.h`](algorithm_recovery.h): a decomposition written
  out as the ⟨L,R,P⟩ triple the field publishes, and read back.
- [`bilinear_rank_aliases.h`](bilinear_rank_aliases.h): the type aliases
  that name the domain.
- [`commands/`](commands/): `operators-to-tensor`, the way in from the
  published ecosystem.

The members, one question each:

- [`greedy_heuristic/`](greedy_heuristic/): rank from above,
  cheaply, with no optimality guarantee past its first step.
- [`exhaustive/`](exhaustive/): the complete decision procedure.
- [`branch_and_bound/`](branch_and_bound/): the same tree, cut by
  the incumbent's cost, stoppable at any moment.
- [`canonical_augmentation/`](canonical_augmentation/): each
  equivalence class exactly once, no memory.
- [`orbit_reduction/`](orbit_reduction/): the quotient by the
  map's automorphisms.
- [`flip_graph/`](flip_graph/): moving a working decomposition
  sideways.
- [`map_construction/`](map_construction/): building the input
  tensors.
- [`search_plan/`](search_plan/): the choices a run records and
  replays.

How to use, from the repository root with the tools installed; the output is
this run's, quoted as printed:

```
$ operators-to-tensor evidence/fixtures/plinopt/2x2x2_7_Strassen_L.sms \
    evidence/fixtures/plinopt/2x2x2_7_Strassen_R.sms \
    evidence/fixtures/plinopt/2x2x2_7_Strassen_P.sms -q 2 | head -1
# read 7 products over GF(2): L is 7x4, R is 7x4, P is 4x7
```

The triple is PLinOpt's own Strassen, and what comes out rebuilds
`evidence/fixtures/matmul_2x2x2.tensor` entry for entry, which the test
suite asserts.
