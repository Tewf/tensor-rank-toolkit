# Searching for rank

The four commands that ask how few multiplications a map needs and disagree
about what they can prove. Precedence and `BILINEAR_TUNABLES`:
[`precedence-and-tunables.md`](precedence-and-tunables.md).

## `minimise-rank`

| Flag | Default | What chose the default |
|---|---|---|
| `--steps 1\|2\|3` | `3` | Measured, and the measurement is a warning: step 3 improved the answer in **two of four** polynomial fixtures, by one product each, and cost **58 to 184 times** steps 1 and 2 together (`../README.md`). Anything outside 1 to 3 is **refused as exit 2** rather than rounded: `--steps 7` used to run three steps and `--steps 0` one, each answering 0 for a pipeline nobody asked for. |
| `--plateau N` | `0`, off | **Measured, and the answer is that small is better.** Off, a strict descent cannot leave 8 products on `⟨2,2,2⟩`. `N=1` is still 8; **`N=2` reaches 7, which is Strassen, in 0.25 s**; 3 takes 4.77 s and 4 takes 141.87 s, after which it is flat to at least 100. So the cost rises with `N` while the answer does not, because a larger budget wanders further before finding the same improvement: `N=2` spends 16 sideways moves over 335 states, and the published `N=200` run spends 5 097 over 5 110, for the same 12 improvements and the same 7 products. **Published runs used 200, which is 560x slower than needed.** Under [`../MEASURING.md`](../MEASURING.md), machine settled, throttle delta 0. |
| `--plateau-states N` | `plateau_state_budget`, `200000` | **Still unmeasured as a tuning knob, but now bounded from below.** The crossing that reaches Strassen on `⟨2,2,2⟩` visits **335** states at `--plateau 2`, and 5 110 at the published `--plateau 200`, so the 200 000 ceiling is between 40x and 600x above what the one measured success needs. It has never been the binding constraint on any run here, which is a different claim from having been tuned. |
| `--json` | off | Nothing to measure: an output shape. |
| `--emit-operators <stem>` | off | Nothing to measure. Writes `⟨L, R, P⟩` in SMS, which is the interface PLinOpt's checkers take. |
| `-s, --symmetry` | `none` | Argument, not measurement: a command not asked for symmetry must answer as it always did. The orbit quotient's own worth is measured elsewhere: **39.2x fewer nodes** refuting `⟨2,2,2⟩` at 6, at a 1.41x surcharge a node ([`../orbit_reduction/what-the-quotient-costs.md`](../orbit_reduction/what-the-quotient-costs.md)). Nodes rather than seconds, because the seconds on that page were taken before the leaf moved on 2026-08-20 and a node count is not a property of the leaf. |
| `--threads N` | `1` | Argument, asserted rather than measured: one worker so a run reproduces what this repository published, none of which was ever given more than one core (`../run_limits/parallel.h`). |
| `--max-memory` | derived | Argument: an eighth of what the machine reports, which is `2G` on the 16 GB laptop every table here was measured on and moves on its own elsewhere. It leaves room for a browser and an editor to survive the run in the same proportion whatever the machine is. |

## `decide-rank`

| Flag | Default | What chose the default |
|---|---|---|
| `--target k` | none, sweep | Nothing to measure: with no target the tool answers "how few", with one it answers "is there one this small". |
| `--node-limit N` | `search_node_limit`, `5000000` | **Nothing.** An argument only: it is a budget and never a refutation, and reaching it is exit 3. |
| `--leaf-limit N` | `search_leaf_limit`, `100000000` | **Measured, and it is why the flag exists.** The node limit bounds how many leaves are reached and nothing inside one, and a leaf is a whole pool scan or a whole subspace walk. At `<4,4,4>` one leaf is 4 294 836 225 maps formed one at a time, which no `--node-limit` could interrupt, and the default is **383x the largest leaf any published run here reaches**, the 261 121-map pool of `<3,3,3>`, so it moves no number in this repository. Reaching it is exit 3, like the node limit. **The two rates this row used to convert into seconds are stale and are not replaced here.** They were 129.1 ns a scanned element and 78 ns a walked one, both measured against the leaf as it stood before 2026-08-20, when the carried residual and the reflected Gray walk moved both; the figure they replaced, 785 ns, was already the third. A rate is only worth quoting beside the leaf it was taken on, and the one that ships has not been re-timed here. [`../CHANGELOG.md`](../CHANGELOG.md) records what moved. |
| `--leaf-route auto\|scan\|walk` | `auto` | **Measured, and the measurement kept the default rather than moved it.** `auto` walks the subspace when `p^dim` is under the pool size and scans the pool otherwise, a rule that assumed the two elements cost the same and had never been checked. Forced onto one route apiece, the rule was right on all four questions timed: it sends `f2_3x8`, `f2_5x5` and `gf16` to the pool, which won by 26.4x, 9.4x and 3.3x, and `matmul_2x2x2` to the walk against a pool of the same size, which won by 1.21x. The cost-weighted variant that was going to replace it would break that last row. **Both routes changed on 2026-08-20 and those four ratios are a comparison of the two older ones**, so what stands is that the rule was right and not by how much: the scan gained a carried residual and the walk a reflected Gray order, and neither ratio has been re-taken. [`../exhaustive_search/which-leaf-route-is-cheaper.md`](../exhaustive_search/which-leaf-route-is-cheaper.md). |
| `--orbit-test full\|generators` | `full` | **Measured, and it is the flag's whole purpose to make the default a measurement.** Only read when `-s` is given: it chooses how a node rejects a repeated branch. `full` keeps the least member of each orbit and walks the orbit to find it; `generators` tests only the images under the surviving generators, which is cheaper a candidate and lets duplicate branches through. Same verdict either way — the cheap rule's tree contains the exact rule's node for node — and the duplication is **5.10x and 17.96x on the two refutations counted**, against a per-node surcharge for being exact of only 1.10x to 1.41x. Node counts, which reproduce anywhere, and no timing: [`../orbit_reduction/what-partial-rejection-leaves.md`](../orbit_reduction/what-partial-rejection-leaves.md). |
| `--general-leaf` | off | **An argument, not a default measurement chose.** It exists so the GF(2) leaf can be timed against the path it replaced *on one question*, since a comparison across two questions is not a comparison. What it measured, 6.0x to 39.6x against a published prediction of 40x to 64x, is in [`../exhaustive_search/gf2_leaf.h`](../exhaustive_search/gf2_leaf.h) and [`../positioning/hardware-and-parallelism.md`](../positioning/hardware-and-parallelism.md), **where every one of those ratios is now an upper bound**: `is_rank_one` made the general path this flag forces faster, so the baseline the ratios are taken against moved under them. |
| `--anchor map\|heuristic` | `map` | Argument, and semantic rather than quantitative: from the map the answer is the true minimum, from the heuristic it is the minimum only among algorithms containing that subspace. **The `heuristic` arm has never been timed**; every published result was anchored at the map (`../descent_search/method/`). |
| `-s, --symmetry` | `none` | As `minimise-rank` above. |
| `--threads N` | `1` | As `minimise-rank` above. |
| `--max-memory` | `2G` | As `minimise-rank` above. |

## `walk-scheme`

| Flag | Default | What chose the default |
|---|---|---|
| `--threads N` | `1` | **Measured, and the only search here where threading is free.** The seeds are independent walks, each `mt19937_64(seed)` over a start and a field nobody writes, so the answers are bit-identical at any thread count: on `⟨3,3,3⟩ --flips 20000 --seeds 8` the reported scheme, seed, flip and reduction counts agree exactly at 1, 4 and 8 workers, and only the elapsed figures move. Measured **3.2x at 4 workers and 5.4x at 8**, on a machine that was not quiet, so those are floors. Unlike `decide-rank`'s, this speedup costs nothing in counts: no shared budget, no early exit, no race. |
| `--flips N` | `20000` | **Nothing.** It is the setting the published runs used, not a tuned choice: `⟨3,3,3⟩` reaches 24 products in 38.1 s at `--flips 20000 --seeds 8`. **`--steps N` is an accepted older spelling of this flag** and means flips here, where in `minimise-rank` the same word means pipeline stages. Both are parsed; only `--flips` is the name the tool leads with. |
| `--seeds N` | `8` | **Nothing**, for the same reason. |
| `--from k` | off, walk from naive | Measured: on `f3_3x6`, four seeds of 20 000 flips reach **12** products from the naive scheme and hold the heuristic's **10** when started there (`../flip_graph/README.md`). |
| `--max-memory N` | `2G` | Argument: same default and source. `--from k` runs the heuristic first, whose span table is p^dim. |

## `lower-the-bound`

Nothing on this page is a tuned default. The command shipped on 2026-08-21 and
its knobs were **explored rather than measured**: what is recorded per fixture is
the answer reached and the node count, in
[`../incumbent_search/what-it-reaches.md`](../incumbent_search/what-it-reaches.md),
and no default below is claimed to be the best value.

| Flag | Default | What chose the default |
|---|---|---|
| `--from basis\|descent` | `descent` | **Explored, and the trade is the point rather than a tuning.** `descent` starts at `descend_from_own_basis`, `basis` at the minimum-weight basis alone. The looser incumbent is a *taller* tree, since `dim V + 1 >= best` cuts later: on `f2_5x5` `basis` reaches **13** where `descent` exhausts at 14, and on `f2_4x7` the same looseness made the run unaffordable. Both effects are one effect. |
| `--width N` | `4` | **Not measured.** Children entered per node, cheapest first; `0` enters every child, which is what makes a `tree exhausted` a statement about the whole tree and is affordable on `matmul_2x2x2` (184 nodes) and on nothing larger here. |
| `--summand-rank r` | `3` | **Not measured, and it is the cost knob.** An element of rank `r` offers `(p^r − 1)p^(r−1)/(p−1)` moves, so 6 at rank 2, 28 at rank 3 and 120 at rank 4 over GF(2). Raising it strictly enlarges the move set and never changes what a move is. |
| `--nodes N` | `20000` | **An argument only.** Spending it withdraws nothing: this search refutes nothing, so a budget that runs out leaves a weaker algorithm rather than a claim. |
| `--rounds N` | `8` | **Measured, and it bought nothing here.** Restarting from the answer starts the next round at a different root under a tighter incumbent, but every improvement in the table was found in the first round. |
| `--whole-pool` | off | **Not measured on anything it could finish.** Offers every rank-one map of the shape instead of the generated moves, at `\|pool\|` minimum-weight bases a node: 16 129 at 7x7 over GF(2), against the 20 678 moves the whole 22-node `cyclic_f2_7` run offered. |
| `--emit-operators <stem>` | off | Nothing to measure. Same three SMS files as `minimise-rank`, and what the external check of the 13-product `cyclic_f2_7` scheme was run on. |
| `--general-span` | off | **An argument, not a default measurement chose.** Step 1's span walk over GF(2) holds a matrix as bits in machine words; this forces the general field path, which holds one `int64_t` an entry, so the two can be timed *on one question*. Same tree, same nodes, same children, same answer: only the arithmetic differs. What it measured is **2.5x to 19.2x**, in [`../descent_search/gf2_span_walk.h`](../descent_search/gf2_span_walk.h), where the spread matters more than the top: the factor is what the span walk was as a share of the run, and at `<2,2,2>` that is move generation rather than the walk. Taken off protocol, on a machine that was not quiet, and the header says so. |
| `--threads N` | `1` | Argument, asserted rather than measured: one worker so a run reproduces what this repository published. The children of one node are prepared in parallel and entered in the same order, so counts are identical at any thread count — asserted on `matmul_2x2x3` (341 nodes, 159 860 children) in `run_limits/tests/check_the_limits_reach_the_commands.sh`. |
| `--max-memory N` | derived | Argument: an eighth of what the machine reports, which is `2G` on the 16 GB laptop every table here was measured on and moves on its own elsewhere. `--summand-rank r` asks for p^r vectors and this is what refuses an r the machine cannot hold. |
