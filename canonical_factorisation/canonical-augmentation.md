# Canonical augmentation, wired and measured

The third route, why it exists, and why it is never the default. The other two
narrowings, one of which works, are in
[`narrowing-the-search.md`](narrowing-the-search.md).

`--route canonical` is McKay canonical augmentation `[mckay1998]`: give every
subspace a group-invariant canonical parent and accept an augmentation only from
that parent's class, so each class is reached exactly once with **no memory** of
what was generated. It is the standard fix for the duplication the plain tree
leaves, and `oracle_guided_search/` measures it at **1982x fewer nodes** when
*counting* solution subspaces.

Deciding is not counting, and here it loses badly. On `matmul_2x2x2`:

| route | group | nodes over the sweep | time |
|---|---|---|---|
| `exhaustive` | 6, the stabiliser | 3815 | **0.19 s** |
| `canonical` | 216, the full product group | **1057** | 24.62 s |

**3.6x fewer nodes for 129x the wall clock.** Two reasons, both structural rather
than fixable by tuning:

- The parent test walks *every* element of the group at every node, and the group
  it needs is the full 216 rather than the 6-element stabiliser the plain route
  quotients by. So a node costs about thirty-six times more.
- The enumerator has **no early exit**: it finishes each level whether or not it
  has an answer, because counting is what it is for. The plain tree returns at
  the first success. The saving was supposed to come from the levels *below* the
  rank, which have to be exhausted anyway, and 3.6x on those does not pay for the
  level that succeeds.

The route ships anyway, behind a flag and never by default, with a slow test
asserting it still returns 7 and still visits strictly fewer nodes. A wired route
that is known to lose is worth more than an unwired one somebody will suggest
again: **when you need one answer, early exit beats deduplication; when you need
a count, it is the other way round.** That is the same trade
`oracle_guided_search/deduplication-cost.md` prices, tipped the other way and much
harder, because there the alternative had no quotient at all.

**The full `GL(n) x GL(m)` cannot work.** The sandwiching action `M -> mu M nu`
is *transitive* on nonzero rank-one matrices: given `u v^T` and `u' v'^T`, take
`mu` with `mu u = u'` and `nu` with `v^T nu = v'^T`, both of which exist because
`GL` is transitive on nonzero vectors. So `P` is a single orbit and quotienting
by the whole group leaves one representative and no problem. Only the subgroup
fixing `span(T)` preserves the question, which is why the stabiliser is the
object and the ambient group is not.

**Branching on cosets of `span(T)` does not pay.** Two maps differing by an
element of the span extend it identically, so in principle the branching only
needs `P` modulo `span(T)`. Measured, it collapses nothing where it would
matter:

| tensor | `\|P\|` | cosets | saving |
|---|---|---|---|
| `f2_2x2` | 9 | 1 | 9.00x |
| `f2_2x3` | 21 | 3 | 7.00x |
| `gf4_multiplication` | 9 | 3 | 3.00x |
| `matmul_2x2x2` | 225 | 198 | 1.14x |
| `f2_5x5` | 961 | 853 | 1.13x |
| `f3_3x6` | 4732 | 4049 | 1.17x |
| `gf16_multiplication` | 225 | 225 | 1.00x |
| `matmul_3x3x3` | 261 121 | 261 121 | **1.00x** |

The saving is real only where the span is a large part of the ambient space,
which is exactly where the search was already trivial. On the two shapes anyone
would want it for it is worth nothing at all, so it is not implemented, and this
table is here so it is not proposed again.
