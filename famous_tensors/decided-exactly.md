# The two tensors decided outright

## The result worth having: rank ⟨2,2,2⟩ = 7, decided here in 0.042 s

Strassen multiplied 2×2 matrices with seven products in 1969 and Winograd proved
in 1971 that six is impossible. Both halves are reproduced from nothing:

```sh
decide-rank fixtures/matmul_2x2x2.tensor --target 6 #     25 399 nodes, 0.0292 s: NO
decide-rank fixtures/matmul_2x2x2.tensor --target 7 #      7 436 nodes, 0.0126 s: FOUND
```

The 7 products are checked against the map they claim to compute, so this is
Strassen's algorithm rediscovered rather than recognised. The same command
settles the W state at exactly 3, which is the tensor textbooks use to show rank
and border rank differ; the border rank of 2 is invisible to this search, and
saying so is the point of listing it.

## The second one decided: multiplication in GF(16) needs exactly 9

Here the two methods finish the job between them, which is the case for keeping
both. The exact search ruled out 5, 6 and 7 in half a minute, then 8 in
**105 600 301 nodes and 4.12 minutes**, exhaustively. The heuristic had already
reached 9. Neither could have done it alone: the search cannot get to 9 from
nothing at this speed, and the heuristic proves nothing.

de Groote's theorem says the bound `2n-1` is attained only for `n ≤ q/2 + 1`, so
GF(16) over GF(2) must need more than 7. **That half no longer needs the
theorem**: the rank-sum bound gets `≥ 8` from the tensor in milliseconds, with no
theorem, no solver and no exhaustion, because all fifteen nonzero contractions
have full rank 4 (the span is a field, so every nonzero element is invertible)
and `60 / 8 = 7.5`. That it needs more than 8 is still decided here by
exhaustion, and that is the half the 4.12 minutes buys.
