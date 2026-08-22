# The caveat

These count **field operations**. Over `Q` those are not constant time; see
[the exact layer](../../linear_algebra/costs.md#the-caveat-that-matters-not-all-field-operations-cost-the-same).
Numerators and denominators grow through elimination, so wall-clock grows faster
than the operation counts above. It has not bitten yet at 7×4 with entries in
ninths; it would on anything substantial.

**The method that runs here returns the minimum**, by Rado-Edmonds:
[`exact-over-q.md`](exact-over-q.md). So did two of the three that left for the
`dominated-methods` branch, and the article proves one of them optimal itself;
[`../dominated.md`](../dominated.md) has the measurement that moved them, which
is cost and never correctness.

**This file said for a long while that nothing here was proved optimal.** That
was wrong twice over: the article proves its own algorithm optimal, and the
evidence was already in the measurements, where the oracles never once came back
heavier.

The greedy by rescaling still guarantees nothing, and still wins where it
matters, because `nnz + nns` is not `nnz`.
