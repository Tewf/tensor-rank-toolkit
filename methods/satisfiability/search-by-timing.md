# Why solve time is not used to choose the next question

Solve time peaks sharply just below the rank, which invites a secant-style
search: probe two ranks, read the times, extrapolate to where the peak must be.
It is not implemented. The schedule this module does use, and the floor that
bounds what any schedule can win, are in [`bracket/`](bracket/README.md).

Three reasons, the first two structural and independent of any measurement.

## The timing curve detects, it does not predict

Secant search needs a signal that varies smoothly across the range being
searched. Solve time does not. It is flat and cheap almost everywhere:

- well below the rank, refuting is easy, the tensor is nowhere near
  decomposable, and the answer comes in hundredths of a second
- above the rank, the instance is satisfiable and a decomposition is found fast
- at `r-1` alone it spikes, by a factor of 400 on GF(16)

So two cheap probes look **identical** at `r-5` and at `r+3`, and carry no
information about which side of the rank they are on or how far away it is. The
signal becomes informative exactly when you reach the question it was supposed
to help you skip. By then it is paid.

## A timing datapoint costs what it would save

Learning that a `k` is dear means running it **to completion**. The informative
points are precisely the expensive ones, so the probe that would tell you where
the boundary lies costs about what jumping to it costs.

Time-boxing the probe escapes that, but it no longer yields a time. It yields
"did not finish in five seconds", which is a **threshold**, not a quantity to
run a secant through. A search over thresholds is the timeout ladder, and that
is already here as `--probe`.

## And the prize is under four percent

Any exact search pays a yes at `r` and a no at `r-1`. On GF(16) that mandatory
floor is 108.461 s and the ascending sweep costs 112.533 s
(measured in [`the-five-schedules.md`](bracket/the-five-schedules.md)), so
**everything a cleverer probe placement could win is 4.07 s, or 3.75%**. It
would be bought with a signal that varies about 13% run to run on this chassis
from thermal throttling alone: the noise is larger than the effect.

## What survives, and it is the useful half

`--probe N` gives the questions asked on the way a small budget and the question
that cannot be avoided the full one. This uses time as a **classifier**, cheap
against dear, rather than as something to extrapolate through, which is the part
of the idea that does not depend on a smooth signal.

An exhausted probe is evidence without being an answer. It moves no bound in
either direction, and the sweep that follows re-asks the question properly,
because treating "gave up quickly" as a refusal would invent a lower bound.

For example, on a run here:

```
$ decide-rank-by-sat evidence/fixtures/matmul_2x2x2.tensor --probe 1 --break-symmetry --plain-cnf
evidence/fixtures/matmul_2x2x2.tensor: 4 slices of 4x4 over GF(2)
  lower bound: rank is at least 6
  naive upper bound: rank is at most 16
  asked 2 questions in 0.516806 s
rank is exactly 7
```

Both mandatory questions finished under the one second probe budget, so the
ladder spent nothing extra and the full sweep needed no second pass.
