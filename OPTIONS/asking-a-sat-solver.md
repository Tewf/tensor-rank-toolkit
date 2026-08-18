# Asking a SAT solver

`decide-rank-by-sat` states the rank question once and hands it to a program
built for questions of that shape. Precedence and `BILINEAR_TUNABLES`:
[`../OPTIONS.md`](../OPTIONS.md).

| Flag | Default | What chose the default |
|---|---|---|
| `--target k` | none | Nothing to measure. With no range at all the tool finds the rank. |
| `--from a` / `--to b` | floor, naive ceiling | The floor is measured to pay: `[yang2025]`'s rank sums raised GF(16) from 4 to **8** and cyclic convolution from 5 to **9**, each skipping solver calls that previously cost a minute. |
| `--ceiling N` | naive upper bound | Nothing to measure: an override for a bound the caller already holds. |
| `--solver <name>` | none; `sat_solver_order` decides | **Measured, and the order is the interesting half.** Native XOR clauses are worth nothing on these instances, **1.559 s against 1.563 s** on the same question, while kissat's raw strength is worth several times: **0.31 s** on that question, **34.2 s against 167.9 s** on the next (`../satisfiability/choices/`). Read the ratio with care: the same preference is written up as "five times" in `choices/` and as "**134x**, 0.270 s against 36.19 s" on GF(16) at 9 products in `results.json`. Those are two questions, not two measurements of one. |
| `--tune sat\|unsat` | none | **Nothing. Unmeasured.** kissat's `--sat` is `--target=2 --restartint=50` and `--unsat` is `--stable=0`; a sweep does know which way each question leans, so the flag ought to pay. No table says whether it does, and the help text says so. |
| `--break-symmetry` | off | Worth is measured, the default is not. Ruling out 6 products for `⟨2,2,2⟩`: **24.71 s to 0.311 s, 79x** under kissat. The "at least 76x" in the help text is the weaker cryptominisat pair (**no answer in 120 s** against 1.57 s), which is a lower bound and not a ratio. Off nonetheless, by argument: an over-strong constraint would turn a satisfiable instance unsatisfiable, which is a false lower bound. Soundness is checked against all six fixtures of known rank. |
| `--plain-cnf` | off | Argument: most solvers here have no native XOR, so this is usually what happens whether it is asked for or not. Its cost is the pair above, which found the difference worth nothing. |
| `--backend cnf\|smt` | `cnf` | Argument: cvc5's finite-field solver needs a CoCoALib build that distribution packages have been known to omit, so a solver that is present may still refuse the query. |
| `--proof <path>` | none | Argument: only kissat writes one here, and asking any other solver is refused rather than silently dropped. |
| `--emit-cnf <path>` | none | Nothing to measure: writes the question and stops. |
| `--probe N` | `0`, off | **Unmeasured.** The argument is that cost concentrates just below the rank, so a probe that exhausts a small budget is itself evidence of being there. No table prices it. |
| `--timeout N` | `sat_timeout_seconds`, `300` | **Nothing.** An argument: this machine has 16 GB and long searches share it. |
| `--max-memory` | `sat_memory_megabytes`, `2G` | **Nothing**, same argument. The memory cap matters as much as the clock; a solver that takes the box down has answered nothing. |
| `-s, --symmetry matmul` | `none` | Measured: the `⟨3,3,3⟩` first-term pool collapses from 261 121 to **13 orbits**. GF(2) and matrix multiplication only, and `auto` is refused here because these orbits are the closed-form ones of `⟨n,m,k⟩`. |

## The search schedule, which has no flag

Only one is implemented: linear UNSAT-SAT, walking up from the floor. It was
**measured against four others** on seven fixtures
([`../satisfiability/search/`](../satisfiability/search/README.md)), and it **lost**
on the only expensive one, coming fourth of five on GF(16): floor 108.461 s,
ascending 112.533, descending 110.421, bisection 113.614, gallop up 110.399,
gallop down **110.094**. The whole choice of schedule is worth about **three
percent** of a run dominated by one 108.2 s question.

It ships anyway, by argument: it never reads the ceiling, so its cost does not
move when the ceiling is loosened.

**That table wants re-running before it is quoted again**, and `search/` says
so itself: it was taken when the GF(16) floor was 4, the floor is now 8, and the
schedule that walks furthest is the one that benefits. Two earlier winners here
were published and withdrawn, one of them measured with five stray `cbc`
processes running.
