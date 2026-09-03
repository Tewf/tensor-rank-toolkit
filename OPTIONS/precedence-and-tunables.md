# Precedence, and the file the numbers come from

Where a number a run is bounded by comes from when three places could have
supplied it, and how a command reaches the file without a library ever opening
it. The flags themselves are the tables beside this page;
[`../OPTIONS.md`](../OPTIONS.md) is the index.

## Precedence

Strongest first:

1. **an explicit flag on the command line**,
2. then **`tunables.conf`**,
3. then **the compiled default**.

A command reads the file into its settings before it walks its arguments, so a
flag that was given always overwrites what the file said, and a flag that was
not leaves the file's number standing. Deleting the file changes nothing,
because every value it ships is the number compiled in.

The chain in one run, on `evidence/fixtures/matmul_2x2x2.tensor --target 6`: a
`tunables.conf` holding `search_node_limit = 100` (compiled default
`5000000`) leaves the file's number standing:

```sh
$ decide-rank evidence/fixtures/matmul_2x2x2.tensor --target 6
  100 nodes in 0.00021733 s
  GAVE UP: the node limit was reached, so nothing is decided.
           Raise --node-limit to search further.
```

Giving `--node-limit 5000000` on the command line, same directory and same
file, overwrites it and the search runs to its exhaustive answer:

```sh
$ decide-rank evidence/fixtures/matmul_2x2x2.tensor --target 6 --node-limit 5000000
  25399 nodes in 0.0330047 s
  NO: there is no algorithm with 6 products. The search was exhaustive.
```

**`auto` is a fourth thing the file may say**, and it resolves to the same
compiled default rather than to something else: where a default is a reading of
the machine, `auto` is how the file asks for that reading instead of pinning a
number over it. It is refused on a tunable that has no machine reading behind it,
by name, rather than accepted and ignored. Which tunables have one is
`machine_read_tunables()` in `infrastructure/cli/tunables.h`, and `show-limits` prints what each
resolved to here.

The rule the code keeps to make that work: **a library never opens the file.**
`cli::tunables()` is called in `*/commands/*_main.cpp` only, and the value is
passed down as the settings or budget argument the library already takes. The
literal beside that argument stays as the compiled default a caller who passes
nothing still gets.

## `BILINEAR_TUNABLES`

`infrastructure/cli/tunables.h` looks for the file in one of two places:

- the path named by `$BILINEAR_TUNABLES`, if that is set. **A file named there
  and missing is an error**, because it was asked for by name;
- otherwise `tunables.conf` in the working directory. Absent is not an error,
  because the defaults are the same numbers it would have held.

It is read once per run and not again, so no command can be bounded by two
different numbers in one run.

| Tunable | Reaches | Flag that beats it |
|---|---|---|
| `search_node_limit` | `decide-rank`, `decide-rank-by-deflation`, `factor-over-canonical-basis` | `--node-limit` |
| `search_leaf_limit` | `decide-rank` | `--leaf-limit` |
| `ilp_node_limit` | `curve-bounds` | `--node-limit` |
| `plateau_state_budget` | `minimise-rank` | `--plateau-states` |
| `sat_memory_megabytes` | `decide-rank-by-sat`, `decide-rank-by-deflation` | `--max-memory` |
| `sat_timeout_seconds` | `decide-rank-by-sat`, `decide-rank-by-deflation` | `--timeout`, `--candidate-timeout` |
| `ilp_time_limit_seconds` | `curve-bounds` | `--solver-timeout` |
| `sat_solver_order` | `decide-rank-by-sat`, `decide-rank-by-deflation` | `--solver` |
| `ilp_backend_order` | `curve-bounds` | none; `--route built-in` sidesteps it, `--solvers` prints it |

**`tighten-rank-bound` reaches no row**, and that is a fact about it rather than an
omission here: its main does not include `infrastructure/cli/tunables.h` at all, so `--nodes`,
`--width`, `--summand-rank` and `--rounds` are compiled defaults the file cannot
move. A run of it is bounded by what is on the line and nothing else.

A name the file spells wrongly is refused rather than ignored, and the refusal
leaves as exit 2. That is asserted end to end in
[`../cli/tests/check_tunables_bound_a_run.sh`](../infrastructure/cli/tests/check_tunables_bound_a_run.sh),
against the built command rather than against the parser.
