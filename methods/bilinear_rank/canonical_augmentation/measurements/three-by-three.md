# The one shape where nothing else answers either

| method | products | cost |
|---|---|---|
| `minimise-rank --steps 3 -s matmul 3 3 3` | 27, the naive cost, no improvement | 4.3 s |
| `walk-scheme --flips 20000 --seeds 8` | **24** | 38.1 s, one worker |

The 38.1 s is one worker, which is the default and which is what it was measured
with. The eight seeds are independent walks sharing nothing, so `--threads` is
free here in a way it is not for `decide-rank`: the schemes, the seed reported and
its flip and reduction counts are bit-identical at 1, 4 and 8 workers, and only
the elapsed figures move. Measured 3.2x at four workers and 5.4x at eight, both on
a machine that was not quiet, so both are floors rather than a claim. The 38.1 s
above is deliberately **not** replaced by a threaded figure: a number measured
under `MEASURING.md` should be superseded by another one, not by a faster run on a
busy machine.
| `find-at-rank --target 23 -s matmul 3 3 3` | nothing, 13 candidates at 60 s each | 780 s |
| `find-at-rank --target 23`, unrestricted | nothing | 300 s |
| `find-at-rank --descend --ceiling 27 -s matmul 3 3 3` | 26, stalling at 25 | 313 s |

Nobody reaches 23. `minimise-rank` cannot move off the naive 27, which is the shortlist
problem again at a larger shape; the flip walk reaches 24, now re-derived on every CI run
in [`../../flip_graph/results.json`](../../flip_graph/results.json); the finder in its
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
- **`minimise_rank` cannot take a first step on `⟨2,2,2⟩`, and two different
  things can.** The shortlist really is 0 of 225, so the greedy stops at the naive
  8, and 7 is the number to beat. The 0.11 s printed here was `walk-scheme`'s and
  not `plateau_search`'s, which is a timing from one tool sitting beside a claim
  about the other. Both do cross, and what each costs is now published rather than
  quoted: [`../../flip_graph/results.json`](../../flip_graph/results.json), which
  `evidence/reproduce/measure.py --check` re-derives.

  The part worth carrying away is what `plateau_search` needs to get there.
  `--plateau 2 --plateau-states 380` reaches 7 having visited 386 subspaces; at
  370 it stays at 8; and at the default budget of 200 000 it reaches the same 7
  over 66 063 subspaces and takes 4.56 s, because it keeps walking long after it
  has seen the best map it will find. The 0.11 s was a real measurement of a real crossing under a state
  budget that was never written down beside it.
