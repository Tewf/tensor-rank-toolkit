# In front of `[plinopt]`: the exact stage before its subexpression pass

The count comparison is [`against-plinopt.md`](against-plinopt.md); this page is
the other half. `bin/optimizer` does the thing this repository structurally
cannot,
[model (c)](what-it-is-worth.md#three-cost-models-and-this-page-is-in-one-of-them):
it finds common subexpressions, so its programs run below `nnz − rows`. Does the
exact stage in front of it beat its own pipeline?

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

It holds on all three operators and not only on `L`. **On two schemes that
already carry a published straight-line program the margin collapses, and on one
operator it reverses.** `3x3x3_23_58` is from arXiv 2512.21980 and
`3x3x3_23_55` is the one whose shipped program runs in 55 additions.

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
both sides, since giving ours more would be the easiest way to fake this.

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
