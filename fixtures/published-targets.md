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
