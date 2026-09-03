# Narrowing the search: one that works, one that loses, two that cannot

The branching factor of the pool route is `|P|`, the number of rank-one maps, so
that is what any improvement has to attack. Every attempt on it is here, with
what each was measured at. See [`complexity.md`](complexity.md) for where that
factor sits in the cost.

## The stabiliser of `span(T)` works, and by how much

A change of coordinates fixing the span maps solutions to solutions, so one map
per orbit suffices. Measured here, on the pool route:

| tensor | search alone | `--symmetry auto` | inferred closed form |
|---|---|---|---|
| `f2_2x2` | 47 us | 6 automorphisms, 117 us | not a product shape |
| `f2_2x3` | 62 us | 6 automorphisms, 1058 us | not a product shape |
| `gf4_multiplication` | 36 us | 18 automorphisms, 92 us | 1 automorphism |
| `matmul_2x2x2` | 1.037 s | **refuses**, 1.077 s | 6 automorphisms, **0.195 s** |

Read the two halves against each other, because they point opposite ways.

On the microsecond fixtures the group costs more than it saves: `auto` builds it,
and building it is most of the run. On `matmul_2x2x2` the search is a second and
the quotient takes it to **a fifth of that, 5.3x**, which also puts the pool
route below the solver's 0.54 s on that tensor. Symmetry is worth nothing on
instances that were already free and a great deal on the ones that are not,
which is the ordinary shape of such a thing and worth having the numbers for.

**`auto` is the weaker route and refuses exactly where it would matter.** It
enumerates the ambient group, which is about four hundred million automorphisms
for a 4x4 map over GF(2), so it declines on every matrix multiplication fixture
here. The closed form needs no group enumerated and holds at any size, so the
product shape is *inferred* from the three dimensions rather than asked for: they
are `nm`, `mk` and `kn`, whose product is `(nmk)^2`, and the sizes divide out of
its square root when they come out whole.

Guessing the shape wrong is safe rather than merely unlikely. `stabiliser_of`
keeps only the elements that actually fix `span(T)`, which is precisely the
precondition `expand_subspace_up_to_symmetry` needs, so a wrong guess yields a
small group and never a false refusal. `gf4_multiplication` is the case in hand:
its dimensions match `<2,2,2>` and it is not a product, and what comes back is
the trivial group rather than a wrong answer.

## Canonical augmentation

Wired as `--route canonical`. It wins on nodes and loses on wall clock, and
the reason is its invariant rather than the method:
[`canonical-augmentation.md`](canonical-augmentation.md).

## The two that cannot

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

