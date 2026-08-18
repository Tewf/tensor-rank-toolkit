# Handing the bracket over, and what it costs

[`descend_from_ceiling`](../../oracle_guided_search/descending_sweep.h) produces
`[floor, upper]` with a decomposition at `upper`, and its header says handing
that back to `find_rank` leaves exactly one refutation to buy instead of one per
rank from the floor upward. Nothing could hand it over. `find_rank` took a bare
number, a bare number cannot say whether the bound was reached or merely
assumed, and so the walk asked at `upper` to find out.

`AchievedCeiling` in [`rank_question.h`](../rank_question.h) is that seam and it
does what the sentence says. **Buying the bracket in order to use it does not
pay.**

## The seven fixtures, both ways, in one process

`--break-symmetry --plain-cnf`, floors from `rank_lower_bound`, ceilings from the
naive count. `plain` is `find_rank` alone; `wired` is the sweep followed by
`find_rank` told its ceiling is in hand.

| fixture | plain, s | asked | wired, s | of which sweep | asked | wired/plain |
|---|---|---|---|---|---|---|
| f2_2x2 | 0.006 | 1 | 0.014 | 0.014 | 0 | 2.2x |
| f2_2x3 | 0.016 | 1 | 0.028 | 0.028 | 0 | 1.7x |
| gf4 | 0.003 | 1 | 0.010 | 0.010 | 0 | 2.9x |
| gf8 | 0.049 | 1 | 0.107 | 0.107 | 0 | 2.2x |
| w_state | 0.005 | 1 | 0.008 | 0.008 | 0 | 1.4x |
| matmul_2x2x2 | 0.560 | 2 | 1.152 | 0.764 | 1 | 2.1x |
| **GF(16)** | **126.06** | 2 | **157.77** | 32.59 | 1 | **1.25x** |

**The seam works and the errand does not.** One question is removed every time,
which is exactly what an achieved ceiling is worth, and the sweep that produces
it costs more than that question does. On GF(16) the sweep spends 32.59 s, of
which 30 s is the candidate budget expiring at `k = 8`, to save the 0.35 s yes at
`k = 9`.

## Why the lever moved

[`what-decides-it.md`](what-decides-it.md) was measured when the floor on GF(16)
was 4 against a rank of 9, so ascending walked five ranks to reach the answer and
a tight ceiling had five questions to save. `rank_lower_bound` now returns 8. The
floor equals the rank on five of these seven fixtures and is one short on the
other two, so `find_rank` already pays the mandatory
`cost(no at r-1) + cost(yes at r)` and no ceiling has anything left to close.

**A better upper bound is still the lever wherever the floor is loose**, and
nothing here refutes that. What it says is that on these seven the floor is not
loose any more, so a sweep bought to tighten a ceiling is a sweep bought to close
a gap a polynomial bound has already closed.

So `find_rank` can take an achieved ceiling and no command spends a sweep to get
one. A caller that already holds a checked decomposition, from a heuristic or
from a previous run, hands it over for nothing, and that is the case the seam is
kept for.
