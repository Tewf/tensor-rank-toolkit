# Committing to candidates

The fixed-k, tree-refutation and isomorph-free routes: one orbit representative
at a time. Precedence and `BILINEAR_TUNABLES`: [`../OPTIONS.md`](../OPTIONS.md).

## `find-at-rank`

Produces an upper bound and never waits for a refutation.

| Flag | Default | What chose the default |
|---|---|---|
| `--target k` / `--descend` | neither; one is required | Nothing to measure. |
| `--ceiling N` | naive cost | Nothing to measure. |
| `--floor N` | `rank_lower_bound` of the tensor | Measured: the weaker flattening bound alone put this command's floor at 4 on GF(16) where its siblings start at **8**, which is four solver calls thrown away. |
| `--candidate-timeout N` | `30` | Argument, with one datapoint beside it. The argument: the sweep's worst case is candidates times this and only the accepting call has to finish. The datapoint: at `⟨2,2,2⟩` and `k = 7`, candidate 0 does not finish in 30 s and candidate 1 answers in **0.50 s**, so passing over is what makes the step land at 30.5 s rather than never. Deliberately **not** `sat_timeout_seconds`: a candidate that exhausts this is passed over, never refuted. |
| `--solver <name>` | none; `sat_solver_order` decides | As `decide-rank-by-sat`. |
| `--max-memory` | `sat_memory_megabytes`, `2G` | **Nothing.** Argument only. |
| `--break-symmetry` | off | Sound beside a cube, which pins term 0 and orders nothing. Worth is measured on the whole instance; see `deflate-strictly` below. |
| `-s, --symmetry matmul` | `none` | `auto` is refused: the candidates are the closed-form orbits of `⟨n,m,k⟩`. |

The command as a whole is measured and it loses: **between 390 and 1500 times
slower** than the baselines on every fixture tried
([`../oracle_guided_search/measurements.md`](../oracle_guided_search/measurements.md)).
The premise it was designed against was also measured and refuted: the assumed
190x to 210x asymmetry between a find and a refutation is, with matched flags,
about **one**.

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
| `--plain` / `--canonical` | **both run** when neither is named | Measured, and running both is the point: the parent test cuts nodes by **247x, 1982x and 3082x** on the three cases while cutting wall clock only **1.2x, 1.6x and 2.1x**. It spends about 99.9% of the node saving on itself, and the 1.2x row is inside the 13% noise floor. |
| `-s, --symmetry matmul` | `none` | Without a group there is nothing to deduplicate up to. |

Neither a node limit nor a timeout exists here.
