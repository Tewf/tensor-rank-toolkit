# The caveat

These count **field operations**. Over `Q` those are not constant time; see
[the exact layer](../../linear_algebra/costs.md#the-caveat-that-matters-not-all-field-operations-cost-the-same).
Numerators and denominators grow through elimination, so wall-clock grows faster
than the operation counts above. It has not bitten yet at 7×4 with entries in
ninths; it would on anything substantial.

**Three of the four methods here return the minimum, and only one does not.**
[`exact-over-q.md`](exact-over-q.md) does, by Rado-Edmonds. So does
[`oracle-bottom-up.md`](oracle-bottom-up.md), which is `[beniamini2020]`'s
Algorithm 3 and is proved optimal by its Theorem 3.22, and so, on a reading,
does [`oracle-top-down.md`](oracle-top-down.md). The row-basis heuristic
guarantees nothing and is the only one ever measured losing, on 37% of 400
random operators and 21% of 203 with a sparse basis hidden behind a change of
basis.

**This file said for a long while that nothing here was proved optimal.** That
was wrong twice over: the article proves its own algorithm optimal, and the
evidence was already in the measurements, where the two oracles never once came
back heavier. What separates the methods is cost, not correctness, and about a
hundredfold of it.
