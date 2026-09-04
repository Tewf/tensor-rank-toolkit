# The one thing the survey says is not implemented, and we ship it

The same survey records that there are **no known implementations of linear
search UNSAT-SAT for MaxSAT**, though it is used elsewhere, for minimal
unsatisfiable subsets. That schedule is this module's default. Not by
independence of mind: it is the right default here for reasons that do not hold
in MaxSAT, namely that the [flattening bound](../bracket/) is often
already the rank, so [ascending](naming-the-problem.md) asks one question and
stops, and that it is the only schedule which never reads the ceiling and so
cannot be misled by a loose one.

On four of the seven fixtures measured in
[`the-five-schedules.md`](../bracket/the-five-schedules.md) the floor already
equals the rank, so ascending asks that one mandatory question and stops:

| Fixture | Floor, s | Ascending, s |
|---|---|---|
| f2_2x2 | 0.006 | 0.006 |
| gf4 | 0.006 | 0.006 |
