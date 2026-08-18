#pragma once

#include "integer_programme.h"

/// Rounding an exact rational to a whole one.
///
/// Three places need this and each would get it subtly wrong on its own:
/// branching picks the two integers around a fractional value, the MPS writer
/// moves a bound inward onto one, and a solver's decimal has to be read back
/// onto the integer it was a rendering of. Givaro divides towards zero, so the
/// naive version is wrong for every negative value and right for every test
/// anyone writes first.
namespace optimisation {

inline bool is_whole(const Number& value) { return value.deno() == 1; }

inline Givaro::Integer floor_of(const Number& value) {
    Givaro::Integer quotient = value.nume() / value.deno();
    if (value.nume() % value.deno() != 0 && value.nume() < 0) quotient -= 1;
    return quotient;
}

inline Givaro::Integer ceiling_of(const Number& value) {
    Givaro::Integer quotient = value.nume() / value.deno();
    if (value.nume() % value.deno() != 0 && value.nume() > 0) quotient += 1;
    return quotient;
}

inline Givaro::Integer nearest_whole(const Number& value) {
    return floor_of(value + Number(1, 2));
}

}  // namespace optimisation
