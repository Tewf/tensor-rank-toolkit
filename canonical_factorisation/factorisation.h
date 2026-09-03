#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "canonical_basis.h"
#include "symmetry_argument.h"

namespace canonical_factorisation {

/// How the rank-one rows are found. The factorisation they produce is the same
/// object and is checked the same way; what differs is the space it costs and
/// what the answer is allowed to claim.
enum class Route {
    /// By the shape. The pool is `(p^n - 1)(p^m - 1)/(p-1)^2` matrices and is
    /// materialised, so past `pool_ceiling` this takes the SAT route instead of
    /// asking for room it may not have.
    Automatic,

    /// The exhaustive tree over a materialised pool. Exponential space, and the
    /// only route whose refusals are proofs this repository walked itself.
    Exhaustive,

    /// A SAT solver, which is handed the question and never forms a pool at
    /// all: the rank-one condition is clauses over the operand vectors, so the
    /// space is polynomial. Needs a solver on `PATH`.
    Satisfiability,

    /// The exhaustive tree with McKay canonical augmentation `[mckay1998]`, so
    /// each solution subspace is reached once per orbit rather than once per
    /// orbit element, with no memory of what was generated.
    ///
    /// This is aimed at the part of the sweep that costs: a question *below* the
    /// rank has to exhaust its tree, and every question below the answer is one
    /// of those. Needs the **full** closed-form group rather than generators,
    /// because the parent test walks it, so it is available only where that group
    /// fits in a list: 216 elements at `<2,2,2>`, and refused at `<3,3,3>` where
    /// it is 4 741 632. Falls back to `Exhaustive` and says so when it cannot be
    /// had.
    CanonicalAugmentation,
};

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

    /// Which route produced it, so a reader knows what `minimal` rests on.
    Route route = Route::Exhaustive;

    /// How many automorphisms the search was actually quotiented by, so a run
    /// that asked for symmetry and got none says so as a number.
    std::size_t group_size = 0;

    /// Why no group was built, when one was asked for and refused. Empty
    /// otherwise. A silent fallback here is how symmetry gets reported as on
    /// while doing nothing.
    std::string symmetry_refusal;

    /// Subspaces built and tested, summed over the sweep. The measure that does
    /// not depend on the machine, and the one the routes are compared by.
    std::size_t nodes_visited = 0;

    /// Canonical images asked for over the sweep, zero on every route but
    /// canonical augmentation. Machine-independent like the node count, and the
    /// one that says what the parent test *cost* rather than what it saved: a node
    /// makes one of these per candidate parent plus one for the distinguished cell,
    /// so `canonisations / nodes_visited` is the branching factor `[mckay1998,
    /// Thm 3]` calls `c` multiplied by the candidate parents per test.
    /// `methods/bilinear_rank/canonical_augmentation/when-canonical-pays.md` is what reads it.
    std::size_t canonisations = 0;

    /// The materialised pool this would have needed, whether or not it formed
    /// one. Reported so the SAT route's advantage is a number and not a claim.
    std::size_t pool_size = 0;

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
    Route route = Route::Automatic;

    /// Where `Automatic` stops trusting the pool, in matrices. The default is
    /// the largest pool measured here that still costs under a second to form.
    std::size_t pool_ceiling = 20000;

    /// 0 asks for `rank_lower_bound`, which is the sharpest floor here.
    std::size_t floor = 0;

    /// 0 asks for the naive ceiling, the sum of the slices' ranks, which is
    /// always reachable by decomposing each slice on its own.
    std::size_t ceiling = 0;

    std::size_t node_limit = 5'000'000;

    /// Which ambient group to narrow to the stabiliser of the slice space
    /// before quotienting the pool search by it. Sound for any subgroup, so
    /// this only ever changes how long the answer takes.
    ///
    /// **`Automatic` is not the strong choice, and on the shapes that matter it
    /// is no choice at all.** It enumerates the ambient group, which
    /// `requested_ambient_group` refuses past a ceiling: a 4x4 map over GF(2)
    /// would need about four hundred million automorphisms. So on every matrix
    /// multiplication fixture here it refuses and the search runs unquotiented.
    /// `MatrixMultiplication` uses the closed form instead, which needs no group
    /// enumerated and works at any size. See
    /// [`narrowing-the-search.md`](narrowing-the-search.md) for what each is
    /// worth, measured.
    cli::Symmetry symmetry{cli::SymmetryKind::Automatic, {}};
};

/// The fewest rank-one rows whose span contains the slices, with the receipt.
///
/// Runs the strongest route available for the shape: the rank-sum floor, then a
/// sweep upward, quotiented by symmetry where a group can be built. It sweeps
/// upward rather than bisecting because every question below the answer is then
/// a refutation that is kept, and the first success is minimal by construction.
///
/// Past `pool_ceiling` it hands the same sweep to a SAT solver, which asks the
/// same questions without ever forming the pool. That is the difference between
/// polynomial and exponential space here, and it is what lets shapes the pool
/// refuses outright be factored at all.
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
