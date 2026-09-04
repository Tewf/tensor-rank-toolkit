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
attains it. Changing basis is what gets past it, and 12 is proved optimal there
too, `[karstadt2017, Thm. 1.2]`. So no search over this fixture can win, and
anything that looks like it has is a measurement error.

## Three cost models, and this page is in one of them

**An addition count in this literature is one of three incomparable things**, and
this page compared two of them until 2026-08-22.

| | how additions are counted | basis change | who reports it |
|---|---|---|---|
| **(a) nnz, standard basis** | `nnz(U) − rows(U)` and so on, no reuse of intermediates | no | Strassen 18, Winograd 15 |
| **(b) nnz, alternative basis** | the same formula on a sparsified `⟨Uφ, Vψ, Wτ⟩`, the basis maps costing `O(n² log n)` | yes | `[karstadt2017]`, `[beniamini2020]`, and **everything on this page** |
| **(c) straight-line program with common subexpressions** | the length of the program, which can be **below** `nnz − rows` | no | `[plinopt]`, and the `⟨3,3,3⟩` record chain |

**This repository measures nnz, so it is in (a) and (b) and structurally cannot
see (c)** on its own. What it can do is hand its answer to something that does,
and [`measured-with-other-tools/before-a-subexpression-pass.md`](measured-with-other-tools/before-a-subexpression-pass.md)
measures that:
handed to `[plinopt]`'s subexpression pass the exact stage takes `Grey-221` from
81 additions to 62, and two already-optimised schemes by 2% and 7%. That is (b)
crossed with (c), a fourth thing again and not comparable to either record below.
It also shows the two models pulling apart: on `3x3x3_23_58_L` six more zeros
cost two additions. The gap is not small: `[plinopt]`'s own `3x3x3_23_55` operators carry
enough nonzeros to imply 122 additions by `nnz − rows`, and the straight-line
program it ships for them runs in **55**. A sparsification-only pipeline reports
roughly twice the literature's number for the same algorithm, and reporting them
side by side without saying which model each is in is the error this section
exists to stop repeating.

For `⟨3,3,3⟩` at rank 23, then, two records and they are not competing:
**61 linear operations in model (b)** (`[beniamini2020]` Table 2, reproduced
exactly by `[holtz2025]`), and **55 additions in model (c)**
(`[karunaratne2026]`). The 55 gives a leading coefficient near 4.93, under
`[holtz2025]`'s lower bound of 5, which is not a contradiction, because that
bound is stated in model (b) and a program with common subexpressions is not
bound by `nnz − rows`.

**That 55 strengthens the finding above rather than competing with it.** The three
`Grey-221` operators at their *proved* minimum carry 128 nonzeros over
`23 + 23 + 9 = 55` rows, so evaluating them the naive way costs `128 − 55 = 73`
additions, where `[karunaratne2026]`'s circuit for a rank-23 `⟨3,3,3⟩` tensor runs
in 55. That is the two models pulling apart one step further out: minimising
nonzeros as far as they go, with a proof that nothing lighter exists, still leaves
a count above a published straight-line program. It is not a scoreboard and nothing
here may be read against that record, for the reason
[`measured-with-other-tools/reading-the-program-length.md`](measured-with-other-tools/reading-the-program-length.md)
gives. What the 55 is made of is in [`../../references.md`](../../references.md).

Computing `q_w` needed Strassen's decoding operator, which is a fixture now, and
it is not asserted to be Strassen's: [`algorithm_check.h`](algorithm_check.h)
verifies the triple against the 2×2 product through the trilinear identity.

The third operator is a Strassen-like algorithm already written in an alternative
basis, so its entries are ninths. It is the case where floating point has
something to go wrong with, because no double holds 4/9.
