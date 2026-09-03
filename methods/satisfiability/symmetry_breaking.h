#pragma once

#include <cstddef>
#include <vector>

#include "dimacs_file.h"

/// Saying that two decompositions differing only by a symmetry are the same
/// one.
///
/// A decomposition into `r` rank-one terms has redundancy the solver cannot see
/// and must otherwise refute one copy at a time, which is why proving a lower
/// bound is expensive. Two symmetries matter here:
///
/// | | |
/// |---|---|
/// | **Order** | the `r` terms are a set, so `r!` orderings describe one decomposition |
/// | **Scale** | `(λa) ⊗ (μb) ⊗ (νc)` is the same term as `a ⊗ b ⊗ c` whenever `λμν = 1` |
///
/// The first applies over any field. The second is empty over GF(2), where the
/// only nonzero scalar is 1, and is `(p−1)²` copies per term over `GF(p)`.
///
/// **This is the most dangerous code in the folder.** Every other mistake here
/// makes a question unanswerable or slow; a symmetry constraint that is too
/// strong removes a decomposition that exists, turns a satisfiable instance
/// into UNSAT, and reports a lower bound that is false. Nothing here is trusted
/// until a decomposition of known rank has been shown to survive it.
namespace satisfiability {

/// Constrain `earlier` to be lexicographically at most `later`, over two equal
/// length lists of variables.
///
/// Sound for the ordering symmetry because permuting the terms of a
/// decomposition gives another decomposition, so if any ordering satisfies the
/// formula then the sorted one does.
void order_lexicographically(formats::Cnf& formula, const std::vector<int>& earlier,
                             const std::vector<int>& later);

/// Constrain a vector of one-hot groups so that its first nonzero entry is 1.
///
/// Sound for the scaling symmetry, applied to two of the three vectors of a
/// term: any nonzero vector has exactly one scalar multiple whose first nonzero
/// entry is 1, and the third vector absorbs what the other two gave up. An
/// all-zero vector is left alone, since the constraint is vacuous there and the
/// term is the zero tensor, which a decomposition is allowed to contain.
///
/// `groups[i][v]` is the variable saying entry `i` holds field value `v`.
void normalise_first_nonzero(formats::Cnf& formula,
                             const std::vector<std::vector<int>>& groups);

}  // namespace satisfiability
