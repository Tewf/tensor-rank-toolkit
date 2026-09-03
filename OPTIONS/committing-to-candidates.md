# Committing to candidates

The tree-refutation and isomorph-free routes: one orbit representative at a time.
Precedence and `BILINEAR_TUNABLES`:
[`precedence-and-tunables.md`](precedence-and-tunables.md).

The third route, `find-at-rank`, is on the `rejected-experiments` branch and its
flags are gone with it. Its defaults and what chose them are recorded there, and
the measurement that retired it is in
[`../methods/bilinear_rank/canonical_augmentation/measurements/README.md`](../methods/bilinear_rank/canonical_augmentation/measurements/README.md).

## `decide-rank-by-deflation`

Waits for a proof, which is what makes it minimality-preserving and dear.

| Flag | Default | What chose the default |
|---|---|---|
| `--target k` | none; required | Nothing to measure. |
| `--refuter solver\|tree` | `solver` | **Measured both ways, and the measurement does not obviously support the default.** At `⟨2,2,2⟩` and `k = 6` the tree answers in **0.025 to 0.048 s** against the solver's **1.32 s**, roughly 50x. At `⟨3,3,3⟩` and `k = 23` the depth is 13 and the tree returned **no verdict on any of thirteen candidates in 723 s**. The crossover is `k - dim(span)`, and the default is the one that degrades gracefully. Nothing in the repository states that as the reason. |
| `--candidate-timeout N` | `sat_timeout_seconds`, `300` | **Nothing.** Argument: a refutation needs the large budget, unlike a find. |
| `--node-limit N` | `search_node_limit`, `5000000` | **Nothing.** It appears once as a budget that was hit rather than chosen: the 723 s run above was against 5 000 000 nodes per candidate. |
| `--max-memory` | `sat_memory_megabytes`, `2G` | **Nothing.** Argument only. |
| `--parallel` | off | Measured worth: **2.6x** on twelve threads, 0.018 s. Off by default for the reason `--threads` is 1 by default, which is reproducibility and not speed. Bounded: five candidates at 30 s is 44 s wall rather than 103 s, and at depth two the longest single cube measured 143 s, more than the whole question. |
| `--break-symmetry` | off | Measured: **12.14 s to 1.26 or 1.65 s, about 7.4x**, on the five pinned cubes at `⟨2,2,2⟩` and `k = 6`. Both figures were measured twice with a spread of about 2x, so nothing here is good to better than one significant figure. |
| `--solver <name>` | none | As above. |
| `-s, --symmetry matmul` | `none` | As above. |

## `enumerate-subspaces`

| Flag | Default | What chose the default |
|---|---|---|
| `--target k` | none; required | Nothing to measure. |
| `--plain` / `--canonical` | **both run** when neither is named | Measured, and running both is the point. The parent test cut nodes by **247x, 1982x and 3082x** on the three cases while cutting wall clock only 1.2x, 1.6x and 2.1x, spending about 99.9% of the node saving on naming an orbit by walking the group. Naming it from a base and strong generating set instead made the middle case **22 778x fewer nodes and 11.8x faster**, so the two columns are no longer three orders of magnitude apart and running both is now a check rather than a warning. |
| `--threads N` | `1` | Argument, and the one command here where nothing but the clock can move. **This walk counts rather than stops**: there is no budget to spend early against and no witness to stop the other workers, so the enumeration reports the same numbers at any worker count. One worker by default for the reason every other command has one, which is reproducing what was published. |
| `-s, --symmetry matmul` | `none` | Without a group there is nothing to deduplicate up to. |

Neither a node limit nor a timeout exists here.

## Both, on one question

`⟨2,2,2⟩` at `k = 6`, the same refutation `asking-a-sat-solver.md`'s
`--break-symmetry` row times by a different route, here with the tree refuter
and quotiented by `-s matmul` instead:

```sh
$ decide-rank-by-deflation evidence/fixtures/matmul_2x2x2.tensor --target 6 --refuter tree -s matmul 2 2 2
  candidate 0: refuted, 0.00324674 s, 45 nodes
  candidate 1: refuted, 0.000665064 s, 45 nodes
  candidate 2: refuted, 0.000659531 s, 72 nodes
  candidate 3: refuted, 0.000635245 s, 72 nodes
  candidate 4: refuted, 0.000618762 s, 45 nodes
  k = 6: every candidate refuted, so the rank is above 6, 0.00605391 s, quotiented tree
```

`enumerate-subspaces` on the same question counts zero, which is the same
fact stated the other way: no rank-6 subspace exists to enumerate.

```sh
$ enumerate-subspaces evidence/fixtures/matmul_2x2x2.tensor --target 6 -s matmul 2 2 2
  pool: 225 rank-one maps, group: 6 elements
  plain: 0 distinct subspaces from 0 paths, 25399 nodes, 0 group visits, 0.404418 s
  canonical: 0 distinct subspaces from 0 paths, 58 nodes, 0 group visits, 0.00844161 s
```
