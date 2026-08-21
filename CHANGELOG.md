# Changelog

All notable changes to this project are recorded here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the version
numbers follow [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- **A predicate that says whether canonical augmentation will pay, before the
  search runs.** `oracle_guided_search/canonical_route_price.h` weighs the node
  saving against the parent test from the characteristic, the shape, the pool
  size, the factored degree, the group order and the generator count.
  [`when-canonical-pays/`](oracle_guided_search/when-canonical-pays/README.md)
  has the derivation, the four operations priced by the new
  `price-canonical-route`, and the ten sweeps it was corrected against. The
  saving side has `[mckay1998, Thm 3]` under it and orbit counting bounding it;
  the price side has `[linton2004]` §3.4 for the canonical image, whose
  candidate bound `k|G|` is the binding one here and whose "polynomial in `n` for
  any `k`" criterion this presentation meets at every shape, and nothing at all
  for the setwise stabiliser, which is GI-hard and measured rather than bounded.

- **`factor-over-canonical-basis --ceiling k`**, so a sweep can stop short of a
  rank neither route can settle, and the run reports its node count and its
  canonical images whether or not it found anything. A refutation sweep used to
  report one sentence and no number, which made the two routes uncomparable at
  every shape above `<2,2,2>`.


- **`--orbit-test full|generators` is documented**, which it was not anywhere
  outside the binary that parses it. It is in
  [`OPTIONS/searching-for-rank.md`](OPTIONS/searching-for-rank.md) with the
  duplication its cheap arm leaves, in
  [`how-the-search-works/parameters.md`](how-the-search-works/parameters.md)
  beside the parameter it moves, and in the browser console's catalogue. The same
  pass added `--anchor` to the parameters page, which had never been there
  either, and `walk-scheme --steps` as the accepted older spelling of `--flips`.

- **Two pages that are about using the twelve rather than about one of them.**
  [`OPTIONS/common-recipes.md`](OPTIONS/common-recipes.md) is the line people
  actually type, one per question, each running against a shipped fixture; and
  [`OPTIONS/one-idea-several-spellings.md`](OPTIONS/one-idea-several-spellings.md)
  is where the twelve disagree with each other — five names for "which machine
  answers", `--node-limit` for four different budgets with defaults 25x apart,
  `--max-memory` in bytes on three tools and megabytes on two, and **five
  different behaviours for a bad enum value across the nine flags that take
  one**. `--orbit-test bogus` names the word and the two right answers;
  `--leaf-route bogus`, four lines away in the same file, says `unrecognised
  option: --leaf-route`; `--anchor` and `--backend` accept anything in silence.
  Nothing was renamed and no behaviour moved: a flag that has been published
  keeps its name, and what was missing was the page saying so.

- **The console teaches the command instead of only running it.** Six worked
  examples in [`web_interface/worked_examples.py`](web_interface/worked_examples.py)
  fill the map, the tool and the flags and stop there, four of them the same map
  one flag apart so that **found**, **proved** and **gave up** are three presses;
  the plan each tool prints — its pool, its leaf, its device, its quotient — is
  lifted out of the stream and put above the verdict by
  [`web_interface/plan_lines.py`](web_interface/plan_lines.py), copied and never
  reworded; and every command shown carries a button that copies it. Each example
  is *run* by the checks rather than read, and the promoted lines are asserted to
  still be the characters the tools print.

### Changed

- **The canonical route stopped scanning the pool once per candidate child, and
  stopped asking the parent test for a minimum it never wanted.** Measured at
  five shapes, seven to fourteen whole pool scans a node were 98% of a canonical
  node at `<2,3,3>`, against 1.0% in the setwise stabiliser and 1.1% in the
  canonical image — the two operations the factored presentation and Linton's
  algorithm exist to make cheap were cheap, and the cost was somewhere else.
  [`pool_cosets.h`](oracle_guided_search/pool_cosets.h) gets every child's
  content from one pass a node by grouping pool residues into cosets of the
  current span, and the parent test now exits at the first candidate parent that
  beats its own rather than naming all of them. `<2,3,3>` at 8 went from **98.0 s
  to 11.4 s**, `<2,2,4>` at 10 from 0.500 s to 0.144 s, and `<3,3,3>` at 10 from
  1.33x faster than the plain route to **2.07x**. Every node count at every shape
  and level is unchanged, which is how it is known that only the clock moved.


- **A budget that ran out no longer shares a colour with a refutation.** The
  console's cards were worded apart and painted alike, so `outcome.py` now
  carries `decides` — **found**, **proved** or **nothing proved** — one branch on
  the exit code, taken before a tool's own badge is applied and never from it.
  Only exit 1 reaches `proved`; a budget, a stop, the wall clock, a crash and a
  status with no name all land in `nothing proved`. It is `decide-rank`'s own
  vocabulary, which prints `FOUND`, `NO` and `GAVE UP`.

- **Three things `web_interface/not-production-ready-yet.md` recorded are
  closed**, and the page now says what closing each one did not reach. A run that
  starts a solver warns *before* it starts, naming the flag and the two numbers,
  where it would leave that solver holding a core after a stop; `serve.py` counts
  and sizes the runs it has accumulated instead of letting them grow unseen; and
  the fixture menu greys the maps a tool cannot read, from the tool's declared
  input rather than from any opinion about the file. The console's catalogue also
  stopped telling four tools that their `--max-memory` came from
  `sat_memory_megabytes`, which reaches only the two that call a solver.

- **Every stale leaf timing quoted outside the leaf's own directory is marked as
  stale rather than replaced.** `OPTIONS/searching-for-rank.md` quoted 129.1 ns a
  scanned element and 78 ns a walked one to convert `--leaf-limit` into seconds,
  and four ratios between the routes; `how-the-search-works/README.md` quoted 9.2
  minutes of one core for a `⟨4,4,4⟩` leaf, which was those same 129.1 ns times
  4 294 836 225. All were taken against the leaf as it stood before 2026-08-20.
  None has a measured replacement here and none is invented: what each was
  measured against is now written beside it, as
  [`positioning/hardware-and-parallelism.md`](positioning/hardware-and-parallelism.md)
  already does. The `-s` and `--general-leaf` rows moved to the node counts and
  the upper bounds that survive the change, since a node count is not a property
  of the leaf.

- **The front page describes the tool as it now is.** It said the rank of
  `⟨2,2,2⟩` was "decided in half a second", which was a wall clock from before
  the leaf moved twice, and it priced the orbit quotient at a wall-clock 28x from
  the same era: both are now the counts, which reproduce anywhere. The leaf's two
  rewrites and the browser console — neither of which the page mentioned — are on
  it, and it is still under its own length.

- **The GF(2) leaf's pool scan carries a residual instead of reducing every
  element.** Reduction modulo a subspace is linear, so `reduce(x ^ d)` is
  `reduce(x) ^ reduce(d)`; the pool is the outer-product grid, so for a fixed
  left mask two elements whose right masks differ in one bit differ by one of
  only `columns` patterns. Reducing those once per left leaves one exclusive-or
  and one zero test an element, the element is never formed, and the dimension
  leaves the inner loop entirely. Survivors are collected in Gray order, sorted
  per left and only then offered to the greedy, so the answer is bit-identical
  rather than merely equivalent — the same argument
  [`gpu_leaf/why-the-answer-is-the-same.md`](gpu_leaf/why-the-answer-is-the-same.md)
  already makes for the kernel. The vector lists are now built whether or not the
  packed table fits, since they cost 2 KB at 9x9 and were the only thing keeping
  this off the shapes anyone runs.

- **Two published figures were withdrawn as artefacts rather than measurements.**
  `canonical-augmentation.md` read a two-point line as a 0.196 s presentation fee
  when the two points sit at different targets, where a node costs seventeen
  times more; the presentation measures 0.013 s. And
  `comparing-against-the-baseline.md` published the baseline's deepest `work`
  level as if it were the array's sum, making the agreement look like 0.8% when
  it is 0.106%. Both corrections moved this repository's own numbers, not
  somebody else's.

- **The GPU harness is compiled on every build and linked on none.** It had not
  compiled since `Gf2Leaf` gained a `field` argument, on any machine, with the
  suite green throughout, because the directory was only read where the CUDA
  toolkit was found and no machine that builds this has `nvcc` on its PATH. A
  directory nothing compiles is a directory that drifts, and its numbers were
  being quoted meanwhile.

- **The general-field leaf decides rank one without computing a rank**, and its
  per-element path now allocates nothing at all. `linear_algebra::is_rank_one` in
  [`linear_algebra/measures.h`](linear_algebra/measures.h) takes a pointer and a
  shape rather than a `Matrix`, finds the first nonzero row, and cross-multiplies
  every later row against it — `a[p]·b[j] = b[p]·a[j]`, which is "is a multiple
  of" with the division cleared, so no modular inverse is taken and the answer
  returns at the first entry that disagrees. Against
  [`rank`](linear_algebra/measures.h), which is `O(r·d·c)` and builds a
  `SpanBasis`, copies every row into it and runs the elimination to the last row
  of a question settled at the second, this is `O(r·c)` in `Θ(1)` space.
  `keep_if_rank_one` in
  [`exhaustive_search/subspace_walk.cpp`](exhaustive_search/subspace_walk.cpp)
  now tests the combination buffer the walk already carries and forms a `Matrix`
  only for an element it keeps, which almost none are. No figure is claimed for
  it here; the timing is taken separately, under
  [`MEASURING.md`](MEASURING.md).

- **Same verdicts, same counts.** `is_rank_one` is `rank == 1` and is held
  against it over every 2x2, 2x3 and 3x2 matrix over GF(2), GF(3) and GF(5),
  exhaustively, plus a sample at 3x3 over GF(5), in
  [`exhaustive_search/tests/test_rank_one_predicate.cpp`](exhaustive_search/tests/test_rank_one_predicate.cpp).
  It is the predicate
  [`gf2_is_rank_one`](linear_algebra/gf2_bits.h) has always applied over GF(2),
  where the only nonzero scalar is 1 and the cross-multiplication degenerates to
  a word comparison; the two must agree about what rank one is, because the
  search sends a leaf down one path or the other on the characteristic alone.

- **The general-field leaf walks its subspace in base-`p` reflected Gray code
  order**, so an element costs one row added or subtracted rather than a rebuild
  from its base-`p` digits: `O(width)` field additions and no multiplication, in
  place of `O(dim * width)` multiply-accumulates. The successor is Knuth's
  Algorithm H, `[knuth2011]` §7.2.1.1, which names the digit to move in constant
  time, in
  [`exhaustive_search/reflected_gray_walk.h`](exhaustive_search/reflected_gray_walk.h);
  the leaf itself moved into
  [`exhaustive_search/subspace_walk.h`](exhaustive_search/subspace_walk.h).
  Measured on one core at **2.52x** an element over GF(3) at 3x6 dimension 12,
  **1.74x** over GF(5) and **1.58x** over GF(7), and the `dim` term is gone
  rather than reduced: the rebuild rises 32 ns an element per dimension added
  and the walk does not move. GF(2) is untouched and keeps its own packed walk
  in [`gf2_leaf.h`](exhaustive_search/gf2_leaf.h).

  **The order changed, so the leaf can hand back a different rank-one basis.**
  Not a different verdict: the leaf answers whether `needed` independent
  rank-one maps exist, which no reordering moves, and nothing asserted anywhere
  pins a decomposition. The route it replaced is kept beside it and the two are
  run against each other on the same span at every odd characteristic the
  fixtures carry.

### Added

- **`fixtures/f5_3x3.tensor`**, polynomial multiplication of 3 coefficients by 3
  over GF(5), and the third characteristic in that directory. It ships for a
  test rather than for a number: the general-field leaf walks a subspace by its
  base-`p` digits and every other fixture is over GF(2) or GF(3), so a walk right
  at `p = 3` and wrong at `p = 5` had nothing to fail against. Its rank is 5,
  which `decide-rank` closes in one node.
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
- **`--leaf-route auto|scan|walk`**, so the two ways a leaf can be answered can
  be timed against each other on one question rather than on two. The rule that
  chooses between them compares `p^dim` with the pool size, pricing a membership
  test and a rank test the same, and nothing had ever checked that. Forced onto
  each route in turn it is right on all four questions timed, including one where
  it sends a 225-element pool to the subspace walk and the walk wins, so **the
  cost-weighted rule this was meant to become is not made**:
  [`exhaustive_search/which-leaf-route-is-cheaper.md`](exhaustive_search/which-leaf-route-is-cheaper.md).
- **A check that the front page is the results files**,
  [`reproduce/front_page.py`](reproduce/front_page.py), run in CI. `index.html`
  cited `descent_search/results.json` for two charts and a table and read it
  never; the charts fetch it now, and the table, which stays hand-typeset so the
  evidence survives JavaScript being off, is refused by CI when it drifts.
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
