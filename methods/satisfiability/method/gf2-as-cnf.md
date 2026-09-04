# GF(2), as CNF

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

A solver without native XOR expands each parity into `r−1` fresh variables,
`4(r−1)` clauses, and one further clause fixing the chain to the wanted value.
That is `--plain-cnf`, and it is why the parities are kept apart from the
clauses until the file is written.

Measured, with `--emit-cnf`:

| Tensor | `r` | Variables | Clauses | Expanded |
|---|---|---|---|---|
| F₂ 2×3 | 5 | 195 | 474 | |
| ⟨2,2,2⟩ | 7 | 644 | 1 744 | 1 028 vars, 3 280 clauses |
| ⟨3,3,3⟩ | 23 | 19 251 | 56 619 | |

`F₂ 2×3` is [`evidence/fixtures/f2_2x3.tensor`](../../../evidence/fixtures/f2_2x3.tensor);
`⟨2,2,2⟩` and `⟨3,3,3⟩` are the matrix multiplication tensors of those shapes,
[`matmul_2x2x2.tensor`](../../../evidence/fixtures/matmul_2x2x2.tensor) and
[`matmul_3x3x3.tensor`](../../../evidence/fixtures/matmul_3x3x3.tensor), each run
as `decide-rank-by-sat <file> --target <r> --emit-cnf out.cnf`.

⟨3,3,3⟩ at Laderman's 23 is a file of about a megabyte. The exhaustive search
cannot go near that tensor; this is a question a solver can be asked.
