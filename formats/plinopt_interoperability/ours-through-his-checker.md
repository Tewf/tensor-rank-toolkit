# Ours through his checker

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
