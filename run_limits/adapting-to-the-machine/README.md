# Does every strand adapt to the machine it runs on?

Asked as one question — *"do all the algorithms use the GPU, or have an
alternative to use it if available"* — and it is really three, because a machine
is three resources and this repository already treats them as three:

| axis | the one place that owns it | how a run moves it |
|---|---|---|
| **(a) cores** | [`../parallel.h`](../parallel.h) — `worker_count`, `parallel_for` | `--threads N`, `0` for every core |
| **(b) memory** | [`../memory_budget.h`](../memory_budget.h) — `require_room`, `memory_budget` | `--max-memory 4G` |
| **(c) device** | [`../device.h`](../device.h) — `chosen_device`, `launch_floor` | `--device auto\|cpu\|gpu` |

They are independent. A strand can be perfect on one and absent on the other
two, and most of them are.

## The short answer, and it is not the flattering one

**No. Two strands of twelve are wired for all three axes, and the card is the
axis that matters least.** The honest ranking of what a user on an unknown
machine loses:

1. **Memory is the one that kills a run.** Seven strands hold an allocation that
   is exponential or unbounded in something read off the command line or the
   tensor file, with nothing between it and the allocator. Those die by
   `Killed`, which tells the caller nothing, where `require_room` would have
   named the number and refused. That is a production defect on any machine.
2. **Cores are the one that wastes a run.** Two strands hold a `parallel_for`
   that no flag can reach, so the parallelism is written, measured, documented —
   and dead. A third accepts `--threads` and then runs the expensive half on one
   core anyway, which is worse than not offering the flag.
3. **The card is the one that almost nobody wants.** Of the twelve strands,
   exactly **two** inner loops are GPU-shaped, and both already have a seam. The
   rest hand work to an external solver, carry greedy state forward, run a few
   big exact-rational eliminations rather than a swarm of tiny ones, or have an
   item count in the hundreds. Building a third seam would be building for
   nobody.

## The pages

- [**the-audit.md**](the-audit.md) — every strand, its dominant inner loop,
  whether that loop is GPU-shaped, and its row against all three axes. This is
  the table the question was asked for.
- [**fitted-or-genuine.md**](fitted-or-genuine.md) — every measured constant in
  [`../device.cpp`](../device.cpp) and [`../../cli/tunables.h`](../../cli/tunables.h),
  sorted into the ones that are facts about the arithmetic and the ones that are
  facts about an RTX 4060 and an i5-12450H. **A fitted number shipped as a
  constant is a bug on someone else's hardware**, and there are four of them.
- [**what-was-closed.md**](what-was-closed.md) — the diff: two dead
  `parallel_for`s given the flag they were waiting for, one that aborted the
  process instead of refusing, five unpriced allocations, six commands given the
  `--max-memory` their own refusals already named, and one line of CMake that
  made the card useless on every card but this one.

## What "GPU-shaped" means here, so the column is falsifiable

Many independent uniform small computations, no loop-carried dependency, little
per-item transfer. A loop fails the test — and the audit says **no** — when any
one of these holds:

- **it hands the work to an external process** (SAT, MILP): there is nothing
  left in this address space to put on a card;
- **it carries state forward**: a greedy that mutates what the next iteration
  reads, a branch-and-bound incumbent that decides what the next node prunes, a
  Markov chain whose step *k+1* is built from step *k*'s output;
- **the items are wildly non-uniform**: a warp runs at the speed of its slowest
  lane, so a loop where most iterations cost nothing and a few cost a `p^dim`
  walk is the worst possible shape;
- **the cost is a few big linear-algebra calls rather than a swarm of tiny
  ones**: one Gaussian elimination is not a kernel, it is a call;
- **the item count is small**: 105 pairs, 13 cubes, 35 subsets. A launch costs
  more than the work, which is what [`../device.h`](../device.h)'s
  `launch_floor` exists to say.

[`../../positioning/hardware-and-parallelism.md`](../../positioning/hardware-and-parallelism.md)
argued the same for the tree and the solver before either seam existed, and it
stands. This page is the sweep it never did over every other strand.
