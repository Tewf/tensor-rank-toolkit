# Where the descent's cost actually is

`p^k` dominates everything, and **`k` grows during the search**, which is the
point of the method, and also its wall. Peak memory is essentially the largest
`minimum_weight_basis` call:

> peak ≈ `p^(k+1) · (8w + 88)` bytes

| Fixture | Largest call | Predicted | Measured peak RSS |
|---|---|---|---|
| F2 5×5 | 2¹¹ | 5.2 MB | 5.4 MB |
| F2 3×8 | 2¹⁴ | 9.2 MB | 9.4 MB |
| F2 4×7 | 2¹⁴ | 9.7 MB | 10.0 MB |
| F3 3×6 | 3¹¹ | 45.7 MB | 42.3 MB |

(including a 4.6 MB baseline process, measured with `--steps 1`.)

So the scaling limit is not time, it is memory, and it is exponential in a
quantity the search deliberately increases. Enumerating the span is the honest
first thing to replace: nothing in the method requires *materialising* it, only
visiting it in nondecreasing rank order.
