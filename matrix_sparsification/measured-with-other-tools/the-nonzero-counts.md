# The nonzero counts on a published rank-23 `⟨3,3,3⟩` scheme

The operators of `3x3x3_23_Grey-221`. The last column is the minimum, and it is
the minimum because Rado-Edmonds says so, not because nothing better was found.

| operator | shape | as given | `[plinopt]` | fastest older method here | **minimum** |
|---|---|---|---|---|---|
| `L` | 23×9 | 69 | 59 | 43 in 35.6 s | **43** in **0.338 s** |
| `R` | 23×9 | 66 | 53 | 42 in 27.6 s | **42** in **0.327 s** |
| `P` | 9×23 | 86 | 55 | 43 in 35.1 s | **43** in **0.434 s** |
| total | | 221 | 167 | | **128** |

**The counts did not move and that is the finding.** Every method then shipped
was already reaching the minimum on these operators, and the article proves one of
them optimal, which this repository had not noticed. Per operator the quickest of
them takes **86 to 112 times** as long as the method that stayed, and running all
five took about 500 s against about a third of a second. Three have since moved
to a branch: [`../dominated.md`](../dominated.md), which measures the gap the
other way round, slowest operator against slowest, and so quotes 88x to 343x.

**Every entry of that minimum is `0`, `+1` or `−1`.** Counted on the matrices the
scan prints: 30 ones and 13 minus-ones on `L`, 31 and 11 on `R`, 29 and 14 on `P`,
and nothing else anywhere. So `nns` is zero, `nnz + nns` is 128 as well, and the
128 is the cost the articles minimise and not only the one this module does.
**That is measured on the bases these runs return and not guaranteed by the
method**, which minimises zeros and breaks ties by the order it walks supports in:
another basis of the same weight could carry an entry that is neither, and on the
alternative-basis fixture a since-retired method left all ten of its entries as
ninths, twenty operations for ten nonzeros.

No operator ever separated the two oracles from the exact method on *count*: not
these three, not 400 random ones, not 203 built the way a real one is, a sparse
basis hidden behind a change of basis. Only the row-basis heuristic was ever
measured losing, on 37% and 21% of those two families. What separated them was
cost.

Timings are one core, fastest of three, per
[`../../MEASURING.md`](../../MEASURING.md).

**Where it stops is combinatorial.** On `4x4x4_49_156_L`, a 16-dimensional space
in `Q^49`, the greedy holds 9 of 16 vectors at weight 4 in 5 s and needs weight 6
or more for the rest, where the scan is 14 million column subsets rising to 451
million at weight 8. Thirty minutes was not enough when it still tried; it prices
the walk now and refuses in milliseconds. What answers that operator is
[`../method/answering-without-searching.md`](../method/answering-without-searching.md),
by not searching.

## Where each column came from

**The two right-hand columns are runs of this repository, on this machine, under
[`../../MEASURING.md`](../../MEASURING.md).** Sparsity is trivial to improve by
returning a different matrix, so the answers were re-checked outside the tool in
exact rational arithmetic: for each of the three, rank 9 before, rank 9 after,
and rank 9 when the two are stacked, so the space is the one it was given.

**The `[plinopt]` column is not, and saying so is the point.** Those three
numbers were measured on this machine on 2026-08-22 with `bin/sparsifier` built
from upstream, and the tool was given its best settings first: `-c 20` and `-c 40`
leave it where the default `-c 11` does. That binary is **not built in the
checkout this repository vendors**, which carries `bin/optimizer` and not
`bin/sparsifier`, so the column is carried from that measurement rather than
re-derived here, and anyone re-running it has to build the tool first. A carried
number said to be a fresh one is the failure this paragraph exists to prevent.

**`P` had to be handed over transposed.** It is 9×23 and computes 9 outputs from
23 products, so the admissible change of basis acts on its rows and the object is
its row space. In its stored 9×23 orientation the sparsifier reports 9 nonzeros,
because it recombined the 23 products, which no change of basis on the outputs
may do; its own output says `(100 alt.)`. Transposed it gives the 55 above. Read
the wrong way it is a number about a different object, which is the reading a
reader is least likely to check.

Over GF(2) and GF(3) `bin/sparsifier` returns the zero matrix and exits 0, so on
the finite fields there is no column to record. The matroid greedy runs over
`q^k` and stays exact there, which is what every operator
[the rank search](../../methods/bilinear_rank/descent_search/README.md) emits is over.

What these nonzeros are worth once a subexpression pass sees them:
[`before-a-subexpression-pass.md`](before-a-subexpression-pass.md).
