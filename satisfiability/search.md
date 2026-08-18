# Locating the rank between the bounds

Two bounds come free: the flattening rank below, the smallest product of two of
the three dimensions above. Finding the rank between them means asking a solver
a sequence of decision questions, and there is more than one order to ask them
in. Encodings are [`method.md`](method.md); costs against the exhaustive search
are [`measurements.md`](measurements.md).

## The floor no strategy can go below

Establishing `rank = r` needs a **yes at r** and a **no at r-1**, since one
refusal at `r-1` refutes everything under it. No schedule skips either, so every
strategy pays `cost(no at r-1) + cost(yes at r)` and competes only for the rest.
**On GF(16) that floor is 108.5 s of a 110 to 114 s search: the whole choice of
schedule is worth about three percent.** That number bounds every future idea in
this direction as well as the five below.

## Every question priced, so every schedule is priced

Each question is a separate deterministic kissat process, so its cost does not
depend on the order it is reached in. Pricing all of them prices all schedules
at once, exactly, including ones nobody implemented. GF(16), rank 9, ceiling 16,
`--break-symmetry --plain-cnf`. **The floor was 4 when this was measured and is
now 8**, so `k = 4` to `k = 7` are no longer asked; they are priced here anyway,
because pricing every question is the point and because their total, 4.10 s, is
what the stronger bound saves:

| k | 4 | 5 | 6 | 7 | **8** | 9 | 10 | 11 | 12 | 13 | 14 | 15 | 16 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| | no | no | no | no | **no** | yes | yes | yes | yes | yes | yes | yes | yes |
| s | .02 | .08 | .30 | 3.7 | **108.2** | .28 | 1.5 | .21 | .04 | .12 | .04 | .04 | .03 |

**Refusals get dear approaching the rank from below; finds are flat and cheap
above it.** `k = 8` costs 400 times `k = 4`, while every yes from 9 to 16 costs
under two seconds. That asymmetry is the whole story.

## The five schedules, summed from that table

Seconds. `floor` is the mandatory two questions.

| fixture | floor | ascending | descending | bisection | gallop up | gallop down |
|---|---|---|---|---|---|---|
| f2_2x2 | 0.006 | **0.006** | 0.015 | **0.006** | **0.006** | 0.015 |
| f2_2x3 | 0.026 | **0.026** | 0.040 | **0.026** | **0.026** | 0.040 |
| gf4 | 0.006 | **0.006** | 0.011 | **0.006** | **0.006** | 0.011 |
| gf8 | 0.085 | 0.112 | 0.142 | **0.104** | 0.128 | 0.142 |
| w_state | 0.009 | **0.009** | 0.012 | **0.009** | **0.009** | 0.012 |
| matmul_2x2x2 | 0.495 | **0.620** | 1.044 | 0.716 | 0.760 | 0.865 |
| **GF(16)** | 108.461 | 112.533 | 110.421 | 113.614 | 110.399 | **110.094** |

**Ascending wins on the cheap fixtures and loses on the only expensive one**,
where it comes fourth of five. It ties the floor wherever the bound already
equals the rank, four of these seven, because it then asks one question and
stops. It lost on GF(16) because the floor was five short there, so it walked
through `k = 7` at 3.7 s: the second dearest question in the table, and one no
other schedule asks.

**That specific handicap is gone.** The floor on GF(16) is now 8 rather than 4,
one short of the rank instead of five, so ascending asks `k = 8` and `k = 9` and
nothing else. The table above was measured before that and is not re-run here;
what can be said without re-running it is that the four questions ascending was
paying for and no other schedule asked, 4.10 s of its 108.461 s, are no longer
asked by anyone. The comparison between schedules should be re-measured before it
is quoted again.

## What decides it, which is not the schedule

Descending from a *tight* upper bound hits the floor exactly. Given `U = 9`,
asking 9 then 8 costs 108.461 s, the floor to three decimals, and nothing
starting from the naive ceiling of 16 matches it.

**So the lever is the upper bound, not the order of questions.** A heuristic
supplying a good `U` saves more than every schedule here put together. That is
why `find_rank` takes bounds rather than discovering them, and why the staged
design on `main` puts the heuristic between the flattening bound and this
decider.

Ascending stays the default: optimal when the bound is tight, within 4% of the
floor when it is not, and the only schedule that never reads the ceiling, so a
loose one cannot mislead it.

## Two corrections, since both wrong answers were published

**Gallop was made the default on one contended instance**, reported 2.3x faster
than ascending. The true difference is under 2% either way.

**Then ascending was published as the winner**, measured end to end while five
stray `cbc` processes held the machine, and generalised from fixtures costing
milliseconds. Ranking millisecond measurements was the error both times.

## Choosing probes by their timing, and why it is not here

Solve time peaks just below the rank, so a secant search could in principle
extrapolate to the rank from timings. It is rejected, for reasons that are
structural rather than a matter of taste:
[`search-by-timing.md`](search-by-timing.md).
