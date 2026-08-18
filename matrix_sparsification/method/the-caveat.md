# The caveat

These count **field operations**. Over `Q` those are not constant time; see
[the exact layer](../../linear_algebra/costs.md#the-caveat-that-matters-not-all-field-operations-cost-the-same).
Numerators and denominators grow through elimination, so wall-clock grows faster
than the operation counts above. It has not bitten yet at 7×4 with entries in
ninths; it would on anything substantial.

Nothing here is proved optimal either. Both oracles are exact for the
sparsest-independent-vector subproblem, but they assemble the answer greedily,
one row at a time.
