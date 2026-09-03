# Where the mechanism does show something

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

The same per-candidate pattern, from the tool that replaced the finder:

```sh
decide-rank-by-deflation evidence/fixtures/matmul_2x2x2.tensor --target 6 -s matmul 2 2 2
  candidate 0: refuted, 0.721823 s
  candidate 1: refuted, 0.644588 s
  candidate 2: refuted, 0.720952 s
  candidate 3: refuted, 0.48914 s
  candidate 4: refuted, 0.591016 s
k = 6: every candidate refuted, so the rank is above 6, 3.17116 s, kissat
```
