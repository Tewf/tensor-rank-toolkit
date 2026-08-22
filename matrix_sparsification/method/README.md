# The method, exactly

Notation: [`../../linear_algebra/README.md`](../../linear_algebra/README.md).
The cost of each primitive:
[`../../linear_algebra/costs.md`](../../linear_algebra/costs.md).
Here `U` is the operator, `r × c`, over `Q`. The oracles work on `Uᵀ`, written
`a × b` with `a = c` and `b = r`, because on that side the question becomes
"replace a row by a sparser vector spanning the same space".

## The problem

> Given `U ∈ Q^{r×c}`, find invertible `V ∈ Q^{c×c}` minimising `nnz(U·V)`.

`V` invertible is what makes it the same operator: `U·V` computes the same
algorithm in a different basis, and fewer nonzeros means fewer additions. Every
result is checked with `same_row_space(Uᵀ, (U·V)ᵀ)`, because sparsity is
trivial to improve by returning a different matrix.

| | |
|---|---|
| [`row-basis-heuristic.md`](row-basis-heuristic.md) | invert a square block of rows, which forces `c` of them to singletons |
| [`the-validator.md`](the-validator.md) | the sparse vector both oracles look for, and when one exists |
| [`oracle-bottom-up.md`](oracle-bottom-up.md) | column subsets one smaller than the row count, keeping the emptiest vector |
| [`oracle-top-down.md`](oracle-top-down.md) | the same subsets walked downwards, taking the first hit |
| [`exact-over-q.md`](exact-over-q.md) | **the one proved minimal**: the matroid greedy, scanning supports upwards |
| [`where-the-scan-stops.md`](where-the-scan-stops.md) | the operator it cannot finish, and the published algorithm that could |
| [`when-the-matroid-is-regular.md`](when-the-matroid-is-regular.md) | **a way past it that is not a search**: an LP, on exactly the operator that needed one |
| [`the-caveat.md`](the-caveat.md) | why these operation counts are not wall clock, and what is not proved |
