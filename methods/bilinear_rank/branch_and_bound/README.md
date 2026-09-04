# Searching down from a bound you already hold

The exact search next door asks *"is there a `k`-dimensional subspace with a
rank-one basis?"* and cuts a branch at `dim V > k`. This asks *"how cheap can a
subspace be?"* and cuts a branch at `dim V + 1 >= best`, where `best` is the
cheapest thing built so far. Same tree, different stopping rule, opposite
guarantee: **this can only find, never refute**, so a spent budget hands back a
weaker algorithm rather than no answer.

| File | Role |
|---|---|
| [`cost_first_search.h`](cost_first_search.h) | the branch and bound, its bound, and why the bound is admissible |
| [`level_lowering_moves.h`](level_lowering_moves.h) | the moves, generated from `V` in closed form instead of scanned out of the pool |

    tighten-rank-bound evidence/fixtures/cyclic_f2_7.tensor --from descent --width 4

    GF(2), start: 15 products over 9 dimensions
    best: 13 products, rank bound 12, gap 1, verified
    # 22 nodes, 17371 children costed, 20678 moves offered, 2 improvements, 68 branches bounded, depth 4, largest single-move drop 1, tree exhausted

## The tree is `[bdez2012]`, and only the stopping rule is new here

`[bdez2012]` Algorithm 2 and `[yang2025]` Theorem 1 walk exactly these
subspaces, and so does
[`expand_subspace`](../exhaustive/exhaustive_search.h). Both of those
sweep a target upward from the floor, which is the right way round for a
**refutation**: `t + 1` has only to be not yet refuted, where an upper bound has
to be built ([`what-a-node-cannot-tell-you.md`](../exhaustive/what-a-node-cannot-tell-you.md),
item 3). This strand exists for the other direction, where the sweep gives
nothing until it finishes and never finishes on the fixtures below.

**The bound is one line.** `cost(V) >= dim V`, because a basis has `dim V`
elements and none has rank zero. Every `W` above a node has `dim W >= dim V + 1`,
so `cost(W) >= dim V + 1`, so nothing under a node with `dim V + 1 >= best` can
beat the incumbent. [`tests/`](tests/test_cost_first_search.cpp) asserts that on
every subspace one adjunction from a fixture rather than leaving it to the
paragraph.

## What it reaches

Every count is verified twice before it is printed. Per-fixture node counts, the
two fixtures it does not move, and where `f2_5x5` now stands proved on both
sides: [`what-it-reaches.md`](what-it-reaches.md).

## Where this stops

It proves nothing. `cost(V) > b` refutes nothing at all
([`sorted_span.h`](../greedy_heuristic/sorted_span.h) has the counterexample), so
no run here is ever a lower bound, and the floors quoted beside every count come
from [`rank_lower_bound.h`](../../../core/linear_algebra/rank_lower_bound.h) and not from
this search.

**It is not a rival to a [flip graph](../flip_graph/).** Every recent
record in the field came from rewriting a decomposition that already works
([`../the-research-front/upper-bounds.md`](../../../writeup/the-research-front/upper-bounds.md)),
and a walk never runs out of moves where this is bounded by
`C(|pool|, best − dim span T)` before any beam is imposed. What it has instead is
a starting point the walk does not: it begins at the minimum-weight basis, which
is exact, and the first few levels above it are small.
