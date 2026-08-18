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

## The part that would vectorise, and the win that needs no GPU

One thing here is embarrassingly parallel and branch-free: **the leaf test.**
`independent_rank_one_maps_in` scans the whole pool asking whether each rank-one
map lies in the current span, and the module's own header says the leaf is where
an exhaustive search spends its life. Those are `|P|` independent reductions of
identical shape.

**But the first order of magnitude is not on a card, it is in the representation.**
Every field element here is an `int64_t` through Givaro's `Modular<int64_t>`, and
there is no specialisation for GF(2) anywhere in the tree. Over GF(2) a span
membership test is an XOR reduction, so:

| shape | one map, as now | as a bitset | factor |
|---|---|---|---|
| 9x9, `⟨3,3,3⟩` | 81 x 8 bytes = 648 B | 81 bits = 2 words, 16 B | **40x** |
| 16x16, `⟨4,4,4⟩` | 256 x 8 bytes = 2 048 B | 256 bits = 4 words, 32 B | **64x** |

The same factor applies to the inner loop, which becomes two or four XORs per
basis row instead of eighty-one or two hundred and fifty-six field operations.
Most fixtures here are over GF(2).

**That is the recommendation: a GF(2) bitset representation before any thought of
a GPU.** It is one representation, it needs no new hardware, it compounds with the
addressed pool already in place, and it attacks the loop this repository has
already identified as the hot one. Whether the leaf test then wants a GPU is a
question worth asking afterwards and not before.

## What the neighbours actually run on

Flip graphs are independent random walks, which parallelise across CPU cores with
no shared state and no divergence problem. AlphaTensor's accelerators train a
network; the tensor arithmetic is not the part on the TPU. Nothing in this
literature reports an exact finite-field rank search on a GPU, and this page is
not evidence that one is impossible, only that the obvious lever here is elsewhere.
