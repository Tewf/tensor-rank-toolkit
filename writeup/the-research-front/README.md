# Where the research front is, and where this repository sits on it

**This folder is about the field, not about us.** It was called
`state-of-the-art/` until 2026-08-22, which read as a label on this repository
when its contents were always the opposite: where the front is, who holds it, and
which side of it we are on. The phrase belongs in a sentence with a number
attached and never as a name, because a name carries no measurement and cannot be
checked.

## What is actually ours, with the number for each

Five things, and each one is a count somebody else can re-derive rather than an
adjective.

**Both directions, in one place.** Flip graphs and learned search produce upper
bounds only, and `[chen2025]` says so in its own words: it is not clear how to use
the technique for checking whether an optimum has been reached. Here `f2_5x5` is
settled at **13 by this repository's own two searches**,
[the exhaustive one](../../methods/bilinear_rank/exhaustive/)
refuting 12 in **146 402 553 nodes** and
[the incumbent one](../../methods/bilinear_rank/branch_and_bound/) exhibiting 13 in
**80**. Until 2026-08-21 the upper half of that was a citation.

**The orbit quotient inside the exact search.** On the same question as
`[yang2025]`'s own implementation, refuting six products for 2x2 matrix
multiplication: **648 nodes against its 25 426**. That is the one comparison here
that is not about the language or the hardware, and the reason it is not is the
row beside it, our unquotiented search visiting **25 399** against its 25 426.
Agreeing to a tenth of a percent on a tree size is what shows the two walk the
same tree, so the 39x is the quotient and nothing else.

**Neither side of the field has both halves.** `[yang2025]` prunes by rank sums
and by rref and has no quotient; `[covanov2019]`'s quotient removes whole orbits
and was never joined to those pruners. Both are here, and no implementation
anywhere combines them.

**Nothing is ever a float.** Every rank is exact over a finite field, so a
reported rank is a fact about the map rather than an artefact of rounding, and a
value that rounds to zero is a different answer rather than a near one.

**A refutation can be rechecked by a program that shares no code with this one.**
The solver strand writes a DRAT proof, the standard machine-checkable refutation
certificate for a SAT solver, and `drat-trim` rereads it, so a lower bound
from that route rests on two programs rather than on our word.

**And one open question has been answered, in the direction nobody wanted.**
`[chen2025]` closes by asking how to quotient the scalar freedom over `GF(p)` and
says it is unclear what the best way is. Two answers were measured here and both
are negative: the quotient makes the method work over `GF(3)` without making it
competitive, and on `f3_3x6` the flip graph over eight seeds and 60 000 flips
reaches 12 where the descent reaches **10**. A measured no to a printed question
is a result, and it is cheaper to publish than a yes.

## The three questions, kept apart

Three questions get called "fast matrix multiplication" and they have almost
nothing to do with each other. Keeping them apart is the first thing, because a
result in one says nothing about the others.

| | The question | Who is winning it |
|---|---|---|
| **Upper bounds** | find a decomposition with fewer products | search, and since 2022 machine learning |
| **Lower bounds** | prove no smaller one exists | orbit classification with certificates, and SAT; still the hard side |
| **The exponent** | how does the cost scale asymptotically | the laser method, a separate field entirely |

One file per question, then one per instrument, then where that leaves us.

| | |
|---|---|
| [`upper-bounds.md`](upper-bounds.md) | how the records moved, and why not one of them came from exhaustive search |
| [`lower-bounds.md`](lower-bounds.md) | the refutation side, which is the side this repository is on |
| [`the-exponent.md`](the-exponent.md) | why the asymptotic question shares no machinery with either |
| [`refutation-baseline.md`](refutation-baseline.md) | `[wang2026]`, and what a refutation here is measured against |
| [`rank-as-a-milp.md`](rank-as-a-milp.md) | an instrument built here, measured, and retired |
| [`rank-one-elements-of-a-subspace.md`](rank-one-elements-of-a-subspace.md) | what the leaf test is called in algebraic geometry, and the MinRank machinery that comes with the name |
| [`where-we-stand.md`](where-we-stand.md) | how close each strand is to the front, question by question |
| [`what-is-missing.md`](what-is-missing.md) | the gaps, kept apart from the ones only another design would call gaps |
| [`a-correction.md`](a-correction.md) | the claim this survey got wrong, and the search that would have caught it |
| [`comparing-against-the-baseline.md`](comparing-against-the-baseline.md) | the baseline's own code, run here on a matched question, and what its numbers do and do not license saying |
| [`the-baseline-prunes-nothing.md`](the-baseline-prunes-nothing.md) | why its tree size is a closed form, which is how the comparison reached questions it cannot finish |
| [`do-the-counters-agree.md`](do-the-counters-agree.md) | whether their `work` and our `nodes` count the same event: they do, and closing it corrected one of the figures above |

Full citations, with what each contributes:
[`../../references.md`](../../references.md).
