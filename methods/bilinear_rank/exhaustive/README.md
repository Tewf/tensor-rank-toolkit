# methods/bilinear_rank/exhaustive/

The exact decision procedure, in contrast to the heuristics: `decide-rank`
decides whether a map admits a bilinear algorithm with exactly `k`
multiplications. A positive answer carries a decomposition verified against
the map before it is printed; a negative answer means the search tree was
walked to its end. The cost is exponential, as the NP-completeness of the
problem leads one to expect, so it settles small maps outright and refuses
instances that cannot fit.

In this folder:

- [`commands/`](commands): `decide-rank` itself.
- [`exhaustive_search.h`](exhaustive_search.h): the search core and
  `SearchBudget`, whose header states exactly what a spent budget does and
  does not prove.
- [`rank_one_basis.h`](rank_one_basis.h) and
  [`subspace_walk.h`](subspace_walk.h): the leaf test and the walk that
  feeds it.
- [`gf2_leaf.h`](gf2_leaf.h) and [`gf2_leaf_on_card.h`](gf2_leaf_on_card.h):
  the packed GF(2) leaf, on the host and on the card.
- [`fewest_products.h`](fewest_products.h): the sweep that turns the yes/no
  question into the rank itself.
- [`search_trace.h`](search_trace.h): `--trace`, the tree as it was walked.
- Four notes, each holding one measured answer:
  [`which-leaf-route-is-cheaper.md`](which-leaf-route-is-cheaper.md),
  [`what-threads-change.md`](what-threads-change.md),
  [`what-a-node-cannot-tell-you.md`](what-a-node-cannot-tell-you.md),
  [`generating-candidates-from-the-span.md`](generating-candidates-from-the-span.md).
- [`tests/`](tests): the classical answers, asserted.

Example of use:

```sh
decide-rank evidence/fixtures/matmul_2x2x2.tensor --target 7
#   FOUND: 7 products, rank bound 6, gap 1
#   verified: they compute the map
```

A first session in plain words: [`../start-here.md`](../../../start-here.md). Every flag:
[`../OPTIONS/searching-for-rank.md`](../../../OPTIONS/searching-for-rank.md). The
algorithm in pseudocode:
[`../how-the-search-works/`](../../../writeup/how-the-search-works/README.md).
