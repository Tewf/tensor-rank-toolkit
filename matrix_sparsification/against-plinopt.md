# Against `[plinopt]`

`[plinopt]` is the reference implementation. It reaches sparsity by a different
route, sparse QLUP elimination and a bounded coefficient search, and its search
is a heuristic bounded to four rows of support with a coefficient set of eleven,
which is why it can be beaten on the count.

What the two reach *together* is the other half and its own page:
[`in-front-of-plinopt.md`](in-front-of-plinopt.md).

## Proved minimal, on a published rank-23 `⟨3,3,3⟩` scheme

The operators of `[plinopt]`'s `3x3x3_23_Grey-221`. Its own `bin/sparsifier`
column is what that tool reached; the last column is the true minimum, and it is
the minimum because Rado-Edmonds says so, not because nothing better was found.

| operator | shape | as given | `[plinopt]` | fastest older method here | **minimum** |
|---|---|---|---|---|---|
| `L` | 23×9 | 69 | 59 | 43 in TOPDOWN_L | **43** in EXACT_L |
| `R` | 23×9 | 66 | 53 | 42 in TOPDOWN_R | **42** in EXACT_R |
| `P` | 9×23 | 86 | 55 | 43 in TOPDOWN_P | **43** in EXACT_P |
| total | | 221 | 167 | | **128** |

**The counts did not move and that is the finding.** All four older methods were
already reaching the minimum on these operators; none of them could say so, and
the cheapest took about a hundred times as long. Running all four takes about
500 s per operator. Nothing has separated the two oracles from the exact method
yet, on these three, on 400 random operators, or on 203 built the way a real one
is, a sparse basis hidden behind a change of basis; only the row-basis heuristic
was ever measured losing, on 37% and 21% of those two families.

Timings are one core, fastest of three, per [`../MEASURING.md`](../MEASURING.md).
The older column is the fastest *single* older method reaching that count.

**Where it stops is combinatorial.** On `4x4x4_49_156_L`, a 16-dimensional space
in `Q^49`, the greedy holds 9 of 16 vectors at weight 4 in 5 s and needs weight 6
or more for the rest, where the scan is 14 million column subsets rising to 451
million at weight 8. Thirty minutes was not enough, and a longer run would not
help: [`method/exact-over-q.md`](method/exact-over-q.md) names the algorithm that
would, and it is somebody else's.


## Where each column came from

**The two right-hand columns are runs of this repository, on this machine, under
[`../MEASURING.md`](../MEASURING.md).** Sparsity is trivial to improve by
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

**`P` needed care there and would have produced a false defeat.** Handed to
`[plinopt]` in its stored 9×23 orientation the sparsifier reports 9 nonzeros,
because it recombined the 23 products, which no change of basis on the outputs
may do; its own output says `(100 alt.)`. Transposed it gives 55.
