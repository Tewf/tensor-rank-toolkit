# What one consumer GPU is worth on the leaf test

[`../positioning/what-a-gpu-would-take.md`](../positioning/what-a-gpu-would-take.md)
named the leaf test as the one part of this repository shaped for a card, and
said exactly what would decide it: a kernel over the pool index, regenerating
each candidate from that index, timed against the leaf that already exists. This
is that measurement, on an RTX 4060 Laptop, 24 SMs, compute capability 8.9.

**It is a proof of concept and not an integration.** Nothing outside this
directory calls it, neither search changed, and the top-level `CMakeLists.txt`
reads this one only behind `find_package(CUDAToolkit QUIET)`, so a machine
without `nvcc` builds and tests exactly as before. What is wanted is a number a
laboratory with real hardware can multiply, not a faster laptop.

    cmake -S . -B build -DCMAKE_CUDA_COMPILER=$(command -v nvcc)
    cmake --build build -j 6 --target measure-leaf
    flock /tmp/bilinear-measure.lock ./build/gpu_leaf/measure-leaf both

## What the kernel does

`RankOnePool::at(i)` is a pure function of `i`: element `i` is
`lefts[i / |rights|] ⊗ rights[i % |rights|]`. Over GF(2) each of those vectors is
a 16-bit mask, so a thread derives its own candidate and **nothing is transferred
per element**. What lives on the card is the two mask tables at 131 KB each, the
span in `__constant__` at 2 KB, and a survivor buffer: under 300 KB against 8 GB,
where the materialised pool `all_rank_one_maps` prices and refuses is 8.2 TiB.
The 8.2 TiB never exists anywhere.

[`pool_scan.cu`](pool_scan.cu) is the scan route, one thread per rank-one map:
derive it, reduce it against the span, report the index if nothing is left.
[`subspace_walk.cu`](subspace_walk.cu) is the walk route, one thread per subspace
element: exclusive or of the span rows the index selects, then the rank-one test.
The part that costs is branch-free in both, so a warp of 32 candidates costs what
one costs. The shape is a template parameter, which is what keeps the candidate
in registers: `ptxas` reports **38 registers and zero spill** for the 16x16 scan
kernel, and zero spill for every other shape and route.

Why that is the same answer the sequential leaf gives, and the thirteen
comparisons that assert it survivor by survivor:
[`why-the-answer-is-the-same.md`](why-the-answer-is-the-same.md).

## The verdict

The bands set before the measurement were **50x worth wiring in, 10x real and
insufficient, under 5x the bandwidth wall**. Against the baseline measured here
rather than against the 785 ns that would have flattered it, all of which is in
[`what-the-card-did.md`](what-the-card-did.md):

| one RTX 4060 against | pool scan | subspace walk |
|---|---|---|
| the leaf as shipped, **1 core** | **3962x** | **461x** |
| the leaf as shipped, 12 threads | 480x | 68x |
| packed generation on the host, 1 core | 544x | 528x |
| packed generation on the host, 12 threads | **81x** | **70x** |

**It clears the 50x band on every comparison, including the hardest one.** The
hardest is the last row: the whole CPU, twelve threads, running the kernel's own
arithmetic rather than the leaf that ships. One RTX 4060 Laptop is 81x that on
the scan and 70x on the walk. Against the configuration everything in this
repository is published at, one core and the leaf as it stands, it is 3962x, and
**one whole `⟨4,4,4⟩` leaf is 1.02 s where one core would spend 67 minutes.**

**It is compute bound, and the prior that said otherwise was wrong.**
[`../positioning/what-a-gpu-would-take.md`](../positioning/what-a-gpu-would-take.md)
expected "a 256-bit exclusive-or reduce is likely bandwidth bound" and made the
middle band the prior. Nothing streams. The scan reads four bytes per element
from a 262 KB table that never leaves L2, which is 17 GB/s against 272 GB/s of
device bandwidth, and the walk reads no global memory at all.

The two kernels agreeing says the same thing from the other side. Each spends
its life in one loop over span rows, 47 of them scanning and 27 walking, and the
measured rates are 4.22e9 and 7.36e9, a ratio of 1.746 against the 1.741 those
row counts predict. Their two fixed costs, the outer product and the rank-one
test, are near enough the same size to cancel, so the agreement is closer than
the argument deserves; what it shows is that the row loop is what the card is
spending on, and not a memory system.

## What this does not show

- **That the win is the card.** 7.3x of the 3962x is not: it is deriving the
  candidate with two shifts instead of 256 Givaro multiplications, and that is
  available on one core today, in this repository, for no new hardware.
- **Anything about the tree.** [`../positioning/hardware-and-parallelism.md`](../positioning/hardware-and-parallelism.md)
  stands: the depth-first walk above the leaf diverges at every node and is the
  wrong shape for a warp. A leaf is one second and the tree above it has a node
  count nothing here bounds, so a card does not finish `⟨4,4,4⟩` either.
- **Anything about another field.** Givaro carries every element as an `int64_t`;
  GF(3), GF(5) and the rationals have none of this shape.
- **Anything about a shape not compiled here.** Four have kernels, 4x4, 5x5, 9x9
  and 16x16, because they are the ones the fixtures reach.

Every number here was taken under rules that differ from the rest of the
repository's, because a kernel is not a slower or faster version of the thing it
replaces: [`measuring-on-the-card.md`](measuring-on-the-card.md).
