#pragma once

#include <cstddef>
#include <vector>

#include "bilinear_rank_aliases.h"

/// Fixing the first term of a decomposition to one representative per orbit.
///
/// A solver looking for `r` rank-one terms explores every choice of the first
/// one, and the group makes most of those choices the same question asked again:
/// if `σ` maps the target subspace to itself then it carries any decomposition
/// to another, so a solution can always be moved until its first term is an
/// orbit representative. That last step is `[covanov2019, Prop. 14]`: a
/// decomposition meeting the orbit `φ ∘ Stab(T)` at all is equivalent to one
/// that contains `φ` itself. Keys are [`../references.md`](../references.md).
/// For `⟨3,3,3⟩` that is thirteen choices instead of 261 121, and for `⟨4,4,4⟩`
/// twenty-six instead of 4 294 836 225.
///
/// **The representatives are not from the paper.** Which thirteen, and why
/// three ranks name an orbit, is
/// [`pool_orbits.h`](pool_orbits.h)'s own construction rather than a published
/// result, and it says so there.
///
/// Emitted as **cubes** rather than as one disjunction: one instance per
/// representative, each with the first term pinned. They are independent, so
/// they run in parallel, and each is a smaller formula than the disjunction
/// would be. The cubes are chosen by the group rather than by a lookahead
/// heuristic, which is the only unusual thing about this cube-and-conquer.
///
/// **This is the dangerous kind of constraint.** An over-strong symmetry break
/// turns a satisfiable formula unsatisfiable, and a wrong "no" is a wrong lower
/// bound, which nothing downstream can catch. Validate on maps of known rank
/// before believing any refutation built on it.
///
/// The boundary this crosses, and the protocol that keeps it sound, are written
/// down once in
/// [`orbit_cube_boundary/`](orbit_cube_boundary/README.md).
namespace bilinear_rank {

/// One cube per orbit, as literals over the consumer's variables.
///
/// `slices` is the tensor the consumer encoded, and it is checked against
/// `⟨rows, inner, columns⟩` rather than taken on trust: the representatives are
/// written for that map in that coordinate order, so naming the wrong shape
/// would pin the first term to a map the tensor does not contain and refute a
/// decomposition that exists. A shape the caller merely asserts is the failure
/// this argument exists to prevent, so it is an argument here and not a comment.
///
/// `left_variables` and `right_variables` are the consumer's variable numbers
/// for the operand vectors, laid out `left[term * rows * inner + coordinate]`
/// and `right[term * inner * columns + coordinate]`, which is the layout
/// `methods/satisfiability/binary_encoding.h` builds. Pass the whole arrays: only the
/// first term is pinned, and the offset arithmetic lives here so that no
/// consumer repeats it. A positive literal asserts the coordinate is one, a
/// negative literal that it is zero, which is the DIMACS convention and needs no
/// header from the encoder to honour.
///
/// **A cube pins the first term and orders nothing.** The consumer's own term
/// ordering must therefore skip term zero: a representative is not obliged to be
/// lexicographically least, so conjoining the two excludes decompositions that
/// exist. Each constraint is sound alone and their conjunction is not.
std::vector<std::vector<int>> orbit_cubes(const Field& field, const std::vector<Matrix>& slices,
                                          std::size_t rows, std::size_t inner, std::size_t columns,
                                          const std::vector<int>& left_variables,
                                          const std::vector<int>& right_variables);

}  // namespace bilinear_rank
