#pragma once

#include <cstddef>
#include <vector>

#include "bilinear_rank_aliases.h"

/// A branch and bound over subspaces that minimises the **cost** instead of
/// fixing the dimension, so that it can be stopped at any moment and still hand
/// back an algorithm.
///
/// **The tree is not new.** It is `[bdez2012]` Algorithm 2, which
/// [`../exhaustive_search/exhaustive_search.h`](../exhaustive_search/exhaustive_search.h)
/// already implements: a node is a subspace `V` containing `span(T)`, a child is
/// `V + <g>` for a rank-one `g`, and the same tree is `[yang2025]`'s Theorem 1.
/// What differs is the question asked of a node and the reason it stops.
///
/// | | `decide-rank` | here |
/// |---|---|---|
/// | asks | is there a `k`-dimensional `V` with a rank-one basis | how cheap can a `V` be |
/// | stops a branch at | `dim V > k`, a target fixed in advance | `dim V + 1 >= best`, an incumbent |
/// | a spent budget gives | nothing, since a partial refutation refutes nothing | the cheapest algorithm reached so far |
///
/// **The bound is admissible, and one line proves it.** `cost(V) >= dim V` for
/// every subspace, because a basis has `dim V` elements and each has rank at
/// least one. Every `W` strictly above a node has `dim W >= dim V + 1`, so
/// `cost(W) >= dim V + 1`, so once `dim V + 1` reaches the incumbent no
/// descendant of that node can beat it. That is what
/// [`tests/test_cost_first_search.cpp`](tests/test_cost_first_search.cpp) asserts
/// rather than what this comment asks to be believed.
///
/// **It can only find, never refute.** `cost(V) <= b` exhibits a `b`-product
/// algorithm: take a minimum-weight basis of `V`, split each element into its
/// rank-one pieces, and those `cost(V)` rank-one maps span something containing
/// `span(T)`. `cost(V) > b` says nothing at all, which
/// [`../descent_search/sorted_span.h`](../descent_search/sorted_span.h) states with
/// the counterexample. So a run that exhausts its tree has proved no lower bound
/// and this file never claims one.
///
/// **Where it sits beside the two searches next door.** The descent
/// ([`../descent_search/minimise_rank.h`](../descent_search/minimise_rank.h))
/// adopts a candidate only when it makes the map strictly cheaper and stops when
/// no single one does, which on `cyclic_f2_7` is 0 candidates out of 16 129. The
/// plateau crossing
/// ([`../flip_graph/plateau_search.h`](../flip_graph/plateau_search.h)) admits
/// equal-cost moves as well and refuses only the ones that cost more. This admits
/// every move and lets the dimension bound do the refusing, which is strictly the
/// larger tree of the three and the reason it needs a budget rather than a
/// stopping rule.
namespace bilinear_rank {

/// How much of the tree a search may take, and how widely it may branch.
struct IncumbentLimits {
    /// Subspaces expanded. A spent budget withdraws nothing, since nothing here
    /// was ever a refutation; it only stops the incumbent improving.
    std::size_t node_limit = 20'000;

    /// Children entered per node, cheapest first. **Zero means every child**,
    /// which is what makes the search a branch and bound rather than a beam, and
    /// what the completeness test uses.
    std::size_t width = 4;

    /// The largest rank of a span element a move is allowed to split. Moves are
    /// generated from `V` rather than scanned out of the pool:
    /// [`level_lowering_moves.h`](level_lowering_moves.h) says which ones and why
    /// they are the only ones that can fail to cost a multiplication.
    std::size_t summand_rank = 3;

    /// Offer the whole rank-one pool at every node instead of the generated
    /// moves. Complete where the generated set is a restriction, and priced at
    /// `|pool|` minimum-weight bases a node against a few hundred.
    bool whole_pool = false;
};

/// What the search spent, and what it found. Counts, not seconds: they are exact
/// whatever else the machine is doing, which [`../MEASURING.md`](../MEASURING.md)
/// requires of anything published.
struct IncumbentReport {
    std::size_t nodes = 0;         ///< subspaces expanded
    std::size_t children = 0;      ///< adjunctions costed, the search's real unit
    std::size_t moves_offered = 0; ///< candidates generated, before `V` filtered them
    std::size_t improvements = 0;  ///< times the incumbent fell
    std::size_t bounded = 0;       ///< branches cut by `dim V + 1 >= best`
    std::size_t deepest = 0;       ///< adjunctions on the longest path entered
    std::size_t best = 0;          ///< the cheapest cost reached
    bool exhausted = true;         ///< false once the node budget stopped it
};

/// Search upward from `start`, which must span what the answer has to span.
///
/// Returns a minimum-weight basis of the cheapest subspace reached, so its ranks
/// sum to `report.best` and
/// [`rank_one_candidates`](../descent_search/candidate_pool.h) turns it into that
/// many products. `start` is the incumbent as well as the root: seeding it from
/// [`descend_from_own_basis`](../descent_search/minimise_rank.h) is what keeps
/// the bound tight enough to cut anything.
std::vector<Matrix> search_from_above(const Field& field, const std::vector<Matrix>& start,
                                      const std::vector<Matrix>& pool,
                                      const IncumbentLimits& limits, IncumbentReport* report);

}  // namespace bilinear_rank
