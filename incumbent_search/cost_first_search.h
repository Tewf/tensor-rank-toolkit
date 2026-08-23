#pragma once

#include <cstddef>
#include <vector>

#include "automorphism.h"
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

    /// Only look for something costing `below` or less, and stop the moment one
    /// is reached. Zero asks the ordinary question, "how cheap can this get".
    ///
    /// **It is the incumbent and not a second mechanism.** The bound already
    /// reads an incumbent, so a caller who cares about nothing above `k` is a
    /// caller handing over `k + 1` as one: `dim V + 1 >= k + 1` cuts at
    /// dimension `k` immediately, and the tree below that is never entered. It
    /// is the same lever [`what-it-reaches.md`](what-it-reaches.md) prices for
    /// `--from descent` against `--from basis`, pushed the other way — a tighter
    /// incumbent is a shorter tree — and pushed past anything the search has
    /// actually built.
    ///
    /// **A run that does not reach `k` has refuted nothing.** The whole file can
    /// only find, and a tree cut by a number nobody has built is cut by a
    /// hypothesis rather than by an algorithm, so the run says which of the two
    /// happened and never turns the second into a bound.
    /// [`IncumbentReport::reached_below`](#) is that answer.
    std::size_t below = 0;

    /// The most a single move may take off the cost. **Zero means the bound
    /// stays as it was**, and any positive value tightens it.
    ///
    /// The standing bound is `cost(W) >= dim(W)`, so a strict descendant costs
    /// at least `dim + 1`. That is nearly tight when the rank sits just above the
    /// dimension and close to useless when it does not: at `<3,4,5>`, dimension
    /// 15 against a naive 60, it first says anything forty-four levels down.
    ///
    /// A second inequality fixes that. If one move can remove at most `s`, then a
    /// descendant `t` levels below satisfies both `cost >= dim + t` and
    /// `cost >= c - s*t`, and the cheapest it can be is the least value of their
    /// maximum. At that same root with `s = 1` the bound is **38** rather than 16.
    ///
    /// **`s` is measured, not proved, so the search checks it as it goes.** Every
    /// child's drop is compared against this value and
    /// [`IncumbentReport::drops_exceeded`](#) counts the violations. A run that
    /// used the bound and saw one has cut a branch it had no right to cut, and the
    /// command refuses to report its answer rather than quietly returning a worse
    /// one. Measured at **1** on seven fixtures; `level_lowering_moves.h` derives
    /// why a move is a level-lowering summand, which is consistent with 1 and is
    /// not a proof that nothing else is reachable.
    std::size_t cost_drop_bound = 0;

    /// Offer one move per orbit at each node instead of every move, under the
    /// group `search_from_above` is handed.
    ///
    /// **Off, and it stays off**, because every count published for this search
    /// was taken without it and because the group it needs exists for almost
    /// none of the fixtures here: `--symmetry auto` refuses to build 9.99872e13
    /// automorphisms for a 5x5 map over GF(2), and the closed form is a matrix
    /// multiplication one. Where a group *is* available it is lossless, for the
    /// reason [`orbit_moves.h`](orbit_moves.h) gives, and both counts are
    /// reported so the saving is a measurement rather than a claim.
    bool quotient_moves = false;
};

/// What the search spent, and what it found. Counts, not seconds: they are exact
/// whatever else the machine is doing, which [`../MEASURING.md`](../MEASURING.md)
/// requires of anything published.
struct IncumbentReport {
    std::size_t nodes = 0;         ///< subspaces expanded
    std::size_t children = 0;      ///< adjunctions costed, the search's real unit
    std::size_t moves_offered = 0; ///< candidates generated, before `V` filtered them
    std::size_t moves_entered = 0; ///< of those, the ones a node actually costed a child for
    std::size_t smallest_stabiliser = 0; ///< over the nodes quotiented, the group's least size
    std::size_t largest_stabiliser = 0;  ///< and its greatest
    std::size_t improvements = 0;  ///< times the incumbent fell
    std::size_t bounded = 0;       ///< branches cut by `dim V + 1 >= best`
    /// The largest amount any single move ever took off the cost, over every
    /// child costed and not only those the beam entered.
    ///
    /// **This is the constant the cost-aware bound needs and does not have.** The
    /// bound in `visit` rests on `cost(W) >= dim(W)`, which is weak exactly when
    /// the rank sits far above the dimension: on `<3,4,5>` it first says anything
    /// forty-four levels down. A second inequality, `cost(child) >= cost(parent)
    /// - s`, would tighten it to `a + (c - a)/(1 + s)`, which is 27 rather than 16
    /// at that root. `s` is currently a guess, `summand_rank`, and a bound built
    /// on a guess cuts the answer. So it is measured before it is used.
    std::size_t largest_drop = 0;

    /// Times a child's drop exceeded `IncumbentLimits::cost_drop_bound`. Nonzero
    /// after a run that used the bound means the bound was wrong on this map and
    /// the answer must not be trusted.
    std::size_t drops_exceeded = 0;
    std::size_t deepest = 0;       ///< adjunctions on the longest path entered
    std::size_t best = 0;          ///< the cheapest cost reached
    bool exhausted = true;         ///< false once the node budget stopped it

    /// True once `IncumbentLimits::below` was met, which is what stopped the
    /// run. False after a `--below` run means the tree or the budget ran out
    /// first, and **that is not a refutation of anything**: the branches this
    /// setting cuts were cut by a number nobody built.
    bool reached_below = false;
};

/// Search upward from `start`, which must span what the answer has to span.
///
/// Returns a minimum-weight basis of the cheapest subspace reached, so its ranks
/// sum to `report.best` and
/// [`rank_one_candidates`](../descent_search/candidate_pool.h) turns it into that
/// many products. `start` is the incumbent as well as the root: seeding it from
/// [`descend_from_own_basis`](../descent_search/minimise_rank.h) is what keeps
/// the bound tight enough to cut anything.
/// `ambient` is the group to take each node's stabiliser from, and is read only
/// where `IncumbentLimits::quotient_moves` is set. It is the **ambient** group
/// and not a stabiliser: what stabilises `span(T)` need not stabilise a span one
/// adjunction above it, so narrowing is this search's job and doing it once for
/// the caller would silently weaken the quotient. Empty is what every caller who
/// was never given one passes, and it means "try every move".
std::vector<Matrix> search_from_above(const Field& field, const std::vector<Matrix>& start,
                                      const std::vector<Matrix>& pool,
                                      const IncumbentLimits& limits, IncumbentReport* report,
                                      const std::vector<Automorphism>& ambient = {});

}  // namespace bilinear_rank
