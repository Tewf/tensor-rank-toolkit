# Searching for rank

The three commands that ask how few multiplications a map needs and disagree
about what they can prove. Precedence and `BILINEAR_TUNABLES`:
[`../OPTIONS.md`](../OPTIONS.md).

## `minimise-rank`

| Flag | Default | What chose the default |
|---|---|---|
| `--steps 1\|2\|3` | `3` | Measured, and the measurement is a warning: step 3 improved the answer in **two of four** polynomial fixtures, by one product each, and cost **58 to 184 times** steps 1 and 2 together (`../README.md`). |
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
| `--anchor map\|heuristic` | `map` | Argument, and semantic rather than quantitative: from the map the answer is the true minimum, from the heuristic it is the minimum only among algorithms containing that subspace. **The `heuristic` arm has never been timed**; every published result was anchored at the map (`../descent_search/method/`). |
| `--bottom-up` | off | **Unmeasured.** It selects `fewest_products_from_scratch`, which its own header calls "the most expensive approach" with no number attached. |
| sweep vs bisection (no flag) | sweep | **Unmeasured, and this is the one to distrust.** `../exhaustive_search/fewest_products.h` calls the sweep "trustworthy but slow" and bisection "faster", with no timing anywhere in the repository. The test runs both, prints `sweep N nodes, bisection M nodes`, and asserts only that they agree. Bisection's "faster" is an argument. |
| `-s, --symmetry` | `none` | As `minimise-rank` above. |
| `--threads N` | `1` | As `minimise-rank` above. |
| `--max-memory` | `2G` | As `minimise-rank` above. |

## `walk-scheme`

| Flag | Default | What chose the default |
|---|---|---|
| `--flips N` | `20000` | **Nothing.** It is the setting the published runs used, not a tuned choice: `⟨3,3,3⟩` reaches 24 products in 38.1 s at `--flips 20000 --seeds 8`. |
| `--seeds N` | `8` | **Nothing**, for the same reason. |
| `--from k` | off, walk from naive | Measured: on `f3_3x6`, four seeds of 20 000 flips reach **12** products from the naive scheme and hold the heuristic's **10** when started there (`../flip_graph/README.md`). |
