# What this branch holds, and why it exists

`main` carries what won. This branch carries what lost, **with the evidence
that decided it and the implementations themselves**, because a rejection with
its evidence deleted is indistinguishable from a whim, and because several of
these are respectable algorithms whose only defect is that something here
measured faster.

Nothing on this branch is broken. Each directory has its own README with the
numbers that retired it.

| directory | what it is | the number that retired it |
|---|---|---|
| [`orbit_walk/`](orbit_walk/) | canonical subspace by walking the whole group | the canonical image reaches the same 1 distinct child in 83 nodes / 3.04 s where the walk needs 954 / 21.9 s |
| [`orbits_by_default/`](orbits_by_default/) | quotienting a product shape by default, as a patch | 2.3x faster on a refutation, 7.4x slower on a find; the default stayed off |
| [`dominated_sparsifiers/`](dominated_sparsifiers/) | `[beniamini2020]`'s Algorithms 3 and 4 (possibly their only public implementation) and the row-basis heuristic | same counts as the exact matroid greedy, 88x to 343x slower |

## In this branch's history rather than in `retired/`

- **`find-at-rank`** and the descending sweep that depended on it, with
  `--descend` and `--pretest-ceiling`:
  `rejected-experiments:oracle_guided_search/commands/find_at_rank_main.cpp`.
  Its assumed 190x asymmetry measured as about one, and once the rank sums
  closed the floor-to-rank gap the sweep's bracket lost on all seven fixtures.
  Every number from that rejection stays in
  `oracle_guided_search/measurements/` on `main`.

## Rejections that live on `main` instead

A rejection whose record is a document rather than an implementation stays on
`main` beside the code it judges: McKay canonical augmentation
(`incumbent_search/what-the-tree-repeats.md`), the GPU for the incumbent
search (`positioning/hardware-and-parallelism.md`), and seeding the search
from a published scheme (`incumbent_search/why-it-cannot-be-seeded.md`).
