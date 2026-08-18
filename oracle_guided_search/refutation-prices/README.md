# What a pinned refutation costs, and which mitigation paid

Measured 2026-08-17, one build, the build lock held for the whole run. Fixture
`⟨2,2,2⟩` over GF(2), five orbit candidates, question `rank <= 6`, which is the
refutation the strict test needs. Solver kissat throughout.

- [The baseline on record does not reproduce](the-baseline.md), which is where
  this began.
- [The mitigations](the-mitigations.md), each priced against the one above it
  rather than against the baseline.
- [Why the tree wins, and where it stops](why-the-tree-wins.md).
- [Mitigations not built](not-built.md), with the reason each was left.
