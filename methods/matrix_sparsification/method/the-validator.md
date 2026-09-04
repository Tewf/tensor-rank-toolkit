# The validator, and the one route that still calls it

A vector in the row space of `rows`, zero on every column of `S`, that does not
lie in the span of the rows already settled. It exists exactly when some
unsettled row is, restricted to `S`, in the span of the others; the
coefficients that say so are the validator, with `−1` in that row's place.

```
find_validator(rows, S, settled):
    R ← rows[:, S]
    for each candidate i ∉ settled:
        if R[i] ∈ span(R[j] : j ≠ i):
            λ ← those coefficients, with λᵢ = −1
            return (λ, i)                        # λᵀ·rows is 0 on every column of S
    return none
```

Cost: Θ(a³·|S|), up to `a` candidates, each a solve with `a−1` unknowns over
`|S|` equations.

The greedy by rescaling is the one caller. On the alternative-basis fixture it
uses this to reach ten nonzeros as ten signs, ten operations
([`../`](../)).
