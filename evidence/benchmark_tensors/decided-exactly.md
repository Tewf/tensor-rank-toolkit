# The tensors decided outright

## The result worth having: rank ⟨2,2,2⟩ = 7, decided here in 0.042 s

Strassen multiplied 2×2 matrices with seven products in 1969 and Winograd proved
in 1971 that six is impossible. Both halves are reproduced from nothing:

```sh
decide-rank evidence/fixtures/matmul_2x2x2.tensor --target 6 #     25 399 nodes, 0.0292 s: NO
decide-rank evidence/fixtures/matmul_2x2x2.tensor --target 7 #      7 436 nodes, 0.0126 s: FOUND
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

## Two more, decided against a citation: F2 5×5 and F3 3×6

The paper and the table identifying these fixtures with it are in
[`README.md`](README.md).

**`f3_3x6`: the heuristic is optimal, and this repository now proves it alone.**
Step 3 returns 10 in 17.28 s, and `decide-rank --target 9` returns NO
exhaustively in 7.65 s over 4729 nodes. Two sides, 25 seconds, no citation
needed: `rank(f3_3x6) = 10`. Their 566-second run agrees, and is no longer what
the claim rests on.

**`f2_5x5`: settled here, and it took both directions.** The descent alone
stops at 14. `decide-rank --target 12` runs to exhaustion in 146 402 553 nodes
and refuses, which reproduces their exclusion and puts the floor at 13, and
[`tighten-rank-bound`](../../methods/bilinear_rank/branch_and_bound/)
exhibits 13 in 80 nodes on 2026-08-21, closing the ceiling. Their 9.65×10⁹
tests agree with the exhaustion and are no longer what the claim rests on.
**Finding a 13 was the part the heuristic missed**, and it is a construction
rather than a refutation, which is why a different search had to make it. The
full account of that run, including a retraction it settles, is
[`../../methods/bilinear_rank/exhaustive/what-it-decides.md`](../../methods/bilinear_rank/exhaustive/what-it-decides.md).

**Why the second half took until 2026-08-17 to notice.** The refutation was
never run. Everything here asked `--target 10`, which is a find, so the answer
arrived as a decomposition and the rank stayed a citation. Asking `--target 9`,
a refutation, is both the cheaper question and the one that settles it. That
asymmetry is the whole reason `decide-rank` takes a target at all, and it was
being used the expensive way round.
