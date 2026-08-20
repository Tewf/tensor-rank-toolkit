# Where the research front is, and where this repository sits on it

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

Full citations, with what each contributes:
[`../references.md`](../references.md).
