#include "pencil_divisors.h"

#include <algorithm>
#include <utility>

namespace pencil_rank {

namespace {

using PolynomialMatrix = std::vector<std::vector<Polynomial>>;

Polynomial quotient_of(const ModularField& field, const Polynomial& left,
                       const Polynomial& right) {
    Polynomial quotient;
    Polynomial remainder;
    divide(field, left, right, quotient, remainder);
    return quotient;
}

/// The pencil `constant + x linear` as a matrix over `GF(p)[x]`.
PolynomialMatrix pencil_over_polynomials(const ModularField& field, const ModularMatrix& constant,
                                         const ModularMatrix& linear) {
    PolynomialMatrix entries(constant.rows(), std::vector<Polynomial>(constant.columns()));
    for (std::size_t row = 0; row < constant.rows(); ++row) {
        for (std::size_t column = 0; column < constant.columns(); ++column) {
            if (!field.isZero(linear(row, column))) {
                entries[row][column] = Polynomial{constant(row, column), linear(row, column)};
            } else if (!field.isZero(constant(row, column))) {
                entries[row][column] = Polynomial{constant(row, column)};
            }
        }
    }
    return entries;
}

/// Diagonalise over `GF(p)[x]`, returning the nonzero diagonal entries, monic.
///
/// The usual Smith form also normalises the diagonal into a divisibility chain,
/// which is what `[gantmacher1959, Vol. 1, Ch. VI §3, Thm. 3]` asserts exists.
/// That step is skipped: it costs another sweep and the only thing read off the
/// result is the multiset of prime powers, which the chain does not change.
///
/// The pivot is always an entry of least degree in the remaining block, so each
/// pass either clears its row and column or replaces the pivot with a strictly
/// smaller remainder. Degrees do not descend for ever, which is the whole of
/// the termination argument.
std::vector<Polynomial> diagonal_form(const ModularField& field, PolynomialMatrix entries) {
    const std::size_t rows = entries.size();
    const std::size_t columns = rows == 0 ? 0 : entries.front().size();
    std::vector<Polynomial> diagonal;

    for (std::size_t step = 0; step < std::min(rows, columns); ++step) {
        while (true) {
            std::size_t pivot_row = rows;
            std::size_t pivot_column = columns;
            std::size_t least_degree = 0;
            for (std::size_t row = step; row < rows; ++row) {
                for (std::size_t column = step; column < columns; ++column) {
                    if (is_zero(entries[row][column])) continue;
                    const std::size_t here = degree(entries[row][column]);
                    if (pivot_row == rows || here < least_degree) {
                        pivot_row = row;
                        pivot_column = column;
                        least_degree = here;
                    }
                }
            }
            // Everything left is zero, so every remaining diagonal entry is too.
            if (pivot_row == rows) return diagonal;

            std::swap(entries[step], entries[pivot_row]);
            for (std::vector<Polynomial>& row : entries) std::swap(row[step], row[pivot_column]);

            bool settled = true;
            for (std::size_t row = step + 1; row < rows; ++row) {
                if (is_zero(entries[row][step])) continue;
                const Polynomial factor = quotient_of(field, entries[row][step],
                                                      entries[step][step]);
                for (std::size_t column = step; column < columns; ++column) {
                    entries[row][column] =
                        subtract(field, entries[row][column],
                                 multiply(field, factor, entries[step][column]));
                }
                if (!is_zero(entries[row][step])) settled = false;
            }
            for (std::size_t column = step + 1; column < columns; ++column) {
                if (is_zero(entries[step][column])) continue;
                const Polynomial factor = quotient_of(field, entries[step][column],
                                                      entries[step][step]);
                for (std::size_t row = step; row < rows; ++row) {
                    entries[row][column] = subtract(field, entries[row][column],
                                                    multiply(field, factor, entries[row][step]));
                }
                if (!is_zero(entries[step][column])) settled = false;
            }
            if (settled) break;
        }
        diagonal.push_back(made_monic(field, entries[step][step]));
    }
    return diagonal;
}

/// Where the coefficients start being nonzero, which is the multiplicity of `x`.
std::size_t multiplicity_of_x(const ModularField& field, const Polynomial& polynomial) {
    for (std::size_t index = 0; index < polynomial.size(); ++index) {
        if (!field.isZero(polynomial[index])) return index;
    }
    return 0;
}

/// Turn a bare diagonalisation into a divisibility chain.
///
/// Repeatedly replace a pair that does not divide by `(gcd, lcm)`, which keeps
/// the product and the module and forces the chain. The pass is run until it
/// settles; each swap strictly lowers the degree of the earlier entry, so it
/// does settle.
void into_a_chain(const ModularField& field, std::vector<Polynomial>& diagonal) {
    bool moved = true;
    while (moved) {
        moved = false;
        for (std::size_t index = 0; index + 1 < diagonal.size(); ++index) {
            const Polynomial& earlier = diagonal[index];
            const Polynomial& later = diagonal[index + 1];
            if (is_zero(earlier) || is_zero(later)) continue;
            if (is_zero(remainder_of(field, later, earlier))) continue;  // already divides

            const Polynomial common = greatest_common_divisor(field, earlier, later);
            const Polynomial product = multiply(field, earlier, later);
            diagonal[index + 1] = quotient_of(field, product, common);
            diagonal[index] = common;
            moved = true;
        }
    }
}

}  // namespace

std::vector<Polynomial> invariant_factors(const ModularField& field,
                                          const ModularMatrix& square) {
    // `x I - square`, which is the characteristic matrix Ja'Ja' takes invariant
    // polynomials of, written in this file's `constant + x linear` convention.
    ModularMatrix negated(square.rows(), square.columns());
    for (std::size_t entry = 0; entry < negated.entry_count(); ++entry) {
        field.neg(negated.data()[entry], square.data()[entry]);
    }
    ModularMatrix identity(square.rows(), square.columns());
    for (std::size_t index = 0; index < square.rows(); ++index) identity(index, index) = 1;

    std::vector<Polynomial> diagonal =
        diagonal_form(field, pencil_over_polynomials(field, negated, identity));
    into_a_chain(field, diagonal);

    std::vector<Polynomial> factors;
    for (const Polynomial& entry : diagonal) {
        if (!is_zero(entry) && degree(entry) > 0) factors.push_back(entry);
    }
    return factors;
}

PencilDivisors elementary_divisors(const ModularField& field, const ModularMatrix& first,
                                   const ModularMatrix& second) {
    PencilDivisors report;

    for (const Polynomial& entry :
         diagonal_form(field, pencil_over_polynomials(field, first, second))) {
        if (is_zero(entry)) continue;
        ++report.generic_rank;
        for (const PrimePower& factor : prime_power_factors(field, entry)) {
            report.divisors.push_back(ElementaryDivisor{factor, false});
        }
    }

    for (const Polynomial& entry :
         diagonal_form(field, pencil_over_polynomials(field, second, first))) {
        if (is_zero(entry)) continue;
        const std::size_t exponent = multiplicity_of_x(field, entry);
        if (exponent > 0) {
            report.divisors.push_back(ElementaryDivisor{PrimePower{1, exponent}, true});
        }
    }

    return report;
}

}  // namespace pencil_rank
