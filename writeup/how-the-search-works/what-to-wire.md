# What to wire, and what to leave where it is

One verdict per piece of [`the-whole-algorithm.md`](the-whole-algorithm.md),
with a measurement behind each. Of the three not in `decide-rank`, one should be,
one belongs somewhere other than where it was written for, and one should stay
behind its flag.

| piece | verdict | the number it rests on |
|---|---|---|
| orbit quotient | **wired**, on `-s` | 39.2x fewer nodes refuting `⟨2,2,2⟩` at 6, at about a 1.3x surcharge a node |
| odometer | **wired**, automatic | the only route at `⟨4,4,4⟩`, where a held pool is 8.2 TiB |
| GPU leaf | **verdict suspended**: the host moved under it | was 81x on the hardest comparison; the host leaf has since changed twice |
| `SortedSpan` | **wire the cost query, not the search** | 1.10x there; refuted at a node, dominated at a leaf |
| McKay | **leave it on `--route canonical`**, and the predicate that says so is now wired to nothing on purpose | the one row it wins is 99x of the baseline's own orbit test |

## The GPU: the verdict is suspended, because the host moved

**Everything below was measured against a host leaf that has since changed
twice**: `is_rank_one` and the carried residual both landed on 2026-08-20, and
the second alone took a scan element from 108 ns to 1.16 ns in a harness. Against
a card at 0.24 ns that is under 5x, which is this page's own "not worth it" band.
Nothing here is retracted; it is suspended until the comparison is re-taken with
`measure-leaf`, which now times the leaf that ships.

The seam is [`../../infrastructure/run_limits/device.h`](../../infrastructure/run_limits/device.h) and it is already
the right shape: a fixed ranking, a probe for availability, a host that always
answers. What decides whether a leaf reaches the card is `launch_floor()`, and
that number was **measured at 8 192 on 2026-08-21** by `measure-leaf floor`: the
smallest count at which every route on every compiled shape beat one core, timed
against the card's wall clock and not its kernel clock. It was `PROVISIONAL` at
100 000 when this page was written, a conventional launch cost times a rate taken
from a kernel that was not wired in. The per-route table is in
[`../../infrastructure/run_limits/device.cpp`](../../infrastructure/run_limits/device.cpp) and the walks cross at
4 096 except 5x5's at 8 192.

That floor decides every shape here at once. A leaf's work is `min(p^dim, |P|)`:

| shape | `\|P\|` | leaf work, published targets | against the measured floor |
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
([`../../infrastructure/gpu_leaf/why-the-answer-is-the-same.md`](../../infrastructure/gpu_leaf/why-the-answer-is-the-same.md)).

## `SortedSpan`: it has three possible homes and two of them are closed

- **At a node**, as `cost(V)` meeting the floor: sound, and measured not to fire.
  The cheapest `cost(V)` seen is 8, 8, 8 over 4000 nodes of `matmul_2x2x2`
  against a floor of 6, so it fires only where the search is two nodes anyway:
  [`../../methods/bilinear_rank/exhaustive/what-a-node-cannot-tell-you.md`](../../methods/bilinear_rank/exhaustive/what-a-node-cannot-tell-you.md).
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

## McKay: the flag is where it belongs, and now for a stated reason

It stays wired and off, because a route known to lose is worth more than an
unwired one somebody proposes again:
[`../../methods/canonical_factorisation/canonical-augmentation.md`](../../methods/canonical_factorisation/canonical-augmentation.md).
What follows is why the predicate that could switch it on is **also** wired to
nothing, which is the harder half and was an open objection until 2026-08-21.

**There is a condition and it is sound.** `L = target - n*k` is the levels of
augmentation. At `L >= 2` the route loses by 1.9x to 15x structurally: a plain
node scans the live suffix `[from, |P|)` and a canonical node must scan the whole
pool, so the per-node price grows with the pool (73x to 3 204x) where the node
saving does not (11x to 226x). At `L == 1` both routes visit the **identical**
tree, `orbits + 1` nodes either way, and the route wins at `<3,3,3>`: 2.26 s
against 4.87 s. `price_canonical_route` now carries both arms and gets ten rows
of ten.

**Nothing consults it, and that is the finding rather than an omission.** Both
roots name one child per orbit from the same six generators; the plain route asks
`least_in_orbit` per pool element, which costs `Theta(sum |O_i|^2)`, where
`orbit_representatives` answers identically in `Theta(|P|)`. At the `<3,3,3>`
root that is **5.05 s against 51.2 ms for the same 13 children**, and the plain
route's whole 4.87 s run *is* that one call. So the row is 99x of the baseline's
own orbit test, on the one level a real sweep never reaches (`rank_lower_bound`
is 14 at `<3,3,3>`), at a pool 13x past where `--route auto` has already left for
the solver. The fix worth making is the `Theta(|P|)` pass, not the route.

The whole argument, the five-shape table behind it, the partial-symmetry-break
verdict and the one question where canonical augmentation wins outright:
[`../../methods/bilinear_rank/canonical_augmentation/when-canonical-pays/why-nothing-consults-it.md`](../../methods/bilinear_rank/canonical_augmentation/when-canonical-pays/why-nothing-consults-it.md).
