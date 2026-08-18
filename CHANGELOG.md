# Changelog

All notable changes to this project are recorded here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the version
numbers follow [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.0] - 2026-08-18

First public release: a C++20 library on Givaro for exact tensor and bilinear rank,
corrected, tested, and extended into four strands.

### Added

- **Exact linear algebra over `GF(p)` and over `Q`**, templated on the field so
  one implementation serves every strand: dense matrices, a basis in reduced row
  echelon form, exact solve and inverse, rank-one decomposition, the three
  flattenings of a tensor and the rank lower bounds built on them. Nothing is a
  float anywhere.
- **The descent heuristic**, three steps that state separately what each
  guarantees: a matroid greedy, optimal for the basis it picks, then two
  first-improvement passes that guarantee nothing. It recovers the encoding
  operators ⟨L, R, P⟩ from its own answer and writes them out.
- **The complete exhaustive search**, deciding whether a map has an algorithm
  with exactly `k` products by enumerating subspaces rather than subsets. A
  refutation that ran to exhaustion is a lower bound; reaching the node limit is
  a third verdict and is reported as itself.
- **A walk on the flip graph**, moving a working scheme sideways instead of
  building one, which gives upper bounds only.
- **Symmetry and orbit reduction**, quotienting all three searches by the group
  that fixes the target subspace, and orbit cubes for a solver to split on.
- **SAT and SMT encodings of the rank question**: Booleans and parity over
  `GF(2)`, one-hot with the field tables over `GF(p)`, and the equations handed
  straight to cvc5's theory of prime fields. Solvers are run as programs and
  never linked. A refutation can be written as a DRAT proof and checked by
  `drat-trim`, and a proof asked of a solver that writes none is refused rather
  than dropped. Håstad's reduction runs the other way too, turning a 3SAT
  formula into a tensor.
- **Matrix sparsification**, minimising the additions the multiplication count
  never sees: a row-basis heuristic, the article's two exact oracles, a greedy
  scoring `nnz + nns`, and an exact determinant-polynomial test of whether a
  wanted pattern is reachable at all.
- **Bounds from algebraic curves**, the two steps of the Chudnovsky roadmap that
  are integer arithmetic, with the two needing Riemann-Roch spaces left out and
  said to be left out.
- **The integer programme layer** the curve strand hands its step 3 to: exact
  simplex and branch and bound, fixed-column MPS output, and a chain of outside
  solvers whose points are re-checked and whose "infeasible" is never believed.
- **Eight command-line tools**: `minimise-rank`, `decide-rank`, `walk-scheme`,
  `decide-rank-by-sat`, `list-solvers`, `curve-bounds`, `sparsify-operator` and
  `make-tensor`, sharing one argument grammar, one clock, one set of exit codes
  and one file of tunables.

[0.1.0]: https://github.com/Tewf/tensor-rank-toolkit/releases/tag/v0.1.0
