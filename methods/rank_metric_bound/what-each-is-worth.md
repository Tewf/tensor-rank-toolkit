# What each bound is worth, on every fixture

The least rank each argument fails to refute, one core, fastest of three, from
`build/methods/rank_metric_bound/tests/test_rank_metric_bound fixtures` under
[`../../MEASURING.md`](../../MEASURING.md). The `flattening + rank sums` column is what
`rank_lower_bound` returned before Griesmer joined it; `floor today` is what a
caller gets now, which is that column and the Griesmer one under one `max`; `k` and `d` are
the deciding axis's, printed by that same command. Cost is
[`joining-the-shared-floor.md`](joining-the-shared-floor.md)'s subject and the column here is
the whole bound from the tensor, taken on one core under the measurement lock on
an otherwise idle machine. The rows under 0.02 ms are a few microseconds of work
quoted to two decimals, so read them as an order of magnitude and not as a
figure; `f3_3x6`, `matmul_3x3x3` and the three `[wang2026]` formats are the only
rows far enough above the noise floor to compare with anything. The six rows that
arrived with
[`../../evidence/fixtures/published-targets.md`](../../evidence/fixtures/published-targets.md) were taken
in a later run of the same command, which moved the other eighteen by less than
the 13% the chassis varies by on its own, so those stand as first taken.

| fixture | rank held | flattening + rank sums | `k` | `d` | Kruskal | Griesmer | cost | floor today |
|---|---|---|---|---|---|---|---|---|
| `f2_5x5` | 13 | 10 | 5 | 5 | 9 | **12** | 0.46 ms | **12** |
| `f2_3x8` | 15 | 14 | 3 | 8 | 10 | 14 | 0.84 ms | **14** |
| `f2_4x7` | 16 | 14 | 4 | 7 | 10 | 14 | 0.94 ms | **14** |
| `f3_3x6` | 10 | 9 | 3 | 6 | 8 | 9 | 4.12 ms | **9** |
| `gf16_multiplication` | 9 | 8 | 4 | 4 | 7 | 8 | 0.02 ms | **8** |
| `gf32_multiplication` | 25 | 12 | 5 | 5 | 9 | 12 | 0.06 ms | **12** |
| `gf64_multiplication` | 36 | 14 | 6 | 6 | 11 | 14 | 0.18 ms | **14** |
| `gf8_multiplication` | 6 | 6 | 3 | 3 | 5 | 6 | 0.01 ms | **6** |
| `gf4_multiplication` | 3 | 3 | 2 | 2 | 3 | 3 | 0.00 ms | **3** |
| `f2_2x2` | 3 | 3 | 2 | 2 | 3 | 3 | 0.00 ms | **3** |
| `f2_2x3` | 5 | 5 | 2 | 3 | 4 | 5 | 0.01 ms | **5** |
| `pencil_irreducible_f2_4` | 6 | 6 | 2 | 4 | 5 | 6 | 0.01 ms | **6** |
| `pencil_singular_f2_2x3` | 3 | 3 | 2 | 2 | 3 | 3 | 0.00 ms | **3** |
| `pencil_split_f3_3` | 3 | 3 | 2 | 2 | 3 | 3 | 0.01 ms | **3** |
| `pencil_nilpotent_f2_3` | 4 | 4 | 2 | 2 | 3 | 3 | 0.01 ms | **4** |
| `matmul_2x2x2` | 7 | 6 | 4 | 2 | 5 | 5 | 0.02 ms | **6** |
| `matmul_2x2x3` | 11 | 9 | 4 | 3 | 7 | 7 | 0.08 ms | **9** |
| `matmul_2x3x4` | 20 | 14 | 12 | 2 | 13 | 13 | 5.91 ms | **14** |
| `matmul_3x3x3` | 23 | 14 | 9 | 3 | 11 | 12 | 2.60 ms | **14** |
| `matmul_3x3x4` | 29 | 18 | 12 | 3 | 14 | 15 | 21.69 ms | **18** |
| `matmul_3x4x4` | 38 | 21 | 16 | 3 | 18 | 19 | 324.62 ms | **21** |
| `cyclic_f2_5` | 10 | 9 | 5 | 1 | 5 | 5 | 0.07 ms | **9** |
| `cyclic_f2_7` | 13 | 12 | 7 | 1 | 7 | 7 | 0.45 ms | **12** |
| `w_state` | 3 | 3 | 2 | 1 | 2 | 2 | 0.00 ms | **3** |

"Rank held" is a rank somebody has exhibited, so no lower bound may exceed it,
the convention `core/linear_algebra/tests/test_rank_sum.cpp` already uses. On `f2_5x5`
the 13 is `[bdez2012]`'s and not ours: the best decomposition exhibited here is
14. `gf32` and `gf64` hold 25 and 36, which is the naive algorithm and nothing
better, because the 13 and 15 published for them were not traced to a table
anybody here has read and this column is the one that has to be safe.
Soundness is asserted on all twenty-four and then on 120 random tensors built
from a known number of rank-one terms, a sixth of which come out bounded at
exactly their rank, so "never above" is a constraint the sweep actually tests.

**Kruskal's bound never wins.** Eleven ties, thirteen losses, and the reason is
structural: `k + d - 1` is the Singleton relaxation and throws away everything
the field size says. On polynomial multiplication it collapses to `n + m - 1`,
the number of output coefficients, which the flattening bound already gives.

**The Griesmer form wins once, and the win needs stating carefully.** On `f2_5x5`
it returns 12 where the other three terms of `rank_lower_bound` reach only 10, in 0.46 ms. It does not move the
repository's bracket: `12 <= rank <= 14` already stood, and its 12 came from
`decide-rank --target 11` running to exhaustion, which `exhaustive_search` priced
at 77 s. What changed is the price of that 12, and that the cheap floor every
caller already reads reached it.

**Then the bracket moved and this win became a near miss.** Exhausting
`--target 12` put the floor at 13, where `[bdez2012]` also puts the rank, so
Griesmer's 12 is now one *below* the proved floor rather than level with it. The
cheap bound is still the best of the four terms here by two products, and it is
still the reason nobody pays 77 s for a floor; what it no longer does is match
what the exhaustive search has since established. Elsewhere Griesmer ties thirteen times and loses ten, and it
meets the rank held exactly on seven fixtures, none of them among the six added
last.

**Where it loses, it loses to a contraction argument.** In all ten losses `d` is
1, 2 or 3 against a slice space of dimension up to 16, so every Griesmer term
past the second is already 1 and the sum barely clears `k + d - 1`.
`matmul_3x4x4` is now the extreme: `k = 16`, `d = 3`, giving `3 + 2 + 1·14 = 19`
where the rank sums reach 21, and `matmul_3x3x3` loses the same way with
`3 + 2 + 1·7 = 12` against 14. They count surviving terms across many
contractions at once and that is a different thing to know. Neither family
dominates the other.

**The cost column now has something in it, and it is the axis.** `matmul_3x4x4`
takes 324.62 ms where `f3_3x6` takes 4.12, some eighty times, and the bound is
not behaving differently: it reads one table entry per vector on an axis, so
`⟨3,4,4⟩`'s axes of 12, 16 and 12 over GF(2) are 73 728 contractions of a
12 × 16 matrix against `f3_3x6`'s 7 317 of a 3 × 6 one. Ten times as many, each
larger. A table walk and not a search, which is why all three stay in a test
that is not labelled `slow`.
