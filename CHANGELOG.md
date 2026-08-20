# Changelog

All notable changes to this project are recorded here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the version
numbers follow [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- **Six order-3 fixtures over GF(2), each aimed at a number somebody has
  published**: `gf32_multiplication` and `gf64_multiplication`, extending the
  field-extension family past the three this repository settles itself;
  `cyclic_f2_7`, whose rank `[wang2026]` closes at 13 from both sides; and
  `matmul_2x3x4`, `matmul_3x3x4` and `matmul_3x4x4`, the three small formats that
  paper leaves open. Shapes, naive costs, targets and the two moduli are in
  [`fixtures/published-targets.md`](fixtures/published-targets.md); the bounds are
  pinned on all six in `rank_metric_bound/tests/`, where they cost 4.1 s.

- **A GPU proof of concept for the leaf test**, `gpu_leaf/`, built only where
  `nvcc` is present and called by nothing else. One whole `<4,4,4>` leaf, all
  4 294 836 225 rank-one maps, in **1.019 s** against 1.12 hours for the same leaf
  on one core, with survivor sets compared map for map against the shipped leaf on
  thirteen questions.
- **A device ranking**, [`run_limits/device.h`](run_limits/device.h), in the shape
  `integer_programme/solver_chain.h` already uses for solvers: the order is fixed,
  the availability is not, and an absent backend is a state reported rather than an
  error found downstream. `decide-rank` prints which device would answer.
  **No GPU backend is compiled in**, and it says so.
- **A span held as its rank filtration**,
  [`descent_search/sorted_span.h`](descent_search/sorted_span.h). The leaf test and
  the minimum-weight cost both become dimensions of `R[1] ⊆ … ⊆ R[16]`, so a sort
  over `p^dim` becomes a counting pass over sixteen buckets and the state that
  survives is 24 KB at `<4,4,4>`.
- **The literature the leaf test sits in**, which was cited nowhere: the Segre and
  bounded-rank line, the three MinRank modellings with their Hilbert series, Yang's
  fixed-parameter result, CUDA finite-field elimination, and Heule's SAT benchmark
  suite.

### Changed

- **The leaf of the quotiented search now packs its bits and can be stopped.** It
  called `rank_one_basis_of` with two arguments defaulted, so every leaf there took
  the general Givaro path and no `--leaf-limit` reached it. **25.7x** on
  `matmul_3x3x3 --target 23 --node-limit 300`, at the same 300 nodes.
- **A pool element is formed in bits where no table holds it.** It was rebuilt as a
  Givaro matrix and packed back one field element at a time; the outer product now
  goes straight into words. **5.87x** on the same question with the pool addressed,
  and `gpu_leaf` measures 940.2 ns an element against 129.1 at `<4,4,4>`.
- **The quotient runs on a pool it cannot hold.** Its candidate list was a vector,
  its positions a table and its struck orbits a byte array, all sized by the pool
  and all now arithmetic or a predicate: **34.3 MB less** at `<3,3,3>` at identical
  node counts, and `--symmetry` is no longer refused where the grid does not fit.
- **`[wang2026]` is now cited at the arXiv version its numbers come from**, v10,
  because that preprint's table grew across ten revisions rather than being
  corrected in place, and three of the four bounds quoted from it are absent from
  v1. A citation that names a document not containing the number is the one
  failure [`references.md`](references.md) exists to prevent.
- **`rank(f2_5x5) >= 13` is this repository's own claim now**, not `[bdez2012]`'s.
  Refuting twelve products was recorded as never run and priced at seven hours; it
  is 146 402 553 nodes and ran, and the seven hours was an extrapolation from the
  general leaf's rate that the GF(2) leaf had already made stale.

### Removed

- **Two functions nothing called**, `lowest_rank_partition` and
  `rank_one_maps_within`, the first orphaned when three search routes were retired
  and the second never called at all.

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
- **The Kronecker canonical form of a two-slice tensor**, exactly and in
  polynomial time, with no candidate pool: the minimal indices from the ranks of
  one block system, the elementary divisors from a Smith diagonal over
  `GF(p)[x]` taken forwards and reversed, and three internal counts that must
  agree before anything is returned. It reports a **bound** rather than a rank,
  because Ja'Ja's formula is a theorem over an algebraically closed field and
  falls short over a small one: twelve pencils settled by exhaustion are
  tabulated, three of which it gets wrong.
- **The rank as a factorisation over the canonical basis**, `S = C A` with every
  row of `A` rank one, returned with the receipt that checks it in one matrix
  product without rerunning the search. Two routes, a materialised pool and a
  solver that forms none, required to agree on every fixture.
- **Twelve command-line tools**: `minimise-rank`, `decide-rank`, `walk-scheme`,
  `decide-rank-by-sat`, `list-solvers`, `deflate-strictly`, `enumerate-subspaces`,
  `decide-rank-by-pencil`, `factor-over-canonical-basis`, `curve-bounds`,
  `sparsify-operator` and `make-tensor`, sharing one argument grammar, one clock,
  one set of exit codes and one file of tunables.

### Removed

- **The fixed-`k` finder** and the descending sweep built on it, to the
  `rejected-experiments` branch. The finder rested on an assumed asymmetry
  between the cost of acceptance and refutation, quoted at two orders of
  magnitude; measured with matched flags it is about one, the original figures
  having compared instances encoded with and without symmetry breaking. It is
  dominated by the descent on every fixture. The sweep existed to hand
  `find_rank` a bracket, and once `[yang2025]`'s rank sums closed the
  floor-to-rank gap there was nothing left for a bracket to save: measured, it
  loses on all seven fixtures. No answer either gave was ever wrong. Both sets of
  numbers stay, because they are the evidence for the removal.

[0.1.0]: https://github.com/Tewf/tensor-rank-toolkit/releases/tag/v0.1.0
