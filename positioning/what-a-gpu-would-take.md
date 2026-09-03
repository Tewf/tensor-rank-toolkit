# What a GPU would take, part by part

[`hardware-and-parallelism.md`](hardware-and-parallelism.md) answers the GPU
question for the exhaustive **tree** (warp divergence) and the **solver** route
(GPU CDCL is a published negative result). It never examined the other two parts
anybody asks about. This page does, and it corrects a number.

**The leaf part is now built and measured**, in
[`../gpu_leaf/README.md`](../infrastructure/gpu_leaf/README.md), on the RTX 4060 Laptop this page
was written beside. The two other parts below stay forecasts, and are marked as
such. This paragraph said "nothing here is built" until 2026-08-19; what changed
is one directory of CUDA, guarded so that a machine without `nvcc` builds and
tests exactly as before.

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
1.0000x. Measured for scale: `factor-over-canonical-basis` on `<2,2,2>` is
0.0102 s and 3815 nodes, essentially all of it the search. That read 0.182 s
until the packed leaf landed, and the multiply did not move, so `F` fell with it.

## Pool generation: 4% at one shape and the whole run at another

This is the number [`hardware-and-parallelism.md`](hardware-and-parallelism.md)
would have got wrong. Building the 261 121 maps of `<3,3,3>` costs **0.14 s of a
3.63 s run, under 4%**, including process start and the bounds. Ceiling 1.04x, so
at that shape it is not worth a kernel.

**At `<4,4,4>` the same reasoning inverts.** The pool cannot be materialised at
all, so `RankOnePool::at(i)` is not a setup step: it is called once per element per
leaf visit, 4 294 836 225 times, at 785 ns each then and **129.1 ns now**. Generation stops
being 4% of the run and becomes almost all of it, `F` approaches 1, and the ceiling
stops being the constraint. **A measurement at one shape said nothing about the
other, and quoting it as though it did would have been the mistake this repository
keeps making.**

That 785 ns was an artefact rather than a floor, and this page said so before it
was fixed: `at(i)` built a 16x16 matrix with **256 Givaro multiplications and a
heap allocation** for an object that is 256 bits.

**The forecast here was 20 to 80 ns for a packed `at(i)`. It is 129.1 ns** on one
core and 19.2 on twelve
([`../gpu_leaf/what-the-card-did.md`](../infrastructure/gpu_leaf/what-the-card-did.md)), so the
guess was 1.6x optimistic about the baseline it was arguing about and fell inside
its band only against a row it was not forecasting.

## The leaf scan: the one part that is actually shaped for it

Four things line up, and they are the reason this section exists:

- **It is most of the run.** The bit-packed leaf's measured 10.1x on
  `f2_5x5 --target 11` puts `F` above 90% by the rule above.
- **It is enormous where it matters.** One `<4,4,4>` leaf is 4.29e9 independent
  iterations, which at the measured rate is **about 9 minutes**.
- **Each iteration is branch-free over GF(2)**: an outer product, a bit-pack, and
  a span-membership test that is exclusive or and a zero check on four words.
- **Nothing needs transferring.** `at(i)` is a pure function of `i`, so a thread
  derives its own element from its index. What lives in memory is the two vector
  lists at about 131 KB each, the span basis at **1.5 KB**, and a survivor buffer:
  **under 300 KB against 8 GB.** The 8.2 TiB never exists anywhere.
  [`../orbit_reduction/pool_orbits.h`](../methods/bilinear_rank/orbit_reduction/pool_orbits.h)'s
  `PoolAction` is the other lookup a kernel would want, and it is arithmetic on
  two shared tables rather than a table read.

The one loop-carried dependency stays on the host: the greedy `try_add` runs only
on survivors, and a leaf needs `k` of them out of 4.29e9.

## What decided it: the bands were cleared, and the prior was wrong

The experiment this section asked for was run, standalone and on no branch of the
searches: a kernel over `i` in `[0, 4.29e9)` that regenerates, packs, and reduces
against a basis in constant memory. The bands were **50x worth wiring in, 10x
real and insufficient, under 5x the bandwidth wall**, and the card clears 50x
against every host baseline, including all twelve threads running the kernel's
own arithmetic. **One whole `<4,4,4>` leaf is 1.02 s.** The tables:
[`../gpu_leaf/what-the-card-did.md`](../infrastructure/gpu_leaf/what-the-card-did.md).
That verdict was taken against the host leaf of its day:
[`../gpu_leaf/README.md`](../infrastructure/gpu_leaf/README.md) now prices the same
comparison at 5x to 14x after the host leaf's later rewrites, and holds its
verdict suspended, not retracted, until `measure-leaf` re-takes it.

**The prior in this paragraph was wrong.** It said a 256-bit exclusive-or reduce
is likely bandwidth bound and made the middle band the prior; it is compute
bound, because nothing streams. It also said to time the kernel against 785 ns,
which would have been the wrong baseline: that figure is the *general* path, and
the GF(2) leaf it replaces was measured at 940 ns per element on an addressed
pool only when someone went and measured it.

**This is GF(2) only**, which was right: Givaro carries every element as an
`int64_t`, so GF(3), GF(5) and the rationals have none of this shape.
