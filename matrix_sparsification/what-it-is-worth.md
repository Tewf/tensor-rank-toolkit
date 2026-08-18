# What fewer additions is worth

Fewer additions lowers the leading coefficient of the algorithm's arithmetic
complexity, not its exponent. The counts are in [`README.md`](README.md); this
page is what they mean and where the literature says the road ends.

Fewer additions lowers the leading coefficient of the algorithm's arithmetic
complexity, which is what decides whether a sub-cubic algorithm beats the
classical one at a size anyone runs. Strassen, every number checked rather than
quoted:

| | `q_u` | `q_v` | `q_w` | total | leading coefficient |
|---|---|---|---|---|---|
| As published | 5 | 5 | 8 | 18 | **7** |
| Sparsified here | 3 | 3 | 6 | 12 | **5** |

Seven is the 7 of `7·N^log₂7 − 6·N²`. Five is what `[karstadt2017]` reports for
the alternative-basis version, reached here from the fixtures.

**Those 12 additions are the end of the road for `⟨2,2,2⟩`, and the literature says
so.** In the standard basis 15 additions are *necessary* for any `⟨2,2,2;7⟩`
algorithm, over an arbitrary ring: `[probert1976]` and `[bshouty1995]`. That is a
bound over every rank-7 decomposition, not over one orbit, and Winograd's variant
attains it. Changing basis is what gets past it, and 12 is stated optimal there too
(`[martensson2026]`). So no search over this fixture can win, and anything that looks
like it has is a measurement error. For `⟨3,3,3⟩` at rank 23 the record is live and
currently 55 additions (`[karunaratne2026]`), against the 61 `[beniamini2020]`
reports.

Computing `q_w` needed Strassen's decoding operator, which is a fixture now, and
it is not asserted to be Strassen's: [`algorithm_check.h`](algorithm_check.h)
verifies the triple against the 2×2 product through the trilinear identity.

The third operator is a Strassen-like algorithm already written in an alternative
basis, so its entries are ninths. It is the case where floating point has
something to go wrong with, because no double holds 4/9.
