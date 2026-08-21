#include "sumi_bound.h"

#include <string>

#include "matrix_ops.h"
#include "memory_budget.h"
#include "pencil_divisors.h"
#include "solver.h"

namespace pencil_rank {

namespace {

/// `x^p - x`, whose divisors are exactly the polynomials that split into
/// distinct linear factors over GF(p).
///
/// **Linear in the characteristic, which is read off the tensor file and checked
/// only for primality.** `field 2147483647` is a legal header, and this then
/// asked for a 17 GB polynomial with nothing in front of it — the one allocation
/// in this module that a *file* rather than a flag can make unsurvivable. Priced
/// like every other bulk allocation here, so it is refused with the number and
/// `--max-memory` moves it where the machine has the room.
Polynomial split_completely(const ModularField& field) {
    const std::size_t characteristic = static_cast<std::size_t>(field.residu());
    bilinear_rank::require_room("the polynomial x^" + std::to_string(characteristic) + " - x",
                                characteristic + 1, sizeof(int64_t));
    Polynomial result(characteristic + 1, 0);
    result[characteristic] = 1;
    field.neg(result[1], static_cast<int64_t>(1));
    return result;
}

/// `weight[0] * slices[0] + weight[1] * slices[1]`.
ModularMatrix combined(const ModularField& field, const std::vector<ModularMatrix>& slices,
                       int64_t first_weight, int64_t second_weight) {
    ModularMatrix result(slices.front().rows(), slices.front().columns());
    for (std::size_t entry = 0; entry < result.entry_count(); ++entry) {
        field.axpyin(result.data()[entry], first_weight, slices[0].data()[entry]);
        if (slices.size() > 1) {
            field.axpyin(result.data()[entry], second_weight, slices[1].data()[entry]);
        }
    }
    return result;
}

}  // namespace

SumiBound sumi_bound(const ModularField& field, const std::vector<ModularMatrix>& slices) {
    SumiBound report;
    if (slices.empty()) return report;
    const std::size_t order = slices.front().rows();
    if (order != slices.front().columns()) return report;  // not `(E^n; A)` shaped

    // A basis of the slice space with an invertible first member. The projective
    // line has `p + 1` points and any of them may serve, so try each in turn.
    const std::size_t characteristic = static_cast<std::size_t>(field.residu());
    ModularMatrix inverse;
    ModularMatrix chosen;
    ModularMatrix other;
    bool found = false;
    for (std::size_t point = 0; point <= characteristic && !found; ++point) {
        // `(1 : point)` for the finite points, then `(0 : 1)` for infinity.
        const bool at_infinity = point == characteristic;
        const int64_t first_weight = at_infinity ? 0 : 1;
        const int64_t second_weight = at_infinity ? 1 : static_cast<int64_t>(point);

        chosen = combined(field, slices, first_weight, second_weight);
        if (!linear_algebra::invert(field, chosen, inverse)) continue;

        // Any second member independent of the first completes the basis, and a
        // different choice is a different `A` with the same invariant structure.
        other = combined(field, slices, at_infinity ? 1 : 0, at_infinity ? 0 : 1);
        found = true;
    }
    if (!found) return report;

    const std::vector<Polynomial> factors =
        invariant_factors(field, linear_algebra::multiply(field, inverse, other));
    const Polynomial splitting = split_completely(field);

    for (const Polynomial& factor : factors) {
        if (!is_zero(remainder_of(field, splitting, factor))) ++report.failing_factors;
        if (degree(factor) > report.largest_degree) report.largest_degree = degree(factor);
    }

    report.applies = true;
    report.bound = order + report.failing_factors;
    report.exact = characteristic >= report.largest_degree;
    return report;
}

}  // namespace pencil_rank
