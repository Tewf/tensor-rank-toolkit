# What the literature already decides, for these exact fixtures

Read this before spending seven hours on `--target 12`. Barbulescu, Detrey,
Estibals and Zimmermann ran the same algorithm this repository implements, their
Algorithm 1 being exactly
[`expand_subspace`](../exhaustive_search/exhaustive_search.h), on the same four
maps, in 2012.

Source: *Finding Optimal Formulae for Bilinear Maps*, WAIFI 2012,
[hal-00640165v2](https://inria.hal.science/hal-00640165v2), Tables 1 and 2.

The identification is not a guess: the generator-set sizes `#G` in their tables
are **961, 1785, 1905 and 4732**, which are exactly the pool sizes
[`all_rank_one_maps`](candidate_pool.h) builds for our four fixtures.

| Fixture | Their row | This repository claims | They report |
|---|---|---|---|
| `f2_5x5` | F2 5×5, `#G` 961 | `12 ≤ rank ≤ 14` | **rank = 13**, 27 solution subspaces, 27 formulae, 9.65·10⁹ tests, 2.28·10⁵ s |
| `f3_3x6` | F3 6×3, `#G` 4732 | **rank = 10**, both sides, 25 s | **rank = 10**, 240 solutions, 4272 formulae, 566 s |
| `f2_3x8` | F2 8×3, `#G` 1785 | `rank ≤ 15` (step 3) | no solution at `k` = 14, 5.27·10¹⁰ tests |
| `f2_4x7` | F2 7×4, `#G` 1905 | `rank ≤ 16` (step 3) | no solution at `k` = 14, 1.47·10¹¹ tests |

Their stated convention for the `k` column: it is the smallest `k` for which
solutions were found, in which case *there are none smaller*; or, when none were
found, the largest `k` attempted.

## What that does to the four numbers

**`f3_3x6`: the heuristic is optimal, and this repository now proves it alone.**
Step 3 returns 10 in 17.28 s, and `decide-rank --target 9` returns NO
exhaustively in 7.65 s over 4729 nodes. Two sides, 25 seconds, no citation
needed: `rank(f3_3x6) = 10`. Their 566-second run agrees, and is no longer what
the claim rests on.

**Why that took until 2026-08-17 to notice.** The refutation was never run.
Everything here asked `--target 10`, which is a *find*, so the answer arrived as
a decomposition and the rank stayed a citation. Asking `--target 9`, a
*refutation*, is both the cheaper question and the one that settles it. That
asymmetry is the whole reason `decide-rank` takes a target at all, and it was
being used the expensive way round.

**`f2_3x8`: the heuristic is optimal too**, if their `k` = 14 row means what the
convention says. No 14 exists, step 3 reaches 15, so the rank is 15.

**`f2_5x5`: the heuristic is one off, and `12 ≤ rank ≤ 14` is two-sided loose.**
The rank is 13. Deciding `--target 12` would *reproduce* their exclusion rather
than settle anything open, which is worth doing as a check and not as a result.
Finding a 13 is the part the heuristic misses.

**`f2_4x7` is the one still open**: `15 ≤ rank ≤ 16`, their lower bound against
our upper one. Closing it means deciding 15, which neither side has done.

## Two cautions on the rows above

The `f2_3x8` and `f2_4x7` readings rest on the convention rather than on
prose naming those maps, and the `8 × 3` row carries no timing, which is
consistent with an estimate rather than a completed run. Verify against the
paper before either is quoted as a bound. The `f2_5x5` and `f3_3x6` rows carry
solution counts, formula counts and timings, so they are complete runs and the
two ranks above are safe to state.

Nothing here diminishes the heuristic. It reaches the true rank on two of four
maps in seconds, against runs the authors measured in core-days, and it never
had a way to know it had arrived. Comparing against a proven optimum is exactly
what [`results.json`](results.json) has been missing.
