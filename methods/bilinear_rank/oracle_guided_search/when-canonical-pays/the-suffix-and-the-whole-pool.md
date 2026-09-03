# Why it loses at two levels, and what it wins on at one

Two regimes, two different answers, and neither of them is about the group. This
page is the mechanism behind [against-the-sweeps.md](against-the-sweeps.md)'s
table; [where-the-time-goes.md](where-the-time-goes.md) is what asked for it.

## At two levels and above: a suffix against the whole pool

`descend(V, from)` scans `[from, |P|)`. A child starts at its parent's chosen
index, so **the live suffix shortens as the tree deepens**, and a refutation
spends nearly all of its nodes deep. The canonical route has no suffix: a
canonical form is a function of the **whole** set, so `PoolCosets` reduces every
pool element at every node whatever the depth.

On the shipped `<2,2,2>` fixture at two levels, `price-canonical-route
fixtures/matmul_2x2x2.tensor -s matmul 2 2 2 --target 6` ends `verdict: the nodes
the group removes do not cover the parent tests they cost`, this section's
argument in the tool's own words.

Divide each route's seconds by its own node count, and both by one pool scan:

| shape | `t` | a plain node | a canonical node | price a node | node saving | net |
|---|---|---|---|---|---|---|
| `<2,2,2>` | 6 | 2.18 us, **0.24** scans | 159 us, 17.4 scans | 73x | 11.2x | 6.5x |
| `<2,2,2>` | 7 | 1.84 us, **0.20** scans | 1 914 us, 210 scans | 1 040x | 226x | 4.6x |
| `<2,2,3>` | 8 | 2.90 us, **0.034** scans | 409 us, 4.80 scans | 141x | 32.3x | 4.4x |
| `<2,2,4>` | 10 | 6.31 us, **0.012** scans | 1 483 us, 2.82 scans | 235x | 121x | 1.9x |
| `<2,3,3>` | 8 | 3.35 us, **0.00053** scans | 10 734 us, 1.71 scans | 3 204x | 212x | 15.1x |

**The plain node's share of the pool falls to nothing and the canonical node's
cannot fall below one.** A plain node costs two to six microseconds whatever the
shape, because a deep node scans almost nothing and the tree is deep nodes; a
canonical node is a pool pass plus its leaf, and converges to about 1.7 of them.

So the price ratio grows with the pool while the node saving does not: 73x to
3 204x against 11x to 226x. `rho <= |G|` allows more, but the measured saving is
0.017% to 5% of it and nothing in the sweep suggests otherwise. **There is no
constant that closes two orders of magnitude, and the model does not try.**

## At one level: two roots, and the difference is an order

At `L = 1` there is no tree to save. Both routes emit exactly one node per
`G`-orbit of the pool, so the node counts are equal: checked as an identity
rather than observed, since **orbits + 1 is the node count of both**, at all five
shapes. What is left is how each route names those children.

| shape | `\|P\|` | orbits | `sum \|O_i\|^2` | `least_in_orbit` | `orbit_representatives` |
|---|---|---|---|---|---|
| `<2,2,2>` | 225 | 5 | 1.08e4 | 29.2 us | 27.3 us |
| `<2,2,3>` | 945 | 5 | 2.42e5 | 286 us | 123 us |
| `<2,2,4>` | 3 825 | 5 | 5.32e6 | 4.19 ms | 502 us |
| `<2,3,3>` | 32 193 | 10 | 1.73e8 | 88.9 ms | 4.27 ms |
| `<3,3,3>` | 261 121 | 13 | 9.94e9 | **5.05 s** | **51.2 ms** |

Same answer, same 13 children, **99x**. The plain route asks `least_in_orbit`
once per pool element; it walks the orbit breadth first and asks `std::find` over
a `seen` list that grows to the whole orbit, so the sweep costs
`Theta(sum |O_i|^2)`, measured at 0.51 ns a unit, within 4% at the two largest
shapes. `orbit_representatives` marks every element once and costs
`Theta(|P|)`, 182 ns an element. **That is a difference in order, not a
constant**, and it is the whole of the `<3,3,3>` margin: the plain route's entire
4.87 s run at target 10 *is* that one call.

## What the group costs, which is not what decides either regime

Subtract every canonisation, every setwise stabiliser and the presentation from
the canonical route's clock, at the one level where the counts and the priced
calls match one to one:

| shape | canonical | of it, the group | with the group free | against plain |
|---|---|---|---|---|
| `<2,2,2>` at 5 | 598 us | 289 us, 48% | 309 us | **1.43x slower** |
| `<2,2,3>` at 7 | 1 781 us | 675 us, 38% | 1 106 us | **1.70x slower** |
| `<2,2,4>` at 9 | 8 892 us | 2 397 us, 27% | 6 495 us | **1.25x slower** |
| `<2,3,3>` at 7 | 120.2 ms | 10.9 ms, 9% | 109.3 ms | **1.12x slower** |
| `<3,3,3>` at 10 | 2 260 ms | 135 ms, 6% | 2 126 ms | 0.44x, and it wins |

**Driving every canonisation cost to zero does not save the route at `<2,2,2>`,
and it is not what wins it at `<3,3,3>`.** What is left in the first four rows is
one pool pass a node and a leaf that scans the pool where the plain route walks
`p^target` elements (1 024 against 261 121 at `<3,3,3>`) through the packed
GF(2) leaf. Both are named in
[where-the-time-goes.md](where-the-time-goes.md) as still open.

The same subtraction over a whole multi-level sweep is **not** reported, and the
reason is worth stating: a canonical image is priced here on a node's own pool
content, and the parent test canonises candidate parents and a marked pair
instead, at sizes the level decides. One level in, the two agree; three levels in
the subtraction goes negative, which is a measurement saying it has left its
range rather than a route that costs nothing.
