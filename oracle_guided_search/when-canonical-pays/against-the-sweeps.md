# Both routes at five shapes, and where the predicate was wrong

`factor-over-canonical-basis --route exhaustive|canonical --floor t --ceiling t`,
one level per row, the lock held, fastest of three. `<2,3,3>` and `<3,3,3>` are
here at all because the axis presentation reached them; before it they refused.

| shape | `t` | levels | plain nodes | canonical nodes | canonical images | plain | canonical |
|---|---|---|---|---|---|---|---|
| `<2,2,2>` | 5 | 1 | 6 | 6 | 10 | 0.000203 s | 0.000557 s |
| `<2,2,2>` | 6 | 2 | 648 | **58** | 784 | 0.00143 s | 0.0164 s |
| `<2,2,2>` | 7\* | 3 | 3 167 | **14** | 2 146 | 0.00595 s | 0.0427 s |
| `<2,2,3>` | 7 | 1 | 6 | 6 | 10 | 0.000909 s | 0.00251 s |
| `<2,2,3>` | 8 | 2 | 2 748 | **85** | 1 092 | 0.00790 s | 0.0927 s |
| `<2,2,4>` | 9 | 1 | 6 | 6 | 10 | 0.00715 s | 0.0129 s |
| `<2,2,4>` | 10 | 2 | 11 130 | **92** | 1 125 | 0.0779 s | 0.500 s |
| `<2,3,3>` | 7 | 1 | 11 | 11 | 20 | 0.105 s | 0.198 s |
| `<2,3,3>` | 8 | 2 | 229 870 | **1 083** | 25 664 | 0.847 s | 98.0 s |
| `<3,3,3>` | 10 | 1 | 14 | 14 | 26 | **4.92 s** | **3.70 s** |

\* `<2,2,2>` at 7 is the only satisfiable row: both routes stop at the first
solution, so its counts are of a search that found something rather than of a level
walked out. Every other row is a refutation and its counts are exact.

**Node counts are exact and machine independent. Seconds are not, and these were
not taken on a quiet machine** — `/proc/loadavg` ran 1.2 to 2.6 while other work
shared the box, which [`../../MEASURING.md`](../../MEASURING.md) says is grounds
for abandoning a timing. They are kept because both routes of a row ran back to
back against the same background, so a *ratio within a row* is evidence where an
absolute is not, and because the ratios are 6x to 116x, far outside the 13%
thermal band. **The one row that needs a quiet machine is the last**, whose 1.33x
is the only win here; it was measured twice, at three repeats and at five, and came
out 1.330x and 1.331x.

## Three things the predicate got wrong

**One. `rho <= |G|` is true and useless.** Orbit counting bounds the node saving by
the group order, and the first version took the bound as the estimate. The measured
saving reaches **5%, 0.5%, 0.017% and 0.13%** of it at the four shapes swept. At
`<2,2,3>` that made the predicate fire where the route loses by 11.7x — wrong by
205x, and wrong in the direction that costs a user an afternoon.

**Two. At one level of augmentation the saving is exactly nothing.** All five
one-level rows have **equal node counts**, 6, 6, 6, 11 and 14. The baseline's
single-generator rejection already emits one child per pool orbit, so there is no
duplication left for a parent test to remove and the canonical route pays for a
quotient that has already been taken. The model had no such case and priced a root
with its leaves as though it were a tree, which is where its 174x-to-55 000x
predictions at those rows came from.

**Three. The price is a pool scan, not a group operation** — which is the finding
that matters and has its own page:
[where-the-time-goes.md](where-the-time-goes.md).

## What replaced it

    rho = min( |G|, (pool scan / plain node) ^ (levels - 1) )

with `levels == 1` priced as a root and its children rather than as a tree. It is
fitted to the four two-level rows and one three-level row and it is not a bound;
its errors against them are 3.5x, 1.5x, 1.0x, 1.3x and 15.9x, the last being
`<2,3,3>`, where it under-predicts the loss. `../tests/test_route_price.cpp` holds
the model to all ten rows and to the count of rows it gets wrong, which is **one**:
`<3,3,3>` at 10, where canonical augmentation wins with a saving of zero. That win
is not a node saving and this model does not carry it —
[where-the-time-goes.md](where-the-time-goes.md) says what it is instead.
