#pragma once

#include <cstdint>
#include <vector>

#include "bilinear_rank_aliases.h"

/// The rank-preserving automorphism group a search may quotient by.
///
/// `[covanov2019]` §3, itself an unpublished improvement by Barbulescu and
/// Zimmermann to the framework [the exact
/// search](../exhaustive_search/exhaustive_search.h) implements; keys are
/// [`../references.md`](../references.md). In one line: an invertible change of
/// coordinates on each of the two operands preserves rank
/// (`[covanov2019, Prop. 9]`, for a single form and for a subspace), so if it
/// also maps the target subspace to itself then it maps solutions to solutions,
/// and the search need visit only one per orbit.
///
/// **The group is the setwise stabiliser of the target subspace, not of the
/// map**, `[covanov2019, Def. 13]`. That is the larger group, because remixing
/// the slices among themselves leaves their span untouched, and the size of the
/// group is exactly what the reduction is worth.
///
/// Where the groups come from is [`group_construction.h`](group_construction.h).
namespace bilinear_rank {

/// `[covanov2019, Def. 7]`: the pair `(μ, ν)` acting on a bilinear form by
/// `(a, b) ↦ Φ(μa, νb)`.
///
/// On the matrix of that form it is `M ↦ μᵀ M ν`, since
/// `(μa)ᵀ M (νb) = aᵀ (μᵀ M ν) b`.
struct Automorphism {
    Matrix left;   // μ, on the left operand: rows x rows
    Matrix right;  // ν, on the right operand: columns x columns
};

/// `μᵀ M ν`.
Matrix act_on(const Field& field, const Automorphism& sigma, const Matrix& form);

/// The pair that does nothing.
Automorphism identity_automorphism(const Field& field, std::size_t rows, std::size_t columns);

/// Doing `first`, then `second`. Componentwise, because
/// `ν'ᵀ(μᵀ M ν)ν' = (μμ')ᵀ M (νν')`.
Automorphism compose(const Field& field, const Automorphism& first, const Automorphism& second);

/// The automorphism that undoes this one. It exists for every element:
/// `[covanov2019, Prop. 8]` is that this really is a group action and that all
/// of its elements are invertible.
Automorphism inverse(const Field& field, const Automorphism& sigma);

/// Everything reachable from `generators` by composition.
///
/// Breadth first, deduplicated by the entries themselves. Refused past the
/// memory budget rather than attempted: a group is perfectly capable of being
/// larger than the machine, and `(GL_3)³` at 14 million is already too large to
/// hold this way.
std::vector<Automorphism> group_closure(const Field& field,
                                        const std::vector<Automorphism>& generators);

/// The elements of `group` that map `span(slices)` into itself: the setwise
/// stabiliser `Stab(T)` of `[covanov2019, Def. 13]`.
std::vector<Automorphism> stabiliser_of(const Field& field, const std::vector<Matrix>& slices,
                                        const std::vector<Automorphism>& group);

/// How the group moves a pool around, as one permutation of indices per
/// element.
///
/// This is what makes the reduction pay rather than cost. Taking orbits at a
/// search node by multiplying matrices is two matrix products per element per
/// group element, which for `⟨2,2,2⟩` is three hundred times what scanning the
/// pool costs, so the pruning would be swallowed by the price of computing it.
/// Done once in advance, a node takes orbits by following indices.
///
/// The pool must be closed under the group, which `all_rank_one_maps` is and
/// `rank_one_candidates` is not; an image that is not in the pool is an error
/// rather than something to skip quietly.
std::vector<std::vector<std::uint32_t>> permutation_action_on(const Field& field,
                                                  const std::vector<Automorphism>& group,
                                                  const std::vector<Matrix>& pool);

/// One representative per orbit of `candidates`, the lowest index of each.
///
/// `action` is what `permutation_action_on` returned. Choosing the lowest index matters for
/// the heuristic: it walks its pool in order and adopts the first candidate that
/// pays, so the representative it meets is the same element the unquotiented
/// walk would have met first.
/// Declared rather than included: `pool_orbits.h` includes this file, so naming
/// the type is all this header may do and the definition arrives in the source.
class PoolAction;

/// The same, reading the action from `PoolAction` rather than from a stored
/// table, so no caller has to build one entry per (automorphism, pool element).
/// [`pool_orbits.h`](pool_orbits.h) says what that costs at `<4,4,4>`.
std::vector<std::uint32_t> orbit_representatives(const PoolAction& action,
                                                 const std::vector<std::uint32_t>& candidates);

std::vector<std::uint32_t> orbit_representatives(const std::vector<std::vector<std::uint32_t>>& action,
                                         const std::vector<std::uint32_t>& candidates);

}  // namespace bilinear_rank
