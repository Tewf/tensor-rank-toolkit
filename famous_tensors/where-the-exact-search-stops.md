# Where the exact search stops, measured

Cost is not the node count, it is nodes times pool: every leaf scans the pool
once. And the node count is `C(pool, k − dim T)`, where the exponent is the gap
between the map's own span and the target, not the target: `⟨2,2,2⟩` has a
4-dimensional span and settles at 7 in seconds, while `⟨2,3,3⟩` has a
6-dimensional span, so asking about 10 is already four levels deep and out of
reach for ever.

**The two count columns are exact and the Time column is three different eras**,
which is worth saying before the table rather than after it. Nodes and pool sizes
are facts about the problem and come out the same on any machine and any leaf.
The times do not: only the first row was retaken on the leaf that ships, on
2026-08-23 and from a clean tree, and every other figure below was measured or
projected at the general leaf's rate of about **1.5×10⁹ field operations a
second**, before the GF(2) leaf and the reflected Gray walk landed on 2026-08-20.

| Run | Nodes | Pool | Time |
|---|---|---|---|
| `⟨2,2,2⟩` rule out 6 | 25 399 | 225 | **0.029 s**, on the leaf that ships |
| `⟨2,2,3⟩` rule out 8 | 446 923 | 945 | 53 s, **retired** |
| cyclic 5 rule out 7 | 461 251 | 961 | 51 s, **retired** |
| GF(16) rule out 7 | 1 897 576 | 225 | 34 s, **retired** |
| GF(16) rule out 8 | 105 600 301 | 225 | 38.8 min |
| `⟨2,2,3⟩` rule out 9 | ~1.4×10⁸ | 945 | ~4.6 h, not run |
| `⟨2,3,3⟩` rule out 10 | `C(32193, 4)` = 4.5×10¹⁶ | 32 193 | stopped at 100 min, hopeless |
| `⟨3,3,3⟩` rule out 10 | ~2.6×10⁵ | 261 121 | ~10 h, not run |
| `⟨3,3,3⟩` heuristic step 3 | 261 121 candidates | | ~4.2 h, stopped at 45 min |

**So every projected time here is an upper bound, and no rate should be derived
across the column.** The one row that was retaken runs at about **fourteen times**
the rate the rest were taken at: 25 399 nodes over a 225-map pool is 5.7×10⁶ leaf
elements, which was 0.41 s and is 0.029 s. Applying that to the rows nobody has
run would be arithmetic and not a measurement, so it is not done here; what it
means is that the projections are ceilings rather than estimates. **The argument
the table exists for survives either way**, because it is about an exponent and
not a rate: `⟨2,3,3⟩` at 10 is `C(32193, 4)` = 4.5×10¹⁶ leaves, which is never at
any of these rates, and fourteen times never is still never.

**"Retired" means the search does not run it at all any more.** `rank_lower_bound`
refuses the target before the first node opens, so the count is 0 and there is no
time to report. The Nodes column keeps what the binary produced when the
flattening rank was the only bound it had, because a figure worth retiring is
worth naming. Three rows here are retired, each bought with about a minute of
exhaustion and now returned in milliseconds, and cyclic 5's floor improves from
the 8 the run proved to **9**. `F₂ 5×5` at 11 joined them on 2026-08-19, when the
Griesmer floor moved that map from 10 to 12 and a 77 second refutation stopped
being a run at all. All of it is recorded under `retired_by_the_bounds` in
[`../descent_search/results.json`](../descent_search/results.json), and
**retirement is the better result**: a floor in milliseconds beats a tree walked
to the end for a minute, and reading it the other way round is how a repository
ends up attached to its slowest proof.

The 38.8 minutes is the one that still buys something, because ruling out 8 on
GF(16) is past what any bound here reaches. Its flags are recorded now,
`--node-limit 200000000`, in the `exhaustive_command` of
`satisfiability/results.json`, and `reproduce/measure.py --slow` re-derives the
count from it exactly. On the GF(2) leaf the same search is 6.6 minutes rather
than 38.8, which is why it is the one question the default run leaves out and
prices in a SKIPPED line instead.

`⟨4,4,4⟩`, where AlphaTensor found 47 products over Z₂, is refused rather than
attempted, and the refusal is worth reading:

```
decide-rank: the pool of rank-one 16x16 maps needs 8.2 TiB (4294836225 items),
over the 2.0 GiB budget. Raise it with --max-memory if the machine has the room.
```

`(2¹⁶−1)²` rank-one maps of 256 entries each. Reaching that allocation is not a
slow run, it is the machine going down, and until tonight nothing stood between
the call and the allocator.
