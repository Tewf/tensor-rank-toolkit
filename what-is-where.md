# What is where

Five groups, each answering one question about the repository, and inside
them one folder per subject. A folder with something to say carries its own
`README.md` with its contents and one worked example; one without says its
purpose at the top of the file that defines it.

```
core/                    what everything stands on
  linear_algebra/          exact arithmetic over GF(p) and over Q
  formats/                 tensor, dense matrix, SMS, DIMACS and SMT-LIB files

methods/                 one folder per question
  bilinear_rank/           the search core, one namespace and its home: the
                           rank-one pool and its addressed odometer, the
                           reflected Gray order, the decomposition <-> (L,R,P)
                           recovery with operators-to-tensor, and under it
    greedy_heuristic/        rank from above, the matroid-greedy heuristic
    exhaustive/              rank decided outright, and what that costs
    branch_and_bound/        the same tree, cut by the incumbent's cost
    canonical_augmentation/  each class exactly once, no memory, [mckay1998]
    orbit_reduction/         quotienting the searches by symmetry
    flip_graph/              moving a decomposition sideways, [kauers2023]
    map_construction/        building the maps every method runs on
    search_plan/             the choices a run records and replays
  pencil_rank/             two slices, read off the Kronecker form
  canonical_factorisation/ the rank as S = C A, with the receipt
  satisfiability/          the same question, to a SAT or SMT solver
  matrix_sparsification/   fewest nonzeros in an operator
  rank_metric_bound/       two search-free lower bounds
  curve_bounds/            bounds from interpolation on a curve
  integer_programme/       the LP and ILP layer the curve strand and the
                           sparsifier's simplex route both use

infrastructure/          how a run happens, bounded
  cli/                     the shared grammar, exit codes, report discipline
  run_limits/              memory, cores, device, and the card-failure note
  testing/                 the one assertion helper
  gpu_leaf/                the card, priced against the host it would replace
  tools/                   scripts outside the build

evidence/                what is claimed, and how a reader re-derives it
  fixtures/                the maps and operators; plinopt/ under CeCILL-B
  benchmark_tensors/       the tensors the literature argues about, and the
                           one owner of what is known about each
  reproduce/               measure.py and the guards CI runs

writeup/                 the argument, as opposed to the machinery
  article/                 definitions, theorems, proofs, negative results
  how-the-search-works/    the exact search's method, whole
  positioning/             what this library adds, and what it does not
  the-research-front/      where the field stands

web_interface/           a browser console for the tools, stdlib only
site/                    the stylesheet, charts and shared navigation
OPTIONS.md + OPTIONS/    every flag, its default, and what measured it
MEASURING.md             how a timing was taken, and what it does not mean
references.md            every paper cited anywhere here, by the keys the
                         code uses
start-here.md            a first session in plain words
```

**Thirteen command-line tools**, and the one question each answers that no
other does: [`OPTIONS/one-question-per-command.md`](OPTIONS/one-question-per-command.md),
which also refuses four tempting merges with what each would cost. Three
binaries are instruments, not tools: `measure-leaf`, `price-canonical-route`
and `show-limits` print facts about one machine and one working directory,
build outside any `commands/`, and deliberately do not install.

Two commands changed their names on 2026-09-03 and two more folders their
homes; the retired spellings and what replaced them are recorded in
[`OPTIONS/one-idea-several-spellings.md`](OPTIONS/one-idea-several-spellings.md).

Every paper any of it implements is named once, in
[`references.md`](references.md), and the code cites a key.
