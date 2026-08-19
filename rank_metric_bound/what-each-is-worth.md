# What each bound is worth, on every fixture

The least rank each argument fails to refute, one core, fastest of three, from
`build/rank_metric_bound/tests/test_rank_metric_bound fixtures` under
[`../MEASURING.md`](../MEASURING.md). `rank_lower_bound` is the flattening bound
and both rank sums together, which is what a caller gets today; `k` and `d` are
the deciding axis's, printed by that same command. Cost is
[`the-edit-not-made.md`](the-edit-not-made.md)'s subject and the column here is
the whole bound from the tensor, taken on one core under the measurement lock on
an otherwise idle machine. The rows under 0.02 ms are a few microseconds of work
quoted to two decimals, so read them as an order of magnitude and not as a
figure; only `f3_3x6` and `matmul_3x3x3` are far enough above the noise floor to
compare with anything.

| fixture | rank held | `rank_lower_bound` | `k` | `d` | Kruskal | Griesmer | cost |
|---|---|---|---|---|---|---|---|
| `f2_5x5` | 13 | 10 | 5 | 5 | 9 | **12** | 0.46 ms |
| `f2_3x8` | 15 | 14 | 3 | 8 | 10 | 14 | 0.84 ms |
| `f2_4x7` | 16 | 14 | 4 | 7 | 10 | 14 | 0.94 ms |
| `f3_3x6` | 10 | 9 | 3 | 6 | 8 | 9 | 4.12 ms |
| `gf16_multiplication` | 9 | 8 | 4 | 4 | 7 | 8 | 0.02 ms |
| `gf8_multiplication` | 6 | 6 | 3 | 3 | 5 | 6 | 0.01 ms |
| `gf4_multiplication` | 3 | 3 | 2 | 2 | 3 | 3 | 0.00 ms |
| `f2_2x2` | 3 | 3 | 2 | 2 | 3 | 3 | 0.00 ms |
| `f2_2x3` | 5 | 5 | 2 | 3 | 4 | 5 | 0.01 ms |
| `pencil_irreducible_f2_4` | 6 | 6 | 2 | 4 | 5 | 6 | 0.01 ms |
| `pencil_singular_f2_2x3` | 3 | 3 | 2 | 2 | 3 | 3 | 0.00 ms |
| `pencil_split_f3_3` | 3 | 3 | 2 | 2 | 3 | 3 | 0.01 ms |
| `pencil_nilpotent_f2_3` | 4 | 4 | 2 | 2 | 3 | 3 | 0.01 ms |
| `matmul_2x2x2` | 7 | 6 | 4 | 2 | 5 | 5 | 0.02 ms |
| `matmul_2x2x3` | 11 | 9 | 4 | 3 | 7 | 7 | 0.08 ms |
| `matmul_3x3x3` | 23 | 14 | 9 | 3 | 11 | 12 | 2.60 ms |
| `cyclic_f2_5` | 10 | 9 | 5 | 1 | 5 | 5 | 0.07 ms |
| `w_state` | 3 | 3 | 2 | 1 | 2 | 2 | 0.00 ms |

"Rank held" is a rank somebody has exhibited, so no lower bound may exceed it,
the convention `linear_algebra/tests/test_rank_sum.cpp` already uses. On `f2_5x5`
the 13 is `[bdez2012]`'s and not ours: the best decomposition exhibited here is
14. Soundness is asserted on all eighteen and then on 120 random tensors built
from a known number of rank-one terms, a sixth of which come out bounded at
exactly their rank, so "never above" is a constraint the sweep actually tests.

**Kruskal's bound never wins.** Four ties, fourteen losses, and the reason is
structural: `k + d - 1` is the Singleton relaxation and throws away everything
the field size says. On polynomial multiplication it collapses to `n + m - 1`,
the number of output coefficients, which the flattening bound already gives.

**The Griesmer form wins once, and the win needs stating carefully.** On `f2_5x5`
it returns 12 against `rank_lower_bound`'s 10, in 0.46 ms. It does not move the
repository's bracket: `12 <= rank <= 14` already stood, and its 12 came from
`decide-rank --target 11` running to exhaustion, which `exhaustive_search` prices
at 77 s. What changed is the price of that 12, and that the cheap floor every
caller already reads now reaches it. The rank is 13 by `[bdez2012]`, so the gap
left is one product. Elsewhere Griesmer ties eleven times and loses six, and it
meets the rank held exactly on seven fixtures.

**Where it loses, it loses to a contraction argument.** In all six losses `d` is
1, 2 or 3 against a slice space of dimension up to 9, so every Griesmer term past
the second is already 1 and the sum barely clears `k + d - 1`. `matmul_3x3x3` is
the extreme: `k = 9`, `d = 3`, giving `3 + 2 + 1·7 = 12` where the rank sums
reach 14, because they count surviving terms across many contractions at once and
that is a different thing to know. Neither family dominates the other.
