# What is missing, and from which of the two lists

**What is missing from this repository as a whole**: `[yang2025]`'s algorithm,
symmetry-aware flip graphs, and any evolutionary or learned search. The last is
a substantial engineering undertaking and needs hardware this laptop does not
have.

**What is missing from the solver strand specifically is a shorter and
different list**, and it is worth separating, because a feature another design
needs is not automatically a gap in this one:

- **Proof logging is no longer on this list**, and it was the first item on it.
  `--proof` writes kissat's DRAT refutation and `drat-trim` rechecks it, so a
  lower bound from this strand now rests on two programs sharing no code instead
  of on one. What each verdict rests on is
  [`methods/satisfiability/correctness.md`](../../methods/satisfiability/correctness.md).
- **Incremental solving.** A sweep re-encodes and re-solves from scratch at
  every `k`, so nothing learned at `k` is reused at `k+1`. The clauses differ
  only in the number of terms.
- **The instances that do not answer**: `f2_5x5` at twelve. `f3_3x6` was on this
  list and is off it, and how it came off is the lesson. Nobody asked it the
  cheap question. The solver was asked `--target 10`, which is a *find*, and it
  is `--target 9`, a *refutation*, that settles the rank: `decide-rank` returns
  NO exhaustively in **7.65 s over 4729 nodes**, so no ten-product algorithm is
  beaten and `rank(f3_3x6) = 10`. The instance was never out of reach; the
  question was being asked the expensive way round.

**Conciseness reduction is not on that list, and an earlier version of this
survey wrongly implied it was.** It is an internal step of a *recursive* search,
which re-compresses the residual tensor at every node; a single monolithic
encoding has no nodes to do it at. Applied once at the top it would help only a
tensor that is not concise, and **every fixture in this repository is concise**,
measured: the flattening ranks equal the shape on all twelve. It would buy
nothing here. Flip graphs are likewise not a gap in this strand, since they
produce upper bounds only and this strand exists for the other direction.
