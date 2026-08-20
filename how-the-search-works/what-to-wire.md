# What to wire, and what to leave where it is

One verdict per piece of [`the-whole-algorithm.md`](the-whole-algorithm.md),
with a measurement behind each. Of the three not in `decide-rank`, one should be,
one belongs somewhere other than where it was written for, and one should stay
behind its flag.

| piece | verdict | the number it rests on |
|---|---|---|
| orbit quotient | **wired**, on `-s` | 39.2x fewer nodes refuting `⟨2,2,2⟩` at 6, at a 1.41x surcharge a node |
| odometer | **wired**, automatic | the only route at `⟨4,4,4⟩`, where a held pool is 8.2 TiB |
| GPU leaf | **verdict suspended** — the host moved under it | was 81x on the hardest comparison; the host leaf has since changed twice |
| `SortedSpan` | **wire the cost query, not the search** | 1.10x there; refuted at a node, dominated at a leaf |
| McKay | **leave it on `--route canonical`** | 53x fewer nodes at 299x a node |

## The GPU: the verdict is suspended, because the host moved

**Everything below was measured against a host leaf that has since changed
twice** — `is_rank_one` and the carried residual both landed on 2026-08-20, and
the second alone took a scan element from 108 ns to 1.16 ns in a harness. Against
a card at 0.24 ns that is under 5x, which is this page's own "not worth it" band.
Nothing here is retracted; it is suspended until the comparison is re-taken with
`measure-leaf`, which now times the leaf that ships.

The seam is [`../run_limits/device.h`](../run_limits/device.h) and it is already
the right shape: a fixed ranking, a probe for availability, a host that always
answers. What decides whether a leaf reaches the card is `launch_floor()`, and
that number is **`PROVISIONAL` at 100 000 and was never measured on this
machine** — it is a conventional launch cost times a rate taken from a kernel
that is not wired in.

That floor decides every shape here at once. A leaf's work is `min(p^dim, |P|)`:

| shape | `\|P\|` | leaf work, published targets | against a floor of 100 000 |
|---|---|---|---|
| 4x4 | 225 | 64 to 225 | host |
| 5x5 | 961 | 961 | host |
| 3x8 | 1 785 | 1 785 | host |
| 9x9 | 261 121 | 131 072 walking, 261 121 scanning | **card** |
| 16x16 | 4 294 836 225 | 2.1e9 walking, 4.3e9 scanning | **card**, and no tree to reach it |

**So wiring the card changes nothing below `⟨3,3,3⟩` and everything at it.**
`⟨4,4,4⟩` stays unreachable for a reason no leaf can fix: the tree above it has a
node count nothing here bounds. One shape is where this is worth anything, and it
is one this repository publishes.

Two things the wiring must respect, both already provided for:
`GpuSurvivors::overflowed` means the indices are **not an answer** and the caller
re-runs in chunks rather than reading them, and the survivors are sorted before
the host greedy walks them, which is what makes the answer bit-identical to the
sequential leaf
([`../gpu_leaf/why-the-answer-is-the-same.md`](../gpu_leaf/why-the-answer-is-the-same.md)).

## `SortedSpan`: it has three possible homes and two of them are closed

- **At a node**, as `cost(V)` meeting the floor: sound, and measured not to fire.
  The cheapest `cost(V)` seen is 8, 8, 8 over 4000 nodes of `matmul_2x2x2`
  against a floor of 6, so it fires only where the search is two nodes anyway:
  [`../exhaustive_search/what-a-node-cannot-tell-you.md`](../exhaustive_search/what-a-node-cannot-tell-you.md).
- **At a leaf**, as `dim R[1] == dim V`: that *is* the walk route, minus its early
  exit and with a Gaussian rank in place of a rank-one test.
- **At the descent's cost query**, which is what it was written for, and stands.

That query is asked once per candidate per pass and only its **number** is read;
the basis is needed once per pass-end and once per adoption. Measured off the
step-1 basis, A-B-A in one process so the comparison does not straddle the
thermal band, three runs:

| route | `f2_5x5`, 200 queries | `f3_3x6`, 40 queries |
|---|---|---|
| minimum-weight basis, as it ships | 0.0831 s (repeat 0.0828) | 0.2910 s (repeat 0.2882) |
| the filtration, given the same known ranks | **0.0751 s** | **0.2731 s** |
| the filtration, as it was written | 0.1411 s | — |

Every query agreed. **The third row is the finding**: switching the callers to
`SortedSpan` as it stood would have cost **1.70x**, because it recomputed the
half of the enumeration `minimum_weight_basis` is handed. It now takes the same
`ranks_without_last` and the gap closes to 1.10x, which is inside the 13% this
chassis moves by and so is not a speed-up this repository may publish. Wire it
for the sort it deletes and the byte an element it holds instead of sixteen, and
claim nothing about the clock.

## McKay: the flag is where it belongs

25.8x slower on the wall clock while removing 53x the nodes. (This said "299x a
node plus a 0.196 s entry fee" until 2026-08-20; the fee was a fitting artefact
and the per-node cost varies seventeen-fold across the sweep's own levels.) Both
sides of that comparison
already had the group, so this is the marginal value of the parent test **over**
the orbit quotient and not over nothing. It stays wired and off, because a route
known to lose is worth more than an unwired one somebody proposes again:
[`../canonical_factorisation/canonical-augmentation.md`](../canonical_factorisation/canonical-augmentation.md).
