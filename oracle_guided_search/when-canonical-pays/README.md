# When canonical augmentation is worth taking

`--route canonical` visits far fewer nodes than `--route exhaustive` and loses on
the clock at every shape measured so far. That is a trade with a crossing in it,
and the question this folder answers is where the crossing is — **before** a node
is opened, from quantities a caller already holds: the characteristic `p`, the
shape `<n,m,k>`, the pool size `|P|`, the factored degree `a+b`, the group order
`|G|` and the generator count.

What the route is and how it was wired:
[`../../canonical_factorisation/canonical-augmentation.md`](../../canonical_factorisation/canonical-augmentation.md).
What deduplicating up to the group is worth when the question is a **count**
rather than a decision: [`../deduplication-cost.md`](../deduplication-cost.md).

## The predicate

    rho = plain nodes / canonical nodes        the saving
    pi  = one canonical node / one plain node  the price
    F   = presenting the group                 the entry fee, paid once

    it pays  <=>  pi / rho + F / (plain sweep)  <  1

[`../canonical_route_price.h`](../canonical_route_price.h) is that predicate,
`price_canonical_route`, and it returns the two ratios beside the verdict so a
reading can be argued with rather than merely obeyed.

## The files

- [The derivation](the-derivation.md): where each side comes from, which of the
  two has a theorem under it and which does not, and the formula that results.
- [What it costs here](what-it-costs-here.md): the four operations priced on this
  machine by `price-canonical-route`, and how each one scales with the shape.
- [Against the sweeps](against-the-sweeps.md): both routes at five shapes, and the
  three things the derived predicate got wrong.
- [Where the time goes](where-the-time-goes.md): the finding that came out of
  correcting it, which is that neither group operation is the cost — and what
  acting on it was worth.

## The state of it, in one line

The predicate refuses at every shape swept, and it is wrong about one of them:
`<3,3,3>` at 10, where the canonical route is **2.07x faster with a node saving of
exactly zero**. **Nothing consults it yet**, and until it stops refusing the one
shape where the route wins, nothing should.
