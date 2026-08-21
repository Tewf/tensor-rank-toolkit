# The six fixtures that ship for a published number

`gf4`, `gf8` and `gf16` are settled here by this repository's own exhaustive
search, so their rank is a measurement and not a citation. These six are the
other kind: each ships because somebody has published a number for it and
nothing here can reach that number yet, so what the file buys is the tensor the
published claim is about, held still and in full.

Every count below is asserted in
[`test_fixtures.cpp`](../linear_algebra/tests/test_fixtures.cpp), under the same
rule [`README.md`](README.md) states for its own: the expected counts live here
as well as in the test code, so a test that disagrees with this table is a
visible disagreement rather than a silent one.

| Fixture | Built by | Shape | Naive | Published target |
|---|---|---|---|---|
| `gf32_multiplication` | `--field 2 1 0 0 1 0 1` | 5 5 5 | 25 | `mu_2(5) = 13` |
| `gf64_multiplication` | `--field 2 1 0 0 0 0 1 1` | 6 6 6 | 36 | `mu_2(6) = 15` |
| `cyclic_f2_7` | `--cyclic 2 7` | 7 7 7 | 49 | rank 13, both sides |
| `matmul_2x3x4` | `--matmul 2 2 3 4` | 8 6 12 | 24 | 19 ≤ rank ≤ 20 |
| `matmul_3x3x4` | `--matmul 2 3 3 4` | 12 9 12 | 36 | 25 ≤ rank ≤ 29 |
| `matmul_3x4x4` | `--matmul 2 3 4 4` | 12 12 16 | 48 | 29 ≤ rank ≤ 38 |

## What this repository gets on them

**These shipped as bytes for six weeks with nothing ever run on them.** A fixture
that is only checked for its shape is a file, not a benchmark, so here is what
both searches actually say. Counts are exact and reproduce anywhere.

| Fixture | floor, proved | step 1 | step 2 | step 3 | shortlist | incumbent | gap to target |
|---|---|---|---|---|---|---|---|
| `gf32_multiplication` | **12** | 25 | 17 | 16 | 9 | **13** | floor 1 under, **ceiling met** |
| `gf64_multiplication` | **14** | 36 | 23 | **20** | 15 | 20 | floor 1 under, ceiling 5 over |
| `cyclic_f2_7` | **12** | 19 | 15 | 15 | 0 | **13** | floor 1 under, **ceiling met** |
| `matmul_2x3x4` | **14** | 24 | 24 | not run | | not run | floor 5 under |
| `matmul_3x3x4` | **18** | 36 | 36 | not run | | not run | floor 7 under |
| `matmul_3x4x4` | **21** | 48 | 48 | not run | | not run | floor 8 under |

The `incumbent` column is
[`lower-the-bound`](../incumbent_search/README.md), which walks the same tree the
exact search does and cuts it at `dim V + 1 >= best` instead of at a target. It
moves the two rows the descent cannot: **`cyclic_f2_7` reaches the published 13
in 22 nodes**, and `gf32_multiplication` reaches 13 in 1 873. Both counts are
verified in the tool and again outside this repository, from the emitted `.sms`
operators. Node counts per fixture:
[`../incumbent_search/what-it-reaches.md`](../incumbent_search/what-it-reaches.md).

**The floor is one product short of the published rank on all three that the
descent can finish**, from `rank_lower_bound` alone and in under a millisecond.
Closing any of them means refuting one more product by exhaustion, which is the
same question `f2_5x5` at 12 answered in 146 402 553 nodes. **`cyclic_f2_7` is
the one where that is now the only thing left**: 12 from below and 13 exhibited
from above, so a refutation at 12 would settle it. Nothing here has run one.

**Step 3 is not run on the three matrix multiplication rows** because their pools
are 257 985, 2 092 545 and 268 365 825 rank-one maps. It would also find nothing:
`cyclic_f2_7` has a shortlist of **0** out of 16 129 and still shows the pattern
this repository records for every matrix multiplication tensor, which is that the
descent cannot take a first step on them. Steps 1 and 2 move none of the three.

Timings are omitted deliberately. The counts above were taken while the machine
was running a browser at load 5.8, which [`../MEASURING.md`](../MEASURING.md)
refuses for a published second and does not care about for an exact count.

Shape is `slices rows cols`, the file's own header line, and every one is over
GF(2). "Built by" is the tail of a `build/map_construction/make-tensor` call,
which writes the file on standard output; each file's first two comment lines are
that tool's, kept as it stamped them.

## Which polynomial, and how that was checked

There is more than one irreducible polynomial of each degree and the tensor
depends on which, so the file's third comment line names it beside the two lines
`make-tensor` stamps: `x^5 + x^2 + 1` for GF(32) and `x^6 + x + 1` for GF(64).

Both were checked rather than copied from a table. Irreducible, by trial
division against every polynomial of degree at most half their own, which is
exhaustive at these degrees; and primitive, `x` having multiplicative order
exactly `2^n - 1` in the quotient, checked against every prime factor of that
order. `field_multiplication_tensor` refuses a reducible modulus outright
through Givaro, so the construction is a second and independent test of the
first property.

## Where each target comes from

**The two field extensions** extend a family this repository is already measured
on. `mu_2(n)` is the bilinear complexity of `F_{2^n}` over `F_2`, and the values
published for `n` up to 6 are 3, 6, 9, 13, 15. The first three of those are
`gf4`, `gf8` and `gf16`, and each is settled here by
[`../exhaustive_search/`](../exhaustive_search/) rather than taken on trust.

**13 and 15 are the two numbers on this page with no key beside them**, and that
is deliberate rather than an omission to fill in later. They are widely quoted,
they are not settled here, and no table stating them was read while these
fixtures were built, so they stand as targets and appear nowhere as a rank this
repository holds. `rank_metric_bound`'s soundness column carries 25 and 36 for
these two, the naive algorithm, for the same reason. Anyone who traces them to a
paper should add the key to [`../references.md`](../references.md) and change
both places at once.

**The other four are `[wang2026]`, and the version has to be cited with them**,
because the table grew across that preprint's ten revisions: v1 carries the
matrix multiplication formats alone, `⟨3,3,4⟩` and `⟨3,4,4⟩` are not in it, and
the polynomial and cyclic convolution bounds arrive later still. The numbers
above are v10, of 30 July 2026, which gives `⟨2,3,4⟩ ≥ 19`, `⟨3,3,4⟩ ≥ 25`,
`⟨3,4,4⟩ ≥ 29` and length-7 cyclic convolution `≥ 13` over F₂, against best known
upper bounds of 20, 29, 38 and 13. Key and full entry in
[`../references.md`](../references.md).

**`cyclic_f2_7` is therefore the one that is closed**: 13 from both sides, so
the length-7 cyclic convolution over `F2` has rank exactly 13. Its neighbour
`cyclic_f2_5` has shipped since the beginning and is bracketed rather than
settled, which is what makes the pair worth having.

## What is not claimed here

No search here reaches any of these six targets, and none of them is in the
descent table [`README.md`](README.md) publishes. The bounds do run on all six,
pinned and priced in
[`../rank_metric_bound/`](../rank_metric_bound/what-each-is-worth.md), and they
land far below: that gap is what these fixtures exist to make visible.
