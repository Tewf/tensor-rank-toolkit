# The method, exactly

`p` is the characteristic, `r` the number of products asked for, and the tensor
is `n₁ × n₂ × n₃`: rows, columns, and slices.

## The problem, as a formula

> Are there vectors `a⁽ˡ⁾ ∈ F^{n₁}`, `b⁽ˡ⁾ ∈ F^{n₂}`, `c⁽ˡ⁾ ∈ F^{n₃}`, for
> `l < r`, with `t[i][j][k] = Σ_l a⁽ˡ⁾[i]·b⁽ˡ⁾[j]·c⁽ˡ⁾[k]` for every `i, j, k`?

`r(n₁+n₂+n₃)` unknowns and `n₁n₂n₃` equations. Everything below is that,
written for a different solver.

## GF(2), as CNF

```
q[l][i][j]    ↔ a[l][i] ∧ b[l][j]        3 clauses
p[l][i][j][k] ↔ q[l][i][j] ∧ c[l][k]     3 clauses
XOR_l p[l][i][j][k] = t[i][j][k]         1 parity constraint
```

| | |
|---|---|
| Variables | `r(n₁+n₂+n₃)` + `r·n₁n₂` + `r·n₁n₂n₃` |
| Clauses | `3r·n₁n₂` + `3r·n₁n₂n₃`, plus `n₁n₂n₃` parities of width `r` |

The two stages are the only optimisation and they matter: `q` does not depend on
`k`, so sharing it saves `r·n₁n₂(n₃−1)` conjunctions against encoding each
triple product directly.

A solver without native XOR expands each parity into `r−1` fresh variables and
`4(r−1)` clauses. That is `--plain-cnf`, and it is why the parities are kept
apart from the clauses until the file is written.

Measured, with `--emit-cnf`:

| Tensor | `r` | Variables | Clauses | Expanded |
|---|---|---|---|---|
| F₂ 2×3 | 5 | 195 | 474 | |
| ⟨2,2,2⟩ | 7 | 644 | 1 744 | 1 028 vars, 3 280 clauses |
| ⟨3,3,3⟩ | 23 | 19 251 | 56 619 | |

⟨3,3,3⟩ at Laderman's 23 is a file of about a megabyte. The exhaustive search
cannot go near that tensor; this is a question a solver can be asked.

## GF(p), one-hot

Each unknown becomes `p` variables with exactly one true; each product and sum
becomes the field's table as implications `(¬x[e₁] ∨ ¬y[e₂] ∨ z[e₁∘e₂])`.

| | |
|---|---|
| Variables | `p·(r(n₁+n₂+n₃) + r·n₁n₂ + 2r·n₁n₂n₃)` |
| Clauses | about `2p²·r·n₁n₂n₃`, plus `1 + p(p−1)/2` per one-hot group |

So about `2p²` clauses per term per entry, nine times the GF(2) cost at `p = 3`.
At-most-one is written pairwise because at these primes that is three clauses
and a ladder encoding would be more machinery than it saves.

The sum is a chain: `s₀ = 0`, `s_{l+1} = s_l + q·c`, and one unit clause fixing
`s_r` to the entry. A wrong entry therefore forces two members of one one-hot
group true at once, which the at-most-one clause catches. That is what makes
the test able to detect a bad encoding at all.

## GF(p), SMT

```
(assert (= (ff.add (ff.mul (ff.mul a_l_i b_l_j) c_l_k) …) (as ff<t> F)))
```

`n₁n₂n₃` assertions over `r(n₁+n₂+n₃)` constants, and nothing else. The cost is
not in the file; it is in the Gröbner-basis procedure behind `QF_FF`
(`[ozdemir2023]`).

## Håstad's reduction

`(2 + n + 2m) × 3n × (3n + m)`, built in `Θ(n·m)` entries, with target rank
`4n + 2m`. The witness from a satisfying assignment costs one rank-one
decomposition per clause and is otherwise arithmetic.

It is a reduction, not a way to solve SAT. A formula of ten variables and
twenty clauses becomes a `52 × 30 × 50` tensor asked for rank 80, which is
enormously harder than the formula it came from. That is what a hardness proof
looks like from the inside, and it is the reason the arrow that gets used in
practice points the other way.

What any of it costs: [`measurements.md`](measurements.md). Which backend,
solver and flags it runs with, and why: [`choices.md`](choices.md).
