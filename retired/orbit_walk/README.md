# The orbit walk, retired 2026-08-19

`canonical_subspace` named a subspace's orbit by taking the least
`subspace_code` over **every element of the group**, and `image_code` served the
distinguished-element test that minimised over the attaining coset. Together they
were the invariant of `main`'s canonical augmentation.

**Why they are here.** `oracle_guided_search/pool_set_canon.h` on `main` answers
both questions from generators, by canonical image under a prescribed permutation
group (`[linton2004]` through `[permlib]`), and dominates them on the same field.
Measured on `enumerate-subspaces fixtures/matmul_2x2x2.tensor --target 7
-s matmul 2 2 2 --canonical`, both on an idle machine:

| invariant | distinct | nodes | wall |
|---|---|---|---|
| this one, walking the group | 1 | 954 | 21.9 s |
| canonical image | 1 | **83** | **3.04 s** |

For scale, the plain route is 36 distinct, 1 890 601 nodes, 35.89 s. So the
canonical route went from 1.7x faster than plain to **11.8x**, and from 1 982x
fewer nodes to 22 779x. No group element is walked at all now.

**What it cost, in its own words.** One parent test was `|G|` reductions, about
36 times a plain node at 216 elements, so 16x fewer nodes did not cover it, and
`canonical-augmentation.md` on `main` shipped the route off by default and
recorded that it lost. That page also named this exact replacement as "the one
change that would plausibly turn this route from losing to winning". It did.

**Nothing here is wrong.** It is correct, it is simpler than what replaced it, and
it is the reference the fast one was checked against while both existed: the two
induced the same partition of all 225 depth-one children of `⟨2,2,2⟩`, five
classes each. `main`'s test now checks against the closed form from the A_3
quiver instead, which is an independent oracle rather than the thing retired.

It is kept because a rejection with its evidence deleted is indistinguishable
from a whim, which is why `find-at-rank` is on this branch too.
