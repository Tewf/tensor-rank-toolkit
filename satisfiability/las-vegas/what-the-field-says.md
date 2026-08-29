# What the field says, before anything was run

Every claim below was verified by a fetch on 2026-08-29 (the arXiv API, Crossref,
DBLP, `gh repo view`, the paper's PDF); keys are
[`../../references.md`](../../references.md). Where a fetch returned nothing it
says so.

## The name of the thing

A solver that flips variables until every clause holds is a **stochastic local
search** (SLS) solver to the SAT community, and a **Las Vegas algorithm** to
complexity theory: right whenever it answers, random in how long it takes, and
never able to say no. In this module the property is what the code enforces, so
the class is called `finds_only`
([`../local_search_solver.h`](../local_search_solver.h)).

## The baseline, which was in the bibliography all along

`[heule2019]`: "Local search SAT solvers outperform CDCL solvers consistently in
this application." The solver is **yalsat** (`[biere2018]`) in both of their
methods, random pairings under streamlining and a neighbourhood search around a
known scheme, and the CDCL solvers they tried "disappointed", which they put down
to an average backtrack level above 100. The instances are `[matrixchallenges]`,
and the front page's own calibration of the satisfiable ones is: **five of ten
solved by yalsat in a few minutes**, "hard for CDCL solvers (and many local search
solvers)". No other SLS solver is named as succeeding. The instances carry no
licence that any fetch could find, so they are downloaded for a run and never
committed.

**The record on those instances is xnfSAT**, `[nawrocki2021]`: yalsat with the
parities kept as parities inside the flip loop, an XOR weight and a break count
extended over XORs. Their Table 1, 192 runs per instance at a 1000 s cap on a
Xeon E5-2690: the XNF that `cnf2xnf` recovers from each CNF beats every one of
eight CNF encodings on every instance, 100% of runs in 0.1 s against 76.6% in
67.4 s on `4-4-4-4-1`, and 2.1% against 0% on the hardest, `2-2-2-4-A`. Among the
CNF encodings performance rises with the cutting number up to 6, and the linear
3-cut, which is exactly what `--plain-cnf` writes, is the worst. The all-false
initial assignment beat a random one; xnfsat starts from it by default
(`--pol=-1`) where yalsat draws one at random (`--pol=0`). Their measure is the
fraction of runs solved within the cap and the mean time at a fixed `r`.

So the baseline is xnfSAT at commit `85a0613` on the XNF `cnf2xnf` recovers,
with yalsat 1.0.1 at `a0fd39f` on the CNF as the figure the front page quotes,
on those ten instances, at a cap this laptop can afford and stated as such. On
this repository's own fixtures the baseline is kissat 4.0.4 on the same file,
which [`../results.json`](../results.json) already prices.

A 2026 audit, `[palladinos2026]`, found the ten challenge-2 formulas the front
page expects unsatisfiable are satisfiable, their pairings being positive unit
clauses that require an incidence without forbidding the others. Nothing here
uses challenge 2 as an UNSAT set.

## The prior against it

A GF(2) tensor equation is a parity. Expanded into clauses it is exactly the
shape the field records as the failure mode of WalkSAT-type solvers: `[jia2004]`
built hard satisfiable formulas from parity constraints and watched local search
stall on them while Gaussian elimination walked through; `[haanpaa2006]` made
that a benchmark family; `[riccitersenghi2010]` gave the physics of it, XORSAT
being glassy for any local dynamics and polynomial for elimination. Heule's
instances are parities too, with Tseitin `and` gates in front, and yalsat still
solves five of ten, so the literature does not settle which of the two this
repository's encoding is closer to. That is what the measurement is for.

## What "local search" is not, here

Not a search over schemes with a CDCL oracle inside. Both of `[heule2019]`'s
methods hand a CNF to yalsat; the neighbourhood search fixes two thirds of a known
scheme's base variables and re-solves the rest with the same solver. The route
here is the plainer half of that: the SLS solver alone on the formula this
module already writes, with no streamlining and no known scheme to start from,
which is exactly what challenge 1 asks for.

## Not found

- A licence for `[matrixchallenges]`: no `LICENSE` file, and the GitHub licence
  endpoint answers 404.
- Any published SLS result on tensor-rank encodings of this module's shape. The
  bibliography said nobody had run the comparison; nothing found says otherwise.
- A hardware statement behind "a few minutes" for challenge 1; the paper names a
  48-core cluster only for its neighbourhood experiment.
