#pragma once

#include <cstddef>
#include <vector>

#include "field.h"
#include "matrix.h"
#include "pencil_divisors.h"

namespace pencil_rank {

/// The Kronecker canonical form of a pencil, as the four things it is made of.
///
/// The form itself is never assembled. Every question the rank formula asks is
/// answered by these counts, and building the block matrix would only give
/// something else to keep consistent.
struct KroneckerStructure {
    /// Degrees of a minimal basis of the pencil's null space, ascending. A zero
    /// here is a column that is zero in both slices.
    std::vector<std::size_t> column_indices;

    /// The same on the other side.
    std::vector<std::size_t> row_indices;

    std::vector<ElementaryDivisor> divisors;

    /// The size of the square regular block, which the two dimension counts
    /// determine twice over. They are required to agree.
    std::size_t regular_size = 0;
};

/// The Kronecker structure of the pencil `first + x second` over GF(p).
///
/// Throws if the two dimension counts disagree with each other or with the
/// degrees of the elementary divisors. Those are three routes to one number and
/// the arithmetic is exact, so a disagreement is a defect and never a rounding.
KroneckerStructure kronecker_structure(const ModularField& field, const ModularMatrix& first,
                                       const ModularMatrix& second);

/// What this module can say about the rank, and how strongly.
struct PencilRank {
    /// The rank over the algebraic closure of GF(p), which Ja'Ja's theorem
    /// gives exactly. Enlarging a field can only add rank-one maps to choose
    /// from, so this is a **proved lower bound** on the rank over GF(p) itself.
    std::size_t over_closure = 0;

    /// The same count taken with the elementary divisors over GF(p) itself,
    /// where an irreducible of degree at least 2 is nonlinear because it has no
    /// root here rather than because a root repeats.
    ///
    /// **PROVISIONAL.** On the twelve pencils in `README.md` it never exceeds
    /// the rank and equals it on nine, which is sharper than `over_closure` on
    /// every one of them. It is not proved to be a lower bound and it is not
    /// proved to be reached, so it is reported beside the proved number rather
    /// than instead of it.
    std::size_t over_the_field = 0;

    /// Whether `over_closure` is also the rank over GF(p).
    ///
    /// True only in the case this module can prove: no minimal indices, and
    /// every elementary divisor a linear form of multiplicity one. Then the
    /// pencil is simultaneously diagonalisable over GF(p) and the `r` matrices
    /// `e_i e_i^T` in that basis are rank one, rational over GF(p), and span
    /// it, so the bound is attained.
    ///
    /// False does **not** mean the bound is loose. It means this module has not
    /// proved it tight, and over a small field it frequently is not: see the
    /// measured table in [`README.md`](README.md).
    bool exact = false;
};

/// The rank over the algebraic closure, from the structure.
///
///     rank = sum (eps_i + 1) + sum (eta_j + 1) + regular_size + delta
///
/// over the nonzero minimal indices, where `delta` counts the elementary
/// divisors of degree at least 2 **over the closure**. An irreducible of degree
/// `d` splits there into `d` distinct linear forms, so a GF(p) divisor `p^e`
/// contributes `deg(p)` closure divisors, each nonlinear exactly when `e >= 2`.
/// That is why the count below multiplies by the base degree rather than adding
/// one per GF(p) divisor, and getting this wrong is what made an earlier
/// version of this file claim 5 for a tensor of rank 6.
std::size_t rank_over_closure(const KroneckerStructure& structure);

/// The same sum with the GF(p) elementary divisors. See
/// `PencilRank::over_the_field`: sharper on every pencil measured, and not
/// proved.
std::size_t rank_over_the_field(const KroneckerStructure& structure);

/// The rank of a tensor with at most two slices, in polynomial time.
///
/// Throws if it is handed more than two slices, which is a different problem
/// and an exponential one.
PencilRank pencil_rank_of(const ModularField& field,
                          const std::vector<ModularMatrix>& slices);

}  // namespace pencil_rank
