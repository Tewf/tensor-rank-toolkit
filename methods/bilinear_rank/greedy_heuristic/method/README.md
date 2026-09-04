# The method, exactly

Notation: [`../../../../core/linear_algebra/README.md`](../../../../core/linear_algebra/). The
cost of each primitive:
[`../../../../core/linear_algebra/costs.md`](../../../../core/linear_algebra/costs.md). Here `p` is the
characteristic, `k` the number of slices, `n × m` their shape, `w = n·m`, and `d`
the dimension of the span.

## The problem

Given a bilinear map `T = (T₁, …, T_k)`, slices of shape `n × m` over `GF(p)`,
find `S = (S₁, …, S_j)` with

> `span(S) ⊇ span(T)`, minimising `Σᵢ rank(Sᵢ)`.

`Σ rank(Sᵢ)` is the number of multiplications, because a rank-one bilinear form
is exactly one product. **`j` may exceed `k`**: `S` only has to *generate* `T`,
so a larger spanning set of lower total rank is a better answer. That is what
Karatsuba's five products for a four-coefficient product is.

## Two methods answer two questions

| | |
|---|---|
| [`descent.md`](descent.md) | the three steps of the heuristic, with the pseudocode and the cost of each |
| [`descent-cost.md`](descent-cost.md) | why the descent's wall is memory rather than time |
| [`../../../../writeup/how-the-search-works/README.md`](../../../../writeup/how-the-search-works/) | the complete decision procedure: is there an algorithm with exactly `k` products, and what it costs |

## One number this strand measures

`minimise-rank` takes the naive 25 multiplications of `evidence/fixtures/f2_5x5.tensor`
down to 14, [`../results.json`](../results.json). A real run of it, command
and printed output: [`what-it-reaches.md`](../what-it-reaches.md).
