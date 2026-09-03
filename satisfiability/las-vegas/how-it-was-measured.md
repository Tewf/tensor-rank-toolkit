# How a Las Vegas cost was measured

[`measure.py`](measure.py) asks every question of every solver and writes
[`results.json`](results.json), one entry per run with the command that produced
it, and [`results.md`](results.md), the table read off it. The protocol of
[`../../MEASURING.md`](../../MEASURING.md) is adapted to a randomised solver: a
cost is a distribution, so each cell is **seeds finished within the cap / seeds,
mean seconds over those that finished**, which is the measure `[nawrocki2021]`
publishes, with the median over all seeds kept in the JSON and stated as above
the cap when half or more of the seeds hit it. kissat is the fastest of three
deterministic runs on the same file.

**Solvers and files.** xnfsat and yalsat get five seeds, xnfsat on the XNF
(`--emit-xnf`'s parities as `x` lines) and yalsat on the 3-cut CNF, which is
the pair `[nawrocki2021]` compares. probSAT and multilinear-sat are the two
control rows the field already predicts, on the 3-cut CNF at three seeds, the
continuous one pinned to one thread since its CPU backend otherwise takes twelve.

**Fixtures.** Nine questions through `decide-rank-by-sat --target k --solver`:
five controls every solver answers in milliseconds and the four the brief names,
each with and without `--break-symmetry`, since the term ordering was measured
for kissat and not for a walk, at a 60 s cap. `matmul_3x3x3` at 23 is this
module's own `⟨3,3,3⟩` formula: 19 251 variables against the 26 541 of a
challenge formula, no streamlining and no hardcoded pairing of the type-3 terms.

**Heule's ten challenge-1 instances** are their own CNF, so they are run
directly, and xnfsat is handed the XNF `cnf2xnf` recovers from each, exactly as
in the paper. Every find is checked against the file's own clauses and parities
rather than taken on the solver's word. The instances live in the ignored
`build/third_party/` and are never committed: no licence could be found for
them.

**What was run on them, and what was not.** A five-second, one-seed smoke of
the driver, run by hand to check the command, is what `results.json` holds and
the README labels as such. The full run, five seeds for xnfsat and yalsat, one
for the controls and kissat, a 300 s cap and three jobs, was started at 11:19
and stopped by hand at 12:16 on its third instance of ten, after the chassis
reached 96 C three times; the two 300 s control rows per instance were most of
the load, and the controls are already priced on the fixtures. Its partial
results were not written. The paper's 192 runs at 1000 s on a Xeon are not
reproduced here. To run it unattended, at night, from the worktree root:

```sh
flock /tmp/bilinear-measure.lock python3 satisfiability/las-vegas/measure.py \
    --build build --only challenges \
    --challenges build/third_party/matrix-challenges/challenge1 \
    --challenge-seeds 5 --control-seeds 0 --challenge-timeout 300 --jobs 2 \
    --cnf2xnf build/third_party/cnf2xnf/cnf2xnf \
    --yalsat build/third_party/yalsat/yalsat --xnfsat build/third_party/xnfsat/xnfsat \
    --probsat <path-to>/multilinear-sat/benchmark/third_party/probSAT/probSAT \
    --multilinear-sat <path-to>/multilinear-sat/build/multilinear-sat
```

`--control-seeds 0` drops the two control rows, `--jobs 2` keeps the chassis
under its throttle point, and the worst case is ten instances at eleven runs of
300 s each over two jobs, about four and a half hours. It rewrites the
`challenges` section and its provenance block and keeps the fixtures.

**One departure from the protocol, stamped in the file.** The sweep ran three
solver processes at once (`--jobs 3`, on a 12-thread chassis), because the
sequential worst case was several hours and the reading that matters is whether
a seed finishes within the cap at all. The seconds beside a finish carry the
chassis's thermal band and are quoted as orders of magnitude, never as a
two-digit ratio.

**What the machine was doing, read off the sensors by hand.** During the
fixture sweep the CPU package sat between 84 and 95 C and touched the 95 C
throttle point twice, the clock averaging 2.3 GHz when read, with three solver
jobs and, for part of it, another session's model runner at about half a
core. The session driving the sweep died of a network error between the two
halves; the challenge sweep was relaunched by hand with the same arguments, not
resumed by `measure.py`, which has no resume. Both facts are in the provenance
block under `conditions`, typed there after the run because no instrument here
records them, and said so.
