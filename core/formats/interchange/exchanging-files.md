# Exchanging files with PLinOpt: the page to read first

If you hold operators from [PLinOpt](https://github.com/jgdumas/plinopt), or
from [Sedoglavic's catalogue](https://fmm.univ-lille.fr/), which publishes the
same triple as Maple matrices, this is how to move them in and out. Everything
below has been run; what it produced is
[`checking-ours-with-another-tool.md`](checking-ours-with-another-tool.md) and
[`bringing-an-algorithm-in.md`](bringing-an-algorithm-in.md).

## What to install

Nothing, to read and write those files: `.sms` is handled here and Givaro is
already this repository's only dependency. To check our output with *their*
binaries you need PLinOpt itself, which wants LinBox ≥ 1.7 and Givaro ≥ 4.2:

```sh
sudo apt install git make g++ pkg-config liblinbox-dev
git clone https://github.com/jgdumas/plinopt && cd plinopt && make
```

On Ubuntu 24.04 the distribution packages are new enough and `make` needs no
flags. Its `make check` runs PLinOpt's own examples and is the fastest way to tell
whether a failure below is ours or a broken build.

## Your files in

Three SMS operators are an algorithm; the map it computes is the tensor every
tool here takes as input.

```sh
operators-to-tensor L.sms R.sms P.sms -q 2 > map.tensor
minimise-rank map.tensor            # now search for a cheaper one
```

The argument shape is `PMchecker`'s, deliberately: same three filenames in the
same order, same `-q`. `--field` is the same flag spelled the way the `.tensor`
header spells it. There is no default, because **SMS carries no field**: the
type letter `R` or `M` says nothing and PLinOpt does not read it either.

## Our files out, and the one command that checks them

```sh
minimise-rank evidence/fixtures/f2_5x5.tensor --emit-operators out
PMchecker out_L.sms out_R.sms out_P.sms -q 2
```

`SUCCESS` from an outside checker is an independent confirmation; why the degree
label it derives makes it one is
[`checking-ours-with-another-tool.md`](checking-ours-with-another-tool.md).
Use `MMchecker` for a matrix-multiplication map and `PMchecker` for a
polynomial one: handing a `matmul_*` result to the wrong one fails exactly as a
bad operator would, which is the first of
[four false failures](four-false-failures.md).

## Known differences

The field-by-field comparison, with the file and line on both sides, is
[`where-the-conventions-differ.md`](where-the-conventions-differ.md). The four that cause failures in practice:

- **One triple per line, and the value last on it.** Two triples on a line are
  read by LinBox as one number. Refused here rather than accepted, so the
  disagreement is loud.
- **Comments only before the header.** LinBox skips `#` above the header line
  and nowhere else. Ours are more permissive on reading and never emit one
  lower down, so a file that has travelled still says where it came from.
- **A rational algorithm has moduli it does not survive.** `-q 2` on an
  algorithm with halves is refused here, naming the entry; those checkers report
  the same triple as not an algorithm, which is true and less specific.
- **Three of the 153 published matrices are not rationals at all**: the `-X` family from
  the accuracy paper carries polynomials in an indeterminate. Refused by name.

Thirteen of those files are vendored so this is exercised by the test suite and not
only by hand: [`evidence/fixtures/plinopt/`](../../../evidence/fixtures/plinopt/README.md), and
what the suite can and cannot carry is
[`what-is-checked-automatically.md`](what-is-checked-automatically.md).
