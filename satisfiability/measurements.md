# What it costs, measured

Every number here comes from [`results.json`](results.json), which
`reproduce/measure.py` last rewrote from a clean tree on **2026-08-23**, on one
core of an i5-12450H, at the commands each row records. The counts are re-derived
on every push; the seconds are not, for the reason
[`../MEASURING.md`](../MEASURING.md) gives. The searches compared against are in
[`../exhaustive_search/`](../exhaustive_search/), on the same fixtures. How the
encodings work is [`method/`](method/README.md); this is only what they cost.

## The rank itself, not one `k` at a time

Sweeping upward turns the decision procedure into a rank finder, and the whole
of `⟨2,2,2⟩` costs about half a second from nothing:

```
lower bound: rank is at least 6                 rank_lower_bound, before any question
k = 6: NO, rank is more than 6   (0.338 s)
k = 7: FOUND a decomposition     (0.185 s)      0.517 s, two questions
```

Strassen's seven products and Winograd's proof that six are impossible, neither
assumed, and the seven checked against the map they compute. **`k = 5` is not
asked at all any more**, and that is the bounds doing to this sweep what they did
to three rows of the exhaustive table: `rank_lower_bound` returns 6 before the
first clause is written, so the question is retired rather than answered. `F₂ 2×3`
the same way is 5 after ruling out 1 through 4, in 50 ms.

**Those are the flags `results.json` records, `--break-symmetry --plain-cnf`, and
the flags are most of the figure.** The same sweep at the defaults costs
**25.04 s** rather than 0.517 s, measured 2026-08-23 under the protocol, because
symmetry breaking is off by default and off for an argument rather than for a
measurement: an over-strong constraint would turn a satisfiable instance
unsatisfiable, which is a false lower bound
([`../OPTIONS/asking-a-sat-solver.md`](../OPTIONS/asking-a-sat-solver.md)).

## What it costs against the exhaustive search

Both columns are the 2026-08-23 run, against the exhaustive searches on the same
fixtures. Every rank agrees in both directions, which is the point of having two
methods.

| Question | Exhaustive | SAT | |
|---|---|---|---|
| `⟨2,2,2⟩` find 7 | 7 436 nodes, **0.0126 s** | 0.185 s | both find Strassen |
| `⟨2,2,2⟩` rule out 6 | 25 399 nodes, **0.0292 s** | 0.338 s | **11.6x**, and it is the tree |
| `⟨2,2,3⟩` rule out 8 | retired, 0 nodes (was 446 923, 53.1 s) | 34.1 s | the bounds refute 8 in ms, beating both |
| GF(16) find 9 | not reachable | **0.288 s** | |
| GF(16) rule out 8 | 105 600 301 nodes at `--node-limit 200000000`, 2328 s | **106.9 s** | **21.8x**, and it is a ceiling |
| GF(8) rule out 5 | | **0.055 s** | |
| Karatsuba, GF(4), W state | | under 0.02 s | |
| F₂ 5×5 rule out 12 | never run | **unresolved** | neither method has an answer |

**The `⟨2,2,2⟩` row reversed, and it reversed for a reason that is not about the
solver.** It read `0.41 s` against `0.31 s` until 2026-08-23 and was quoted in
three places as the two methods being *comparable*. The exhaustive end of it is
now 0.029 s, because the GF(2) bit-packed leaf and the reflected Gray walk landed
on 2026-08-20 and every published search figure was retaken from a clean tree
three days later. The solver's end barely moved, and would not: it is another
program's second, and kissat did not change. So the pair is a fact about this
repository's leaf and not a finding about SAT.

**The last row's 21.8x is a ceiling, and the file it comes from says so.** The
2328 s is older than the count beside it: it predates the same GF(2) leaf, and
`results.json`'s `exhaustive_note` records that the shipped binary does that
search about six times faster with **no replacement timing published**, because
the runs that showed it were taken on a machine that was not quiet. So the
solver's margin on GF(16) is at most 21.8x and the honest reading is "still
ahead, by less than this". `measure.py --check --slow` is what would settle it,
at 6.6 minutes of one core a run.

**The SAT column is kissat, fastest of three, and it says so because it once did
not.** Three of these cells were cryptominisat timings sitting under a heading
naming kissat, which is a worse fault than a stale number: `⟨2,2,2⟩` find 7 was
0.469 s, which is what cryptominisat costs against kissat's 0.185 s, and GF(16)
find 9 was 36.19 s against 0.288 s. Solver by solver, per column, in
[`results.json`](results.json).

`⟨2,2,3⟩` at 8 is retired rather than measured: the bounds refuse it before the
search opens a node. Under `retired_by_the_bounds` in
[`../descent_search/results.json`](../descent_search/results.json).

The last row is open on both sides, and the earlier version of it was wrong in
this repository's worst way: it gave the exhaustive column "146 402 553 nodes,
3610 s on 8 threads", which is not a measurement. No such run happened. That
column was an extrapolation from `descent_search/method/`, where k=12 is priced
at `C(961,3)` and about seven hours and is labelled extrapolated, and it arrived
here as a time and a node count in a table headed "Measured". What this repository
proves by itself is **rank = 13**, with 12 refused here and 13 exhibited by
[`../incumbent_search/`](../incumbent_search/README.md); it was 13 ≤ rank ≤ 14
until 2026-08-21, the heuristic reaching only 14. Eleven went first: an exhaustion of 459 239 nodes and 77 s until
2026-08-19, when the Griesmer floor moved this map to 12 and returned that
refusal in milliseconds. Twelve then went by exhaustion, 146 402 553 nodes. The
solver had 700 s on 12, returned unknown, and wrote no proof, which is the
comparison this file exists to record: the tree answered the question the solver
could not.

**The rank itself is 13 and has been published since 2012.** `[bdez2012]` ran the
same algorithm on the same map over a complete run: 27 solution subspaces,
9.65×10⁹ tests, 2.28×10⁵ s, and their `#G` of 961 is exactly the pool
`all_rank_one_maps` builds, so the row is certainly this fixture. Deciding 12 here
would reproduce a published exclusion, which is worth doing as a check and settles
nothing open.

**The advantage grows with the instance, and it now starts from behind.** The
solver is 11.6 times *slower* than the tree on `⟨2,2,2⟩`, at most 21.8 times
faster on GF(16), and was 1.6 times faster on `⟨2,2,3⟩` until that comparison
lost its exhaustive end to the bounds. That is a stronger version of the same
claim, not a different one: the table used to have a tie at the small end and a
win at the large one, and now it has a loss and a win, so **the crossover is
inside the table rather than argued from its ends**. The exhaustive search prunes
subspaces and, with the orbit quotient, whole orbits at once; the solver learns
clauses, and the harder the instance the more there is to learn. What moved is
the small end, where there is least to learn and the tree is a few thousand
bit-packed rank tests.

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
