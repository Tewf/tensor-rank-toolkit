# Two accelerations that were priced and not built

The wall is [`where-the-scan-stops.md`](where-the-scan-stops.md). Both of these
would have made the walk cheaper, and both were real candidates, but both were
rejected by measurement rather than by taste, which is why they are written down
here instead of quietly dropped.

**A covering-design restriction of the enumeration.** Partition the columns into
blocks and take one from each: any codeword lighter than the smallest block has a
zero in every block, so the family provably covers, with a two-line proof and
about **21x** fewer subsets at these shapes. It does not change a single outcome.
Where the scan already finishes it is under half a second, and on
`4x4x4_49_156_L` the worst level is `C(49,18)`, eleven and a half trillion
subsets; 21x leaves five hundred and fifty billion, which is not a rescue.

**Brouwer-Zimmermann's per-`(w, j)` refinement of the bound**, which
`[sanjose2025]`'s implementation omits and `[lisonek2016]` states. Same verdict
for the same reason: it prunes a walk that is either already fast enough or
hopeless.

**What made both pointless is
[`answering-without-searching.md`](answering-without-searching.md).** On every
operator large enough to time, the linear programme returns the same count as the
scan between four and fifteen times faster, and returns an answer at all on the
one the scan refuses; the Grey-221 and `4x4x4_49_156_L` rows are there. The same
margin holds on every other timed fixture:

| operator | search | simplex |
|---|---|---|
| `3x3x3_23_58_L` | 43 in 0.395 s | **43 in 0.025 s** |
| `3x3x3_23_55_L` | 38 in 0.088 s | **38 in 0.022 s** |
| `2x2x2_7_Strassen_L` 7×4 | **10 in 0.0002 s** | 10 in 0.0006 s |

The scan wins only on the 7×4 fixture above, where the programme's setup costs
more than the whole walk and both are under a millisecond.

**The scan is not obsolete, and that is the reason to keep it fast enough rather
than faster.** It is the only route here that *proves* its answer minimal; the
linear programme returns an upper bound that happens, on every operator tried, to
be that minimum. Speeding up a proof that already takes a third of a second buys
nothing, and speeding it up 21x does not reach the one case where it fails.
