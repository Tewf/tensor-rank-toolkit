# The method, exactly

Notation: [`../../linear_algebra/README.md`](../../linear_algebra/README.md).
The cost of each primitive:
[`../../linear_algebra/costs.md`](../../linear_algebra/costs.md).
Here `U` is the operator, `r × c`, over `Q`. The search works on `Uᵀ`, written
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
| [`the-validator.md`](the-validator.md) | the sparse vector the rescaling greedy looks for, and when one exists |
| [`../dominated.md`](../dominated.md) | the three methods that reached the same answer more slowly, and where they went |
| [`exact-over-q.md`](exact-over-q.md) | **the one proved minimal**: the matroid greedy, scanning supports upwards |
| [`where-the-scan-stops.md`](where-the-scan-stops.md) | the operator it cannot finish, and the published algorithm that could |
| [`accelerations-not-built.md`](accelerations-not-built.md) | two that would have made the walk cheaper, priced and rejected |
| [`answering-without-searching.md`](answering-without-searching.md) | **a way past it that is not a search**: an LP, and the theorem that turned out not to apply |
| [`the-caveat.md`](the-caveat.md) | why these operation counts are not wall clock, and what is not proved |

On the three operators of the published `Grey-221` scheme, `exact-over-q.md`'s
method reaches 43, 42 and 43 nonzeros, the proved minimum over every invertible
`V` ([`../dominated.md`](../dominated.md)).
