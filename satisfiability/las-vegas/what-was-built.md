# What was built

A class of solvers that can only answer yes, `SatSolver::finds_only` in
[`../solver_process.h`](../solver_process.h), assigned by name in
[`../local_search_solver.h`](../local_search_solver.h): `yalsat`, `xnfsat`,
`probSAT` and `multilinear-sat`, a continuous relaxation with restarts local to
this machine. `--solver` pins one, by name or now by path, since none of the
four is installed. The class is enforced rather than described: `--proof` is
refused, and a `s UNSATISFIABLE` from it, which yalsat does print when unit
propagation alone closes a formula, is discarded into the third answer. So a
sweep under one of them can only raise the upper bound; it leaves as 3 below
the rank, never as 1. xnfsat is handed each parity as one `x` line, the XNF of
`[nawrocki2021]`, which `--emit-xnf` now writes for anyone; the other three
never see one. One tunable, `local_search_seed`, flag `--seed`; the restart
policies stay at each solver's default, because they are not commensurable and
the baselines are yalsat and xnfsat at their defaults.

Tested without a solver installed, against one stub script under two names,
`yalsat` and `xnfsat`, that claims a refutation and copies what it was handed,
so that the expansion and the `x` lines are each asserted on the file that
reached the solver ([`../tests/test_finds_only.cpp`](../tests/test_finds_only.cpp));
then, for each real one on `PATH`, end to end on `f2_2x2`: a yes that
reconstructs and a no that is never a no. The exit code below the rank is
asserted in [`../tests/check_exit_codes.sh`](../tests/check_exit_codes.sh).
`--emit-xnf` was checked against `cnf2xnf` on the 3-cut CNF of the same
question: on `⟨3,3,3⟩` at 23 both files carry 729 parities, this module's of
width 23 over 19 251 variables and the recovered ones of width 24 over 19 980,
one auxiliary variable per parity left behind; xnfsat solves either.
