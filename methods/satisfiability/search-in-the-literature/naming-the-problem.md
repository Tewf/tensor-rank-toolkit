# Naming the problem, which is the step that was skipped

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

The ascending sweep often needs only its first question: on two of the seven
fixtures recorded in [`the-unimplemented-schedule.md`](the-unimplemented-schedule.md)
it stops in 0.006 s.
