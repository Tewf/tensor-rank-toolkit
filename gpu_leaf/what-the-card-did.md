# What the card did, and what it was measured against

The tables behind [`README.md`](README.md)'s verdict. Measured under
[`../MEASURING.md`](../MEASURING.md): one core where it says one core, fastest of
three, machine otherwise quiet, under `flock /tmp/bilinear-measure.lock`, at
`-O3` with no `-march=native`, which is how the rest of this repository is built.

## The baseline, which did not exist before this

The only per-element figure this repository had was **785 ns**, the *general*
Givaro path at `⟨4,4,4⟩`. The GF(2) leaf is 6.0x to 39.6x faster than that path on
the same work
([`../exhaustive_search/gf2_leaf.h`](../exhaustive_search/gf2_leaf.h)), so timing
a kernel against 785 ns would have flattered the card by up to 23x. So the first
thing measured is the GF(2) leaf itself, on an **addressed** pool at 16x16 over
GF(2), one core. Three host rows follow and they are three different claims:

- **the shipped leaf** was `Gf2Leaf<Addressed>` as the search called it *that
  morning*, deriving each candidate through `RankOnePool::at` at 256 Givaro
  multiplications and a heap allocation. **It stopped shipping at 08:21 the same
  day**, when `d85fd32` gave `Gf2Leaf` the masked derivation below, so this row
  now prices a path nothing takes;
- **packed generation on the host** is the kernel's own arithmetic on one core,
  so the card is not credited with a win that belongs to the representation;
- **twelve threads** is the same range on all twelve, not a partition of one
  leaf: a `Gf2Leaf` scan starts at index 0 and cannot be started elsewhere
  without changing it, so the row prices twelve cores on this work.

## The pool scan, 16x16 over GF(2), dimension 47

This is the route `⟨4,4,4⟩` takes at a leaf. The walk is chosen only while
`2^dim` is under the 4 294 836 225 maps of the pool, so from dimension 32 upward
the leaf is a scan, and a target of 47 products is a scan.

| what | elements | seconds | ns/element | elements/s |
|---|---|---|---|---|
| former leaf, addressed pool, 1 core | 13 107 000 | 12.323 | 940.2 | 1.06e6 |
| former leaf, addressed pool, 12 threads | 157 284 000 | 17.915 | 113.9 | 8.78e6 |
| **the leaf as it ships now**, 1 core | 131 070 000 | 16.917 | **129.1** | 7.75e6 |
| **the leaf as it ships now**, 12 threads | 1 572 840 000 | 30.209 | 19.2 | 5.21e7 |
| one RTX 4060, the rows the host was given | 131 070 000 | 0.035 | 0.27 | 3.77e9 |
| one RTX 4060, **the whole pool** | 4 294 836 225 | **1.019** | 0.24 | **4.22e9** |

**129.1 ns is the baseline now, and 940.2 ns was for seven hours.** Confirmed
independently on 2026-08-20 by differencing two `--leaf-limit` values on
`decide-rank --matmul 2 4 4 4 --target 47`, so process start-up and the mask
tables cancel: 40 000 000 further elements cost 4.81 s, or **120.3 ns each**,
which is the packed rate inside the thermal band and nowhere near 940.2.

**What the old baseline was.** The 785 ns is close to it because at `⟨4,4,4⟩` the
two measure nearly the same thing: the bit-packed leaf replaces the membership
test but not `RankOnePool::at`, and on an addressed pool the rebuild is most of
the element. That is also why packed generation is worth 7.3x here on one core,
with no accelerator involved.

The two card rows are the control on the four host rows. A host row covers a
prefix of the outer-product grid and the last card row covers all of it; they
agree to 12%, which is the thermal band, so the prefix is representative.

**One whole `⟨4,4,4⟩` leaf, 4 294 836 225 rank-one maps, is 1.02 s on one card.**
At the rates above, the same leaf is **9.2 minutes** of one core as the search
runs today and 82 seconds of all twelve threads; it was 67 minutes on the path
that shipped when this was written. Those three are arithmetic on a measured
rate and are not measurements; only the 1.02 s was timed end to end.

## The subspace walk, 16x16 over GF(2), dimension 27

Every row walks the **whole** subspace and not a prefix. Both host routes skip a
basis row whose digit is zero and the kernel cannot, so a prefix would have
compared an average popcount against a full one.

| what | elements | seconds | ns/element | elements/s |
|---|---|---|---|---|
| shipped leaf, 1 core | 134 217 728 | 8.405 | **62.6** | 1.60e7 |
| shipped leaf, 12 threads | 1 610 612 736 | 14.882 | 9.2 | 1.08e8 |
| packed generation on the host, 1 core | 134 217 728 | 9.636 | 71.8 | 1.39e7 |
| packed generation on the host, 12 threads | 1 610 612 736 | 15.278 | 9.5 | 1.05e8 |
| one RTX 4060 | 134 217 728 | **0.018** | 0.14 | **7.36e9** |

**Packed generation buys nothing here, and that is the expected result.** The
walk never touches the pool, so there is no `RankOnePool::at` to replace. The two
host rows differ by 15%, which is inside the thermal band, and the difference
between them is not a result.

Dimension 31 is the widest walk `⟨4,4,4⟩` can pose, since past it `2^dim` exceeds
the pool and the leaf becomes a scan. The card walks all 2 147 483 648 of its
elements in **0.290 s**, at 7.42e9 elements per second. No host row accompanies
it: one core would be minutes, and the ratio is already established above.

## What repeating the whole thing showed

Every table above was taken twice, on two builds a code review apart.

**The card reproduced to three digits**: 4.216e9 elements per second on the whole
pool both times, 1.019 s both times, 0.290 s on the widest walk both times.
**Every host row came back 3% to 12% slower**, the shipped ones included, and
those are untouched by anything that changed between the builds, so the spread is
the chassis: the second run followed fifteen minutes of somebody else's
single-core job, and 13% run to run from throttling alone is what
[`../MEASURING.md`](../MEASURING.md) documents this machine at. By that file's
rule the minimum over runs is the closest estimate of the work, so each row above
keeps its faster observation, which is the first run for every host row and a tie
for the card. **The published ratios are the conservative ones**, taken where the
host did best.

The repeat also settles a doubt a review raised. The host reference is inline in
a header and its result was discarded, which is the shape a compiler may delete
outright; it now goes to a `volatile`, and the rows moved no further than the
shipped rows beside them. Nothing was being deleted.
