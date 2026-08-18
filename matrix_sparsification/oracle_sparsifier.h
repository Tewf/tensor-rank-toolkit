#pragma once

#include <cstddef>
#include <vector>

#include "types.h"

/// The two exact oracles of the article, and the validator both rest on.
///
/// `[beniamini2020, Alg. 3]` is `sparsify_bottom_up` and `[beniamini2020,
/// Alg. 4]` is `sparsify_top_down`, each with the driver `[gottlieb2010]`
/// wrapped around it rather than beside it. Keys are
/// [references.md](../references.md).
///
/// These are exact methods based on the article's algorithms, unlike the
/// [heuristic](heuristic_sparsifier.h) which offers no optimality guarantee.
/// They are exact for the question they ask, which is narrower than "is this the
/// sparsest operator": each round replaces one row by the vector of its row
/// space with the most zeros that no settled row depends on.
namespace matrix_sparsification {

/// A vector in the row space of `rows`, zero on every column of `columns`, whose
/// support meets `settled` nowhere.
///
/// This is the article's Omega validator. It exists exactly when some row
/// outside `settled` is, on those columns, in the span of the others; the
/// coefficients that say so are the validator.
struct Validator {
    bool found = false;
    std::size_t replaces = 0;      // the row this vector may take over
    std::vector<Element> combination;  // coefficients over the rows
};

Validator find_validator(const Field& field, const Matrix& rows,
                         const std::vector<std::size_t>& columns,
                         const std::vector<std::size_t>& settled);

/// Bottom-up: consider every column subset of size one less than the number of
/// rows, and keep the validator whose vector has the most zeros.
Matrix sparsify_bottom_up(const Field& field, Matrix rows);

/// Top-down: walk column subsets from the largest down and take the first
/// validator found, because a vector forced to zero on more columns cannot be
/// beaten by one forced on fewer. Algorithms 2.4 and 4.
///
/// This variant handles the case where the search finds no candidate, returning
/// an empty result gracefully.
Matrix sparsify_top_down(const Field& field, Matrix rows);

}  // namespace matrix_sparsification
