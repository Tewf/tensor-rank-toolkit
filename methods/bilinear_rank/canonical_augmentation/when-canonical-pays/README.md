# When canonical augmentation is worth taking

`--route canonical` visits far fewer nodes than `--route exhaustive` and loses on
the clock at every shape measured so far. That is a trade with a crossing in it,
and the question this folder answers is where the crossing is (**before** a node
is opened, from quantities a caller already holds): the characteristic `p`, the
shape `<n,m,k>`, the pool size `|P|`, the factored degree `a+b`, the group order
`|G|` and the generator count.

What the route is and how it was wired:
[`../../canonical_factorisation/canonical-augmentation.md`](../../../canonical_factorisation/canonical-augmentation.md).
What deduplicating up to the group is worth when the question is a **count**
rather than a decision: [`../deduplication-cost.md`](../deduplication-cost.md).

## The predicate, which has two arms because the question has two regimes

Write `L = target - n*k` for the levels of augmentation. At `L >= 2` the
comparison is between two trees:

    rho = plain nodes / canonical nodes        the saving
    pi  = one canonical node / one plain node  the price
    F   = presenting the group                 the entry fee, paid once

    it pays  <=>  pi / rho + F / (plain sweep)  <  1

At `L == 1` there is no tree: both routes emit one node per `G`-orbit of the pool
and `rho` is exactly 1, so a per-node ratio prices nothing and the two **roots**
are written out instead:

    plain root       R * sum |O_i|^2                       one orbit test an element
    canonical root   S + A|P| + Stab + (r+1) I + r*leaf    one pool pass, one orbit pass

    it pays  <=>  the left side is larger

[`../canonical_route_price.h`](../canonical_route_price.h) is both arms,
`price_canonical_route`, and it returns each side beside the verdict so a reading
can be argued with rather than merely obeyed.

Run directly on the shipped `<2,2,3>` fixture, `price-canonical-route
evidence/fixtures/matmul_2x2x3.tensor -s matmul 2 2 3 --target 7` reports `group 6048, 6
generators` and `price ratio 78.45`, ending `verdict: one level of augmentation:
the orbits of this pool are small enough that naming them one element at a time
still costs less than a pool pass a leaf`.

**One input is not free and is stated as its own type.** `PoolOrbits` (the
orbits of the pool under the stabiliser and the sum of their squared sizes) has
no closed form here and costs one `O(|P| * generators)` pass: 27 us at `<2,2,2>`
and 51 ms at `<3,3,3>`, the latter against the 4.9 s decision it prices.
Withheld, the one-level arm refuses rather than falling back on `|O| <= |G|`,
which is **125x** the truth at `<3,3,3>`.

## The files

- [The derivation](the-derivation.md): where each side comes from, which of the
  two has a theorem under it and which does not, and the formula that results.
- [What it costs here](what-it-costs-here.md): the operations priced on this
  machine by `price-canonical-route`, and how each one scales with the shape.
- [Against the sweeps](against-the-sweeps.md): both routes at five shapes, and the
  three things the derived predicate got wrong.
- [The suffix and the whole pool](the-suffix-and-the-whole-pool.md): the mechanism
  of both regimes, and what is left when every group cost is driven to zero.
- [Why nothing consults it](why-nothing-consults-it.md): the predicate is sound
  and is still wired to nothing, because the one row it fires on measures the
  baseline's orbit test. With the partial-symmetry-break verdict.
- [Where the time goes](where-the-time-goes.md): that neither group operation is
  the cost, and what acting on it was worth.

## The state of it, in one line

The predicate now gets **all ten** swept rows, including the one it used to miss.
It says the route pays at exactly one of them, `<3,3,3>` at 10. **Nothing
consults it, deliberately**, because the margin there is a quadratic in the
baseline's orbit test rather than anything canonical augmentation does:
[`../../how-the-search-works/what-to-wire.md`](../../../../writeup/how-the-search-works/what-to-wire.md)
carries that verdict and the 99x it points at instead.
