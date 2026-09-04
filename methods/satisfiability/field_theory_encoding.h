#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "smtlib_file.h"
#include "tensor_file.h"
#include "types.h"

/// `rank(T) ≤ r` over `GF(p)`, handed to a solver that already knows the field.
///
/// The second of the two GF(p) backends, and the one with nothing of the field
/// in it. cvc5 implements a decision procedure for prime fields
/// (`[ozdemir2023]`), so the tensor equations go across as equations:
///
/// ```
/// (assert (= (ff.add (ff.mul (ff.mul a_0_i b_0_j) c_0_k) …) (as ff1 F)))
/// ```
///
/// No one-hot groups, no multiplication table, no addition chain: none of the
/// machinery that [the one-hot encoding](prime_field_encoding.h) has to get
/// right. That is its entire advantage, and the reason the two are worth
/// running against each other: they can only agree by both being correct, and
/// they fail in completely different ways.
///
/// The two are not both meant to survive. Which one stays is settled by
/// measurement, and recorded in [`method/`](method/).
namespace satisfiability {

struct FieldTheoryEncoding {
    formats::SmtProblem problem;

    std::size_t characteristic = 0;
    std::size_t products = 0;
    std::size_t rows = 0;
    std::size_t columns = 0;
    std::size_t slices = 0;

    /// The constant names, so a model can be read back as matrices.
    static std::string left_name(std::size_t term, std::size_t row);
    static std::string right_name(std::size_t term, std::size_t column);
    static std::string output_name(std::size_t term, std::size_t slice);
};

/// Encode "is there a decomposition of `tensor` into `products` rank-one terms".
///
/// Works for any prime including 2, where it is a third opinion on a question
/// the Boolean encoding already answers cheaply.
FieldTheoryEncoding encode_field_rank_at_most(const formats::Tensor& tensor,
                                              std::size_t products);

}  // namespace satisfiability
