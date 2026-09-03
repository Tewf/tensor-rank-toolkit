# The same operators, measured with the other tools in this area

`[plinopt]` is the near neighbour here and it reaches sparsity by a different
route, sparse QLUP elimination and a coefficient search bounded to four rows of
support with a set of eleven coefficients. Its published operators are inputs
this repository reads, and its subexpression pass is the only instrument on this
machine that can price model (c). Keys are
[`../../references.md`](../../references.md).

These pages record what those runs measured, so that a number quoted anywhere
else has a page saying where it came from. They are a record and not a
scoreboard: what this repository promises is the `nnz` column, and it promises it
as a minimum over every change of basis. The three cost models a count like this
can be read in are [`../what-it-is-worth.md`](../what-it-is-worth.md).

On `Grey-221`, that promise is 128 nonzeros total; handed to `[plinopt]`'s own
subexpression pass those come down to 62 additions, the pair
[`before-a-subexpression-pass.md`](before-a-subexpression-pass.md) measures.

| | |
|---|---|
| [`the-nonzero-counts.md`](the-nonzero-counts.md) | what each tool reached on the three operators of `Grey-221`, column by column |
| [`before-a-subexpression-pass.md`](before-a-subexpression-pass.md) | what the exact stage is worth to a pass that finds common subexpressions |
| [`reading-the-program-length.md`](reading-the-program-length.md) | the comparison that the resulting program length does not license |
