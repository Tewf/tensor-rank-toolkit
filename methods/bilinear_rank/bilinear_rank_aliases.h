#pragma once

#include "linear_algebra.h"

/// The type aliases that name the domain.
///
/// One field, one matrix type, one element type, and one span type, shared by
/// every search under this namespace, so the representation is chosen once
/// rather than once per search and a change to it happens in one place.
namespace bilinear_rank {

/// One field for the whole strand: the primes in play are tiny.
using Field = linear_algebra::ModularField;
using Matrix = linear_algebra::ModularMatrix;
using Element = Field::Element;

/// A span of slices, spelled once. Written out in full it is
/// `linear_algebra::SpanBasis<linear_algebra::ModularField>`, which appeared
/// five times in one function and said nothing the alias does not.
using ReducedBasis = linear_algebra::SpanBasis<Field>;

}  // namespace bilinear_rank
