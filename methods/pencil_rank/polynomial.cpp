#include "polynomial.h"

#include <stdexcept>

namespace pencil_rank {

namespace {

/// Drop the trailing zeros, so that a degree is never a lie.
void trim(const ModularField& field, Polynomial& polynomial) {
    while (!polynomial.empty() && field.isZero(polynomial.back())) polynomial.pop_back();
}

}  // namespace

bool is_zero(const Polynomial& polynomial) { return polynomial.empty(); }

std::size_t degree(const Polynomial& polynomial) {
    return polynomial.empty() ? 0 : polynomial.size() - 1;
}

Polynomial monomial(std::size_t exponent) {
    Polynomial result(exponent + 1, 0);
    result.back() = 1;
    return result;
}

Polynomial one() { return Polynomial{1}; }

Polynomial add(const ModularField& field, const Polynomial& left, const Polynomial& right) {
    Polynomial result(left.size() > right.size() ? left.size() : right.size(), 0);
    for (std::size_t index = 0; index < left.size(); ++index) result[index] = left[index];
    for (std::size_t index = 0; index < right.size(); ++index) {
        field.addin(result[index], right[index]);
    }
    trim(field, result);
    return result;
}

Polynomial subtract(const ModularField& field, const Polynomial& left, const Polynomial& right) {
    Polynomial result(left.size() > right.size() ? left.size() : right.size(), 0);
    for (std::size_t index = 0; index < left.size(); ++index) result[index] = left[index];
    for (std::size_t index = 0; index < right.size(); ++index) {
        field.subin(result[index], right[index]);
    }
    trim(field, result);
    return result;
}

Polynomial multiply(const ModularField& field, const Polynomial& left, const Polynomial& right) {
    if (is_zero(left) || is_zero(right)) return Polynomial();
    Polynomial result(left.size() + right.size() - 1, 0);
    for (std::size_t i = 0; i < left.size(); ++i) {
        if (field.isZero(left[i])) continue;
        for (std::size_t j = 0; j < right.size(); ++j) {
            field.axpyin(result[i + j], left[i], right[j]);
        }
    }
    trim(field, result);
    return result;
}

void divide(const ModularField& field, const Polynomial& left, const Polynomial& right,
            Polynomial& quotient, Polynomial& remainder) {
    if (is_zero(right)) throw std::runtime_error("pencil_rank: division by the zero polynomial");

    remainder = left;
    quotient.clear();
    if (is_zero(left) || left.size() < right.size()) return;

    int64_t leading_inverse = 0;
    field.inv(leading_inverse, right.back());

    quotient.assign(left.size() - right.size() + 1, 0);
    for (std::size_t step = quotient.size(); step-- > 0;) {
        if (field.isZero(remainder[step + right.size() - 1])) continue;

        int64_t factor = 0;
        field.mul(factor, remainder[step + right.size() - 1], leading_inverse);
        quotient[step] = factor;

        int64_t negated = 0;
        field.neg(negated, factor);
        for (std::size_t index = 0; index < right.size(); ++index) {
            field.axpyin(remainder[step + index], negated, right[index]);
        }
    }
    trim(field, quotient);
    trim(field, remainder);
}

Polynomial remainder_of(const ModularField& field, const Polynomial& left,
                        const Polynomial& right) {
    Polynomial quotient;
    Polynomial remainder;
    divide(field, left, right, quotient, remainder);
    return remainder;
}

Polynomial greatest_common_divisor(const ModularField& field, const Polynomial& left,
                                   const Polynomial& right) {
    Polynomial current = left;
    Polynomial next = right;
    while (!is_zero(next)) {
        Polynomial rest = remainder_of(field, current, next);
        current = next;
        next = rest;
    }
    return made_monic(field, current);
}

Polynomial made_monic(const ModularField& field, const Polynomial& polynomial) {
    if (is_zero(polynomial)) return polynomial;
    int64_t leading_inverse = 0;
    field.inv(leading_inverse, polynomial.back());

    Polynomial result = polynomial;
    for (int64_t& coefficient : result) field.mulin(coefficient, leading_inverse);
    return result;
}

Polynomial derivative(const ModularField& field, const Polynomial& polynomial) {
    if (polynomial.size() < 2) return Polynomial();
    Polynomial result(polynomial.size() - 1, 0);
    for (std::size_t index = 1; index < polynomial.size(); ++index) {
        int64_t count = 0;
        field.init(count, static_cast<int64_t>(index));
        field.mul(result[index - 1], polynomial[index], count);
    }
    trim(field, result);
    return result;
}

Polynomial power_modulo(const ModularField& field, const Polynomial& base, std::uint64_t exponent,
                        const Polynomial& modulus) {
    Polynomial result = remainder_of(field, one(), modulus);
    Polynomial square = remainder_of(field, base, modulus);
    for (std::uint64_t rest = exponent; rest > 0; rest >>= 1) {
        if (rest & 1U) result = remainder_of(field, multiply(field, result, square), modulus);
        square = remainder_of(field, multiply(field, square, square), modulus);
    }
    return result;
}

Polynomial characteristic_root(const ModularField& field, const Polynomial& polynomial) {
    if (is_zero(polynomial)) return polynomial;
    const std::size_t characteristic = static_cast<std::size_t>(field.residu());

    Polynomial result(degree(polynomial) / characteristic + 1, 0);
    for (std::size_t index = 0; index < polynomial.size(); ++index) {
        if (field.isZero(polynomial[index])) continue;
        if (index % characteristic != 0) {
            throw std::runtime_error("pencil_rank: asked for the p-th root of a polynomial that "
                                     "is not a p-th power");
        }
        result[index / characteristic] = polynomial[index];
    }
    trim(field, result);
    return result;
}

}  // namespace pencil_rank
