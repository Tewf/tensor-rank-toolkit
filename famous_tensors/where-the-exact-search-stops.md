# Where the exact search stops, measured

Cost is not the node count, it is nodes times pool: every leaf scans the pool
once. And the node count is `C(pool, k − dim T)`, where the exponent is the gap
between the map's own span and the target, not the target: `⟨2,2,2⟩` has a
4-dimensional span and settles at 7 in seconds, while `⟨2,3,3⟩` has a
6-dimensional span, so asking about 10 is already four levels deep and out of
reach for ever. Measured at about 1.5×10⁹ field operations a second:

| Run | Nodes | Pool | Time |
|---|---|---|---|
| `⟨2,2,2⟩` rule out 6 | 25 399 | 225 | 0.41 s |
| `⟨2,2,3⟩` rule out 8 | 446 923 | 945 | 53 s, now free |
| cyclic 5 rule out 7 | 461 251 | 961 | 51 s, now free |
| GF(16) rule out 7 | 1 897 576 | 225 | 34 s, now free |
| GF(16) rule out 8 | 105 600 301 | 225 | 38.8 min |
| `⟨2,2,3⟩` rule out 9 | ~1.4×10⁸ | 945 | ~4.6 h, not run |
| `⟨2,3,3⟩` rule out 10 | `C(32193, 4)` = 4.5×10¹⁶ | 32 193 | stopped at 100 min, hopeless |
| `⟨3,3,3⟩` rule out 10 | ~2.6×10⁵ | 261 121 | ~10 h, not run |
| `⟨3,3,3⟩` heuristic step 3 | 261 121 candidates | | ~4.2 h, stopped at 45 min |

**"Now free" means the rank-sum bound already returns that floor**, so the run
below it need never be made again. Three of them: `⟨2,2,3⟩`'s `≥ 9` and
cyclic 5's floor were each bought with about a minute of exhaustion and are
returned in milliseconds, and cyclic 5's improves from the 8 the run proved to
**9**. The 38.8 minutes is the one that still buys something, because ruling out
8 on GF(16) is past what any bound here reaches. It is also the one run here
whose flags were never written down, so nothing re-derives it:
[`../MEASURING.md`](../MEASURING.md) says why, and
`satisfiability/results.json` carries the statement beside the figure.

`⟨4,4,4⟩`, where AlphaTensor found 47 products over Z₂, is refused rather than
attempted, and the refusal is worth reading:

```
decide-rank: the pool of rank-one 16x16 maps needs 8.2 TiB (4294836225 items),
over the 2.0 GiB budget. Raise it with --max-memory if the machine has the room.
```

`(2¹⁶−1)²` rank-one maps of 256 entries each. Reaching that allocation is not a
slow run, it is the machine going down, and until tonight nothing stood between
the call and the allocator.
