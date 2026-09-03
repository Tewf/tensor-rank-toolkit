# Common recipes

The lines people actually type, one per question. Every one runs from the
repository root against a shipped fixture, so each can be pasted and then edited
rather than assembled from the tables. What each flag costs is in those tables;
this page is only the shape of the line.

## Rank

| You want | The line |
|---|---|
| an upper bound in milliseconds | `minimise-rank evidence/fixtures/f2_5x5.tensor` |
| the same, without the expensive third step | `minimise-rank evidence/fixtures/f2_5x5.tensor --steps 2` |
| is there an algorithm this small, decided | `decide-rank evidence/fixtures/matmul_2x2x2.tensor --target 7` |
| and is one product fewer impossible | `decide-rank evidence/fixtures/matmul_2x2x2.tensor --target 6` |
| the rank itself, no target, sweeping | `decide-rank evidence/fixtures/f2_2x3.tensor` |
| the same refutation, quotiented | `decide-rank evidence/fixtures/matmul_2x2x2.tensor --target 6 -s matmul 2 2 2` |
| an answer with a receipt to multiply out | `factor-over-canonical-basis evidence/fixtures/f2_2x3.tensor` |
| two slices, read off rather than searched | `decide-rank-by-pencil evidence/fixtures/pencil_split_f3_3.tensor` |

**`--target k` is the difference between two questions.** With it the tool asks
whether `k` products suffice and answers yes or no; without it, it sweeps and
answers how few. A sweep holds its pool, so it is the one shape that cannot run
on a pool too large to materialise.

## A refutation somebody else can check

```sh
decide-rank-by-sat evidence/fixtures/matmul_2x2x2.tensor --target 6 --proof six.drat
#   k = 6 [kissat]: NO, rank is more than 6  (…s), refutation verified
```

The parenthesised seconds are elided above: a wall clock is that run's own,
not the answer's, which is [`../MEASURING.md`](../MEASURING.md)'s division.

The run hands the certificate to `drat-trim` itself and stops if it fails to
verify, so what `six.drat` is for is a reader who wants to check it again
elsewhere. `--proof` is kissat only; any other solver, and the `smt` backend,
refuse the flag rather than write nothing. `--break-symmetry` is sound and off by
default, and is worth at least 76x on a question expected to answer no.

## Bounding a run so it cannot outlive you

| You want | The line |
|---|---|
| a search that stops and says so | `decide-rank <map> --target k --node-limit 200000` |
| one leaf that cannot run for hours | `decide-rank <map> --target k --leaf-limit 10000000` |
| a solver that gives the core back | `decide-rank-by-sat <map> --target k --timeout 60` |

All three leave as **exit 3**, which is not exit 1: a budget that ran out proves
nothing in either direction. `infrastructure/cli/exit_code.h` is where that is stated and
[`../web_interface/what-it-will-not-say.md`](../web_interface/what-it-will-not-say.md)
is a front end keeping it.

## The one pipeline, and building a map to run it on

```sh
make-tensor --matmul 2 2 2 2 > strassen.tensor      # or --polynomial, --cyclic, --field
minimise-rank strassen.tensor --plateau 2 --emit-operators out  # 8 -> 7 products
sparsify-operator out_L.sms                         # then the additions
curve-bounds --solvers                              # which backends this machine has
```

## A published algorithm, read in and put to work

```sh
operators-to-tensor stem_L.sms stem_R.sms stem_P.sms -q 2 > map.tensor
sparsify-operator stem_L.sms          # fewer additions in someone else's scheme
decide-rank map.tensor --target k     # or any tool: it is a map like any other
```

The shipped example: PLinOpt's own Strassen triple in
[`../evidence/fixtures/plinopt/`](../evidence/fixtures/plinopt/README.md) rebuilds
`evidence/fixtures/matmul_2x2x2.tensor` entry for entry, which the test suite asserts.
The whole flow, and the differences that bite:
[`../core/formats/interchange/bringing-an-algorithm-in.md`](../core/formats/interchange/bringing-an-algorithm-in.md).

## Timing one choice against another on one question

A comparison across two questions is not a comparison, so four flags exist to
force a route on a single one. None of them changes an answer.

```sh
decide-rank <map> --target k --leaf-route scan      # against --leaf-route walk
decide-rank <map> --target k --general-leaf         # against the packed GF(2) leaf
decide-rank <map> --target k -s matmul n m k --orbit-test generators   # against full
tighten-rank-bound <map> --general-span                # against the packed GF(2) span walk
```

Flags that mean the same thing under different names, and the ones that mean
different things under the same name:
[`one-idea-several-spellings.md`](one-idea-several-spellings.md).
