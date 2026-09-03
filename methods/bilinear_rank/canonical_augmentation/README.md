# methods/bilinear_rank/canonical_augmentation/

Refinements that reduce what the exact search spends. `decide-rank-by-deflation`
commits to one candidate at a time, so a negative answer is a proof rather
than a spent budget;
`enumerate-subspaces` generates each candidate subspace once per symmetry
class instead of once per member; `price-canonical-route` is the instrument
that says what that canonical generation costs before an hour is spent on it.

In this folder:

- [`commands/`](commands): `decide-rank-by-deflation` and `enumerate-subspaces`.
- [`strict_deflation.h`](strict_deflation.h) and
  [`tree_refutation.h`](tree_refutation.h): the commit-and-refute loop and
  the tree route that buys a proof without a solver.
- [`canonical_augmentation.h`](canonical_augmentation.h),
  [`canonical_parent.h`](canonical_parent.h),
  [`pool_set_canon.h`](pool_set_canon.h), [`pool_cosets.h`](pool_cosets.h),
  [`factored_lex_min.h`](factored_lex_min.h): generating each class exactly
  once, with no memory of what was seen.
- [`canonical_route_price.h`](canonical_route_price.h) and
  `price_canonical_route.cpp`: the pricing instrument.
- [`measurements/`](measurements/README.md): every number, including the ones
  that retired `find-at-rank` to the `rejected-experiments` branch.
- Two notes and two folders of evidence:
  [`deduplication-cost.md`](deduplication-cost.md),
  [`enumerating-on-every-core.md`](enumerating-on-every-core.md),
  [`refutation-prices/`](refutation-prices),
  [`when-canonical-pays/`](when-canonical-pays).
- [`tests/`](tests): agreement with the plain search, asserted.

Example of use:

```sh
decide-rank-by-deflation evidence/fixtures/matmul_2x2x2.tensor --target 6 -s matmul 2 2 2
#   candidate 4: refuted, 0.592915 s
#   k = 6: every candidate refuted, so the rank is above 6, 3.12693 s, kissat
```

Which questions these answer that `decide-rank` does not:
[`../../../OPTIONS/one-question-per-command.md`](../../../OPTIONS/one-question-per-command.md).
Their flags:
[`../../../OPTIONS/committing-to-candidates.md`](../../../OPTIONS/committing-to-candidates.md).
