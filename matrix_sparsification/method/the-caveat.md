# The caveat

These count **field operations**. Over `Q` those are not constant time; see
[the exact layer](../../linear_algebra/costs.md#the-caveat-that-matters-not-all-field-operations-cost-the-same).
Numerators and denominators grow through elimination, so wall-clock grows faster
than the operation counts above. It has not bitten yet at 7×4 with entries in
ninths; it would on anything substantial.

**One method here is proved optimal and the rest are not.**
[`exact-over-q.md`](exact-over-q.md) returns the minimum over every invertible
`V`, by Rado-Edmonds. The two oracles are exact for the
sparsest-independent-vector subproblem over a restricted set of candidates and
assemble greedily one row at a time, and that assembly carries no guarantee; the
row-basis heuristic carries none at all and is the only one measured losing.

**What is not proved is not the same as what is wrong.** On every operator tried
here, including 400 random ones and 203 built as a sparse basis hidden behind a
change of basis, both oracles returned the same count as the proved method. What
they lacked was the guarantee and about a hundredfold in time.
