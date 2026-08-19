# A bound from the shape of the slice space

Read a slice space as a **rank-metric code**: `k` is its dimension, `d` the least
rank of a nonzero element. Two lower bounds on tensor rank follow from `(k, d)`
alone, and the second hands over for a table scan a number that until now cost an
exhaustive refutation.

| | |
|---|---|
| **Kruskal**, `rank(T) >= k + d - 1` | `[byrne2021, Thm. 5.4]`, after Kruskal 1977, Cor. 1 |
| **Griesmer**, `rank(T) >= Σ_{j<k} ceil(d / p^j)` | `[bnrs2019, Cor. 4.14(2)]`, plus Griesmer on the block code it produces |

Proofs, full citations, which attribution was checked against a paper and which
was not, the finite-field hypothesis and the cost model are stated once in
[`rank_metric_bound.h`](rank_metric_bound.h). What the two are worth is a
separate question, is measured, and is these two pages:

| | |
|---|---|
| [`what-each-is-worth.md`](what-each-is-worth.md) | the value of each bound on every fixture, where Griesmer wins and where it loses, and why |
| [`the-edit-not-made.md`](the-edit-not-made.md) | what it costs against the floor already in use, and the one-line change to `linear_algebra` that would make it a default |

**Step 1 of the descent already had `d` and threw it away.**
`minimum_weight_basis` sorts `span(T)` by ascending rank, so the smallest rank in
the span is the first thing it sees. It used that to choose a basis and never as
a bound.

**The headline, stated as narrowly as it is true.** On `f2_5x5` the Griesmer form
returns **12 where `rank_lower_bound` returns 10**, in under half a millisecond.
That does not move the repository's bracket, which was already `12 <= rank <= 14`.
It moves the *cheap* floor onto that 12, which until now came from an exhaustive
refutation at `--target 11`. The reading is in
[`what-each-is-worth.md`](what-each-is-worth.md).
