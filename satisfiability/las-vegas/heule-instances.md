# Heule's challenge-1 instances, what this branch holds

**Heule's ten challenge-1 instances: a 5 s, one-seed smoke, and nothing more.**
Run by hand to check the driver before the full sweep, which was started and
stopped for heat on its third instance. One seed at a 5 s cap is not
`[nawrocki2021]`'s 192 runs at 1000 s and is not presented as its
reproduction; it is the only reading of those instances this branch holds.
xnfsat on the XNF `cnf2xnf` recovers, yalsat on the CNF, the two controls and
kissat on the CNF.

| instance | kissat | xnfsat, XNF | yalsat, CNF | probSAT | multilinear-sat |
|---|---|---|---|---|---|
| `MM-23-4-4-4-4-1` | none in 5 s | **0.03 s** | 0.76 s | none | none |
| `MM-23-2-2-2-2-A` | none in 5 s | **2.39 s** | none | none | none |
| `MM-23-2-2-2-2-D` | none in 5 s | **0.83 s** | none | none | none |
| the other seven | none in 5 s | none | none | none | none |

Three of ten for xnfsat within five seconds on one seed, every find checked
against the file's clauses and parities; the paper's figure for `4-4-4-4-1` is
100% of runs in 0.1 s, and 0.03 s here agrees with it. yalsat one of ten,
where the front page says five in a few minutes and five seconds is not a few
minutes. kissat none of ten, as `[heule2019]` reports of CDCL.
