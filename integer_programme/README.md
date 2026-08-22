# Constrained minimisation

A linear objective under linear constraints, with some variables required to be
whole, and whichever solver this machine has to answer it.

## Whose work this is

None of the mathematics is new here. Keys are
[`../references.md`](../references.md), which says for each what was read.

| Result | What rests on it |
|---|---|
| `[dantzig1951, Ch. XXI]` | the simplex method, and the standard form it wants |
| `[dantzig1955, p. 193]` | phase I and phase II as two explicit phases |
| `[bland1977, Thm. 1.1]` | that the smallest-subscript pivot cannot cycle, which is the only reason the built-in terminates |
| `[landdoig1960, §3]` | branch and bound on a linear relaxation |
| `[dakin1965, (6)-(7)]` | the `x ≤ ⌊v⌋` / `x ≥ ⌈v⌉` dichotomy, which is *not* Land and Doig's equality branching |
| `[oslmps]` | the fixed-column record layout and the integer markers, in IBM's own words; `[mps360]` and `[murtagh1981]` name the origin, and neither was read |

Several things here minimise something subject to constraints and used to do it
by enumeration. The curve strand's interpolation bound is now handed here
instead, through [`../curve_bounds/interpolation_by_solver.h`](../curve_bounds/interpolation_by_solver.h),
and it keeps its own enumeration as the fallback and the cross-check. The
sparsification strand now calls it: `matrix_sparsification/lightest_vector_by_simplex.cpp`
asks one continuous programme per coordinate and was the first caller to want a
relaxation rather than an integer answer.

**The rank question is not one of them, and was.** Brent's equations were written
here as a MILP so a third instrument could answer beside the SAT solvers and the
tree search; it was measured, it lost by two to three orders of magnitude, and it
is retired. The numbers and the argument:
[`../the-research-front/rank-as-a-milp.md`](../the-research-front/rank-as-a-milp.md).
So this folder is a layer the
curve strand uses and not a strand of its own.

## The chain

    gurobi  →  cbc  →  glpk  →  lp_solve  →  built-in

The ranking is fixed; what is installed is not. A machine with nothing gets the
built-in and a slower answer, a machine that acquires a Gurobi licence tomorrow
uses it without a line changing, and `curve-bounds --solvers` says which is
which — it was the command `list-solvers` until 2026-08-21, and that spelling now
prints this line and stops:

    $ ./build/curve_bounds/curve-bounds --solvers

Backends are found on `PATH` at run time and never linked, which is how the
satisfiability strand already treats `kissat` and `cvc5`. The tree therefore
builds identically everywhere and the choice stays a run-time one.

## What is believed, and from whom

An outside solver reports in decimal, so what comes back from one is a **point,
not a result**. It is read onto exact rationals, whole variables are put back on
the integers their decimals were a rendering of, and the whole thing goes through
`satisfies` before anyone sees it. The objective is then recomputed from the
model rather than taken from what the solver printed.

**A `no` is only ever believed from the built-in.** A point can be checked and
is; a claim that no point exists cannot be, so `solve` treats that claim as one
more backend declining to answer and carries on down the chain. Proving a
programme infeasible is therefore as slow as the exact solver, deliberately: a
false `Infeasible` is the one answer here that nothing downstream would catch.

## The built-in

Two-phase simplex under Bland's rule over the relaxation, then branch and bound
on a variable the relaxation left fractional. Exact rationals throughout, so
there is no tolerance to tune and no degenerate pivot that is only degenerate to
fifteen digits. It is the slowest backend and the only one that never has to be
believed, which is why it is both the last resort and the arbiter.

`node_limit` bounds the tree. Reaching it returns `Exhausted` carrying the best
point found, which bounds the optimum without proving it; `Optimal` is a proof.
## What the file format cost

MPS is fixed-column, and the integer markers go in fields 3 and 5, not 4 and 6
where a whitespace-token reading puts them; settled against IBM's own text now
rather than against the solvers: [`the-file-format.md`](the-file-format.md).

## Verified here

`test_optimisation` runs one battery through every installed backend and the
built-in, and they must agree. On this machine that is CBC 2.10.11, GLPK 5.0 and
lp_solve 5.5. **The Gurobi recipe follows its documented `ResultFile` output and
is unverified**, there being no licence here to test it against.
