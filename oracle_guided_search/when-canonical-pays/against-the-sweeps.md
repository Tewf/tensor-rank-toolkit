# Both routes at five shapes, and where the predicate was wrong

`factor-over-canonical-basis --route exhaustive|canonical --floor t --ceiling t`,
one level per row, the lock held, fastest of three. `<2,3,3>` and `<3,3,3>` are
here at all because the axis presentation reached them; before it they refused.

| shape | `t` | levels | plain nodes | canonical nodes | canonical images | plain | canonical |
|---|---|---|---|---|---|---|---|
| `<2,2,2>` | 5 | 1 | 6 | 6 | 10 | 0.000286 s | 0.000705 s |
| `<2,2,2>` | 6 | 2 | 648 | **58** | 784 | 0.00119 s | 0.0112 s |
| `<2,2,2>` | 7\* | 3 | 3 167 | **14** | 2 146 | 0.00639 s | 0.0359 s |
| `<2,2,3>` | 7 | 1 | 6 | 6 | 10 | 0.000905 s | 0.00194 s |
| `<2,2,3>` | 8 | 2 | 2 748 | **85** | 1 092 | 0.00850 s | 0.0443 s |
| `<2,2,4>` | 9 | 1 | 6 | 6 | 10 | 0.00555 s | 0.00978 s |
| `<2,2,4>` | 10 | 2 | 11 130 | **92** | 1 125 | 0.0808 s | 0.164 s |
| `<2,3,3>` | 7 | 1 | 11 | 11 | 20 | 0.104 s | 0.130 s |
| `<2,3,3>` | 8 | 2 | 229 870 | **1 083** | 25 664 | 0.846 s | 13.1 s |
| `<3,3,3>` | 10 | 1 | 14 | 14 | 26 | 5.23 s | **2.41 s** |

\* `<2,2,2>` at 7 is the only satisfiable row: both routes stop at the first
solution, so its counts are of a search that found something rather than of a level
walked out. Every other row is a refutation and its counts are exact.

These are the timings **after** [where-the-time-goes.md](where-the-time-goes.md)'s
first item was done, which took the canonical route from 116x against at `<2,3,3>`
to 15.5x and turned `<3,3,3>` from a 1.33x win into a **2.17x** one. Every node and
canonical-image count is identical to the run before it, which is how it is known
that only the clock moved.

**Node counts are exact and machine independent. Seconds are not, and these were
not taken on a quiet machine** — `/proc/loadavg` ran 1.2 to 4.4 while other work
shared the box, which [`../../MEASURING.md`](../../MEASURING.md) says is grounds
for abandoning a timing. They are kept because both routes of a row ran back to
back against the same background, so a *ratio within a row* is evidence where an
absolute is not. Two of the ratios, `<2,2,4>` at 10 and `<2,3,3>` at 7, are now
2.03x and 1.26x: the second is inside the band that
[`../../MEASURING.md`](../../MEASURING.md) forbids quoting, and it is printed as a
number rather than as a ratio for that reason. **The row that most needs a quiet
machine is the last**, the only win; the plain side of it came out 5.24 s, 4.92 s
and 5.23 s on three separate occasions.

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
fitted, not derived; its errors against the ten rows are 2.4x, 2.4x, 1.1x, 1.0x,
1.2x, 1.2x, 1.0x, 1.2x, **13.0x** and 2.2x. `../tests/test_route_price.cpp` holds
the model to every row and to the count of rows whose *verdict* it gets wrong,
which is **one**: `<3,3,3>` at 10, where canonical augmentation wins 2.17x with a
node saving of zero. That win is not a node saving and this model does not carry
it — [where-the-time-goes.md](where-the-time-goes.md) says what it is instead, and
it is the model's most consequential remaining defect, since it refuses the route
at exactly the largest shape where the route wins.
