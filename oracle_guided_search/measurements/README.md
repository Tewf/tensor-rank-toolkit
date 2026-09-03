# What the finder costs, against the baselines it has to beat

> `find-at-rank` and the descending sweep are on the `rejected-experiments`
> branch. Every number they produced stays here, because these numbers are the
> evidence for removing them and a rejection with its evidence deleted is
> indistinguishable from a whim. Commands beginning `find-at-rank` cannot be run
> from this branch.

Measured 2026-08-17, one build, one lock held for the whole run so no two timings
overlapped. Solver kissat, `--candidate-timeout 60` except where stated.

The baselines are the two upper-bound producers. `find_rank` is not one of them: it
returns a minimal decomposition when it terminates, so it answers a harder question
and beating it is not the job.

One of the two baselines, run here on the smallest fixture, taking neither step:

```sh
minimise-rank fixtures/matmul_2x2x2.tensor --steps 3 -s matmul 2 2 2
fixtures/matmul_2x2x2.tensor
  naive: 8 multiplications, 4 slices, 0 s cumulative
  step 1: 8 multiplications, 4 slices, 3.2592e-05 s cumulative
  step 2: 8 multiplications, 4 slices, 8.0161e-05 s cumulative
# step 3 pool: 225 rank-one maps
# step 3 ambient generators: 6
# step 3 round 1: stabiliser 6, 5 orbits of 225
  step 3: 8 multiplications, 4 slices, 0.000768557 s cumulative
# algorithm: 8 products, rank bound 6, gap 2, L is 8x4, R is 8x4, P is 4x8
```

No step improves on the naive 8, the empty shortlist the rest of this folder
starts from.

- [Against the heuristics](against-the-heuristics.md), where the finder is
  dominated on every fixture it can be compared on.
- [The one shape where nothing else answers either](three-by-three.md), which is
  `<3,3,3>` and where it also does not.
- [Where the mechanism does show something](where-the-mechanism-shows.md): the
  parts worth keeping, and the zero-term drop that was a correctness fix.
- [The honest reading](the-honest-reading.md), which is why the finder is on the
  `rejected-experiments` branch and what survived it.
