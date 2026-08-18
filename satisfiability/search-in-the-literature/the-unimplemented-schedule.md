# The one thing the survey says is not implemented, and we ship it

The same survey records that there are **no known implementations of linear
search UNSAT-SAT for MaxSAT**, though it is used elsewhere, for minimal
unsatisfiable subsets. That schedule is this module's default. Not by
independence of mind: it is the right default here for reasons that do not hold
in MaxSAT, namely that the flattening bound is often already the rank, so
ascending asks one question and stops, and that it is the only schedule which
never reads the ceiling and so cannot be misled by a loose one.
