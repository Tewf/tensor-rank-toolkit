# The mitigations, each measured against the one above it

| # | mitigation | per candidate | all five | gain |
|---|---|---|---|---|
| 0 | matched flags, no ordering | 1.63 to 2.67 s | 12.14 s | the honest start |
| 0b | `--break-symmetry`, ordering terms 1 onward | 0.24 to 0.35 s | **1.26 to 1.65 s** | 7.4x |
| 4 | `--refuter tree`, the quotiented tree | 0.0043 to 0.0099 s | **0.025 to 0.048 s** | 34 to 51x |
| 5 | `--parallel`, twelve threads | unchanged | **0.018 s** wall | 2.6x |

Ranges on rows 0b and 4 because both were measured twice, the second time as a
reproducibility check; the spread is about 2x, so nothing here is good to better than
one significant figure. Cumulative 12.14 s to 0.018 s, roughly **700x**. The tree also
beats the *whole-instance* refutation, 0.025 s against 1.32 s, by about 50x, which is
the comparison that matters: pinning is no longer a tax on the work.

On the accepting side at `k = 7`, where the strict step ends:

| refuter | candidate 0 | candidate 1 | total |
|---|---|---|---|
| solver | refuted, 1.22 s | accepted, 0.44 s | 1.66 s |
| tree | refuted, 0.80 s, 4584 nodes | accepted, 0.39 s, 2150 nodes | **1.18 s** |

Both return a verified 7-product algorithm through `recovers_map`.
