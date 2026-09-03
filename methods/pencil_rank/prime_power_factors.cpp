#include "prime_power_factors.h"

#include <map>
#include <stdexcept>
#include <string>

namespace pencil_rank {

namespace {

/// `parts[exponent]` is the product of the distinct irreducibles that occur in
/// the argument with that exponent exactly.
using SquarefreeParts = std::map<std::size_t, Polynomial>;

void record(const ModularField& field, SquarefreeParts& parts, std::size_t exponent,
            const Polynomial& part) {
    auto found = parts.find(exponent);
    if (found == parts.end()) {
        parts.emplace(exponent, part);
        return;
    }
    found->second = multiply(field, found->second, part);
}

Polynomial quotient_of(const ModularField& field, const Polynomial& left,
                       const Polynomial& right) {
    Polynomial quotient;
    Polynomial remainder;
    divide(field, left, right, quotient, remainder);
    return quotient;
}

/// Musser's squarefree decomposition, with the characteristic-p tail.
///
/// `scale` multiplies every exponent this call reports, which is how the tail
/// works: a factor left over when the derivative runs out is a p-th power, and
/// its own decomposition is the same decomposition with every exponent p times
/// larger. The recursion is at most log_p(degree) deep.
void squarefree_parts(const ModularField& field, const Polynomial& polynomial, std::size_t scale,
                      SquarefreeParts& parts) {
    const Polynomial monic = made_monic(field, polynomial);
    if (degree(monic) == 0) return;

    Polynomial repeated = greatest_common_divisor(field, monic, derivative(field, monic));
    Polynomial distinct = quotient_of(field, monic, repeated);

    for (std::size_t exponent = 1; degree(distinct) > 0; ++exponent) {
        const Polynomial shared = greatest_common_divisor(field, distinct, repeated);
        const Polynomial exactly_here = quotient_of(field, distinct, shared);
        if (degree(exactly_here) > 0) record(field, parts, scale * exponent, exactly_here);

        distinct = shared;
        repeated = quotient_of(field, repeated, shared);
    }

    // Whatever the derivative could not see is a p-th power, because that is
    // the only way a factor survives with a zero derivative.
    if (degree(repeated) > 0) {
        const std::size_t characteristic = static_cast<std::size_t>(field.residu());
        squarefree_parts(field, characteristic_root(field, repeated), scale * characteristic,
                         parts);
    }
}

/// How many irreducible factors of each degree a squarefree polynomial has.
///
/// `x^(p^d) - x` is the product of every irreducible whose degree divides `d`,
/// so its gcd with what is left names the degree-`d` factors once the smaller
/// degrees have been divided out. Each gcd is taken against the *remainder*, so
/// the count is `degree / d` and never a fraction.
std::map<std::size_t, std::size_t> counts_by_degree(const ModularField& field,
                                                    const Polynomial& squarefree) {
    std::map<std::size_t, std::size_t> counts;
    const std::uint64_t characteristic = static_cast<std::uint64_t>(field.residu());
    const Polynomial variable = monomial(1);

    Polynomial rest = squarefree;
    Polynomial frobenius = variable;
    for (std::size_t base = 1; 2 * base <= degree(rest); ++base) {
        frobenius = power_modulo(field, frobenius, characteristic, rest);
        const Polynomial found =
            greatest_common_divisor(field, subtract(field, frobenius, variable), rest);
        if (degree(found) == 0) continue;

        counts[base] += degree(found) / base;
        rest = quotient_of(field, rest, found);
        frobenius = remainder_of(field, frobenius, rest);
    }

    // Anything that survived every split below half its degree is irreducible.
    if (degree(rest) > 0) counts[degree(rest)] += 1;
    return counts;
}

}  // namespace

std::vector<PrimePower> prime_power_factors(const ModularField& field,
                                            const Polynomial& polynomial) {
    SquarefreeParts parts;
    squarefree_parts(field, polynomial, 1, parts);

    std::vector<PrimePower> factors;
    for (const auto& [exponent, part] : parts) {
        for (const auto& [base, count] : counts_by_degree(field, part)) {
            for (std::size_t repeat = 0; repeat < count; ++repeat) {
                factors.push_back(PrimePower{base, exponent});
            }
        }
    }

    std::size_t total = 0;
    for (const PrimePower& factor : factors) total += factor.degree();
    if (total != degree(polynomial)) {
        throw std::runtime_error("pencil_rank: prime powers of degree " + std::to_string(total) +
                                 " for a polynomial of degree " +
                                 std::to_string(degree(polynomial)));
    }
    return factors;
}

}  // namespace pencil_rank
