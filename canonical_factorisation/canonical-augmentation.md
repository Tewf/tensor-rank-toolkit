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

Deciding is not counting, and the first wiring of it lost by 129x. Fixing the
part that was mine to fix took most of that back. On `matmul_2x2x2`, over the
whole sweep from the floor:

| route | group | nodes | time |
|---|---|---|---|
| `exhaustive` | 6, the stabiliser | 3815 | **0.18 s** |
| `canonical`, as first wired | 216, the full product group | 1057 | 24.62 s |
| `canonical`, with an early exit | 216 | **235** | 2.76 s |

The early exit is the whole of that 9x. The enumerator was written to *count*
solution subspaces, so it finishes every level; a rank search only asks whether
the level is empty, and the level that answers does not have to be finished. It
is now an argument, off by default, because a count that stopped early is not a
count.

**What is left is the honest result: 16.2x fewer nodes for 15x the wall clock.**
The quotient works, and pays for itself in the only currency that does not depend
on the machine. What it does not pay for is its own invariant.

## The invariant, not the method

`canonical_subspace` finds a canonical code by walking the whole group and taking
the least, so one parent test costs `|G|` reductions. At 216 elements that is
about thirty-six times the cost of a plain node, and 16x fewer nodes does not
cover it.

**That is a property of this implementation and not of canonical augmentation.**
`oracle_guided_search/deduplication-cost.md` measured the same thing while
counting, at 1982x fewer nodes and only 1.6x faster, and names the fix: a real
canonical labelling with **refinement**, which is `nauty`'s actual contribution
rather than the augmentation scheme built on top of it. That would make the code
sublinear in `|G|`. With 16x fewer nodes already in hand, it is the one change
that would plausibly turn this route from losing to winning, and it is the
substantial piece of work neither module has done.

So the route ships behind a flag, never by default, with a slow test asserting it
still reaches 7, still engages rather than falling back, and still visits strictly
fewer nodes than the plain route. **A wired route known to lose is worth more than
an unwired one somebody will propose again**, and it is worth more still when the
reason it loses is named and fixable.

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
