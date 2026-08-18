#include "multivariate_polynomial.h"

#include <algorithm>

namespace matrix_sparsification {

Polynomial Polynomial::constant(const Field& field, const Element& value) {
    Polynomial result;
    result.add_term(field, Monomial(), value);
    return result;
}

Polynomial Polynomial::variable(const Field& field, std::size_t index) {
    Polynomial result;
    result.add_term(field, Monomial{index}, field.one);
    return result;
}

std::size_t Polynomial::degree() const {
    std::size_t highest = 0;
    for (const auto& term : terms_) highest = std::max(highest, term.first.size());
    return highest;
}

/// Accumulate one term, and erase it again if it cancelled.
///
/// The erase is the point. Without it a polynomial that is mathematically zero
/// keeps a term with a zero coefficient, `is_zero` answers false, and a pattern
/// that cannot be realised is reported as feasible.
void Polynomial::add_term(const Field& field, const Monomial& monomial,
                          const Element& coefficient) {
    if (field.isZero(coefficient)) return;

    Monomial key = monomial;
    std::sort(key.begin(), key.end());

    auto position = terms_.find(key);
    if (position == terms_.end()) {
        terms_.emplace(std::move(key), coefficient);
        return;
    }
    field.addin(position->second, coefficient);
    if (field.isZero(position->second)) terms_.erase(position);
}

void Polynomial::add_in(const Field& field, const Polynomial& other) {
    for (const auto& term : other.terms_) add_term(field, term.first, term.second);
}

void Polynomial::subtract_in(const Field& field, const Polynomial& other) {
    for (const auto& term : other.terms_) {
        Element negated;
        field.neg(negated, term.second);
        add_term(field, term.first, negated);
    }
}

Polynomial Polynomial::multiplied_by(const Field& field, const Polynomial& other) const {
    Polynomial result;
    for (const auto& mine : terms_) {
        for (const auto& theirs : other.terms_) {
            Monomial product = mine.first;
            product.insert(product.end(), theirs.first.begin(), theirs.first.end());

            Element coefficient;
            field.mul(coefficient, mine.second, theirs.second);
            result.add_term(field, product, coefficient);
        }
    }
    return result;
}

namespace {

/// The matrix with `skip_row` and `skip_column` removed.
PolynomialMatrix minor_of(const PolynomialMatrix& matrix, std::size_t skip_row,
                          std::size_t skip_column) {
    PolynomialMatrix minor;
    minor.reserve(matrix.size() - 1);
    for (std::size_t row = 0; row < matrix.size(); ++row) {
        if (row == skip_row) continue;
        std::vector<Polynomial> kept;
        kept.reserve(matrix.size() - 1);
        for (std::size_t column = 0; column < matrix.size(); ++column) {
            if (column == skip_column) continue;
            kept.push_back(matrix[row][column]);
        }
        minor.push_back(std::move(kept));
    }
    return minor;
}

}  // namespace

Polynomial determinant(const Field& field, const PolynomialMatrix& matrix) {
    if (matrix.empty()) return Polynomial::constant(field, field.one);
    if (matrix.size() == 1) return matrix[0][0];

    Polynomial total;
    for (std::size_t column = 0; column < matrix.size(); ++column) {
        // Skip the whole minor when the entry is zero: on a pattern matrix most
        // entries are, and this is what keeps the factorial from being paid.
        if (matrix[0][column].is_zero()) continue;

        const Polynomial cofactor =
            matrix[0][column].multiplied_by(field, determinant(field, minor_of(matrix, 0, column)));
        if (column % 2 == 0) {
            total.add_in(field, cofactor);
        } else {
            total.subtract_in(field, cofactor);
        }
    }
    return total;
}

}  // namespace matrix_sparsification
