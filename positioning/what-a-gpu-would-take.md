# What a GPU would take, part by part

[`hardware-and-parallelism.md`](hardware-and-parallelism.md) answers the GPU
question for the exhaustive **tree** (warp divergence) and the **solver** route
(GPU CDCL is a published negative result). It never examined the other two parts
anybody asks about. This page does, and it corrects a number.

Nothing here is built. There is no `.cu`, `.cl` or `.hip` in this repository and
no build wiring for one. The hardware present is an RTX 4060 Laptop, 8188 MiB,
compute capability 8.9; `nvcc` is absent but `nvidia-cuda-toolkit` is in the
Ubuntu archive, so the toolchain is one install away rather than a project.

## The rule this page is applied with

If a fraction `F` of the runtime is in the part you accelerate and you make that
part `S` times faster, the whole run improves by `1 / ((1 - F) + F/S)`. As
`S` grows without bound the ceiling is `1 / (1 - F)`. So the first question about
any candidate is never how fast a kernel could be, it is what `F` is.

## The `C A` multiply: dead, at any speed

`factorisation.cpp`'s recovery loop is `slices x rows*columns x k` field
operations: **448** on `<2,2,2>` and about 16 800 on `<3,3,3>`. It looks like the
GPU-shaped part because it is a matrix product with independent iterations, and it
is, but it runs once at the end and `F` is on the order of `0.0001`. The ceiling is
1.0000x. Measured for scale: `factor-over-canonical-basis` on `<2,2,2>` is 0.182 s
and 3815 nodes, essentially all of it the search.

## Pool generation: 4% at one shape and the whole run at another

This is the number [`hardware-and-parallelism.md`](hardware-and-parallelism.md)
would have got wrong. Building the 261 121 maps of `<3,3,3>` costs **0.14 s of a
3.63 s run, under 4%**, including process start and the bounds. Ceiling 1.04x, so
at that shape it is not worth a kernel.

**At `<4,4,4>` the same reasoning inverts.** The pool cannot be materialised at
all, so `RankOnePool::at(i)` is not a setup step: it is called once per element per
leaf visit, 4 294 836 225 times, **measured at 785 ns each**. Generation stops
being 4% of the run and becomes almost all of it, `F` approaches 1, and the ceiling
stops being the constraint. **A measurement at one shape said nothing about the
other, and quoting it as though it did would have been the mistake this repository
keeps making.**

That 785 ns is also an artefact rather than a floor. `at(i)` builds a 16x16 matrix
over GF(2) with **256 Givaro multiplications and a heap allocation**, for an object
that is 256 bits and an outer product of two 16-bit words. The bit-packed leaf
elsewhere here measured 6.0x to 39.6x against the general path on the same work,
so a packed `at(i)` plausibly lands near 20 to 80 ns before any accelerator.

## The leaf scan: the one part that is actually shaped for it

Four things line up, and they are the reason this section exists:

- **It is most of the run.** The bit-packed leaf's measured 10.1x on
  `f2_5x5 --target 11` puts `F` above 90% by the rule above.
- **It is enormous where it matters.** One `<4,4,4>` leaf is 4.29e9 independent
  iterations, which at the measured rate is **0.9 hours**.
- **Each iteration is branch-free over GF(2)**: an outer product, a bit-pack, and
  a span-membership test that is exclusive or and a zero check on four words.
- **Nothing needs transferring.** `at(i)` is a pure function of `i`, so a thread
  derives its own element from its index. What lives in memory is the two vector
  lists at about 131 KB each, the span basis at **1.5 KB**, and a survivor buffer:
  **under 300 KB against 8 GB.** The 8.2 TiB never exists anywhere.
  [`../orbit_reduction/pool_orbits.h`](../orbit_reduction/pool_orbits.h)'s
  `PoolAction` is the other lookup a kernel would want, and it is arithmetic on
  two shared tables rather than a table read.

The one loop-carried dependency stays on the host: the greedy `try_add` runs only
on survivors, and a leaf needs `k` of them out of 4.29e9.

## What would decide it, and what would not

The honest experiment is standalone and needs no branch: a kernel over
`i` in `[0, 4.29e9)` that regenerates, packs, and reduces against a basis in
constant memory, timed against the measured 785 ns per element. Against 0.9 hours
a leaf: **50x makes it 65 s and is worth wiring in; 10x makes it 5.4 minutes,
which is real and still does not finish `<4,4,4>`; under 5x is the memory
bandwidth wall and the answer is no.**

Two things to hold. A 256-bit exclusive-or reduce is likely bandwidth bound rather
than compute bound, so the middle band is the prior. And **this is GF(2) only**:
Givaro carries every element as an `int64_t`, so GF(3), GF(5) and the rationals
have none of this shape.
