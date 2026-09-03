#pragma once

#include <cstddef>
#include <vector>

#include "polynomial.h"

namespace pencil_rank {

/// One prime power in the factorisation of a polynomial over GF(p), described
/// by its shape rather than by its coefficients.
///
/// The irreducible itself is never formed. An elementary divisor enters the
/// rank formula through two numbers only, its degree and whether that degree is
/// 1, so naming the polynomial would be work done for a caller that does not
/// exist. It also keeps this deterministic: separating two irreducibles of the
/// same degree is exactly the step that needs a random split, and counting them
/// does not.
struct PrimePower {
    /// The degree of the irreducible.
    std::size_t base_degree = 0;

    /// The exponent it is raised to.
    std::size_t multiplicity = 0;

    std::size_t degree() const { return base_degree * multiplicity; }

    /// Whether this is one of the linear elementary divisors, the ones that do
    /// not cost the pencil an extra product.
    bool is_linear() const { return degree() == 1; }
};

/// The prime powers whose product is `polynomial`, by degree and exponent.
///
/// A squarefree decomposition separates the exponents and a distinct-degree
/// split counts the irreducibles of each degree within each of them. Both are
/// deterministic, and together they answer the only question asked here.
/// Equal-degree splitting, the randomised step, is what would be needed to say
/// *which* irreducibles they are, and nothing needs to know.
///
/// Throws if the degrees do not sum to the degree of the argument, which is the
/// decomposition checking itself: every route through it is an identity that
/// either holds or has a defect behind it.
std::vector<PrimePower> prime_power_factors(const ModularField& field,
                                            const Polynomial& polynomial);

}  // namespace pencil_rank
