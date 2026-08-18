# The tensors the literature argues about, run through both searches

The fixtures here are polynomial multiplication, because that is what the
polynomial fixtures cover. The code is not: a bilinear map `F^n × F^m → F^d` is an
order-3 tensor, so every tensor the complexity literature cares about fits the
same file format and the same two searches. This is what happened when they were
fed the famous ones, over GF(2), on one core of an i5-12450H.

Build them with `make-tensor --matmul n m k` and `--cyclic n`.

## What each search did

| Tensor | Naive | Heuristic | Exact says | Known |
|---|---|---|---|---|
| `⟨2,2,2⟩` matrix multiplication | 8 | **8**, stuck | **exactly 7** | 7 |
| `⟨2,2,3⟩` | 12 | **12**, stuck | **≥ 9** | 11 |
| `⟨2,3,3⟩` | 18 | **18**, stuck | nothing reachable | 15 |
| `⟨3,3,3⟩` | 27 | **27**, stuck | out of reach | open, 19–23 |
| W state | 3 | 3 | **exactly 3** | 3, border rank 2 |
| Cyclic convolution, length 5 | 25 | **10** | **≥ 9** | |
| GF(16) over GF(2) | 16 | **9** | **exactly 9** | ≥ 8 by de Groote |

## The result worth having: rank ⟨2,2,2⟩ = 7, decided here in 0.55 s

Strassen multiplied 2×2 matrices with seven products in 1969 and Winograd proved
in 1971 that six is impossible. Both halves are reproduced from nothing:

```sh
decide-rank fixtures/matmul_2x2x2.tensor --target 6   #     25 399 nodes, 0.41 s: NO
decide-rank fixtures/matmul_2x2x2.tensor --target 7   #      7 436 nodes, 0.14 s: FOUND
```

The 7 products are checked against the map they claim to compute, so this is
Strassen's algorithm rediscovered rather than recognised. The same command
settles the W state at exactly 3, which is the tensor textbooks use to show rank
and border rank differ; the border rank of 2 is invisible to this search, and
saying so is the point of listing it.

## The second one decided: multiplication in GF(16) needs exactly 9

Here the two methods finish the job between them, which is the case for keeping
both. The exact search ruled out 5, 6 and 7 in half a minute, then 8 in
**105 600 301 nodes and 38.8 minutes**, exhaustively. The heuristic had already
reached 9. Neither could have done it alone: the search cannot get to 9 from
nothing at this speed, and the heuristic proves nothing.

de Groote's theorem says the bound `2n-1` is attained only for `n ≤ q/2 + 1`, so
GF(16) over GF(2) must need more than 7. **That half no longer needs the
theorem**: the rank-sum bound gets `≥ 8` from the tensor in milliseconds, with no
theorem, no solver and no exhaustion, because all fifteen nonzero contractions
have full rank 4 — the span is a field, so every nonzero element is invertible —
and `60 / 8 = 7.5`. That it needs more than 8 is still decided here by
exhaustion, and that is the half the 38.8 minutes buys.

## The finding about the heuristic: it cannot move on matrix multiplication

Three matrix multiplication tensors run through the whole method, three times no
improvement whatsoever, while the same heuristic takes cyclic convolution from
25 to 10 and GF(16) from 16 to 9. `⟨3,3,3⟩` matches through steps 1 and 2; its
step 3 was given forty-five minutes and did not finish. Projected from the
measured `⟨2,3,3⟩` scan, 4.77 ms a candidate over 32 193 of them, and scaled by
the span it enumerates and the size of each rank, it wants **about 4.2 hours**.
That scan is one independent test per candidate, so twelve threads would bring
it under half an hour; nothing here is parallel yet, and that is the next thing
worth measuring.

It is not bad luck in the search order, and the tool says why:

```
step 3 pool: 225 rank-one maps      step 3 pool: 225 rank-one maps
step 3 shortlist: 0        ⟨2,2,2⟩  step 3 shortlist: 4        GF(16)
```

**Not one rank-one map of the 225 improves ⟨2,2,2⟩**, so a first-improvement
greedy has nowhere to step. Nor does one of the 945 for `⟨2,2,3⟩`, nor one of
the 32 193 for `⟨2,3,3⟩`: the shortlist is empty every time, on every size the
scan can finish. The reason is structural, and it is the same
property that makes step 1 trustworthy: step 1 returns a minimum-weight basis of
`span(T)` and is provably optimal for that, so 8 is the true minimum **over all
bases of the span**. Strassen's seven products are not a basis of `span(T)`,
which has dimension 4; they span a 7-dimensional space containing it. The
heuristic only ever moves between spanning sets that pay off one candidate at a
time, and the road to 7 does not.

So the two methods here are not a fast one and a slow one. They fail and
succeed on different tensors, and matrix multiplication is exactly where the
heuristic stops and the exact search starts.

## Where the exact search stops, measured

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
8 on GF(16) is past what any bound here reaches.

`⟨4,4,4⟩`, where AlphaTensor found 47 products over Z₂, is refused rather than
attempted, and the refusal is worth reading:

```
decide-rank: the pool of rank-one 16x16 maps needs 8.2 TiB (4294836225 items),
over the 2.0 GiB budget. Raise it with --max-memory if the machine has the room.
```

`(2¹⁶−1)²` rank-one maps of 256 entries each. Reaching that allocation is not a
slow run, it is the machine going down, and until tonight nothing stood between
the call and the allocator.

For the state of the art on exactly these families, including matrix
multiplication over F₂ and cyclic convolution over F₂ and F₃, see *Automated
Lower Bounds for Bilinear Complexity over Finite Fields* (arXiv 2603.07280),
which raised the F₂ lower bound for `⟨3,3,3⟩` from 19 to 20 using orbit
classification under a symmetry group. That is the direction this search would
have to go to reach the sizes it currently cannot.
