# Asking a SAT solver

`decide-rank-by-sat` states the rank question once and hands it to a program
built for questions of that shape. Precedence and `BILINEAR_TUNABLES`:
[`precedence-and-tunables.md`](precedence-and-tunables.md).

| Flag | Default | What chose the default |
|---|---|---|
| `--target k` | none | Nothing to measure. With no range at all the tool finds the rank. |
| `--from a` / `--to b` | floor, naive ceiling | The floor is measured to pay: `[yang2025]`'s rank sums raised GF(16) from 4 to **8** and cyclic convolution from 5 to **9**, each skipping solver calls that previously cost a minute. |
| `--ceiling N` | naive upper bound | Nothing to measure: an override for a bound the caller already holds. |
| `--solver <name>` | none; `sat_solver_order` decides | **Measured, and the order is the interesting half.** Native XOR clauses are worth nothing on these instances, **1.559 s against 1.563 s** on the same question, while kissat's raw strength is worth several times: **0.339 s** on that question, **34.3 s against 167.9 s** on the next (`../methods/satisfiability/choices/`). Read the ratio with care: the same preference is written up as "five times" in `choices/` and as "**126x**, 0.2866 s against 36.19 s" on GF(16) at 9 products in `results.json`. Those are two questions, not two measurements of one. That 126 was 134 while the kissat end was the 2026-08-16 figure, and the two are one claim: a solver second is another program's, and 6% apart is inside the noise floor. |
| `--tune sat\|unsat` | none | **Measured, and the premise holds: matching helps and mismatching hurts.** kissat's `--sat` is `--target=2 --restartint=50` and `--unsat` the opposite. On the **unsatisfiable** `matmul_2x2x2 --target 6`, where the work is: none **25.20 s**, `--tune unsat` **15.54 s** (1.62x faster), `--tune sat` **34.77 s** (1.38x slower). Both outside the 13% floor. On the satisfiable `--target 7` the direction repeats but the magnitudes do not survive scrutiny: none 0.130 s against `--tune sat` 0.104 s is process start, not a ratio, while `--tune unsat` at 0.284 s is clearly worse. **So the flag pays on refutations, which is where a lower bound comes from and where the time goes.** Note it was silently broken until this was measured: the configuration reached kissat with a leading space and was read as a filename. |
| `--break-symmetry` | off | Worth is measured, the default is not. Ruling out 6 products for `⟨2,2,2⟩`: **24.81 s to 0.339 s, 73x** under kissat, both ends taken on 2026-08-23 and the off end the only run here that asks that question without it. The "at least 76x" in the help text is the weaker cryptominisat pair (**no answer in 120 s** against 1.57 s), which is a lower bound and not a ratio. Off nonetheless, by argument: an over-strong constraint would turn a satisfiable instance unsatisfiable, which is a false lower bound. Soundness is checked against all six fixtures of known rank. |
| `--plain-cnf` | off | Argument: most solvers here have no native XOR, so this is usually what happens whether it is asked for or not. Its cost is the pair above, which found the difference worth nothing. |
| `--backend cnf\|smt` | `cnf` | Argument: cvc5's finite-field solver needs a CoCoALib build that distribution packages have been known to omit, so a solver that is present may still refuse the query. |
| `--proof <path>` | none | Argument: only kissat writes one here, and asking any other solver is refused rather than silently dropped. |
| `--emit-cnf <path>` | none | Nothing to measure: writes the question and stops. |
| `--probe N` | `0`, off | **Unmeasured.** The argument is that cost concentrates just below the rank, so a probe that exhausts a small budget is itself evidence of being there. No table prices it. |
| `--timeout N` | `sat_timeout_seconds`, `300` | **Nothing.** An argument: this machine has 16 GB and long searches share it. |
| `--max-memory` | `sat_memory_megabytes`, `2G` | **Nothing**, same argument. The memory cap matters as much as the clock; a solver that takes the box down has answered nothing. What it caps here is not what it caps on the three searches: [`one-idea-several-spellings.md`](one-idea-several-spellings.md). |
| `--threads N` | `1` | **Nothing measured the default**, which is the reproducibility default `infrastructure/run_limits/parallel.h` sets everywhere. What *is* measured is what the flag buys: `-s matmul` splits the question into one independent solver process per orbit of the first term, and five cubes of `matmul_2x2x2 --target 6` are **3.42x** together (3.36 s sequentially against 0.982 s, ideal 4.25x), recorded in `methods/satisfiability/rank_question.cpp`. Each worker's `--max-memory` is the shared cap divided by the workers, so the aggregate ceiling stays the number the flag names. A refutation is the same at any count; a yes may come back from a different cube. |
| `-s, --symmetry matmul` | `none` | Measured: the `⟨3,3,3⟩` first-term pool collapses from 261 121 to **13 orbits**. GF(2) and matrix multiplication only, and `auto` is refused here because these orbits are the closed-form ones of `⟨n,m,k⟩`. |

## The search schedule, which has no flag

Only one is implemented: linear UNSAT-SAT, walking up from the floor. It was
**measured against four others** on seven fixtures
([`../methods/satisfiability/bracket/`](../methods/satisfiability/bracket/)), and it **lost**
on the only expensive one, coming fourth of five on GF(16): floor 108.461 s,
ascending 112.533, descending 110.421, bisection 113.614, gallop up 110.399,
gallop down **110.094**. The whole choice of schedule is worth about **three
percent** of a run dominated by one 108.2 s question.

It ships anyway, by argument: it never reads the ceiling, so its cost does not
move when the ceiling is loosened.

**That table wants re-running before it is quoted again**, and `bracket/` says
so itself: it was taken when the GF(16) floor was 4, the floor is now 8, and the
schedule that walks furthest is the one that benefits. Two earlier winners here
were published and withdrawn, one of them measured with five stray `cbc`
processes running.
