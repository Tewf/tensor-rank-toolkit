# A bound from the shape of the slice space

Read a [slice space](rank_metric_bound.h) as a **rank-metric code**: `k` is its
dimension, `d` the least rank of a nonzero element. Two lower bounds on tensor
rank follow from `(k, d)` alone, and the second hands over for a table scan a
number that until now cost an exhaustive refutation.

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
| [`joining-the-shared-floor.md`](joining-the-shared-floor.md) | what it costs against the floor already in use, and the three edits that made it the default |

**Step 1 of the descent already had `d` and threw it away.**
`minimum_weight_basis` sorts `span(T)` by ascending rank, so the smallest rank in
the span is the first thing it sees. It used that to choose a basis and never as
a bound.

**The Griesmer form's one win.** On `f2_5x5` it beat the existing floor and
became the new default term, though the bracket has since moved past it: what
it was worth and what joining the shared floor cost are in
[`what-each-is-worth.md`](what-each-is-worth.md) and
[`joining-the-shared-floor.md`](joining-the-shared-floor.md).
