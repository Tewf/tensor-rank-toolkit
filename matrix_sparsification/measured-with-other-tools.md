# The same operators, measured with the other tools in this area

`[plinopt]` is the near neighbour here and it reaches sparsity by a different
route, sparse QLUP elimination and a coefficient search bounded to four rows of
support with a set of eleven coefficients. Its published operators are inputs
this repository reads, and its subexpression pass is the only instrument on this
machine that can price model (c). This page records what those runs measured, so
that a number quoted anywhere else has a page saying where it came from. It is a
record and not a scoreboard: what this repository promises is the `nnz` column,
and it promises it as a minimum.

## The counts on a published rank-23 `⟨3,3,3⟩` scheme

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
to a branch: [`dominated.md`](dominated.md), which measures the gap the other way
round, slowest operator against slowest, and so quotes 88x to 343x.

No operator ever separated the two oracles from the exact method on *count* — not
these three, not 400 random ones, not 203 built the way a real one is, a sparse
basis hidden behind a change of basis. Only the row-basis heuristic was ever
measured losing, on 37% and 21% of those two families. What separated them was
cost.

Timings are one core, fastest of three, per [`../MEASURING.md`](../MEASURING.md).

**Where it stops is combinatorial.** On `4x4x4_49_156_L`, a 16-dimensional space
in `Q^49`, the greedy holds 9 of 16 vectors at weight 4 in 5 s and needs weight 6
or more for the rest, where the scan is 14 million column subsets rising to 451
million at weight 8. Thirty minutes was not enough when it still tried; it prices
the walk now and refuses in milliseconds. What answers that operator is
[`method/answering-without-searching.md`](method/answering-without-searching.md),
by not searching.

### Where each column came from

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
[the rank search](../descent_search/README.md) emits is over.

## The exact stage before a subexpression pass

The table above counts nonzeros. `bin/optimizer` does the thing this repository
structurally cannot,
[model (c)](what-it-is-worth.md#three-cost-models-and-this-page-is-in-one-of-them):
it finds common subexpressions, so its programs run below `nnz − rows`. Handing
it the output of the exact stage measures the two models crossed.

Every operator is taken the way up it is run. `P` is 9×23 and computes 9 outputs
from 23 products; handing `bin/optimizer` its transpose measures a different
program with a different count.

| operator | as published | | | after the exact stage | | |
|---|---|---|---|---|---|---|
| | nnz | naive | **CSE** | nnz | naive | **CSE** |
| `L` 23×9 | 69 | 46 | **21** | 43 | 20 | **18** |
| `R` 23×9 | 66 | 43 | **20** | 42 | 19 | **15** |
| `P` 9×23 | 86 | 77 | **40** | 43 | 34 | **29** |
| total | 221 | 166 | **81** | 128 | 73 | **62** |

The program gets shorter on all three operators and not only on `L`. **On two
schemes that already carry a published straight-line program the margin
collapses, and on one operator it reverses.** `3x3x3_23_58` is from arXiv
2512.21980 and `3x3x3_23_55` is the one whose shipped program runs in 55
additions.

| scheme | as published | | after the exact stage | | |
|---|---|---|---|---|---|
| | nnz | **CSE** | nnz | **CSE** | |
| `Grey-221` | 221 | **81** | 128 | **62** | −23% |
| `3x3x3_23_58` | 175 | **57** | 119 | **56** | −2% |
| `3x3x3_23_55` | 177 | **56** | 117 | **52** | −7% |

Per operator, eight of nine improve and one gets worse: `3x3x3_23_58_L` arrives
with 49 nonzeros, the exact stage takes it to 43, and the subexpression pass then
finds **18** additions where it found **16** on the original.

**So minimising nonzeros is not minimising additions.** A basis of least weight
is not a basis with the most shared subexpressions. Fewer nonzeros is a *proxy*
for fewer additions, and these three rows measure how good a proxy: 23% on a raw
scheme, 2% and 7% on schemes somebody has already worked over, and on one
operator the wrong sign.

**One caution on counting the evidence.** `3x3x3_23_58_R` and `3x3x3_23_55_L`
have identical row-weight profiles and identical numbers at every stage here. The
files are not byte-identical and neither is a row permutation of the other, but
they are almost certainly one operator relabelled, so that pair is one data point
and not two.

### Two things that make a single run of this not a number

**`bin/optimizer` is a randomized search and takes no seed.** Its `-O` flag sets
the loop count, 100 by default. Four consecutive default runs on one unchanged
file gave 22, 22, 22, 21, and raising the effort changes the answer too: on
`Grey-221_L` the default reaches 21-22 where `-O 10000` reaches 20-21. Every
column above is `-O 10000`, five runs a side, best of each. The same effort on
both sides, since giving the right-hand side more would be the easiest way to
fake this.

**A minimum-weight basis is not unique.** The count is the minimum and that is
proved; *which* basis of that weight comes back depends on the order the scan
walks its subsets, and different ones give the downstream pass different work. An
earlier run with a different 43-nonzero basis of the same operator reached 17 on
`Grey-221_L` where this one reaches 18. **A number from this table is a number
about one basis, not about the method**, which promises only the nnz column.

### What the 62 is, and what it is not

The right-hand column is an **alternative-basis** count, model (b) crossed with
model (c). Three changes of basis, one per operator, are not charged in it: they
are what the sparsification produced and they are charged separately in the
alternative-basis accounting of `[karstadt2017]` and `[beniamini2020]`, where
they cost `O(n² log n)` per recursion level and so do not touch the leading
coefficient. The left-hand column charges nothing because it changes no basis.

**None of these may be read against the 55 of `[karunaratne2026]`.** That record
is model (c) in the *standard* basis, a straight-line program that makes no basis
change at all and therefore has nothing uncharged. The two count different
things, which is the error [`what-it-is-worth.md`](what-it-is-worth.md) exists to
stop repeating.

The temptation is concrete and worth naming, because a reader will do the
arithmetic. On the record scheme itself the table above says **52**, and the
program `[plinopt]` ships for it runs in **55**. That is not a record and this
page does not claim one: the 52 leaves three 9×9 changes of basis uncharged and
the 55 has none to charge, and nothing here computes the leading coefficient of
an alternative-basis algorithm with common subexpressions, which is the quantity
the two would have to be compared through. What the 56 in the same row does say
is that `bin/optimizer` at `-O 10000` roughly reproduces its own shipped record,
one addition above it, which is the check that the left-hand column is being
measured properly at all.
