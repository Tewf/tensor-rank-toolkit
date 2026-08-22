#pragma once

#include <cstddef>
#include <map>
#include <vector>

#include "bilinear_rank_aliases.h"
#include "subspace_canon.h"

/// How often one run reaches the same subspace by more than one route.
///
/// **The question this exists to answer.** `search_from_above` already refuses
/// two moves that open the same child *at one node*: `residue_of` reduces each
/// candidate against `V` and scales it, so two moves with one residue are one
/// child. Nothing refuses two *nodes* that arrive at one subspace by adjoining
/// the same moves in a different order, and that is exactly the duplication
/// McKay canonical augmentation removes, at one canonisation a node. Whether it
/// could pay here is a measurement and not an argument, so this counts the
/// duplication and the decision is taken against the number.
///
/// **A reduced row echelon form is already the canonical form of a subspace, so
/// no canonisation is needed to count the duplication.** The RREF of a matrix is
/// unique and depends only on its row space, so two bases of one subspace reduce
/// to the same rows whatever order they were built in; fixing the row order
/// makes that a name. [`subspace_code`](../orbit_reduction/subspace_canon.h) is
/// that name, and this asks it rather than repeating it. It is the same
/// function the canonical augmentation next door identifies its own objects by,
/// which is what makes a count taken here comparable with that route's.
///
/// The code is stored whole rather than hashed, so a repeat counted here is a
/// repeat and never a collision.
///
/// **It is a census and not a filter.** Nothing here changes which nodes the
/// search enters or in which order, so a run with it on visits exactly the nodes
/// of the run with it off and reports the same counts.
namespace bilinear_rank {

/// One population of subspaces, and how often each was reached.
class SpanTally {
public:
    /// Fingerprint this basis's span and add one to its count.
    void record(const Field& field, const std::vector<Matrix>& basis);

    std::size_t recorded() const { return recorded_; }
    std::size_t distinct() const { return tally_.size(); }

    /// The most times any single subspace was reached. One means every member
    /// of this population was a different subspace and there is nothing for a
    /// parent test to remove.
    std::size_t most_repeated() const;

private:
    std::map<SubspaceCode, std::size_t> tally_;
    std::size_t recorded_ = 0;
};

/// The three populations a subspace can repeat in, which have three prices.
///
/// **A repeat is worth removing only in proportion to what it costs**, and the
/// three differ by orders of magnitude, so one rate over all of them would
/// answer no question. A repeated **entry** may cost nothing at all: most nodes
/// this search enters are turned back by `dim V + 1 >= best` on arrival. A
/// repeated **expansion** costs a whole subtree. A repeated **child** costs one
/// `minimum_weight_basis_with`, which is `p^dim` ranks and is where nearly all
/// of the run's time goes; there are two to five orders of magnitude more of
/// them than of nodes, so this is the population any saving would have to come
/// from.
struct SpanCensus {
    SpanTally entered;   ///< subspaces `visit` was called on, the root included
    SpanTally expanded;  ///< of those, the ones that survived the bound and branched
    SpanTally children;  ///< subspaces costed as children, entered or not
};

}  // namespace bilinear_rank
