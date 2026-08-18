# Exchanging operators with PLinOpt

`[plinopt]` is the reference toolchain for exactly these operators, so SMS is
not a convenience here. It is the interface to the established implementation,
and every disagreement about it is a disagreement about whether the two sides can
exchange anything at all. Both directions have been run against its binaries,
built from source and installed nowhere.

What the type letter means, and what is not known about it, is
[`sms_file.h`](sms_file.h). The `stem_{L,R,P}.sms` naming is his checkers'
calling convention and is explained where the files are written,
[`../descent_search/commands/minimise_rank_main.cpp`](../descent_search/commands/minimise_rank_main.cpp).

## Ours through his checker

`minimise-rank <fixture> --emit-operators <stem>`, then
`PMchecker <stem>_{L,R,P}.sms -q p`. `P` is outputs by products.

| fixture | products | shapes | PMchecker |
|---|---|---|---|
| `f2_2x2` | 3 | L 3x2, R 3x2, P 3x3 | `SUCCESS: correct 1o1o2 ... modulo 2` |
| `f2_2x3` | 5 | L 5x2, R 5x3, P 4x5 | `SUCCESS: correct 1o2o3 ... modulo 2` |
| `f2_5x5` | 14 | L 14x5, R 14x5, P 9x14 | `SUCCESS: correct 4o4o8 ... modulo 2` |
| `f3_3x6` | 10 | L 10x3, R 10x6, P 8x10 | `SUCCESS: correct 2o5o7 ... modulo 3` |

He derives the `1o1o2` and `4o4o8` degree labels from the matrix dimensions
alone, with no knowledge of the fixture, and they match it. So **his checker
independently confirms the published 14 on `f2_5x5` and 10 on `f3_3x6`**, which
is a stronger statement than our own `recovers_map` makes: that one checks our
arithmetic against our arithmetic.

The files also round-trip through his optimiser.
`optimizer | compacter | SLPchecker -M` on `f2_5x5` gives `SUCCESS` for both the
9x14 `P`, at 21 additions instead of 40, and the 14x5 `R`, at 9 instead of 17.
Read the last line, not the `Found D:` one: on `R` the common-subexpression pass
reaches 10 and the kernel pass then takes it to 9.

## His file through ours

`sparsify-operator fixtures/plinopt/2x2x2_7_Winograd_L.sms` reads his shipped 7x4
integer operator, header `7 4 R`, 14 nonzeros including negatives. It reaches 10
nonzeros by the row-basis heuristic and 10 by each exact oracle, and none of the
three tripped the not-the-same-operator guard.

Comments travel both ways. His matrices carry one `#` line before the header and
ours carry a two-line provenance block, and every tool on both sides skipped them
without a word.

## Four ways to produce a failure that is not about SMS

Each of these looks like a defect in this layer. None is.

**Match the checker to the map.** `PMchecker` checks polynomial multiplication,
which is what everything in [`../fixtures/`](../fixtures/) is. `MMchecker` is the
matrix-multiplication one and is a separate binary. Handing a `matmul_*` fixture
to `PMchecker` fails exactly as a bad operator would.

**Give `-q` a modulus.** With the flag absent or its value empty, `PMchecker`
falls back to rational arithmetic and reports
`****** ERROR, not a 1o2o3 MM algorithm******`, sometimes after dumping core. A
correct `GF(2)` algorithm fails this way, and nothing in the message says the
modulus is missing.

**Do not copy `-E -N` from his Makefile.** The `slpcheck` recipe passes exhaustive
common-subexpression elimination and exhaustive nullspace permutations, which
stalls past 300 s on a 9x14. Without them the round trip above finishes in
moments. Roughly 10x10 is where they stop being usable.

**Name the file `.sms`.** `sparsify-operator` picks its reader by that literal
four-character suffix, so any other name silently gets the dense reader. That is
ours to fix and is part of the deferred CLI work.

`optimizer` with no arguments aborts with a `LinBox::MatrixStreamError` rather
than printing usage. Ask it for `-h`.

## What is checked automatically

[`tests/test_sms_interoperability.cpp`](tests/test_sms_interoperability.cpp), over
the three of his files vendored in [`../fixtures/plinopt/`](../fixtures/plinopt/),
so nothing here reaches outside the repository for its inputs. The runs above are
not in the suite: they need his binaries, which are not a dependency of this
build.
