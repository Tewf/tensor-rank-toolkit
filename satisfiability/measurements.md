# What it costs, measured

Every number here was produced on 2026-08-16 on one core of an i5-12450H, by
the commands in [the README](README.md), against the exhaustive searches in
[`../exhaustive_search/`](../exhaustive_search/) on the same fixtures. How the encodings
work is [`method/`](method/README.md); this is only what they cost.

## The rank itself, not one `k` at a time

Sweeping upward turns the decision procedure into a rank finder, and the whole
of `⟨2,2,2⟩` costs less than a second from nothing:

```
k = 5: NO, rank is more than 5   (0.10 s)
k = 6: NO, rank is more than 6   (0.32 s)
k = 7: FOUND a decomposition     (0.18 s)      0.60 s in total
```

Strassen's seven products and Winograd's proof that six are impossible, neither
assumed, and the seven checked against the map they compute. `F₂ 2×3` the same
way is 5 after ruling out 1 through 4, in 50 ms.

## What it costs against the exhaustive search

Measured 2026-08-16 on one core, against the exhaustive searches on the same
fixtures. Every rank agrees in both directions, which is the point of having two
methods.

| Question | Exhaustive | SAT | |
|---|---|---|---|
| `⟨2,2,2⟩` find 7 | 7 436 nodes | **0.18 s** | both find Strassen |
| `⟨2,2,2⟩` rule out 6 | 25 399 nodes, 0.41 s | **0.31 s** | |
| `⟨2,2,3⟩` rule out 8 | 446 923 nodes, 53.1 s | **34.3 s** | 1.6x |
| GF(16) find 9 | not reachable | **0.27 s** | |
| GF(16) rule out 8 | 105 600 301 nodes, 2328 s | **108.0 s** | **21x** |
| GF(8) rule out 5 | | **0.038 s** | |
| Karatsuba, GF(4), W state | | under 0.02 s | |
| F₂ 5×5 rule out 12 | never run | **unresolved** | neither method has an answer |

**The SAT column is kissat, fastest of three, and it says so because it once did
not.** Three of these cells were cryptominisat timings sitting under a heading
naming kissat, which is a worse fault than a stale number: `⟨2,2,2⟩` find 7 was
0.48 s, which is what cryptominisat costs against kissat's 0.18 s, and GF(16)
find 9 was 36.7 s against 0.27 s. Solver by solver, per column, in
[`results.json`](results.json).

The last row is open on both sides, and the earlier version of it was wrong in
this repository's worst way: it gave the exhaustive column "146 402 553 nodes,
3610 s on 8 threads", which is not a measurement. No such run happened. That
column was an extrapolation from `descent_search/method/`, where k=12 is priced
at `C(961,3)` and about seven hours and is labelled extrapolated, and it arrived
here as a time and a node count in a table headed "Measured". What this repository
proves by itself is **12 ≤ rank ≤ 14**, with 11 ruled out exhaustively and 14
reached by the heuristic; the solver had 700 s, returned unknown, and wrote no
proof.

**The rank itself is 13 and has been published since 2012.** `[bdez2012]` ran the
same algorithm on the same map over a complete run: 27 solution subspaces,
9.65×10⁹ tests, 2.28×10⁵ s, and their `#G` of 961 is exactly the pool
`all_rank_one_maps` builds, so the row is certainly this fixture. Deciding 12 here
would reproduce a published exclusion, which is worth doing as a check and settles
nothing open.

**The advantage grows with the instance**, which is the interesting part: level
on `⟨2,2,2⟩`, 1.6 times on `⟨2,2,3⟩`, twenty-one times on GF(16). The exhaustive
search prunes subspaces and, with the orbit quotient, whole orbits at once; the
solver learns clauses, and the harder the instance the more there is to learn.

**This table replaces one that said the opposite.** The first measurements had
this method losing badly, and both reasons were defaults of mine rather than
properties of the method. They are worth stating because they are the whole
lesson:

Which backend, which solver and which flags produced these numbers, and
the reasoning that measurement overturned to get there:
[`choices/`](choices/README.md).

The driver that puts one question to every backend at once and tabulates what
each cost is [`tools/compare_backends.py`](../tools/compare_backends.py). It is
what makes a row here comparable with a tree-search row at all: the instruments
count in different currencies, nodes against conflicts, and wall clock on one
machine is the only one they share.
