# What the file format cost

MPS is a punched-card layout from 1970 and reading it wrongly is how a solver is
handed a different programme from the one intended. The chain that consumes it is
in [`README.md`](README.md).

Everything reaches a solver as fixed-column MPS, and three of its traps are
silent. All three were found by running the solvers, not by reading about them.

- **An integer variable with no stated upper bound is binary** to CBC and to
  GLPK, and unbounded to lp_solve. The same file is then two different problems.
  Every variable states both bounds, `PL` and `MI` included.
- **The integrality markers belong in fields 3 and 5**, not 4 and 6, and free-form
  MPS is not an escape: CBC parses `BOUNDS` by character position whatever the
  rest of the file looks like.
- **MPS has no direction.** The objective row is always minimised, so a
  maximisation is written negated. Without that every maximisation comes back at
  its minimum, which is feasible, and therefore passes every check except
  comparison with the right answer.

Rows are scaled by the lowest common denominator of their own entries, so
coefficients and bounds arrive as integers and nothing is lost on the way in.
