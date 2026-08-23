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
                         grammar, the stdout/stderr split, removing a scratch
                         file when a run is interrupted, the tunables
tunables.conf            the numbers a run is bounded by, in a file not in code
testing/                 the assertion helper every module's tests use
run_limits/              how much memory, how many cores, and which processor
                         one run may take; adapting-to-the-machine/ audits
                         every strand against all three
search_plan/             the seven choices a run makes about how it will be
                         carried out, in one place that owns no rule of its
                         own; --plan-out writes them down and --plan-in
                         replays them, so a run on other hardware differs in
                         its machine and not in its decisions
descent_search/          rank from above, by descent
exhaustive_search/       rank decided outright, and what that costs
map_construction/        building the maps every method runs on
orbit_reduction/         quotienting all three searches by symmetry
gpu_leaf/                what one consumer GPU is worth on the leaf test,
                         measured; built only where nvcc is present, and called
                         by nothing else here
flip_graph/              moving a decomposition sideways instead of building one
incumbent_search/        the exact search's tree cut by what has been built
                         rather than by a target: upper bounds, and an answer
                         whenever it is stopped
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
fixtures/                the maps and operators everything is run on; its
                         plinopt/ is twelve of PLinOpt's own, under CeCILL-B,
                         so the interoperability is tested against his bytes
famous_tensors/          the tensors the literature argues about, and where each
                         search stops on them
reproduce/               regenerates every published number, with its provenance
references.md            every paper cited anywhere here, by the keys the code uses
how-the-search-works/    the exact search in pseudocode, every parameter, all
                         five pieces composed, and the verdict on wiring each
the-research-front/      where the research front is, and which parts of it
                         are here
positioning/             what this library adds to it, and what it does not
MEASURING.md             how a timing here was taken, and what it does not mean
OPTIONS.md               every flag of every tool, its default, and what
                         measured that default; links OPTIONS/ for the tables
article/                 the write-up: definitions, theorems, proofs, negative results
site/                    the stylesheet, the hand-drawn charts and the shared
                         navigation index.html is assembled from
web_interface/           a browser console for every tool here, and the
                         commands behind it
tools/                   scripts that are not part of the build: one asks every
                         backend the same question and tabulates the cost
```

**Thirteen command-line tools**, and the one question each answers that no other
does: [`OPTIONS/one-question-per-command.md`](OPTIONS/one-question-per-command.md),
which is also where four tempting merges are refused with what each would cost.
It is the list; this file is the folders under it, and neither restates the
other. Three of the folders above hold a binary that is **not** on it:
`measure-leaf`, `price-canonical-route` and `show-limits` are instruments, built
outside a `commands/` because none of them answers a question about a map: the
first two print nanoseconds ([`MEASURING.md`](MEASURING.md)) and the third prints
what this machine and this working directory bound a run to,
and `list-solvers` was a command until 2026-08-21 and now prints the line that
replaced it, `curve-bounds --solvers`.

`find-at-rank` is on the `rejected-experiments` branch. It asked only questions
it expected to be satisfiable, on an assumed asymmetry between acceptance and
refutation that measured as about one, and it is dominated by the descent on
every fixture.

Every paper any of it implements is named once, in
[`references.md`](references.md), and the code cites a key.
