# The tensors the literature argues about, run through both searches

The fixtures here are polynomial multiplication, because that is what the
polynomial fixtures cover. The code is not: a bilinear map `F^n × F^m → F^d` is an
order-3 tensor, so every tensor the complexity literature cares about fits the
same file format and the same two searches. This is what happened when they were
fed the famous ones, over GF(2), on one core of an i5-12450H.

Build them with `make-tensor --matmul p n m k` and `--cyclic p n`, `p` being the
field characteristic (2 throughout this table).

## What each search did

| Tensor | Naive | Strict descent | Plateau walk | Exact says | Known |
|---|---|---|---|---|---|
| `⟨2,2,2⟩` matrix multiplication | 8 | **8**, no first step | **7** · 0.11 s | **exactly 7** | 7 |
| `⟨2,2,3⟩` | 12 | **12**, no first step | not measured | **≥ 9** | 11 |
| `⟨2,3,3⟩` | 18 | **18**, no first step | not measured | nothing reachable | 15 |
| `⟨3,3,3⟩` | 27 | **27**, no first step | **24** · 38.1 s | out of reach | open, 19–23 |
| W state | 3 | 3 | | **exactly 3** | 3, border rank 2 |
| Cyclic convolution, length 5 | 25 | **10** | | **≥ 9** | |
| GF(16) over GF(2) | 16 | **9** | 9 · 0.69 s | **exactly 9** | ≥ 8 by de Groote |

"No first step" is the step 3 shortlist coming back empty, and it stops
`minimise-rank` and nothing else. The plateau column is the same heuristic
strand allowed to cross equal-cost maps, and on both matrix multiplication
shapes it was run on it improves on the descent. Its three timings are
`walk-scheme`, measured in
[`../oracle_guided_search/measurements/README.md`](../../methods/bilinear_rank/canonical_augmentation/measurements/README.md);
`⟨3,3,3⟩`'s 24 is `--flips 20000 --seeds 8`.

## A second family: polynomial multiplication, checked against a 2012 paper

This repository also ships four polynomial and field multiplication maps that
Barbulescu, Detrey, Estibals and Zimmermann already searched exhaustively in
*Finding Optimal Formulae for Bilinear Maps*, WAIFI 2012,
[hal-00640165v2](https://inria.hal.science/hal-00640165v2), Tables 1 and 2,
whose Algorithm 1 is exactly
[`expand_subspace`](../../methods/bilinear_rank/exhaustive/exhaustive_search.h). The
identification is not a guess: the generator-set sizes `#G` in their tables are
**961, 1785, 1905 and 4732**, exactly the pool sizes
[`all_rank_one_maps`](../../methods/bilinear_rank/candidate_pool.h) builds for these four
fixtures.

| Fixture | Their row | This repository claims | They report |
|---|---|---|---|
| `f2_5x5` | F2 5×5, `#G` 961 | **rank = 13**, both sides | **rank = 13**, 27 solution subspaces, 27 formulae, 9.65·10⁹ tests, 2.28·10⁵ s |
| `f3_3x6` | F3 6×3, `#G` 4732 | **rank = 10**, both sides, 25 s | **rank = 10**, 240 solutions, 4272 formulae, 566 s |
| `f2_3x8` | F2 8×3, `#G` 1785 | `rank ≤ 15` (step 3) | no solution at `k` = 14, 5.27·10¹⁰ tests |
| `f2_4x7` | F2 7×4, `#G` 1905 | `rank ≤ 16` (step 3) | no solution at `k` = 14, 1.47·10¹¹ tests |

Their stated convention for the `k` column: it is the smallest `k` for which
solutions were found, in which case *there are none smaller*, or, when none
were found, the largest `k` attempted. The `f2_5x5` and `f3_3x6` rows carry
solution counts, formula counts and timings, so they are complete runs and the
two ranks are safe to state; the `f2_3x8` and `f2_4x7` rows rest on the
convention alone, and the `8×3` row carries no timing, so verify either
against the paper before quoting it as a bound.

`f2_5x5` and `f3_3x6` are decided outright here, in
[`decided-exactly.md`](decided-exactly.md). `f2_3x8` and `f2_4x7` stay
bounded, in [`where-the-exact-search-stops.md`](where-the-exact-search-stops.md).

## Then one file per reading of that table

| | |
|---|---|
| [`decided-exactly.md`](decided-exactly.md) | the tensors this repository settles outright: `⟨2,2,2⟩`, GF(16), `f2_5x5` and `f3_3x6` |
| [`where-the-descent-stops.md`](where-the-descent-stops.md) | why no single rank-one map improves a matrix multiplication tensor, and what that does and does not bound |
| [`where-the-exact-search-stops.md`](where-the-exact-search-stops.md) | the price of every run made and of the ones refused, node by node, and the two polynomial-multiplication fixtures still bounded rather than settled |

For the state of the art on exactly these families, including matrix
multiplication over F₂ and cyclic convolution over F₂ and F₃, see `[wang2026]`,
*Automated Lower Bounds for Bilinear Complexity over Finite Fields*, which
raised the F₂ lower bound for `⟨3,3,3⟩` from 19 to 20 using orbit classification
under a symmetry group. That is the direction this search would have to go to
reach the sizes it currently cannot.
