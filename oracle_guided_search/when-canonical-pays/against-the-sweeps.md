# Both routes at five shapes, and where the predicate was wrong

`factor-over-canonical-basis --route exhaustive|canonical --floor t --ceiling t`,
one level per row, the lock held, fastest of three. `<2,3,3>` and `<3,3,3>` are
here at all because the axis presentation reached them; before it they refused.

| shape | `t` | levels | plain nodes | canonical nodes | canonical images | plain | canonical |
|---|---|---|---|---|---|---|---|
| `<2,2,2>` | 5 | 1 | 6 | 6 | 6 | 0.000225 s | 0.000639 s |
| `<2,2,2>` | 6 | 2 | 648 | **58** | 483 | 0.00151 s | 0.00957 s |
| `<2,2,2>` | 7\* | 3 | 3 167 | **14** | 1 451 | 0.00695 s | 0.0295 s |
| `<2,2,3>` | 7 | 1 | 6 | 6 | 6 | 0.000920 s | 0.00190 s |
| `<2,2,3>` | 8 | 2 | 2 748 | **85** | 676 | 0.00832 s | 0.0357 s |
| `<2,2,4>` | 9 | 1 | 6 | 6 | 6 | 0.00550 s | 0.0101 s |
| `<2,2,4>` | 10 | 2 | 11 130 | **92** | 698 | 0.0791 s | 0.144 s |
| `<2,3,3>` | 7 | 1 | 11 | 11 | 11 | 0.103 s | 0.127 s |
| `<2,3,3>` | 8 | 2 | 229 870 | **1 083** | 15 754 | 0.843 s | 11.4 s |
| `<3,3,3>` | 10 | 1 | 14 | 14 | 14 | 4.90 s | **2.36 s** |

\* `<2,2,2>` at 7 is the only satisfiable row: both routes stop at the first
solution, so its counts are of a search that found something rather than of a level
walked out. Every other row is a refutation and its counts are exact.

These are the timings **after** the two changes
[where-the-time-goes.md](where-the-time-goes.md) asked for. What they were worth,
against the same sweep before either:

| shape | `t` | canonical, before | after | canonical against plain |
|---|---|---|---|---|
| `<2,2,2>` | 6 | 0.0164 s | 0.00957 s | 11.5x -> **6.3x** |
| `<2,2,2>` | 7 | 0.0427 s | 0.0295 s | 7.2x -> **4.2x** |
| `<2,2,3>` | 8 | 0.0927 s | 0.0357 s | 11.7x -> **4.3x** |
| `<2,2,4>` | 10 | 0.500 s | 0.144 s | 6.4x -> **1.8x** |
| `<2,3,3>` | 8 | 98.0 s | 11.4 s | 116x -> **13.5x** |
| `<3,3,3>` | 10 | 3.95 s | 2.36 s | 1.33x in favour -> **2.07x** |

**Every node count is identical to the run before either change**, at every shape
and every level, which is how it is known that only the clock moved. The canonical
images fell 31% to 45% with the second change and the node counts did not move
then either.

**Node counts are exact and machine independent. Seconds are not, and these were
not taken on a quiet machine** — `/proc/loadavg` ran 1.2 to 4.4 while other work
shared the box, which [`../../MEASURING.md`](../../MEASURING.md) says is grounds
for abandoning a timing. They are kept because both routes of a row ran back to
back against the same background, so a *ratio within a row* is evidence where an
absolute is not. `<2,3,3>` at 7 is now 1.23x, close enough to the 13% thermal band
that it is printed as two numbers rather than claimed as a ratio. **The row that
most needs a quiet machine is the last**, the only win; the plain side of it came
out 5.24 s, 4.92 s, 5.23 s and 4.90 s on four separate occasions, which is the
spread that band predicts and no more.

## Three things the predicate got wrong

**One. `rho <= |G|` is true and useless.** Orbit counting bounds the node saving by
the group order, and the first version took the bound as the estimate. The measured
saving reaches **5%, 0.5%, 0.017% and 0.13%** of it at the four shapes swept. At
`<2,2,3>` that made the predicate fire on a route that loses — by 11.7x when the
error was found, 5.2x now — so it was wrong by two orders of magnitude, and wrong
in the direction that costs a user an afternoon.

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
fitted, not derived; its errors against the ten rows are 2.1x, 2.6x, 1.0x, 1.0x,
1.2x, 1.2x, 1.0x, 1.1x, **11.7x** and 2.1x. `../tests/test_route_price.cpp` holds
the model to every row and to the count of rows whose *verdict* it gets wrong,
which is **one**: `<3,3,3>` at 10, where canonical augmentation wins 2.07x with a
node saving of zero. That win is not a node saving and this model does not carry
it — [where-the-time-goes.md](where-the-time-goes.md) says what it is instead, and
it is the model's most consequential remaining defect, since it refuses the route
at exactly the largest shape where the route wins.
