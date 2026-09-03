#include "field_theory_encoding.h"

#include <stdexcept>

namespace satisfiability {

namespace {

/// Nest a binary operator over the terms, since `ff.add` and `ff.mul` are
/// binary and an empty sum is the field's zero.
std::string folded(const std::string& op, const std::vector<std::string>& terms,
                   const std::string& empty) {
    if (terms.empty()) return empty;

    std::string built = terms.front();
    for (std::size_t index = 1; index < terms.size(); ++index) {
        built = "(" + op + " " + built + " " + terms[index] + ")";
    }
    return built;
}

std::size_t reduced(int64_t value, int64_t characteristic) {
    return static_cast<std::size_t>((value % characteristic + characteristic) % characteristic);
}

}  // namespace

std::string FieldTheoryEncoding::left_name(std::size_t term, std::size_t row) {
    return "a_" + std::to_string(term) + "_" + std::to_string(row);
}
std::string FieldTheoryEncoding::right_name(std::size_t term, std::size_t column) {
    return "b_" + std::to_string(term) + "_" + std::to_string(column);
}
std::string FieldTheoryEncoding::output_name(std::size_t term, std::size_t slice) {
    return "c_" + std::to_string(term) + "_" + std::to_string(slice);
}

FieldTheoryEncoding encode_field_rank_at_most(const formats::Tensor& tensor,
                                              std::size_t products) {
    // cvc5's `QF_FF` is a decision procedure for *prime* fields, so a composite
    // characteristic would be handed a query it does not model and whatever came
    // back would answer a different question. Both CNF encoders already refuse
    // this; the SMT route only checked for a characteristic of at least two, so
    // `field 4` reached the solver.
    if (!linear_algebra::is_prime(tensor.characteristic)) {
        throw std::invalid_argument("GF(" + std::to_string(tensor.characteristic) +
                                    ") is not a prime field, and the theory of finite fields "
                                    "decides prime fields only");
    }
    if (tensor.slices.empty()) throw std::invalid_argument("a tensor with no slices has rank 0");

    FieldTheoryEncoding encoding;
    encoding.characteristic = static_cast<std::size_t>(tensor.characteristic);
    encoding.products = products;
    encoding.rows = tensor.rows();
    encoding.columns = tensor.columns();
    encoding.slices = tensor.slices.size();
    encoding.problem.characteristic = encoding.characteristic;

    for (std::size_t term = 0; term < products; ++term) {
        for (std::size_t row = 0; row < encoding.rows; ++row) {
            encoding.problem.constants.push_back(FieldTheoryEncoding::left_name(term, row));
        }
        for (std::size_t column = 0; column < encoding.columns; ++column) {
            encoding.problem.constants.push_back(FieldTheoryEncoding::right_name(term, column));
        }
        for (std::size_t slice = 0; slice < encoding.slices; ++slice) {
            encoding.problem.constants.push_back(FieldTheoryEncoding::output_name(term, slice));
        }
    }

    const std::string zero = formats::SmtProblem::literal(0);
    for (std::size_t row = 0; row < encoding.rows; ++row) {
        for (std::size_t column = 0; column < encoding.columns; ++column) {
            for (std::size_t slice = 0; slice < encoding.slices; ++slice) {
                std::vector<std::string> summands;
                summands.reserve(products);
                for (std::size_t term = 0; term < products; ++term) {
                    summands.push_back(folded(
                        "ff.mul",
                        {FieldTheoryEncoding::left_name(term, row),
                         FieldTheoryEncoding::right_name(term, column),
                         FieldTheoryEncoding::output_name(term, slice)},
                        zero));
                }

                const std::size_t wanted =
                    reduced(tensor.slices[slice](row, column), tensor.characteristic);
                encoding.problem.assertions.push_back("(= " + folded("ff.add", summands, zero) +
                                                      " " +
                                                      formats::SmtProblem::literal(wanted) +
                                                      ")");
            }
        }
    }
    return encoding;
}

}  // namespace satisfiability
