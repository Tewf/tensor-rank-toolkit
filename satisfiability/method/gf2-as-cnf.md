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

A solver without native XOR is handed each parity expanded into `r−1` fresh
variables and `4(r−1)` clauses. That is `--plain-cnf`, and it is why the
parities are kept apart from the clauses until the file is written. **It is one
expansion of several, and for a local search the worst.** It is the linear
*3-cut*: every fresh variable is the parity of two literals. `[nawrocki2021]`
measured eight CNF encodings of the same parities under the same solver and
found performance rising with the cutting number up to 6, with the 3-cut last,
and keeping the parity as one `x` line, which `--emit-xnf` writes and
`cnf2xnf` recovers from a CNF at no loss, beating every one of them. For kissat
the choice is worth nothing measurable ([`../choices/`](../choices/README.md));
for a walk it is the whole difference, priced in
[`../las-vegas/`](../las-vegas/README.md).

Measured, with `--emit-cnf`:

| Tensor | `r` | Variables | Clauses | Expanded |
|---|---|---|---|---|
| F₂ 2×3 | 5 | 195 | 474 | |
| ⟨2,2,2⟩ | 7 | 644 | 1 744 | 1 028 vars, 3 280 clauses |
| ⟨3,3,3⟩ | 23 | 19 251 | 56 619 | |

⟨3,3,3⟩ at Laderman's 23 is a file of about a megabyte. The exhaustive search
cannot go near that tensor; this is a question a solver can be asked.
