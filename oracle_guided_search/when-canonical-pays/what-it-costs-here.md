# The four operations, priced on this machine

`price-canonical-route <tensor> -s matmul n m k --target t` times each of them,
fastest of the calls it is given, under [`../../MEASURING.md`](../../MEASURING.md).
Measured 2026-08-21, the lock held, on a machine that was not idle — every
absolute number below is therefore an over-estimate and the shape-to-shape
*scaling*, which is what the predicate uses, is what survives that.

| shape | `\|P\|` | degree `a+b` | `\|G\|` | pool scan | canonical image | setwise stabiliser | presentation |
|---|---|---|---|---|---|---|---|
| `<2,2,2>` | 225 | 30 | 216 | 9.07 us | 13.0 us | 29.7 us | 97.6 us |
| `<2,2,3>` | 945 | 78 | 6 048 | 75.2 us | 37.4 us | 90.3 us | 247 us |
| `<2,2,4>` | 3 825 | 270 | 725 760 | 607 us | 59.9 us | 240 us | 736 us |
| `<2,3,3>` | 32 193 | 574 | 169 344 | 6.37 ms | 42.6 us | 892 us | 1.57 ms |
| `<3,3,3>` | 261 121 | 1 022 | 4 741 632 | 114 ms | 86.7 us | 9.22 ms | 4.71 ms |

Four readings, in the order they matter.

**The membership test is the one thing here that behaves like a constant.** A pool
scan is `|P|` reductions of a slice-shaped matrix against a basis, so it should
cost `|P|` times the span dimension times the entries of a slice. Dividing it out:
0.60, 0.55, 0.62, 0.61 and 0.60 ns. **Within 7% over a range of 11x**, which is
why every other cost in the model is quoted in units of it.

**The canonical image barely moves.** 13.0 us at degree 30 and 86.7 us at degree
1 022: a factor of 6.7 across a factor of 34 in degree, and per axis point it
*falls*, 434 ns to 85 ns. That is `[linton2004]`'s analysis showing through — the
degree enters only through the base changes and the orbit computations, both linear
up to logarithms, while the candidate count that could have dominated does not,
exactly as he predicted it would not.

**The setwise stabiliser has no law through it.** 1.0, 1.2, 0.9, 1.6 and 9.0 ns a
point; 106x one canonical image at `<3,3,3>` against 2.3x at `<2,2,2>`; and not
monotone in `|G|`, since `<2,3,3>` costs 3.7x `<2,2,4>` with a group 4.3x smaller.
This is what an operation with no proven bound looks like from outside, and it is
why `CanonicalPrices::stabiliser_nanoseconds_per_point` is marked PROVISIONAL.

**The presentation stopped being an obstacle when the axes did.**
`canonical-augmentation.md` records 13 ms to present `<2,2,2>`; it is 98 us now,
133x less, because that measurement was of the **grid** presentation on 225 points
and this one is of the **axis** presentation on 30. The fee is under 5 ms even at
`<3,3,3>`, so the gate it guards — that a plain sweep must cost more than the
presentation it would replace — only ever bites at `<2,2,2>`.

## Where the constants live, and why not in `tunables.conf`

In `CanonicalPrices`, in code, for the reason
[`../../run_limits/device.cpp`](../../run_limits/device.cpp) keeps its crossover
table in code and puts only `device_launch_floor` in the file: **a measured table is
not a knob.** What a reader may want to move is the decision; what they must not
silently move is the measurement. When the predicate is wired to a route, the
threshold it fires on is what belongs in `tunables.conf`, not these.
