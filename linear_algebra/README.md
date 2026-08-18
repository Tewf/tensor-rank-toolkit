# The exact layer

Everything both strands stand on: a dense matrix, a basis kept in reduced row
echelon form, and the operations that go with them. Templated on the field, so
one implementation serves the rank search over `GF(p)` and the sparsification
over `Q`.

| File | Role |
|---|---|
| [`matrix.h`](matrix.h) | Dense row-major matrix, templated on the element type |
| [`field.h`](field.h) | The two fields: `ModularField` = `GF(p)`, `RationalField` = `Q` |
| [`span_basis.h`](span_basis.h) | A basis in reduced row echelon form, built one vector at a time, and the span of a set of slices |
| [`measures.h`](measures.h) | What a thing costs: rank, multiplications, nonzeros |
| [`span_queries.h`](span_queries.h) | What a span contains: `spans_all`, `raises_rank`, `same_row_space` |
| [`solver.h`](solver.h) | Exact solve in a row space, and the inverse built on it |
| [`matrix_ops.h`](matrix_ops.h) | Transpose, product, row and column selection |
| [`decomposition.h`](decomposition.h) | A matrix as a sum of rank-one matrices |
| [`tensor_flattening.h`](tensor_flattening.h) | The three flattenings of a tensor, the rank lower bound `max_d rank(T⁽ᵈ⁾)`, and conciseness |
| [`tensor_contraction.h`](tensor_contraction.h) | Collapsing one axis of a tensor by a vector, which the bounds below are built from |
| [`tensor_rank_sum.h`](tensor_rank_sum.h) | Two rank-sum lower bounds out of one table of contraction ranks: over one affine line, and over every vector |
| [`rank_lower_bound.h`](rank_lower_bound.h) | The largest bound the three methods give, which is what callers wire in |
| [`linear_algebra.h`](linear_algebra.h) | An umbrella including all of the above, and no code of its own |

One file per role, because the umbrella used to be the layer: twelve functions
over five roles in one header, which is what the folder was reorganised to stop
happening elsewhere. Include the part you need; the umbrella is for callers who
want the layer as a whole.

Reading and writing files is [`../formats/`](../formats/), which depends on this
and not the other way round.

> **On the name.** This directory was called `exact/`, for exact arithmetic.
> That collided once the searches were filed by whether they are *exact methods*
> or heuristics, which is a different sense of the word. Exactness of the
> arithmetic is a property of everything here and needs no folder of its own.

## Notation

`p` the characteristic · `r × c` a matrix's shape · `d` the dimension of the
span in play, always `d ≤ min(r, c)` · `k` slices of shape `n × m` · `w = n·m`,
the width a slice occupies when flattened.

## What each operation costs

The complexity table, the rank sums' place in it, and the caveat that not
every field operation is constant time: [`costs.md`](costs.md).
