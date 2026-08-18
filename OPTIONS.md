# Options

Every flag of every command, its default, and **the measurement that chose the
default**. Where nothing measured it, the row says so rather than leaving the
column blank: a default with an argument behind it and a default with a number
behind it are different claims, and this file exists so the two are never read
as one.

Timings quoted here were taken under [`MEASURING.md`](MEASURING.md), whose
**13% noise floor** applies to all of them. Two timings inside that band are not
distinguishable and are not reported here as a ratio.

## Precedence

Strongest first:

1. **an explicit flag on the command line**,
2. then **`tunables.conf`**,
3. then **the compiled default**.

A command reads the file into its settings before it walks its arguments, so a
flag that was given always overwrites what the file said, and a flag that was
not leaves the file's number standing. Deleting the file changes nothing,
because every value it ships is the number compiled in.

The rule the code keeps to make that work: **a library never opens the file.**
`cli::tunables()` is called in `*/commands/*_main.cpp` only, and the value is
passed down as the settings or budget argument the library already takes. The
literal beside that argument stays as the compiled default a caller who passes
nothing still gets.

## `BILINEAR_TUNABLES`

`cli/tunables.h` looks for the file in one of two places:

- the path named by `$BILINEAR_TUNABLES`, if that is set. **A file named there
  and missing is an error**, because it was asked for by name;
- otherwise `tunables.conf` in the working directory. Absent is not an error,
  because the defaults are the same numbers it would have held.

It is read once per run and not again, so no command can be bounded by two
different numbers in one run.

| Tunable | Reaches | Flag that beats it |
|---|---|---|
| `search_node_limit` | `decide-rank`, `deflate-strictly` | `--node-limit` |
| `ilp_node_limit` | `curve-bounds` | `--node-limit` |
| `plateau_state_budget` | `minimise-rank` | `--plateau-states` |
| `sat_memory_megabytes` | `decide-rank-by-sat`, `deflate-strictly` | `--max-memory` |
| `sat_timeout_seconds` | `decide-rank-by-sat`, `deflate-strictly` | `--timeout`, `--candidate-timeout` |
| `ilp_time_limit_seconds` | `curve-bounds` | `--solver-timeout` |
| `sat_solver_order` | `decide-rank-by-sat`, `deflate-strictly` | `--solver` |
| `ilp_backend_order` | `curve-bounds`, `list-solvers` | none; `--route built-in` sidesteps it |

A name the file spells wrongly is refused rather than ignored, and the refusal
leaves as exit 2. That is asserted end to end in
[`cli/tests/check_tunables_bound_a_run.sh`](cli/tests/check_tunables_bound_a_run.sh),
against the built command rather than against the parser.

## The tables, one page per question

- [Searching for rank](OPTIONS/searching-for-rank.md): `minimise-rank`,
  `decide-rank`, `walk-scheme`
- [Asking a SAT solver](OPTIONS/asking-a-sat-solver.md): `decide-rank-by-sat`
- [Committing to candidates](OPTIONS/committing-to-candidates.md):
  `deflate-strictly`, `enumerate-subspaces`
- [Bounding from a curve](OPTIONS/bounding-from-a-curve.md): `curve-bounds`,
  `list-solvers`
- [Sparsifying operators](OPTIONS/sparsifying-operators.md): `sparsify-operator`
- [Building maps](OPTIONS/building-maps.md): `make-tensor`

`decide-rank-by-pencil` and `factor-over-canonical-basis` are documented in
[`OPTIONS/reading-the-answer-off.md`](OPTIONS/reading-the-answer-off.md).

`find-at-rank` is gone from this branch, and with it the `--descend`,
`--candidate-timeout` and `--pretest-ceiling` flags. It is on
`rejected-experiments`; the numbers that retired it are in
[`oracle_guided_search/measurements.md`](oracle_guided_search/measurements/README.md).
