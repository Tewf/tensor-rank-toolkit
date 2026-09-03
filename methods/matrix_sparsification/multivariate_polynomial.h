#pragma once

#include <cstddef>
#include <map>
#include <vector>

#include "types.h"

/// Polynomials in the unknown entries of a target pattern.
///
/// One question needs them and it is [`pattern_feasibility.h`](pattern_feasibility.h):
/// whether a change of basis realising a wanted pattern of zeros exists at all.
/// That is the non-vanishing of a determinant whose entries are unknowns, and a
/// determinant is decided by expanding it, not by evaluating it somewhere and
/// hoping. Evaluating at a point can only ever prove non-vanishing; a zero
/// there proves nothing, which is the wrong way round for a feasibility test
/// that has to be able to answer "no".
///
/// Deliberately the smallest thing that answers it: exact rational
/// coefficients, sparse monomials, add and multiply. No division, no ordering,
/// no Groebner anything.
namespace matrix_sparsification {

/// A monomial as the sorted list of the variables in it, with repeats.
///
/// `x2 * x0 * x2` is `{0, 2, 2}`. Sorted so that equal monomials compare equal,
/// which is what makes the map below collect like terms.
using Monomial = std::vector<std::size_t>;

/// A sparse multivariate polynomial over the rationals.
///
/// Zero coefficients are erased rather than stored, so `is_zero` is `empty` and
/// the identically-zero polynomial has exactly one representation. That is the
/// whole reason this type exists: `is_zero` has to be a decision, not a
/// comparison against a tolerance.
class Polynomial {
public:
    Polynomial() = default;

    /// The constant `value`.
    static Polynomial constant(const Field& field, const Element& value);

    /// The single variable `index`.
    static Polynomial variable(const Field& field, std::size_t index);

    bool is_zero() const { return terms_.empty(); }
    std::size_t term_count() const { return terms_.size(); }
    const std::map<Monomial, Element>& terms() const { return terms_; }

    /// The highest total degree present, and zero for the zero polynomial.
    std::size_t degree() const;

    void add_in(const Field& field, const Polynomial& other);
    void subtract_in(const Field& field, const Polynomial& other);

    Polynomial multiplied_by(const Field& field, const Polynomial& other) const;

private:
    void add_term(const Field& field, const Monomial& monomial, const Element& coefficient);

    std::map<Monomial, Element> terms_;
};

/// A square matrix of polynomials, which is what a change of basis becomes once
/// the pattern it has to realise is left as unknowns.
using PolynomialMatrix = std::vector<std::vector<Polynomial>>;

/// The determinant, expanded exactly.
///
/// Cofactor expansion, so the cost is `Theta(order!)` polynomial
/// multiplications and the caller is expected to have refused a large order
/// first; [`pattern_feasibility.h`](pattern_feasibility.h) is where that
/// refusal lives, because it is the one that bounds what the order may be.
Polynomial determinant(const Field& field, const PolynomialMatrix& matrix);

}  // namespace matrix_sparsification
