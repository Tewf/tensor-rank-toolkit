# Which GF(p) backend survives

**The one-hot CNF encoder, and this time on merit.** Ubuntu's `cvc5` 1.1.2 is
built without CoCoALib and cannot run its finite-field solver at all, but the
upstream 1.3.4 GPL build can, so the comparison the two backends were built for
actually happened. Ground truth from the exhaustive search: `GF(9)` and F₃
2×2-term both rank 3, F₃ 2×3-term rank 4. Measured in
[`results.json`](../results.json), under `prime_field_backends`.

| Question | one-hot CNF | cvc5 finite fields |
|---|---|---|
| `GF(9)` find 3 | **0.010 s** | 5.44 s |
| `GF(9)` rule out 2 | **0.008 s** | 0.085 s |
| F₃ 2×2 find 3 | **0.014 s** | 3.00 s |
| F₃ 2×2 rule out 2 | **0.011 s** | 0.022 s |
| F₃ 2×3 find 4 | **0.051 s** | no answer in 150 s |
| F₃ 2×3 rule out 3 | **0.099 s** | 2.22 s |

**Every verdict they both produced agrees**, and agrees with the exhaustive
search. That is what the second backend was for, and it did its job: the
hand-written multiplication table, addition chain and one-hot constraints are
corroborated by an encoding that shares none of them.

So `cvc5` stays, demoted to exactly that role. It is not dead code and it is not
a rival; it is the independent check on arithmetic that would otherwise be
mine alone, and it is reachable with `--backend smt`. Neither backend settles
F₃ 3×6 at its known rank of ten within five minutes.
