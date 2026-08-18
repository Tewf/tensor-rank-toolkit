# Narrowing the search, and two ways that do not

The branching factor of the pool route is `|P|`, the number of rank-one maps, so
that is what any improvement has to attack. See
[`complexity.md`](complexity.md) for where that factor sits in the cost.

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
