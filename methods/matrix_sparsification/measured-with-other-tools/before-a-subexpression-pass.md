# The exact stage before a subexpression pass

[`the-nonzero-counts.md`](the-nonzero-counts.md) counts nonzeros. `bin/optimizer`
does the thing this repository structurally cannot,
[model (c)](../what-it-is-worth.md#three-cost-models-and-this-page-is-in-one-of-them):
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

What the right-hand column may and may not be read against:
[`reading-the-program-length.md`](reading-the-program-length.md).

## Two things that make a single run of this not a number

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
