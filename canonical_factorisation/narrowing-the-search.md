# Narrowing the search, and two ways that do not

The branching factor of the pool route is `|P|`, the number of rank-one maps, so
that is what any improvement has to attack. See
[`complexity.md`](complexity.md) for where that factor sits in the cost.

**The stabiliser of `span(T)` works, and is used.** A change of coordinates
fixing the span maps solutions to solutions, so one map per orbit suffices.
`orbit_reduction/` measures the `<3,3,3>` pool collapsing from 261 121 to **13
orbits**, and deeper levels are handled by canonical augmentation
`[mckay1998]`, which is **1982x fewer nodes** on `<2,2,2>`.

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
