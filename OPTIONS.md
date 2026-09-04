# Options

Every flag of every command, its default, and **the measurement that chose the
default**. Where nothing measured it, the row says so rather than leaving the
column blank: a default with an argument behind it and a default with a number
behind it are different claims, and this file exists so the two are never read
as one.

Timings quoted here were taken under [`MEASURING.md`](MEASURING.md), whose
**13% noise floor** applies to all of them. Two timings inside that band are not
distinguishable and are not reported here as a ratio.

**`--help` is the one flag every command shares**, the three instruments included.
It prints the usage and leaves as **exit 2**: a line asking for help asked no
question, and `infrastructure/cli/exit_code.h` has one code for a line that did not parse. It is
not repeated in the tables below, and it is asserted against every built binary
rather than described, in
[`infrastructure/cli/tests/check_argument_grammar.sh`](infrastructure/cli/tests/check_argument_grammar.sh).

**A word to script authors before `set -e`.** Nonzero here is an answer, not a
failure: a proved refutation is **1**, `--help` is **2**, and a budget that ran
out is **3**, which proves nothing in either direction. A script that treats
any of them as an error aborts on the tool's most useful answers; test the code
you mean, the way [`.github/workflows/ci.yml`](.github/workflows/ci.yml) does
around every refusal it asserts.

## Before the tables

- [**One question per command**](OPTIONS/one-question-per-command.md): the one
  thing each tool answers that no other does, why there are thirteen rather than
  eight, and which three binaries are instruments rather than tools.
- [**Precedence, and the file the numbers come from**](OPTIONS/precedence-and-tunables.md):
  flag, then `tunables.conf`, then the compiled default; where the file is looked
  for; and which tunable reaches which command.
- [**Common recipes**](OPTIONS/common-recipes.md): the lines people actually
  type, one per question, each running against a shipped fixture.
- [**One idea, several spellings**](OPTIONS/one-idea-several-spellings.md):
  where two tools spell one idea differently, where one spelling means two
  things, and what each of the ten enum-like flags does with a bad value:
  four different things.

## The tables, one page per question

- [Searching for rank](OPTIONS/searching-for-rank.md): `minimise-rank`, `tighten-rank-bound`,
  `decide-rank`, `walk-scheme`
- [Asking a SAT solver](OPTIONS/asking-a-sat-solver.md): `decide-rank-by-sat`
- [Committing to candidates](OPTIONS/committing-to-candidates.md):
  `decide-rank-by-deflation`, `enumerate-subspaces`
- [Bounding from a curve](OPTIONS/bounding-from-a-curve.md): `curve-bounds`, and
  the `--solvers` that was the command `list-solvers`
- [Sparsifying operators](OPTIONS/sparsifying-operators.md): `sparsify-operator`
- [Building maps](OPTIONS/building-maps.md): `make-tensor`, `operators-to-tensor`

`decide-rank-by-pencil` and `factor-over-canonical-basis` are documented in
[`OPTIONS/reading-the-answer-off.md`](OPTIONS/reading-the-answer-off.md).

`find-at-rank` is gone from this branch, to `rejected-experiments`, and with it
the `--descend` and `--pretest-ceiling` flags; the numbers that retired it are in
[`methods/bilinear_rank/canonical_augmentation/measurements/`](methods/bilinear_rank/canonical_augmentation/measurements/).
**`--candidate-timeout` is not gone**, and this sentence used to say it was:
`find-at-rank` had a flag of that name and so does `decide-rank-by-deflation`, which
still accepts it and is documented in
[`OPTIONS/committing-to-candidates.md`](OPTIONS/committing-to-candidates.md).
Checked against the branch rather than guessed:
`rejected-experiments:methods/bilinear_rank/canonical_augmentation/commands/find_at_rank_main.cpp`
parses its own.
