# What is where

One folder per subject, grouped by what it serves rather than by file type. A
folder with something to say carries its own `README.md`; one without says its
purpose at the top of the file that defines it: its `CMakeLists.txt` where it
builds something, and the script's or stylesheet's own header where it does not.
Each method folder holds the code, its `tests/`, and where it has an entry point
a `commands/`.

```
linear_algebra/          exact arithmetic over GF(p) and over Q, shared by everything
formats/                 tensor, dense matrix, SMS, DIMACS and SMT-LIB files
cli/                     what every command shares: clock, exit codes, argument
                         grammar, the stdout/stderr split, the tunables
tunables.conf            the numbers a run is bounded by, in a file not in code
testing/                 the assertion helper every module's tests use
run_limits/              how much memory and how many cores one run may take
descent_search/          rank from above, by descent
exhaustive_search/       rank decided outright, and what that costs
map_construction/        building the maps every method runs on
orbit_reduction/         quotienting all three searches by symmetry
flip_graph/              moving a decomposition sideways instead of building one
oracle_guided_search/    fixed-k search, tree refutation, canonical augmentation
canonical_factorisation/ the rank as A B, with the receipt that checks it
pencil_rank/             two slices, where the answer is read off a canonical
                         form instead of searched for
rank_metric_bound/       two lower bounds from the dimension and the least rank
                         of a slice space, and from nothing else
matrix_sparsification/   fewest nonzeros in an operator
satisfiability/          the same rank question put to a SAT or SMT solver
curve_bounds/            bounds from interpolation on an algebraic curve
integer_programme/       the linear and integer programme layer the curve strand uses
fixtures/                the maps and operators everything is run on
famous_tensors/          the tensors the literature argues about, and where each
                         search stops on them
reproduce/               regenerates every published number, with its provenance
references.md            every paper cited anywhere here, by the keys the code uses
state-of-the-art/        where the research front is, and which parts of it are here
positioning/             what this library adds to it, and what it does not
MEASURING.md             how a timing here was taken, and what it does not mean
OPTIONS.md               every flag of every tool, its default, and what
                         measured that default; links OPTIONS/ for the tables
article/                 the write-up: definitions, theorems, proofs, negative results
site/                    the stylesheet, the hand-drawn charts and the shared
                         navigation index.html is assembled from
tools/                   scripts that are not part of the build: one asks every
                         backend the same question and tabulates the cost
```

**Twelve command-line tools.** Three ask how few multiplications a map needs and
disagree about what they can prove: `minimise-rank` (descent), `decide-rank`
(complete), `walk-scheme` (a walk that moves sideways). `decide-rank-by-sat` puts
that question to somebody else's solver and `list-solvers` says which backends
this machine has. `deflate-strictly` refutes a committed candidate from the tree
rather than from a solver, and `enumerate-subspaces` counts solution subspaces
once per orbit. `decide-rank-by-pencil` reads the answer off a canonical form for
two slices and searches for nothing at all, and `factor-over-canonical-basis`
returns the rank as the factorisation it is, with a receipt anybody can multiply
out. `curve-bounds` bounds the rank from a curve's points rather than searching.
Then `sparsify-operator` for the other strand, and `make-tensor` to build a map
to run any of them on.

A thirteenth, `find-at-rank`, is on the `rejected-experiments` branch. It asked
only questions it expected to be satisfiable, on an assumed asymmetry between
acceptance and refutation that measured as about one, and it is dominated by the
descent on every fixture.

Every paper any of it implements is named once, in
[`references.md`](references.md), and the code cites a key.
