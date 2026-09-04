# What the file format cost

MPS is a fixed-column punched-card layout from the IBM MPS/360 era, its
manual's date deliberately unstated in `[mps360]`'s ledger entry, and reading
it wrongly is how a solver is
handed a different programme from the one intended. The chain that consumes it is
in [`README.md`](./).

Everything reaches a solver as fixed-column MPS, and three of its traps are
silent. All three were found by running the solvers, not by reading about them.

- **An integer variable with no stated upper bound is binary** to CBC and to
  GLPK, and unbounded to lp_solve, so the same file means two different
  problems; [`mps_format.h`](mps_format.h) writes every variable's bounds out
  rather than leaning on a default.
- **The integrality markers belong in fields 3 and 5**, not 4 and 6, and free-form
  MPS is not an escape: CBC parses `BOUNDS` by character position whatever the
  rest of the file looks like. This one is now checked against IBM as well as
  measured: `[oslmps]` puts the keyword in field 5 and says field 4 *"must be
  blank"*, and the reason CPLEX's manual says field 4 is that it counts tokens,
  not columns. [`mps_format.h`](mps_format.h) has the whole of it.
- **MPS has no direction.** The objective row is always minimised, so a
  maximisation is written negated, as commented where the negation happens in
  [`mps_format.cpp`](mps_format.cpp). Without that every maximisation comes back
  at its minimum, which is feasible, and therefore passes every check except
  comparison with the right answer.

`tests/test_integer_programme`'s `check_written_model` pins the second trap by
running the writer and reading the bytes back, columns fifteen and forty
included, rather than trusting the prose:

    $ ./build/methods/integer_programme/tests/test_integer_programme
      ok    the marker names its section in field three = 1
      ok    the marker opens the integers in field five = 1

Rows are scaled by the lowest common denominator of their own entries, so
coefficients and bounds arrive as integers and nothing is lost on the way in.
