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

- [Against the heuristics](against-the-heuristics.md), where the finder is
  dominated on every fixture it can be compared on.
- [The one shape where nothing else answers either](three-by-three.md), which is
  `<3,3,3>` and where it also does not.
- [Where the mechanism does show something](where-the-mechanism-shows.md): the
  parts worth keeping, and the zero-term drop that was a correctness fix.
- [The honest reading](the-honest-reading.md), which is why the finder is on the
  `rejected-experiments` branch and what survived it.
