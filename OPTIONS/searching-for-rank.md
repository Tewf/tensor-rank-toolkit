# Searching for rank

The three commands that ask how few multiplications a map needs and disagree
about what they can prove. Precedence and `BILINEAR_TUNABLES`:
[`../OPTIONS.md`](../OPTIONS.md).

## `minimise-rank`

| Flag | Default | What chose the default |
|---|---|---|
| `--steps 1\|2\|3` | `3` | Measured, and the measurement is a warning: step 3 improved the answer in **two of four** polynomial fixtures, by one product each, and cost **58 to 184 times** steps 1 and 2 together (`../README.md`). Anything outside 1 to 3 is **refused as exit 2** rather than rounded: `--steps 7` used to run three steps and `--steps 0` one, each answering 0 for a pipeline nobody asked for. |
| `--plateau N` | `0`, off | **Unmeasured at any value.** What is measured is that a strict descent cannot move at all: **0 of 225** candidates improve `⟨2,2,2⟩`, **0 of 945** `⟨2,2,3⟩`, **0 of 32 193** `⟨2,3,3⟩`, and `⟨2,2,2⟩` needs three sideways steps before the count moves (`../flip_graph/plateau_search.h`). That argues against the default and no run has priced the alternative. |
| `--plateau-states N` | `plateau_state_budget`, `200000` | **Nothing.** PROVISIONAL: never measured, and until this flag existed the number was a literal at the two call sites, so no run had ever tried another value. |
| `--json` | off | Nothing to measure: an output shape. |
| `--emit-operators <stem>` | off | Nothing to measure. Writes `⟨L, R, P⟩` in SMS, which is the interface PLinOpt's checkers take. |
| `-s, --symmetry` | `none` | Argument, not measurement: a command not asked for symmetry must answer as it always did. The orbit quotient's own worth is measured elsewhere (**28x on a refutation**, `../README.md`). |
| `--threads N` | `1` | Argument, asserted rather than measured: one worker so a run reproduces what this repository published, none of which was ever given more than one core (`../run_limits/parallel.h`). |
| `--max-memory` | `2G` | Argument: it leaves room on a 16 GB desktop for a browser and an editor to survive the run. |

## `decide-rank`

| Flag | Default | What chose the default |
|---|---|---|
| `--target k` | none, sweep | Nothing to measure: with no target the tool answers "how few", with one it answers "is there one this small". |
| `--node-limit N` | `search_node_limit`, `5000000` | **Nothing.** An argument only: it is a budget and never a refutation, and reaching it is exit 3. |
| `--leaf-limit N` | `search_leaf_limit`, `100000000` | **Measured, and it is why the flag exists.** The node limit bounds how many leaves are reached and nothing inside one, and a leaf is a whole pool scan or a whole subspace walk. At `<4,4,4>` one leaf is 4 294 836 225 maps rebuilt one at a time, timed here at **785 ns each, so 0.9 hours**, which no `--node-limit` could interrupt; the subspace-walk route timed at **78 ns an element** (dimension 27, 1.34e8 elements in 10.44 s). The default is 78 s of the first and 7.8 s of the second, and **383x the largest leaf any published run here reaches**, the 261 121-map pool of `<3,3,3>`, so it moves no number in this repository. Reaching it is exit 3, like the node limit. |
| `--general-leaf` | off | **An argument, not a default measurement chose.** It exists so the GF(2) leaf can be timed against the path it replaced *on one question*, since a comparison across two questions is not a comparison. What it measured, 6.0x to 39.6x against a published prediction of 40x to 64x, is in [`../exhaustive_search/gf2_leaf.h`](../exhaustive_search/gf2_leaf.h) and [`../positioning/hardware-and-parallelism.md`](../positioning/hardware-and-parallelism.md). |
| `--anchor map\|heuristic` | `map` | Argument, and semantic rather than quantitative: from the map the answer is the true minimum, from the heuristic it is the minimum only among algorithms containing that subspace. **The `heuristic` arm has never been timed**; every published result was anchored at the map (`../descent_search/method/`). |
| `--bottom-up` | off | **Unmeasured.** It selects `fewest_products_from_scratch`, which its own header calls "the most expensive approach" with no number attached. |
| sweep vs bisection (no flag) | sweep | **Unmeasured, and this is the one to distrust.** `../exhaustive_search/fewest_products.h` calls the sweep "trustworthy but slow" and bisection "faster", with no timing anywhere in the repository. The test runs both, prints `sweep N nodes, bisection M nodes`, and asserts only that they agree. Bisection's "faster" is an argument. |
| `-s, --symmetry` | `none` | As `minimise-rank` above. |
| `--threads N` | `1` | As `minimise-rank` above. |
| `--max-memory` | `2G` | As `minimise-rank` above. |

## `walk-scheme`

| Flag | Default | What chose the default |
|---|---|---|
| `--threads N` | `1` | **Measured, and the only search here where threading is free.** The seeds are independent walks, each `mt19937_64(seed)` over a start and a field nobody writes, so the answers are bit-identical at any thread count: on `⟨3,3,3⟩ --flips 20000 --seeds 8` the reported scheme, seed, flip and reduction counts agree exactly at 1, 4 and 8 workers, and only the elapsed figures move. Measured **3.2x at 4 workers and 5.4x at 8**, on a machine that was not quiet, so those are floors. Unlike `decide-rank`'s, this speedup costs nothing in counts: no shared budget, no early exit, no race. |
| `--flips N` | `20000` | **Nothing.** It is the setting the published runs used, not a tuned choice: `⟨3,3,3⟩` reaches 24 products in 38.1 s at `--flips 20000 --seeds 8`. |
| `--seeds N` | `8` | **Nothing**, for the same reason. |
| `--from k` | off, walk from naive | Measured: on `f3_3x6`, four seeds of 20 000 flips reach **12** products from the naive scheme and hold the heuristic's **10** when started there (`../flip_graph/README.md`). |
