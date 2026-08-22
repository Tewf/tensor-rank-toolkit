#pragma once

#include <cstddef>
#include <vector>

#include "types.h"

/// The Ω validator: the vector both `[beniamini2020]` oracles look for, and the
/// one piece of them the rest of this module still needs.
///
/// One role: given a set of columns and the rows already settled, produce a
/// vector of the row space that vanishes on those columns and does not lie in
/// the settled span, or say there is none.
///
/// `[beniamini2020, Def. 3.2]` calls `S` **Ω-valid** when some `λ` with
/// `supp(λ) ⊄ Ω` has `λᵀU_{:,S} = 0`, and `λ` its **Ω-validator**. That is
/// exactly the search below: a row outside `settled` whose restriction to `S`
/// lies in the span of the others gives such a `λ`, with `-1` in its own place.
/// The `-1` is what makes the result independent of the settled rows, since the
/// rows are a basis and that coefficient sits outside `Ω`.
///
/// The prose is [`method/the-validator.md`](method/the-validator.md).
namespace matrix_sparsification {

struct Validator {
    bool found = false;
    std::size_t replaces = 0;          // the row this vector may take over
    std::vector<Element> combination;  // coefficients over the rows
};

Validator find_validator(const Field& field, const Matrix& rows,
                         const std::vector<std::size_t>& columns,
                         const std::vector<std::size_t>& settled);

}  // namespace matrix_sparsification
