# Every question priced, so every schedule is priced

Each question is a separate deterministic kissat process, so its cost does not
depend on the order it is reached in. Pricing all of them prices all schedules
at once, exactly, including ones nobody implemented. GF(16), rank 9, ceiling 16,
`--break-symmetry --plain-cnf`. **The floor [was 4 when this was measured and is
now 8](what-decides-it.md)**, so `k = 4` to `k = 7` are no longer asked; they are priced here anyway,
because pricing every question is the point and because their total, 4.10 s, is
what the stronger bound saves:

| k | 4 | 5 | 6 | 7 | **8** | 9 | 10 | 11 | 12 | 13 | 14 | 15 | 16 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| | no | no | no | no | **no** | yes | yes | yes | yes | yes | yes | yes | yes |
| s | .02 | .08 | .30 | 3.7 | **108.2** | .28 | 1.5 | .21 | .04 | .12 | .04 | .04 | .03 |

**Refusals get dear approaching the rank from below; finds are flat and cheap
above it.** `k = 8` costs over five thousand times `k = 4` (108.2 s against 0.02 s
in the table above), while every yes from 9 to 16 costs
under two seconds. That asymmetry is the whole story.
