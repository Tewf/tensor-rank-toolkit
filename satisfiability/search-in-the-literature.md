# What the field already settled about asking a solver in order

The order this module asks its questions in was chosen twice by argument and
twice wrongly, then settled by measurement in [`search.md`](search.md). This file
is the other half: the question has a literature, the literature had already
answered it, and reading it first would have saved both wrong answers.

## Naming the problem, which is the step that was skipped

Say it in one sentence: **find the least `k` at which a decision oracle answers
yes, by asking it a sequence of yes/no questions.** That shape is not ours and
neither is the difficulty. The field calls it **SAT-based optimisation**, or
**iterative MaxSAT solving** when the objective is a clause count, and its three
orders have standard names that predate this repository by a decade.

| Their name | Ours | Their gloss |
|---|---|---|
| linear search **UNSAT-SAT** | the ascending sweep | refines a lower bound; all calls but the last return unsatisfiable |
| linear search **SAT-UNSAT**, model-improving | descending | refines an upper bound |
| **binary search** | bisection | fewest calls |

Our names were invented here, which is why three searches for them returned
nothing: zero results meant the query was wrong, not that nothing existed.

## What the survey actually says, since it was cited backwards

`[morgado2013]` prices the calls, at **Table 6, p. 498** and §4.3 p. 497: the two
linear searches need, in the worst case, a number of oracle calls exponential in
the instance size, where binary search needs a linear number. That pricing is the
survey's and it is what this module borrows.

**It does not conclude that bisection loses.** Close to the opposite. Its own
controlled experiment reimplements every schedule inside MSUnCore, so the schedule
is the only variable, and on unweighted partial crafted instances (§7 p. 520)
**BIN solves 261 against LIN-SU's 185**, with core-guided BIN-C top at 266. Its
comment: "Interestingly, iterative algorithms such as bit-based (BIT) and binary
search (BIN) perform better than several core-guided MaxSAT algorithms which can
be explained by their linear number of calls to a SAT oracle in the worst case."

The pessimistic sentence often quoted at bisection, that it "has seldom been used
in practical MaxSAT solvers", is **`[heras2011]`**, AAAI 2011 p. 36, and neither
"seldom" nor "rarely" occurs in the survey at all. `heras2011`'s contribution is
*core-guided* binary search, BIN-C and BIN-C-D; plain BIN is Fu and Malik, SAT
2006, LNCS 4121:252-265, which the survey credits at §1.3 p. 482. Citing the
survey for the verdict, as this file once did, cites it against itself.

## Why our result is a contrast with it, not a confirmation

On GF(16) bisection makes the fewest calls and finishes **last**, 113.614 s
against 110.094 s for the schedule that wins. That disagrees with the survey, and
the reason is ours to give rather than the survey's to have missed.

The exponential-versus-linear gap is in the *size of the cost range*, which for
MaxSAT is exponential in the number of soft clauses. Here the range is
`[flattening bound, n₁n₂]`, a dozen values at most, so log versus linear is a
handful of calls either way. **The asymptotic advantage has no room to act on a
range this narrow**, and what remains is dominated by which questions a schedule
happens to ask. That is why the whole choice is worth about 3% here, and the
survey does not say it because nobody had a range this small.

One nuance, since "nobody bisects" would be too strong: no solver bisects as its
*primary* schedule, but UWrMaxSat bisects as a second-phase fallback when
core-guided search stalls, and the 2024 weighted runner-up at 442 solved was
configured that way, with `-no-bin` absent. Its author enables it for weighted
instances and disables it for unweighted ones.

## The one thing the survey says is not implemented, and we ship it

The same survey records that there are **no known implementations of linear
search UNSAT-SAT for MaxSAT**, though it is used elsewhere, for minimal
unsatisfiable subsets. That schedule is this module's default. Not by
independence of mind: it is the right default here for reasons that do not hold
in MaxSAT, namely that the flattening bound is often already the rank, so
ascending asks one question and stops, and that it is the only schedule which
never reads the ceiling and so cannot be misled by a loose one.

## Positionnement, stated so it can be contradicted

**Not new**: the three schedules, the call-count pricing, the hybrid instinct.
Nor is the observation that bisection can lose on time despite winning on calls,
which is `[heras2011]`'s about core-guided binary search.

**New, as far as reading found**: the per-question price table in
[`search.md`](search.md). Every question here is a separate deterministic
process, so its cost is independent of the order it is reached in, and pricing
all of them prices every schedule exactly and at once, including unimplemented
ones. **A MaxSAT solver cannot do this**, because it is incremental: a call's
cost there depends on what the solver learned in the calls before it. The
no-linking rule that costs this module incrementality is what buys it exact
schedule pricing. That is a trade, not a gap.

**The baseline** for this strand is therefore `[morgado2013]`'s taxonomy and the
MaxSAT Evaluation record beside it, and the review is finished because that
baseline can now be named. On the other side of the module, a refutation is
measured against `[wang2026]`: [`../state-of-the-art.md`](../state-of-the-art.md).
