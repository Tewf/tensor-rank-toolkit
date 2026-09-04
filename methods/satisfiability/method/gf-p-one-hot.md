# GF(p), one-hot

Each unknown becomes `p` variables with exactly one true; each product and sum
becomes the field's table as implications `(¬x[e₁] ∨ ¬y[e₂] ∨ z[e₁∘e₂])`.

| | |
|---|---|
| Variables | `p·(r(n₁+n₂+n₃) + r·n₁n₂ + (2r+1)·n₁n₂n₃)` |
| Clauses | about `2p²·r·n₁n₂n₃`, plus `1 + p(p−1)/2` per one-hot group |

So about `2p²` clauses per term per entry, nine times the GF(2) cost at `p = 3`.
At-most-one is written pairwise because at these primes that is three clauses
and a ladder encoding would be more machinery than it saves.

The sum is a chain: `s₀ = 0`, `s_{l+1} = s_l + q·c`, and one unit clause fixing
`s_r` to the entry. A wrong entry therefore forces two members of one one-hot
group true at once, which the at-most-one clause catches. That is what makes
the test able to detect a bad encoding at all.
