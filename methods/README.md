# methods/

One folder per question. Each member holds its code, its `commands/`, its
`tests/`, and its own measured record, so a strand can be read whole
without leaving it.

In this group:

- [`bilinear_rank/`](bilinear_rank/): the search core, eight
  members under one namespace, its shared vocabulary at the group root.
- [`pencil_rank/`](pencil_rank/): two slices, read off the
  Kronecker canonical form in polynomial time.
- [`canonical_factorisation/`](canonical_factorisation/): the rank
  as S = C A, with a receipt anybody can multiply out.
- [`satisfiability/`](satisfiability/): the rank question put to a
  SAT or SMT solver, refutations checkable as DRAT, a proof of
  unsatisfiability that a program sharing no code with the solver can verify.
- [`matrix_sparsification/`](matrix_sparsification/): fewest
  nonzeros in an operator, proved minimal over every change of basis.
- [`rank_metric_bound/`](rank_metric_bound/): two search-free
  lower bounds read off a slice space.
- [`curve_bounds/`](curve_bounds/): upper bounds from
  interpolation on an algebraic curve.
- [`integer_programme/`](integer_programme/): the LP and ILP layer
  the curve strand and the sparsifier's simplex route both use.

How to use: every strand's README opens with its one command and a run
whose output was produced, not typed;
[`../OPTIONS/one-question-per-command.md`](../OPTIONS/one-question-per-command.md)
says which tool answers which question and why the count is thirteen.
