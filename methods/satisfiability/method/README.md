# The method, exactly

`p` is the characteristic, `r` the number of products asked for, and the tensor
is `n₁ × n₂ × n₃`: rows, columns, and slices.

## The problem, as a formula

> Are there vectors `a⁽ˡ⁾ ∈ F^{n₁}`, `b⁽ˡ⁾ ∈ F^{n₂}`, `c⁽ˡ⁾ ∈ F^{n₃}`, for
> `l < r`, with `t[i][j][k] = Σ_l a⁽ˡ⁾[i]·b⁽ˡ⁾[j]·c⁽ˡ⁾[k]` for every `i, j, k`?

`r(n₁+n₂+n₃)` unknowns and `n₁n₂n₃` equations. Everything below is that,
written for a different solver.

| | |
|---|---|
| [`gf2-as-cnf.md`](gf2-as-cnf.md) | the GF(2) encoding, its two Tseitin stages, and the size it comes to |
| [`gf-p-one-hot.md`](gf-p-one-hot.md) | `p` Booleans per unknown, with the field's tables written out as implications |
| [`gf-p-smt.md`](gf-p-smt.md) | the same equations handed to cvc5's theory of finite fields, with no encoding at all |
| [`hastad-reduction.md`](hastad-reduction.md) | the arrow in the other direction, and why nobody travels it |

What any of it costs: [`../measurements.md`](../measurements.md). Which backend,
solver and flags it runs with, and why:
[`../choices/README.md`](../choices/).

For scale: `⟨3,3,3⟩` at Laderman's rank of 23 already comes to 19 251 variables
and 56 619 clauses under the GF(2) encoding, a CNF file of about a megabyte
(measured in [`gf2-as-cnf.md`](gf2-as-cnf.md)).
