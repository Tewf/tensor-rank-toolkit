# The audit: every strand, its inner loop, and its three axes

Read with [`README.md`](README.md), which defines the three axes and what
"GPU-shaped" has to mean for the column to be falsifiable. **A ✔ means the
strand reaches the mechanism from its own command line**; a strand that
transitively touches `require_room` inside a callee but whose command has no
`--max-memory` gets a ✖, because a budget nobody can move is not adaptation.

Taken 2026-08-21, before any of it was changed.

## The table

| strand | dominant inner loop | GPU-shaped? | (a) threads | (b) memory | (c) device |
|---|---|---|---|---|---|
| **exhaustion** `exhaustive_search/` | the leaf test: millions of rank-one maps reduced against one span, `gf2_leaf.cpp` | **YES** — and it is *the* measured case: 4.29e9 maps in 1.02 s on a card against 9.2 min on a core | ✔ `--threads`, `parallel_for` over subtrees `exhaustive_search.cpp:151` | ✔ `--max-memory`, `require_room` on the pool | ✔ `--device`, `chosen_device` per leaf `gf2_leaf.cpp:237,284` |
| **descent** `descent_search/` | `span_element_ranks`: `p^dim` ranks of one span, then a greedy over them | **YES** for the rank sweep — the second seam. **No** for the greedy above it, which carries the basis forward | ✔ `--threads`, prefetch window `worker_count()*4` `minimise_rank.cpp:39` | ✔ `--max-memory`, `require_room` ×3 | ~ `chosen_device(combinations)` `minimum_weight_basis.cpp:118`, no `--device` flag |
| **incumbent** `incumbent_search/` | branch-and-bound over `V + <g>`; per node one `p^dim` sweep then one `minimum_weight_basis_with` per surviving move `cost_first_search.cpp:121` | **NO** — the incumbent `ceiling` decides what the next node prunes; items are six orders of magnitude apart in cost; hundreds of nodes, not millions | ✖ **no `--threads` at all**, and the per-move loop is the twin of the one `descent` already threads | ✖ **no `--max-memory`** — and `require_room`'s refusal names a flag this command did not have. Plus `p^r` unguarded at `level_lowering_moves.cpp:25` | ~ reaches the span seam through `descent`; no flag |
| **flip graph** `flip_graph/` (walk) | a Markov chain: rebuild all `3n²` moves, pick one, apply, re-merge `flip_graph.cpp:190` | **NO** — step *k+1* is built from step *k*'s scheme; serial RNG *is* the walk's identity; `n` is 8 to 24 | ✔ `--threads` — and it is the **only** correctly threaded loop here: per seed, bit-identical at any count | ✖ **no `--max-memory`** | ✖ |
| **plateau crossing** `flip_graph/plateau_search.cpp` | per state, one `p^dim` sweep then one `minimum_weight_basis_with` per candidate `:95` | **NO** — same shape as the incumbent: `visited` and `best_cost` carried forward, depth-first recursion on the first improvement | ✖ **worse than absent**: `minimise-rank --threads 8 --plateau` *accepts* the flag and then runs the whole crossing on one core | ✔ via `minimise-rank`; `visited` bounded by `plateau_state_budget`, `level` not bounded by anything | ~ via the span seam |
| **orbit reduction** `orbit_reduction/` | the isomorph-rejected tree walk `orbit_search.cpp:64` | **NO** — the span grows and the residual group narrows down each branch; subtree cost "wildly uneven" by `parallel.cpp`'s own admission | ✔ `parallel_for` `:220`, reached by 3 commands' `--threads` | ~ `require_room` ×4, but `pool_orbits.cpp:54,120` builds the *same shape of table* the guarded site prices, unguarded | ✖ |
| **isomorph-free** `orbit_reduction/isomorph_rejection.cpp` | `least_in_orbit`: BFS closure of one point under the residual generators, linear `std::find` over `seen` `:31` | **NO** — a BFS frontier is loop-carried by definition, and it runs *inside* the tree walk, one call per node | ✔ inherits the walk's workers | ✔ bounded by the orbit | ✖ |
| **SAT** `satisfiability/` | `answer_from` forks kissat/cadical and blocks on `waitpid` `solver_process.cpp:99` | **NO** — the work is in another process. GPU CDCL is a published negative result; 13 cubes is not a swarm; 0.52 s to 143 s per cube is not uniform | ✖ **`parallel_for` at `rank_question.cpp:222` that no flag can reach.** The comment beside it measures 3.42× — dead from this command line | ~ `--max-memory` exists but means the **child's `RLIMIT_AS`**, not `set_memory_budget`. No `require_room` anywhere; the encoders are capped by a private variable budget | ✖ |
| **pencils** `pencil_rank/` | `diagonal_form`: a Smith-form sweep over `GF(p)[x]`, pivot search reading what the previous elimination wrote `pencil_divisors.cpp:57` | **NO** — loop-carried in the strongest sense; and `column_minimal_indices` is a few big eliminations, not a swarm. **Whole strand: 105 canonical forms in 0.94 ms** | ✖ (correct: nothing to spread on the live path) | ✖ **`split_completely` allocates `p+1` `int64_t` straight from the file's characteristic** — `field 2147483647` asks for 17 GB with nothing in front of it | ✖ |
| **sparsification** `matrix_sparsification/` | `find_validator`: one Gaussian elimination over **exact `Q` rationals** per column subset `oracle_sparsifier.cpp:30` | **NO** — every `axpyin` is a heap-allocating GMP rational with a gcd; greedy state rewritten each round; 35 subsets on the operators shipped | ✖ (35 items, sub-millisecond — correct to leave) | ✖ **`combinations()` materialises every subset with no `reserve` and no guard**: `C(47,23)` ≈ 1.6e13 on a `⟨4,4,4⟩` operator, which the README invites via `--emit-operators` | ✖ |
| **rank sums** `linear_algebra/tensor_rank_sum.h` | `contraction_ranks`: for each of up to 2²⁰ index vectors, decode its digits and rank the contraction `:149` | **YES in form** — the closest match in the repo to `device.h`'s own words, *"formed from its own index and reduced against a basis held in common"*. See the verdict below | ✖ **no `parallel_for`, and three commands' `--threads` stop at its door** | ✖ two exponential tables (`:146` 8 MB, `:194` **160 MB**) capped by `constexpr` only; `--max-memory` cannot reach either | ✖ |
| **curve bounds** `curve_bounds/` + `integer_programme/` | a knapsack DP where `by_points[p+1][·]` is written from `by_points[p][·]` `interpolation_programme.cpp:76`; or an exact-rational simplex; or a forked MILP solver | **NO** — three ways at once: a serial DP, a branch-and-bound incumbent, GMP rationals, and an external process | ✖ | ✖ **`O(--degree²)` of `size_t` and of `Step` with no guard**: `--degree 100000` asks for ~240 GB and is simply killed | ✖ |
| **canonical factorisation** `canonical_factorisation/` | none of its own — a sweep that hands each `k` to another strand `factorisation.cpp:299` | **NO** — external solver, or a tree, or a few small solves | ✔ `--threads` (honest about which routes read it) | ✔ `--max-memory` | ✖ |
| **map construction** `map_construction/` | builds `rows*columns` slices and writes them out `:98` | **NO** — one item, fixed work, output is a file | n/a by design: `make-tensor`'s flags are modes, not settings | ✖ `--matmul 2 100 100 100` asks for ~10¹² `int64_t` unguarded | ✖ |
| **linear algebra** `linear_algebra/span_basis.h` | the `axpyin` row loop of one small elimination, *"hundreds of millions of times, once per pool element per leaf"* | **NO as a library** — one call is one item. The swarm belongs to the caller, and that caller (`gf2_leaf`) is already routed | n/a — the parallelism belongs to the callers, and they have it | n/a — every allocation is shape-sized | n/a — correctly device-free |

## The two seams cover the two GPU-shaped loops, and that is the whole list

`chosen_device` is called from exactly four places:
`exhaustive_search/gf2_leaf.cpp:237` and `:284` (the leaf, both routes),
`descent_search/minimum_weight_basis.cpp:118` (`span_element_ranks`), and
`search_plan/search_plan.cpp:123` (the plan, which reports what the other three
will do). **No strand decides on its own**, so the "route it through" half of the
brief had nothing to route.

## The one candidate for a third seam, and why it stays unbuilt

`contraction_ranks` is genuinely the right shape and it is not one of the two.
Three reasons it does not get a kernel:

1. **It is the general-field path, and that field has none of the shape.**
   `gpu_leaf/README.md` already says it: *"Givaro carries every element as an
   `int64_t`; GF(3), GF(5) and the rationals have none of this shape."* The two
   existing kernels are GF(2) bit arithmetic. A rank kernel over GF(p) for
   arbitrary `p` is a new capability, not a new seam.
2. **The Amdahl ceiling is unmeasurable.** The whole rank-sum bound is 3 ms to
   469 ms, once, before the search starts. Even at the card's measured 500× that
   saves under half a second off runs measured in seconds to hours — inside
   `MEASURING.md`'s 13% band on this chassis, so it could not be reported as a
   ratio even if it were real.
3. **`parallel_for` gets most of it and works on every machine.** 469 ms on one
   core is under 50 ms on twelve, needs no toolkit, no card, no kernel and no
   fourth `handles()` list of shapes. That is the fix that was made.

So: **the two existing seams cover everything worth putting on a card, and no
third seam was built.** That is the answer, not a deferral.

## Where `launch_floor` is the wrong knob, and is not bent to fit

`launch_floor` counts **elements of one uniform item**. It is the right knob for
the leaf and for `span_element_ranks`, whose items are one bit-mask each.

It would be the **wrong** knob for `contraction_ranks`, whose item is a whole
Gaussian elimination on a matrix whose size varies by axis — 2²⁰ of those is not
comparable to 2²⁰ bit reductions, and comparing them against one floor would
have made the floor mean two things. It would be equally wrong for the
incumbent's per-move loop, where most items cost a set lookup and a few cost a
`p^dim` walk. Rather than add a second unit to the floor, neither is routed
through it: they get cores, which do not care whether items are uniform.
