# What a pinned refutation costs, and which mitigation paid

Measured 2026-08-17, one build, the build lock held for the whole run. Fixture
`⟨2,2,2⟩` over GF(2), five orbit candidates, question `rank <= 6`, which is the
refutation the strict step needs. Solver kissat throughout.

The tree route on that same question, run just now from `build/` against the
shipped fixture:

```
$ build/methods/bilinear_rank/canonical_augmentation/deflate-strictly fixtures/matmul_2x2x2.tensor \
    --target 6 --refuter tree -s matmul 2 2 2 --break-symmetry
  candidate 0: refuted, 0.000661804 s, 45 nodes
  candidate 1: refuted, 0.000635864 s, 45 nodes
  candidate 2: refuted, 0.000644546 s, 72 nodes
  candidate 3: refuted, 0.000633262 s, 72 nodes
  candidate 4: refuted, 0.000620161 s, 45 nodes
  k = 6: every candidate refuted, so the rank is above 6, 0.00341727 s, quotiented tree
```

The node counts, 45 and 72, are exact and reproduce run to run; the timings are
not pinned anywhere in this folder and should be read the way the rest of it
reads timings, as an order of magnitude rather than a fixed figure.

- [The baseline on record does not reproduce](the-baseline.md), which is where
  this began.
- [The mitigations](the-mitigations.md), each priced against the one above it
  rather than against the baseline.
- [Why the tree wins, and where it stops](why-the-tree-wins.md).
- [Mitigations not built](not-built.md), with the reason each was left.
