# A Las Vegas route: local search on the GF(2) encodings

**What this route is for.** It is not the main line of the research. It is a
tool for finding decompositions, which is to say for establishing upper bounds
on rank and lowering them, and it will be used that way. A solver that can only
find never refutes, so it never touches a lower bound: a sweep under one can
raise the ceiling and nothing else.

**The question.** Can a solver that can only find, a stochastic local search
with restarts, right whenever it answers and random in how long it takes, find
a rank-`r` decomposition on this module's GF(2) encodings, and at what cost
against kissat on the same file, on the fixtures of known rank and on Heule's
`⟨3,3,3⟩` instances?

**Why it is worth asking.** `[heule2019]` found new `⟨3,3,3⟩` schemes with
exactly such a solver, yalsat, and wrote that "local search SAT solvers
outperform CDCL solvers consistently in this application"; the bibliography
here named that as the standard this module should be tested against and had
never been. The field also records the opposite for parity constraints, where a
walk stalls and elimination wins, and a GF(2) tensor equation is a parity. The
record on Heule's own instances reconciles the two: xnfSAT, `[nawrocki2021]`,
is yalsat with the parity kept as a parity inside the flip loop, and it beats
every CNF expansion on every instance, the linear 3-cut that `--plain-cnf`
writes being the worst of them. **On Brent equations the useful ingredient in
the literature is native XOR**, and the question here is whether that carries
over to this module's own formula. The claims, the baseline they name, and what
could not be found: [`what-the-field-says.md`](what-the-field-says.md).

## What was built

A class of solvers that can only answer yes, `SatSolver::finds_only`: `yalsat`,
`xnfsat`, `probSAT` and `multilinear-sat`, pinned with `--solver` by name or by path,
`--proof` refused and a `s UNSATISFIABLE` from them discarded, so a sweep under one can
raise the upper bound and nothing else; `--emit-xnf` for the file xnfsat reads; the tests
that need no solver and the ones gated on a real one. Piece by piece, with what each run
established: [`what-was-built.md`](what-was-built.md).

## How it was measured

[`measure.py`](measure.py) writes [`results.json`](results.json), one entry per run with
the command that produced it, and [`results.md`](results.md), the table read off it.
How the protocol of [`../../MEASURING.md`](../../MEASURING.md) was adapted to a
randomised solver, what the caps and seeds were, how a find on Heule's instances is
checked, and the one departure from the protocol, stamped in the file:
[`how-it-was-measured.md`](how-it-was-measured.md).

## The table

Every run, with its seed and command, is in [`results.json`](results.json);
all eighteen fixture rows and the ten challenge rows are in
[`results.md`](results.md). A cell is *seeds finished within the cap / seeds,
mean seconds over those that finished*; kissat is the fastest of three.

**The fixtures, 60 s cap, `--break-symmetry` off.** The five controls
(`f2_2x2`, `w_state`, `gf4`, `gf8`, `f2_2x3`) are found by yalsat, xnfsat and
probSAT on every seed in under a second, and every one of them is a question
kissat answers in under 15 ms; the continuous control finds three of the five.

| fixture | find | kissat | xnfsat, XNF | yalsat, 3-cut CNF | probSAT | multilinear-sat |
|---|---|---|---|---|---|---|
| `matmul_2x2x2` | 7 | **0.129 s** | 0/5 | 1/5, that seed 0.13 s | 0/3 | 0/3 |
| `cyclic_f2_7` | 13 | unknown at 60 s | 0/5 | 0/5 | 0/3 | 0/3 |
| `f2_5x5` | 14 | unknown at 60 s | 0/5 | 0/5 | 0/3 | 0/3 |
| `matmul_3x3x3` | 23 | unknown at 60 s | 0/5 | 0/5 | 0/3 | 0/3 |

With `--break-symmetry` on, the same four rows are 0/5 and 0/3 for every walk,
yalsat's one lucky seed included, while kissat goes from 0.129 s to 0.207 s on
`⟨2,2,2⟩`; on the controls the ordering costs the walks a factor of two to
thirty where it costs kissat nothing. Measured, as the brief asked, and it
does hurt.

On the three rows where kissat is unknown at 60 s nobody finished, so those
rows say only that a walk did not win in a minute where kissat did not either;
a 300 s rerun of kissat's six cells was queued and did not run
([`how-it-was-measured.md`](how-it-was-measured.md)).

**Heule's ten challenge-1 instances.** What this branch holds is a 5 s, one-seed smoke
of the driver and not the paper's table: xnfsat three of ten within five seconds, the
0.03 s on `4-4-4-4-1` agreeing with the paper's 0.1 s; yalsat one; kissat and the
controls none. The rows, and why the full run is a command to run at night:
[`heule-instances.md`](heule-instances.md).

## Verdict

1. On this module's own GF(2) encodings a stochastic local search, native XOR
   included, ties kissat only where everything is instant: the five controls
   are found on every seed in under a second and by kissat in under 15 ms.
2. It loses the moment the question has any depth. `matmul_2x2x2` at 7 is 0/5
   for xnfsat and 0/3 for the controls at 60 s against kissat's 0.13 s, and
   yalsat's one seed in five that found Strassen in 0.13 s is what a Las Vegas
   distribution looks like from its lucky tail; the three larger fixtures are
   0/5 everywhere and unknown for kissat at the same cap. `--break-symmetry`
   hurts every walk and costs kissat nothing.
3. On Heule's instances the same xnfsat, on the XNF `cnf2xnf` recovers, finds
   three of ten within five seconds on one seed where kissat finds none, and
   its 0.03 s on `4-4-4-4-1` is the paper's 0.1 s. So the difference between
   their instances and this module's `⟨3,3,3⟩` at 23 is the formula, their
   hardcoded pairings of the type-3 terms and streamlining, and not the solver.
4. The continuous control fails as `[jia2004]` and `[haanpaa2006]` predict of
   a walk on CNF-encoded parity, and it cannot be handed the parities until it
   supports XOR natively, which is the route's open item
   ([`not-built.md`](not-built.md)).
5. So this route is what its first paragraph says: a tool for upper bounds on
   the formulas the literature has already shaped for it, never a refutation,
   and on this module's plain formula not yet a tool at all.

## What was deliberately not built

A portfolio mode, flip counts as the machine-independent cost, and the streamlining
and neighbourhood search that make yalsat productive in `[heule2019]`: each with its
reason in [`not-built.md`](not-built.md).
