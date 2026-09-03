#pragma once

#include <cstddef>
#include <vector>

#include "automorphism.h"
#include "bilinear_rank_aliases.h"

/// Where the automorphism groups come from, for the maps this repository
/// builds.
///
/// The same division as [`map_construction.h`](../map_construction/map_construction.h), which builds
/// the maps: this builds the groups worth quotienting them by. Keys are
/// [`../references.md`](../references.md). Two sources.
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
std::vector<Matrix> general_linear_group(const Field& field, std::size_t order);

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

/// Refuse a `⟨n,m,k⟩` that is not the shape of these slices.
///
/// A matrix multiplication tensor `⟨n,m,k⟩` has `n·k` slices of `(n·m) × (m·k)`,
/// which is the identity `inferred_matmul_shape` inverts. Handing a group built
/// for one shape to a tensor of another is not a weak quotient, it is undefined:
/// `act_on` multiplies matrices whose sizes do not meet, and
/// `matrix_ops.h::multiply` checks no shapes, so the write runs off the end of
/// the result. Measured on `decide-rank-by-deflation --refuter tree`, that is
/// `free(): invalid pointer` and a core dump, and on `minimise-rank` it is worse
/// than a crash: it exits 0 having quotiented by nothing, printing
/// "stabiliser 0, 961 orbits of 961".
///
/// So this is the guard both routes to a matmul group must pass, and it throws
/// rather than returning a flag, because every caller's honest response is to
/// stop. `orbit_cubes.cpp` already refuses the same mistake in the same words for
/// the cube route; this is that refusal made available to the rest.
void require_matmul_shape(const std::vector<Matrix>& slices, std::size_t rows,
                          std::size_t inner, std::size_t columns);

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
/// **This is the whole stabiliser and not a subgroup of it.**
/// `[covanov2019, Thm. 17]` shows the setwise stabiliser of the matrix
/// multiplication subspace is exactly the pairs `(P ⊗ Rᵀ, Q ⊗ R⁻¹)`, which is
/// what is built here with `R = Y⁻¹` and `Q = (Z⁻¹)ᵀ`. Nothing is being left on
/// the table by using a closed form instead of a search.
///
/// This is the group that is enormous where it matters: 216 elements at
/// `⟨2,2,2⟩` over GF(2), and `|GL_3(F_2)|³ = 4 741 632` at `⟨3,3,3⟩`, which is
/// past what a list can hold and is refused.
std::vector<Automorphism> matrix_multiplication_symmetries(const Field& field, std::size_t rows,
                                                           std::size_t inner,
                                                           std::size_t columns);

}  // namespace bilinear_rank
