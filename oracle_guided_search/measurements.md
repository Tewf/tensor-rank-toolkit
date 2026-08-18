# What the finder costs, against the baselines it has to beat

Measured 2026-08-17, one build, one lock held for the whole run so no two timings
overlapped. Solver kissat, `--candidate-timeout 60` except where stated.

The baselines are the two upper-bound producers. `find_rank` is not one of them: it
returns a minimal decomposition when it terminates, so it answers a harder question
and beating it is not the job.

## The fixed-k finder against the heuristics

| fixture | known | `minimise-rank` | `walk-scheme` | `find-at-rank --descend` |
|---|---|---|---|---|
| `⟨2,2,2⟩` GF(2) | 7 | 8, 0.01 s | **7, 0.11 s** | 7, 42.9 s |
| `gf16` | 9 | **9, 0.04 s** | 9, 0.69 s | 9, 61.3 s |
| `f2_5x5` | 12 to 14 | **14, 7.2 s** | **14, 10.8 s** | 15, 89.2 s |
| `f3_3x6` GF(3) | 10 | **10, 18.9 s** | 12, 10.9 s | not found at 10, 300 s |

**The finder is dominated on every fixture.** It reaches the known answer on
`⟨2,2,2⟩` and `gf16` and is between 390 and 1500 times slower doing it; it returns a
*worse* bound on `f2_5x5`, 15 against 14; and on `f3_3x6` it does not reach 10 at all
in 300 s while the greedy returns a verified 10-product algorithm in 18.9 s.

## `⟨3,3,3⟩`, the one shape where nothing else answers either

| method | products | cost |
|---|---|---|
| `minimise-rank --steps 3 -s matmul 3 3 3` | 27, the naive cost, no improvement | 4.3 s |
| `walk-scheme --flips 20000 --seeds 8` | **24** | 38.1 s |
| `find-at-rank --target 23 -s matmul 3 3 3` | nothing, 13 candidates at 60 s each | 780 s |
| `find-at-rank --target 23`, unrestricted | nothing | 300 s |
| `find-at-rank --descend --ceiling 27 -s matmul 3 3 3` | 26, stalling at 25 | 313 s |

Nobody reaches 23. The greedy cannot move off the naive 27, which is the shortlist
problem again at a larger shape; the plateau walk reaches 24 in 38 s; the finder in its
best mode reaches 26 in 313 s. The flattening floor here is 9 against a true rank of at
least 19, so the free bound refuses no part of a descent.

Candidate 0 timed out at both 27 and 26 and candidate 1 answered both, at 20 s and
32 s. One systematically bad representative asked first costs a full budget per rank,
which is the `⟨2,2,2⟩` observation below repeating at scale.

Two premises this run corrected:

- **`f3_3x6` at 10 was not unanswered.** `minimise-rank --steps 3` delivers 10
  products, checked by `recovers_map`, in 18.9 s. What no backend settles is the
  *decision*, proving 10 minimal. The upper bound was already had, by the tool the
  finder was meant to beat.
- **`minimise_rank` cannot take a first step on `⟨2,2,2⟩`, but `plateau_search` can.**
  The shortlist really is 0 of 225, so the greedy stops at the naive 8. The plateau
  walk crosses to 7 in 0.11 s, which is the number to beat and not 8.

## Where the mechanism does show something

The `⟨2,2,2⟩` descent, itemised:

| k | outcome | cost |
|---|---|---|
| 16 | found, and the model has 9 nonzero terms, so the sweep jumps to 8 | 0.013 s |
| 8 | found, candidate 0 | 0.27 s |
| 7 | **candidate 0 timed out at 30 s, candidate 1 answered in 0.50 s** | 30.5 s |
| 6 | all five candidates refused | 12.1 s |

- **Dropping zero terms is worth seven ranks of the sweep.** A question at `k` above
  the rank is satisfied with terms to spare and the solver spends them on zero terms.
  Keeping them also made `recovers_map` reject the find outright, a zero matrix not
  being rank one, so this was a correctness fix before it was a speed one.
- **Committing to the wrong representative first is expensive.** At `k = 7` the
  unrestricted question takes about 0.2 s. Pinned to candidate 0 it does not finish in
  30 s; pinned to candidate 1 it takes 0.50 s. The commitment is not a reliable
  narrowing of an easy question, it is a reshaping that can make one hard.

## The honest reading

The commitment applies to two of these five fixtures. A cube is GF(2) only and its
representatives are the closed-form orbits of `⟨n, m, k⟩`, so on `gf16`, `f2_5x5` and
`f3_3x6` the finder is a descending sweep of plain oracle calls with one candidate.
The general orbit route does not rescue them: the ambient group for a 5x5 map over
GF(2) is refused at about `10^14` elements, and a polynomial multiplication tensor's
has about four, which quotients nothing.

No known answer came out wrong. `⟨2,2,2⟩` gives 7 and `gf16` gives 9, both exact, and
`f2_5x5` gives 15, which is a true bound on a rank known to be 12 to 14 and simply a
weak one.

What is worth keeping is not the finder: it is the descending schedule, which hands
`find_rank` a bracket needing one refutation instead of one per rank, the zero-term
drop, which is a correctness fix, and the shared base encoding in `decide_rank`.
