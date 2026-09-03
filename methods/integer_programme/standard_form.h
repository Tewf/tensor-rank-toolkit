#pragma once

#include <cstddef>
#include <vector>

#include "integer_programme.h"

/// A programme rewritten as `min cᵀx` subject to `Ax = b`, `x ≥ 0`, `b ≥ 0`.
///
/// The simplex in [`simplex.h`](simplex.h) knows only this shape, which is
/// `[dantzig1951, Ch. XXI]`'s; keys are [`../../references.md`](../../references.md).
/// Getting here costs four rewritings, none of them deep: maximising is
/// minimising the negative; a variable with a lower bound is shifted until that
/// bound is zero; a variable with no lower bound is the difference of two that
/// have one; and an inequality becomes an equality by paying for its own slack.
///
/// Upper bounds become ordinary rows rather than a bounded-variable simplex.
/// That is the wasteful choice and the deliberate one: branch and bound adds
/// bounds at every node, and a bound that is just another row is a bound the
/// solver below cannot get subtly wrong.
namespace integer_programme {

struct StandardForm {
    std::vector<std::vector<Number>> rows;  // A, each of `columns` entries
    std::vector<Number> bound;              // b, every entry non-negative
    std::vector<Number> cost;               // c, always to be minimised
    std::size_t columns = 0;

    /// Where each original variable went: `x = shift + positive`, less
    /// `negative` when it was free below and had to be split.
    struct Origin {
        std::size_t positive = 0;
        std::size_t negative = 0;
        bool split = false;
        Number shift = Number(0);
    };
    std::vector<Origin> origin;
};

StandardForm standard_form_of(const IntegerProgramme& programme);

/// The point in the original variables that a standard-form point stands for.
std::vector<Number> original_point(const StandardForm& form, const std::vector<Number>& values);

}  // namespace integer_programme
