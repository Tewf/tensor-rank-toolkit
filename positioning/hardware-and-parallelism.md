# What machine this is the right shape for

Asked twice and worth answering once: could a GPU make these searches faster?
**Mostly no, and the reason says something useful about where the time actually
goes.** What is on the table instead is cheaper, larger, and on the CPU.

## The tree is the wrong shape for a GPU

An exhaustive rank search is a depth-first walk with data-dependent early exits at
every node: a candidate inside the current span is skipped, a budget is checked, a
subtree returns as soon as it succeeds. Threads in a GPU warp that take different
branches serialise, so a warp of 32 nodes runs at the speed of the slowest path
through all 32. Divergence is the whole of this algorithm.

The pool is the second problem. Where a GPU would help most is where the pool is
largest, and that is exactly where it does not fit in device memory: `⟨4,4,4⟩` is
8.2 TiB of materialised maps, against tens of gigabytes on a card.

## The solver route is worse, and backwards

`decide-rank-by-sat` hands the question to a CDCL solver, and **GPU CDCL is a
known negative result**: the algorithm is sequential, clause learning is
memory-hungry, and per-thread memory on a GPU is small, so GPU-enabled CDCL
solvers are reported as performing worse than CPU ones.

What does run well on a GPU is **continuous local search**, where gradients are
computed for many assignments at once: FastFourierSAT reports its gradient over a
hundred times faster than a CPU prototype. But continuous local search is
**incomplete**. It finds satisfying assignments and cannot refute.

That is backwards for this repository. A satisfying assignment gives an *upper*
bound on the rank, which the descent already produces in milliseconds. The
expensive half is the refutation, which is what a *lower* bound is made of and
what DRAT lets a reader check. A GPU accelerates the half already cheap.

## The part that would vectorise, and the win that needed no GPU

One thing here is embarrassingly parallel and branch-free: **the leaf test**,
where an exhaustive search spends its life. **But the first order of magnitude
was never on a card, it was in the representation**: over GF(2) a span
membership test is an exclusive or, not a loop of Givaro calls on `int64_t`, and
`exhaustive_search/gf2_leaf.h` now answers the leaf with one bit per entry.

### This page predicted 40x to 64x. Measured, it is 6.0x to 39.6x

That prediction was a ratio of storage widths, 648 bytes of `int64_t` against 16
bytes of bitset at 9x9, quoted onward as if something had timed it. Nothing had.
Measured under [`../MEASURING.md`](../MEASURING.md), one core, fastest of three,
each question asked twice with and without `decide-rank --general-leaf`, which
forces the general path over GF(2) so both columns are the same tree:

| question | leaf route | general | GF(2) | factor |
|---|---|---|---|---|
| `gf16_multiplication --target 8 --node-limit 200000` | scan the pool | 4.47 s | 0.750 s | **6.0x** |
| `f2_3x8 --target 14 --node-limit 20000` | scan the pool | 8.27 s | 0.995 s | **8.3x** |
| `f2_5x5 --target 11` | scan the pool | 77.88 s | 7.69 s | **10.1x** |
| `matmul_2x2x2 --target 6` | walk the subspace | 0.560 s | 0.0347 s | **16.2x** |
| `matmul_3x3x3 --target 23 --node-limit 300` | scan the pool | 82.28 s | 3.55 s | **23.2x** |
| `matmul_3x3x3 --target 17 --node-limit 60` | walk the subspace | 16.66 s | 0.420 s | **39.6x** |

**The general column moved on 2026-08-20 and every ratio above is now an upper
bound.** `is_rank_one` replaced a full Gaussian elimination on that path, so the
baseline these are measured against got faster. One row re-run: `matmul_2x2x2
--target 6` is **0.1305 s** where the table says 0.560 s, taken under load, which
can only slow it — so its 16.2x is at most **3.8x**. The rest are untaken.

**The band was too high and the reasoning behind it was wrong twice over.** The
loop this page named, the pool scan, reaches 23.2x and not 40x: a bitset shortens
a reduction by the machine word, so the width ratio was never going to arrive
whole. The one figure that does reach 40x is on the route this page never
mentioned, walking the subspace, where the gain is not the width but
`gf2_is_rank_one` replacing a Gaussian elimination. The two 4x4 rows separate the
two cleanly, 6.0x scanning against 16.2x walking at one shape. The 64x row for
`<4,4,4>` stays untested: no fixture here reaches it.

### The same lever, one search further in: 2.5x to 19.0x

The leaf is where the *exact* search lives. The **span walk of step 1** is where
the incumbent search lives, and it is the same shape of loop over the same
field: `lower-the-bound` costs one `minimum_weight_basis_with` per child, and
each of those walks a span taking the rank of a matrix at every element. So the
same representation went there on 2026-08-22,
[`../descent_search/gf2_span_walk.h`](../descent_search/gf2_span_walk.h), and
was measured the same way, each question asked twice with and without
`lower-the-bound --general-span`:

| question | slice | nodes | children | general | GF(2) | factor |
|---|---|---|---|---|---|---|
| `matmul_2x2x2 --width 4` | 4x4 | 21 | 2 251 | 0.05 s | 0.02 s | 2.5x |
| `gf64_multiplication --width 4 --nodes 5` * | 6x6 | 10 | 19 780 | 112.95 s | 12.74 s | **8.9x** |
| `cyclic_f2_7 --width 4` * | 7x7 | 22 | 17 371 | 12.15 s | 0.64 s | **19.0x** |
| `cyclic_f2_7 --width 8` * | 7x7 | 74 | 57 958 | 39.74 s | 2.09 s | **19.0x** |
| `<3,4,5> --width 4 --nodes 1 --summand-rank 4` | 12x20 | 1 | 26 040 | 233.93 s | 24.31 s | **9.6x** |

**Every node count, child count and answer is identical in both columns**, which
is the same discipline `--general-leaf` buys above: one tree, two clocks.

**This page's prediction is wrong here in the other direction, and that is the
finding.** A storage-width argument says the gain rises with the matrix. The
widest matrix in the table loses: the 12x20 slice of `<3,4,5>`, four words a
map, gains **9.6x** against 7x7's 19.2x, and `gf64`'s 6x6 gains 8.1x with a
deeper tree than either. What the two low rows share is the dimension, so a
call walks 2^16 and 2^17 slots where `cyclic_f2_7` walks 2^14: the rank and the
walk step got cheap, and the index arithmetic, the floor's lookup, the candidate
list and its sort did not, and those grow as `p^dim`. So the lever is not the
width alone in either loop, and the leaf's own table said as much from the other
side. Unprofiled, because `perf_event_paranoid` is 4 here.

The full table, the small-fixture row that is dominated by move generation
rather than by the walk, and the protocol caveat, are in the header linked
above. **Three of its seven rows are protocol numbers and four are not.** The
three were re-taken on 2026-08-23 at load 0.98 and 43 C, fastest of three under
the measurement lock. The other four were taken the day before at load 2.5 to
5.0, which [`../MEASURING.md`](../MEASURING.md) abandons, so their seconds are
upper bounds.

**The re-take is the useful part, because it says which half was fragile.** Every
second fell by 7 to 9% once the machine was quiet and **every ratio held or
improved**, `gf64`'s 8.1x becoming 8.9x. Load was slowing both columns by about
the same amount, which is what interleaving the attempts was meant to achieve and
is now measured rather than hoped.

**This page covers the tree and the solver only.** The other two parts people
ask about, generating the pool and the `C A` recovery, are in
[`what-a-gpu-would-take.md`](what-a-gpu-would-take.md), along with the one part
that is genuinely shaped for a card. One correction lives there and is worth
knowing here: pool generation is 4% of a `⟨3,3,3⟩` run and almost all of a
`⟨4,4,4⟩` one, so a ceiling computed at the first shape says nothing about the
second.

**That one part has since been built and measured**, and the leaf really is worth
a card: [`../gpu_leaf/README.md`](../gpu_leaf/README.md). It changes nothing on
this page, which is about the tree above the leaf and stands.

**The sweep this page never did**, over every other strand and over cores and
memory as well as the card, is
[`../run_limits/adapting-to-the-machine/`](../run_limits/adapting-to-the-machine/README.md).
Its finding is that the card was the least of the three: two seams cover every
GPU-shaped loop there is, and what was actually missing was `--threads` and
`require_room` on strands nobody had asked the question of.

**The recommendation survives, on less evidence than it claimed.** An order of
magnitude on the CPU for no new hardware, measured now rather than forecast.


## What the neighbours actually run on

Flip graphs are independent random walks, which parallelise across CPU cores with
no shared state and no divergence problem. AlphaTensor's accelerators train a
network; the tensor arithmetic is not the part on the TPU. Nothing in this
literature reports an exact finite-field rank search on a GPU, and this page is
not evidence that one is impossible, only that the obvious lever here is elsewhere.
