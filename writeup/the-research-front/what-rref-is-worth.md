# What `rref` is worth here, measured before porting it

[`lower-bounds.md`](lower-bounds.md) named `[yang2025]`'s `rref` pruner as the one
candidate in the literature aimed at the gap our floors leave, and said
establishing whether it closes that gap meant reading the thesis. It did not. It
meant running the author's code, which took under an hour and returned a clean
no.

**`rref` never beats `rank_lower_bound` on any fixture here.** It ties on five and
loses on fifteen. Nothing is ported.

## What `--pruners rref` actually runs, which is not what this repository said

Two different things carry the name in `coolcomputery/tensor-cpd-search`, and an
earlier reading here described the wrong one.

**`prune_rref0` in `cpd/original/cpd_search.py` is the pruner.** Given a rank-`R`
CPD, pick `Q` with `Q A_0 = rref(A_0)`. Each row of an `n_0 x R` rref of rank
`n_0` has at most `R - n_0 + 1` nonzeros, so every slice of `Q ._0 T` has rank at
most that. Therefore **`n_0` linearly independent vectors `v` with
`rk(v ._0 T) <= R - n_0 + 1` must exist**, and when they do not, no rank-`R` CPD
does. It costs `|F|^{n_d}` matrix ranks per axis, which is 4 096 of them at the
widest shape here, and it is checked on all three rotations.

**`k_th_rref_prune0` in `other/k-th order rref pruning.ipynb` is a separate
experiment**, not reachable from the CLI: it contracts with `k`-tuples, wedges
them, and refuses when the good `k`-planes fail to span the exterior power. Its
cost is `C(|F|^{n_0}, k)` recursive searches. This repository's description of
`rref` was a description of that notebook, and `--pruners rref` does not call it.

## The measurement

Every fixture, both floors, the same question. Ours is
`rank_lower_bound`, printed by `test_rank_metric_bound fixtures`; theirs is the
largest `R` their `prune_rref` refutes, plus one. Counts are exact and carry no
timing, so they reproduce anywhere.

| fixture | ours | `rref` | | fixture | ours | `rref` |
|---|---|---|---|---|---|---|
| `cyclic_f2_5` | **9** | 8 | | `gf8_multiplication` | **6** | 5 |
| `cyclic_f2_7` | **12** | 9 | | `gf16_multiplication` | **8** | 7 |
| `f2_2x2` | 3 | 3 | | `gf32_multiplication` | **12** | 9 |
| `f2_2x3` | 5 | 5 | | `gf64_multiplication` | **14** | 11 |
| `f2_3x8` | **14** | 12 | | `matmul_2x2x2` | **6** | 5 |
| `f2_4x7` | **14** | 12 | | `matmul_2x2x3` | **9** | 7 |
| `f2_5x5` | **12** | 11 | | `matmul_2x3x4` | **14** | 13 |
| `f3_3x6` | 9 | 9 | | `matmul_3x3x3` | **14** | 11 |
| `gf4_multiplication` | 3 | 3 | | `matmul_3x3x4` | **18** | 14 |
| `w_state` | 3 | 3 | | `matmul_3x4x4` | **21** | 18 |

Four rows were re-taken with the author's own `prune_rref` rather than with the
reimplementation that produced the table, and agreed on all four: `f2_5x5`
refuted through 10 and not 11, `cyclic_f2_7` through 8, `f3_3x6` through 8, and
`matmul_2x2x2` refuted nothing and instead **returned the naive 8-term CPD**.

## Why it loses, and the one thing it does that we do not

The argument bounds the weight of an rref row and reads a rank off it. Griesmer
bounds the whole block code that the same decomposition induces, so it uses every
codeword where `rref` uses `n_0` of them. On `matmul_3x3x3` that is 14 against 11.

What it does that no bound here does is **construct**: when the chosen slice ranks
sum to at most `R` it emits a CPD rather than a verdict. That half is real and it
is also beaten, by the strand built for it. It returns 8 on `matmul_2x2x2` where
[`methods/bilinear_rank/greedy_heuristic/`](../../methods/bilinear_rank/greedy_heuristic/README.md) returns 7, and 12 on `f3_3x6` where the
descent returns 10.

## What this does not settle

It was measured **as a top-level floor only**. `[yang2025]` applies it at every
node to the conciseness-reduced residual, and their recursion has a residual
tensor where ours extends a subspace, so there is no node here to hand it without
first deciding what the residual is. That is a different question and this page
does not answer it.
