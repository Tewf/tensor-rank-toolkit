# Committing to candidates

The tree-refutation and isomorph-free routes: one orbit representative at a time.
Precedence and `BILINEAR_TUNABLES`: [`../OPTIONS.md`](../OPTIONS.md).

The third route, `find-at-rank`, is on the `rejected-experiments` branch and its
flags are gone with it. Its defaults and what chose them are recorded there, and
the measurement that retired it is in
[`../oracle_guided_search/measurements/README.md`](../oracle_guided_search/measurements/README.md).

## `deflate-strictly`

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
| `--plain` / `--canonical` | **both run** when neither is named | Measured, and running both is the point. The parent test cut nodes by **247x, 1982x and 3082x** on the three cases while cutting wall clock only 1.2x, 1.6x and 2.1x, spending about 99.9% of the node saving on naming an orbit by walking the group. Naming it from a base and strong generating set instead made the middle case **22 779x fewer nodes and 11.8x faster**, so the two columns are no longer three orders of magnitude apart and running both is now a check rather than a warning. |
| `-s, --symmetry matmul` | `none` | Without a group there is nothing to deduplicate up to. |

Neither a node limit nor a timeout exists here.
