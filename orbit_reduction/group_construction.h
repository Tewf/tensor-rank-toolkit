#pragma once

#include <cstddef>
#include <vector>

#include "automorphism.h"
#include "bilinear_rank_aliases.h"

/// Where the automorphism groups come from, for the maps this repository
/// builds.
///
/// The same division as [`map_construction.h`](../map_construction/map_construction.h), which builds
/// the maps: this builds the groups worth quotienting them by. Two sources.
/// Brute force enumerates every invertible pair and works only on small shapes;
/// the closed form for matrix multiplication works at any size but has to be
/// derived correctly.
///
/// **They cannot be compared directly on any fixture that exists**, which is
/// worth saying because this file used to claim they check each other. The
/// smallest matrix multiplication shape has 4x4 slices, and that is already past
/// what `all_automorphisms` will build. What is checked instead, and what does
/// pin the closed form, is the third route against the second: closing
/// `matrix_multiplication_symmetry_generators` must give
/// `matrix_multiplication_symmetries` exactly, element for element, and
/// [`../oracle_guided_search/tests/test_canonical_augmentation.cpp`](../oracle_guided_search/tests/test_canonical_augmentation.cpp)
/// asserts that on `⟨2,2,2⟩`, 216 both ways.
namespace bilinear_rank {

/// Every invertible `order x order` matrix over the field.
///
/// `|GL_2(F_2)| = 6`, `|GL_3(F_2)| = 168`, `|GL_4(F_2)| = 20 160`. Enumerated by
/// walking all `p^(order²)` matrices, so it is refused above a shape the machine
/// can walk.
std::vector<Matrix> all_invertible(const Field& field, std::size_t order);

/// Every RP-automorphism of the given shape: all pairs of invertibles.
///
/// Tractable only when both general linear groups are small, which for the
/// shapes here means the polynomial and convolution fixtures rather than matrix
/// multiplication.
std::vector<Automorphism> all_automorphisms(const Field& field, std::size_t rows,
                                            std::size_t columns);

/// A generating set of `GL_order(F_p)`, rather than all of it.
///
/// A transvection and a cycle generate `GL_n(F_2)`; over larger fields a scaling
/// joins them. This is what makes the big groups usable at all: `|GL_3(F_2)|³`
/// is 4.7 million elements and will not be held in a list, but nine generators
/// will, and orbits only ever need generators.
std::vector<Matrix> general_linear_generators(const Field& field, std::size_t order);

/// The same symmetries as below, from generators, so the group is never
/// enumerated. Use this for `⟨3,3,3⟩` and up.
std::vector<Automorphism> matrix_multiplication_symmetry_generators(const Field& field,
                                                                    std::size_t rows,
                                                                    std::size_t inner,
                                                                    std::size_t columns);

/// The symmetries of the matrix multiplication tensor `⟨n, m, k⟩`.
///
/// `A ↦ X A Y⁻¹` and `B ↦ Y B Z⁻¹` give `A·B ↦ X (A·B) Z⁻¹`, so the product is
/// the same problem written in different coordinates, and the span of the slices
/// is preserved because the output is remixed invertibly. On the flattened
/// operands that is the pair of Kronecker products `(X ⊗ (Y⁻¹)ᵀ, Y ⊗ (Z⁻¹)ᵀ)`.
///
/// This is the group that is enormous where it matters: 216 elements at
/// `⟨2,2,2⟩` over GF(2), and `|GL_3(F_2)|³ = 4 741 632` at `⟨3,3,3⟩`, which is
/// past what a list can hold and is refused.
std::vector<Automorphism> matrix_multiplication_symmetries(const Field& field, std::size_t rows,
                                                           std::size_t inner,
                                                           std::size_t columns);

}  // namespace bilinear_rank
