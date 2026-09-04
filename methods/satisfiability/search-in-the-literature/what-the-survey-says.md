# What the survey actually says, since it was cited backwards

`[morgado2013]` prices the calls, at **Table 6, p. 498** and §4.3 p. 497: the two
linear searches need, in the worst case, a number of oracle calls exponential in
the instance size, where binary search needs a linear number. That pricing is the
survey's and it is what this module borrows.

**It does not conclude that [bisection](naming-the-problem.md) loses.** Close to the opposite. Its own
controlled experiment reimplements every schedule inside MSUnCore, so the schedule
is the only variable, and on unweighted partial crafted instances (§7 p. 520)
**BIN solves 261 against LIN-SU's 185**, with core-guided BIN-C top at 266. In
this module's own names, BIN is bisection and LIN-SU is [descending](naming-the-problem.md); BIN-C, being
core-guided, has no analogue here. Its comment: "Interestingly, iterative
algorithms such as bit-based (BIT) and binary search (BIN) perform better than
several core-guided MaxSAT algorithms which can be explained by their linear
number of calls to a SAT oracle in the worst case."

| Schedule, `[morgado2013]` §7 p. 520 | Solved |
|---|---|
| LIN-SU | 185 |
| BIN | 261 |
| BIN-C (core-guided) | 266 |

The pessimistic sentence often quoted at bisection, that it "has seldom been used
in practical MaxSAT solvers", is **`[heras2011]`**, AAAI 2011 p. 36, and neither
"seldom" nor "rarely" occurs in the survey at all. `heras2011`'s contribution is
*core-guided* binary search, BIN-C and BIN-C-D; plain BIN is Fu and Malik, SAT
2006, LNCS 4121:252-265, which the survey credits at §1.3 p. 482. Citing the
survey for the verdict, as this file once did, cites it against itself.
