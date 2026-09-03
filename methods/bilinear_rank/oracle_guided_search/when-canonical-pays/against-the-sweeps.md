# Both routes at five shapes, and where the predicate was wrong

`factor-over-canonical-basis --route exhaustive|canonical --floor t --ceiling t`,
one level per row, the lock held, fastest of three. `<2,3,3>` and `<3,3,3>` are
here at all because the axis presentation reached them; before it they refused.

| shape | `t` | levels | plain nodes | canonical nodes | canonical images | plain | canonical |
|---|---|---|---|---|---|---|---|
| `<2,2,2>` | 5 | 1 | 6 | 6 | 6 | 0.000217 s | 0.000598 s |
| `<2,2,2>` | 6 | 2 | 648 | **58** | 483 | 0.00141 s | 0.00921 s |
| `<2,2,2>` | 7\* | 3 | 3 167 | **14** | 1 451 | 0.00582 s | 0.0268 s |
| `<2,2,3>` | 7 | 1 | 6 | 6 | 6 | 0.000649 s | 0.00178 s |
| `<2,2,3>` | 8 | 2 | 2 748 | **85** | 676 | 0.00796 s | 0.0347 s |
| `<2,2,4>` | 9 | 1 | 6 | 6 | 6 | 0.00519 s | 0.00889 s |
| `<2,2,4>` | 10 | 2 | 11 130 | **92** | 698 | 0.0702 s | 0.136 s |
| `<2,3,3>` | 7 | 1 | 11 | 11 | 11 | 0.0972 s | 0.120 s |
| `<2,3,3>` | 8 | 2 | 229 870 | **1 083** | 15 754 | 0.769 s | 11.6 s |
| `<3,3,3>` | 10 | 1 | 14 | 14 | 14 | 4.87 s | **2.26 s** |

\* `<2,2,2>` at 7 is the only satisfiable row: both routes stop at the first
solution, so its counts are of a search that found something rather than of a level
walked out. Every other row is a refutation and its counts are exact.

**The seconds were re-taken on 2026-08-21 and every count came out the same**, at
every shape and every level, which is the check that a rebuild moved a clock and
not an answer. The five one-level rows have a second exact cross-check since:
counting the orbits of each pool under the stabiliser gives 5, 5, 5, 10 and 13,
and `orbits + 1` is the node count of **both** routes at all five. That is not an
observation that the two agree, it is the reason they must.

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
not taken on a quiet machine**: `/proc/loadavg` ran 1.2 to 4.4 while other work
shared the box, which [`../../MEASURING.md`](../../../../MEASURING.md) says is grounds
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
`<2,2,3>` that made the predicate fire on a route that loses (by 11.7x when the
error was found, 5.2x now), so it was wrong by two orders of magnitude, and wrong
in the direction that costs a user an afternoon.

**Two. At one level of augmentation the saving is exactly nothing.** All five
one-level rows have **equal node counts**, 6, 6, 6, 11 and 14, and they are the
orbit counts 5, 5, 5, 10 and 13 plus a root. The baseline's rejection rule is
`least_in_orbit`, which is exact, so it already emits one child per pool orbit
and there is no duplication left for a parent test to remove: the canonical route
pays for a quotient that has been taken. (This paragraph said "single-generator
rejection" until 2026-08-21, which is the *other* rule, `--orbit-test generators`,
and is not the default nor what any row here was taken with.) The model had no
such case and priced a root with its leaves as though it were a tree, which is
where its 174x-to-55 000x predictions at those rows came from.

**Three. The price is a pool scan, not a group operation.** This is the finding
that matters and has its own page:
[where-the-time-goes.md](where-the-time-goes.md).

## What replaced it

    rho = min( |G|, (pool scan / plain node) ^ (levels - 1) )

It is fitted, not derived, and at two levels and above it is unchanged.

At one level it used to be priced as "a root and its children" with the plain
root charged one pool scan, and **that was the model's last wrong verdict**: it
refused `<3,3,3>` at 10, the one row where the route wins.

Since 2026-08-21 the one-level arm writes the two roots out instead, because they
differ in *order* and not in a constant: `Theta(sum |O_i|^2)` against
`Theta(|P|)` for the identical children, which
[the-suffix-and-the-whole-pool.md](the-suffix-and-the-whole-pool.md) measures at
99x. The model now gets **ten rows of ten**, with errors 1.3x, 2.5x, 1.1x, 1.1x,
1.2x, 1.1x, 1.0x, 1.1x, **13.0x** and 1.00x.

`../tests/test_route_price.cpp` holds it to every row, to the count of verdicts
it gets wrong, which is now **zero**, and to the count of rows it is more than
12x out on, which is **one and is named**: `<2,3,3>` at 8, where the fitted `rho`
reads 966 against a measured 212 and the per-node price reads 1 100 against a
measured 3 204, so the two errors compound instead of cancelling. Widening the
tolerance until it fitted would have hidden that, the way taking `rho <= |G|` as
an estimate once did.

**Getting the row right is not a reason to take the route**, and the test says so
in the only way a test can: two sabotages sit beside the verdict, and either of
them (withholding the orbit statistics, or making the baseline's orbit test 100x
cheaper) takes `<3,3,3>` back off the canonical route. What that row measures is
the baseline, and what to do about it is in
[`../../how-the-search-works/what-to-wire.md`](../../../../how-the-search-works/what-to-wire.md).
