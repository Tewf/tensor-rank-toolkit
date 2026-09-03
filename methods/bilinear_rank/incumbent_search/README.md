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

    lower-the-bound fixtures/cyclic_f2_7.tensor --from descent --width 4

## The tree is `[bdez2012]`, and only the stopping rule is new here

`[bdez2012]` Algorithm 2 and `[yang2025]` Theorem 1 walk exactly these
subspaces, and so does
[`expand_subspace`](../exhaustive_search/exhaustive_search.h). Both of those
sweep a target upward from the floor, which is the right way round for a
**refutation**: `t + 1` has only to be not yet refuted, where an upper bound has
to be built ([`what-a-node-cannot-tell-you.md`](../exhaustive_search/what-a-node-cannot-tell-you.md),
item 3). This strand exists for the other direction, where the sweep gives
nothing until it finishes and never finishes on the fixtures below.

**The bound is one line.** `cost(V) >= dim V`, because a basis has `dim V`
elements and none has rank zero. Every `W` above a node has `dim W >= dim V + 1`,
so `cost(W) >= dim V + 1`, so nothing under a node with `dim V + 1 >= best` can
beat the incumbent. [`tests/`](tests/test_cost_first_search.cpp) asserts that on
every subspace one adjunction from a fixture rather than leaving it to the
paragraph.

## What it reaches

Every count is verified in the tool: the answer is decomposed, the algorithm
rebuilt and multiplied out against the map before the number is printed.
Measurements, and the two it does not move:
[`what-it-reaches.md`](what-it-reaches.md).

**`cyclic_f2_7` from 15 to 13 in 22 nodes**, where the descent's step 3 has a
shortlist of 0 out of 16 129 and cannot take a first step. **`f2_5x5` from 14 to
13**, which with the exhaustive refutation at 12 settles `rank = 13` in this
repository alone. **`matmul_2x2x2` to 7 with the tree exhausted**, from a
generated move set that never forms the pool.

## Where this stops

It proves nothing. `cost(V) > b` refutes nothing at all
([`sorted_span.h`](../descent_search/sorted_span.h) has the counterexample), so
no run here is ever a lower bound, and the floors quoted beside every count come
from [`rank_lower_bound.h`](../../../linear_algebra/rank_lower_bound.h) and not from
this search.

**It is not a rival to a flip graph.** Every recent record in the field came
from rewriting a decomposition that already works
([`../the-research-front/upper-bounds.md`](../../../the-research-front/upper-bounds.md)),
and a walk never runs out of moves where this is bounded by
`C(|pool|, best − dim span T)` before any beam is imposed. What it has instead is
a starting point the walk does not: it begins at the minimum-weight basis, which
is exact, and the first few levels above it are small.
