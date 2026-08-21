# What both sides do with an SMS file, field by field

Read off the sources rather than off either project's prose, and every row that
says the two differ was run against `plinopt/bin/sms2pretty` on bytes written for
it. His side is LinBox 1.7.0 as packaged on Ubuntu 24.04, which is what PLinOpt
links; ours is [`../sms_file.cpp`](../sms_file.cpp).

| | His | Ours | Same? |
|---|---|---|---|
| header | `rows columns letter`, `strtoul(…, 0)` so `0x10` is hex, `linbox/util/formats/sms.h:56-94` | three tokens, decimal only | letter and counts agree |
| after the letter | **nothing may follow on that line**, `sms.h:89` | a trailing `#` comment is stripped and accepted | ours is looser |
| index base | 1-based, `sms.h:115-116` | 1-based, `sms_file.cpp:143-147` | yes |
| comments | `#` skipped **only above the header**, `linbox/util/matrix-stream.inl:167-180`; one below it is a format error | `#` to end of line, anywhere | ours is looser; ours never writes one below the header |
| field | **not in the file.** `PMchecker` reads every operator over `QRat` and takes the modulus from `-q`, `src/PMchecker.cpp:213-219` | not in the file; `read_sms(…, field)` takes it as a parameter | yes |
| type letter | ignored. 15 of his 153 matrices are `M`, 9 of those with negative entries and no modulus | `R` for rationals, `M` for GF(p), on writing; all eight letters accepted on reading | yes, and neither side depends on it |
| triples per line | **one.** The value is read by Givaro's line-greedy rational operator, so `1 1 5 3 2 7` is a single entry of 5327, silently | refused, naming the value | **no**, and ours refuses what his misreads |
| triple across lines | accepted | accepted | yes |
| entry | whatever ring the stream was built over: `sms2pretty` uses `Q[X]` and three of his files carry `2X`, `3*X` | `Q`, or `GF(p)` with a field; a polynomial is refused by name | **no**, ours is narrower |
| explicit zero | dropped, `matrix-stream.inl:244` | stored as zero | yes in effect |
| ordering | not required, and `DPS-smallrat` does not obey it | not required | yes |
| terminator | `0 0 v` for any `v`, `sms.h:113`; EOF without one is an error | `0 0` and the value ignored; EOF without one throws | yes |
| after the terminator | his checkers stop; `sms2pretty` loops with `newmatrix()` and `4o4o4_F32_Montgomery_P.sms` really holds four | stops | yes with the checkers, no with `sms2pretty` |
| the triple | `stem_L.sms`, `stem_R.sms`, `stem_P.sms`; L is products×(m·n), R products×(n·k), P (m·k)×products, row-major, `src/MMchecker.cpp:7-10` | the same, written by `--emit-operators` and read by `operators-to-tensor` | yes, and asserted in `descent_search/tests/test_operators_to_tensor.cpp` |

## What was actually round-tripped

All 153 `.sms` in his `data/`, read here and written straight back, then both
files put through `sms2pretty`: **149 print identically, entry for entry**.

- `4o4o4_F32_Montgomery_P.sms` is the 150th: its first matrix comes back
  identical and the other three are dropped, as his checkers drop them.
- `2x2x2_7_DPS-accurate-X_{L,R,P}.sms` are the three refused, for the
  indeterminate.

Reading the other way, his published triples rebuild maps this repository
constructs from their definitions: Strassen and Winograd give `matmul_2x2x2`,
`3x3x3_JS` and `3x3x3_23_55` give `matmul_3x3x3`, Karatsuba gives `f2_2x2`,
Toom-3 gives `f5_3x3`, and `3x4x7_63_rational` gives ⟨3,4,7⟩ over GF(3), GF(5)
and GF(7) alike. Where `operators-to-tensor` refuses a modulus, `MMchecker`
refuses the same triple at the same modulus and accepts it at the same ones:
`DPS-smallrat` fails at 2, 3 and 5 on both sides and succeeds at 7 and 11.
