# How a number here was measured

Every timing in this repository, in the READMEs, in the `results.json` files and
on the published page, was produced the same way. This file states it once, so no
measurement restates it and a reader can tell what a number is evidence of.

## The protocol

**One core. Fastest of three runs. An otherwise quiet machine.**

The reference machine is a single core of a **12th Gen Intel Core i5-12450H at
2.2 GHz**. That is why the two columns can be compared at all: the hardware is
the same and only the language differs, Julia there and C++ here.

## Why the fastest run and not the mean

Wall clock is noisy in one direction only. Nothing makes a run finish sooner than
the work it does, and a great many things make it finish later: another process
waking, a page fault, a migration between cores, a clock that has stepped down.
So a slow run does not measure the code, it measures what else the machine was
doing, and averaging it in carries that into the published figure. The minimum
over the runs is the closest available estimate of the work itself.

Three runs is a floor, not a ceiling. It is enough to notice that a value is
unstable, which is the thing worth noticing; where the spread between runs is
larger than the difference being argued about, the difference is not reported.

## The thermal caveat

This chassis varies by about **13% run to run from thermal throttling alone**,
before any other load is on the machine. Two timings inside that band are not
distinguishable, and **must not be reported as a ratio**. A speed-up of 1.1x
measured here is not a speed-up, it is the fan.

Ratios are quoted in this repository only where the two ends are far enough apart
that the band cannot account for them, which is why the ones that appear are 28x,
79x and 134x rather than 1.2x.

## Defaults a published number depends on

Every number here was produced with these in force. A run that changes one is
measuring a different thing, so **the changed flag belongs beside the number**.

| Flag | Default | Set by |
|---|---|---|
| `--threads` | 1 | `run_limits/parallel.h` |
| `--max-memory` | 2 GiB | `run_limits/memory_budget.h` |
| `--node-limit` | 5 000 000 | `search_node_limit` in `tunables.conf` |
| `--timeout` | 300 s | `sat_timeout_seconds` in `tunables.conf` |

More threads change the wall clock, and on a **satisfiable** question also a
count, which this file used to deny: a refutation visits the same nodes at any
thread count, a witness stops early and the workers already running spend against
the same budget, so a tight `--node-limit` can turn exit 0 into exit 3. Measured,
with the consequence and what to do about it:
[`exhaustive_search/what-threads-change.md`](exhaustive_search/what-threads-change.md).

The heuristic is unaffected and adopts the same candidates in the same order.

**A published node count above 5 000 000 means a non-default `--node-limit` was
used.** The GF(16) row of `satisfiability/results.json` records 105 600 301
exhaustive nodes, twenty-one times the default ceiling, and the flag behind it
was never written down. That row says so in its own `_not_reproducible` key, and
`reproduce/measure.py` prints a SKIPPED line naming it on every run: the one
figure the driver does not cover is the one it is loudest about.

## What is reproducible, and what is not

**Counts are.** Ranks, node counts, nonzero counts, the verdicts of both
searches: these are facts about the problem, computed in exact arithmetic, and
they come out the same on any machine. They are asserted by the test suite, CI
reruns them on every push, and `Containerfile` builds an image for reproducing
them somewhere else.

**Timings are not.** No timing is asserted anywhere, no test fails on one, and CI
does not check one. A CI runner is a shared virtual machine with an unknown
neighbour, and a container adds nothing to that; either would turn a real
regression and a noisy afternoon into the same red tick. Timings are measured by
hand, on the reference machine, under this protocol, and are quoted as evidence
of an order of magnitude rather than of a digit.

## One run at a time

Two measurements sharing a machine measure each other. Nothing else may be
running: not a second fixture, not a browser, not a build. `ctest -j2` is for
checking that the suite passes, never for timing anything inside it.
