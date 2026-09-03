#include "level_lowering_moves.h"

#include <limits>
#include <set>

#include "candidate_pool.h"
#include "matrix.h"
#include "memory_budget.h"
#include "row_space_coordinates.h"
#include "span_basis.h"
#include "span_enumeration.h"

namespace bilinear_rank {

namespace {

/// Every vector of `GF(p)^length`, counting with the first coordinate fastest,
/// which is the order [`coefficient_vector`](../greedy_heuristic/span_enumeration.h)
/// already fixes for spans. Used for `b`, which is not normalised: `bᵀa = 1`
/// pins the scalar that `a`'s normalisation leaves free.
///
/// **`p^length` is priced, because `--summand-rank` reaches it.** `length` is the
/// rank of the element being split, which `level_lowering_moves` bounds by the
/// cutoff that flag sets, and the flag takes any count. At `--summand-rank 34`
/// over GF(2) this asked for 17 billion vectors and the run was killed; now it is
/// refused with the number. The count is accumulated with its overflow checked,
/// since a wrapped `p^length` reserves a plausible figure and then fills it to
/// the real end.
std::vector<std::vector<int64_t>> every_vector(const Field& field, std::size_t length) {
    const auto characteristic = static_cast<std::size_t>(field.characteristic());
    const std::size_t how_many = std::numeric_limits<std::size_t>::max();
    std::size_t count = 1;
    for (std::size_t position = 0; position < length; ++position) {
        if (count > how_many / characteristic) {
            run_limits::require_room("the vectors of GF(" + std::to_string(characteristic) + ")^" +
                             std::to_string(length) + " a level-lowering summand needs",
                         how_many, sizeof(int64_t) * length);
        }
        count *= characteristic;
    }
    run_limits::require_room("the vectors of GF(" + std::to_string(characteristic) + ")^" +
                     std::to_string(length) + " a level-lowering summand needs",
                 count, sizeof(std::vector<int64_t>) + sizeof(int64_t) * length);

    std::vector<std::vector<int64_t>> vectors;
    vectors.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
        vectors.push_back(coefficient_vector(index, length, field.characteristic()));
    }
    return vectors;
}

/// `bᵀa`, over the `r` coordinates the factorisation runs in.
Element inner_product(const Field& field, const std::vector<int64_t>& left,
                      const std::vector<int64_t>& right) {
    Element total;
    field.assign(total, field.zero);
    for (std::size_t position = 0; position < left.size(); ++position) {
        field.axpyin(total, left[position], right[position]);
    }
    return total;
}

}  // namespace

std::vector<Matrix> level_lowering_summands(const Field& field, const Matrix& matrix) {
    // `matrix == coordinates * (its chosen rows)`, so `coordinates` is the `C` of
    // the header and the chosen rows are its `R`. Both have full rank `r` by
    // construction, which is what makes `rank(C X R) == rank(X)` and the whole
    // argument work.
    std::vector<std::size_t> chosen;
    const Matrix coordinates = linear_algebra::coordinates_over_independent_rows(field, matrix,
                                                                                 chosen);
    const std::size_t order = chosen.size();
    if (order == 0) return {};

    std::vector<Matrix> summands;
    // `a` one per scalar class, `b` over the whole space: the pair `(λa, λ⁻¹b)`
    // builds the same map, and normalising `a` picks one member of each class.
    for (const std::vector<int64_t>& a : normalised_vectors(field, order)) {
        // `C a`, the left factor of every summand this `a` gives.
        std::vector<Element> left(matrix.rows());
        for (std::size_t row = 0; row < matrix.rows(); ++row) {
            field.assign(left[row], field.zero);
            for (std::size_t position = 0; position < order; ++position) {
                field.axpyin(left[row], coordinates(row, position), a[position]);
            }
        }
        for (const std::vector<int64_t>& b : every_vector(field, order)) {
            if (!field.isOne(inner_product(field, b, a))) continue;

            // `bᵀ R`, then the outer product with `C a`.
            std::vector<Element> right(matrix.columns());
            for (std::size_t column = 0; column < matrix.columns(); ++column) {
                field.assign(right[column], field.zero);
                for (std::size_t position = 0; position < order; ++position) {
                    field.axpyin(right[column], b[position], matrix(chosen[position], column));
                }
            }
            Matrix summand(matrix.rows(), matrix.columns());
            for (std::size_t row = 0; row < matrix.rows(); ++row) {
                for (std::size_t column = 0; column < matrix.columns(); ++column) {
                    field.mul(summand(row, column), left[row], right[column]);
                }
            }
            summands.push_back(std::move(summand));
        }
    }
    return summands;
}

std::vector<Matrix> level_lowering_moves(const Field& field, const std::vector<Matrix>& slices,
                                         const std::vector<std::size_t>& element_ranks,
                                         std::size_t cutoff) {
    if (slices.empty()) return {};
    const std::size_t width = slices.front().entry_count();

    // Elements of the span already spanned are no move at all, and the same map
    // reached from two elements is one move: both are filtered here rather than
    // by the caller, so the count it reports is the count it costs.
    const linear_algebra::SpanBasis<Field> span = linear_algebra::span_of(field, slices);
    std::set<std::vector<Element>> already;
    std::vector<Element> scratch;

    std::vector<Matrix> moves;
    std::vector<int64_t> coefficients;
    Matrix element(slices.front().rows(), slices.front().columns());
    for (std::size_t index = 0; index < element_ranks.size(); ++index) {
        if (element_ranks[index] < 2 || element_ranks[index] > cutoff) continue;
        coefficient_vector_into(index, slices.size(), field.characteristic(), coefficients);
        linear_combination_into(field, slices, coefficients, element);
        for (Matrix& summand : level_lowering_summands(field, element)) {
            if (span.contains(summand, scratch)) continue;
            std::vector<Element> flat(summand.data(), summand.data() + width);
            if (!already.insert(std::move(flat)).second) continue;
            moves.push_back(std::move(summand));
        }
    }
    return moves;
}

}  // namespace bilinear_rank
