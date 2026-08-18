#pragma once

#include <cstddef>
#include <vector>

#include "canonical_basis.h"

namespace canonical_factorisation {

/// A factorisation `S = C A` of the slice matrix, where every row of `A` reads
/// as a rank-one matrix over the canonical basis.
///
/// `A` is the answer and `C` is the receipt. Together they are checkable in one
/// matrix product by somebody who did not run the search and does not trust it,
/// which is the point of returning both.
struct Factorisation {
    /// `A`: `components` by `nm`, each row a rank-one matrix.
    ModularMatrix chosen;

    /// `C`: `k` by `components`, the combination of the rows of `A` that gives
    /// each slice back.
    ModularMatrix recovery;

    std::size_t components = 0;

    /// The floor the sweep started from, which is a proved lower bound.
    std::size_t floor = 0;

    /// Whether `components` is the rank, rather than merely a number of rows
    /// that worked.
    ///
    /// True when the sweep began at a proved floor and every question below the
    /// answer was refuted with the tree walked to its end. False the moment a
    /// budget runs out, because a question nobody finished asking is not a
    /// question answered no.
    bool minimal = false;
};

struct FactorisationSettings {
    /// 0 asks for `rank_lower_bound`, which is the sharpest floor here.
    std::size_t floor = 0;

    /// 0 asks for the naive ceiling, the sum of the slices' ranks, which is
    /// always reachable by decomposing each slice on its own.
    std::size_t ceiling = 0;

    std::size_t node_limit = 5'000'000;

    /// Quotient the search by the stabiliser of the slice space. Sound for any
    /// subgroup, so this only ever changes how long the answer takes.
    bool use_symmetry = true;
};

/// The fewest rank-one rows whose span contains the slices, with the receipt.
///
/// Runs the strongest route available for the shape: the rank-sum floor, then a
/// sweep upward, quotiented by symmetry where a group can be built. It sweeps
/// upward rather than bisecting because every question below the answer is then
/// a refutation that is kept, and the first success is minimal by construction.
Factorisation factor_over_canonical_basis(const ModularField& field,
                                          const std::vector<ModularMatrix>& slices,
                                          const FactorisationSettings& settings);

/// Whether a factorisation is what it says: every row of `A` of rank one, and
/// `C A` equal to `S` entry by entry.
///
/// Nothing about the search is consulted, so this checks the answer rather than
/// the machinery, and a factorisation read from a file is checked the same way
/// as one just computed.
bool recovers_slices(const ModularField& field, const std::vector<ModularMatrix>& slices,
                     const Factorisation& factorisation);

}  // namespace canonical_factorisation
