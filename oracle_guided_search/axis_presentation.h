#pragma once

// Before any PermLib header: PermLib calls `boost::next` in three places and
// includes nothing that declares it, and a qualified name inside a template is
// looked up where the template is defined. `vendor/permlib/README.md` says so.
#include <boost/next_prior.hpp>

// PermLib reaches a Boost header that announces its own deprecation with a
// `#pragma message`. A SYSTEM include directory silences warnings and not pragmas,
// so this is the only way to keep the build quiet, and a build that prints one line
// nobody can act on is how a build comes to print twenty.
#define BOOST_ALLOW_DEPRECATED_HEADERS

#include <permlib/permlib_api.h>

#include <cstddef>
#include <vector>

#include "factored_lex_min.h"

// The fault that cost an afternoon, made impossible to repeat rather than fixed
// once. PermLib's `dom_int` is `unsigned short` unless `PERMLIB_DOMAIN_INT` is
// defined, and a domain past 65 535 then wraps and writes out of bounds. Losing the
// define is easy: a new vendor drop, a second build system, somebody compiling a
// file by hand. The failure it causes is not.
//
// The check lives here because this is where the domain is declared. Presenting on
// the axes rather than on the grid lowers the degree by orders of magnitude and
// does **not** clear this ceiling: `⟨4,4,4⟩`'s two vector lists are 65 535 each, so
// the axis domain is 131 070, still twice the default type's reach.
static_assert(sizeof(permlib::dom_int) >= 4,
              "PermLib's point type is too narrow: define PERMLIB_DOMAIN_INT. "
              "Without it a domain larger than 65 535 wraps and corrupts memory, "
              "which is what `vendor/permlib/CMakeLists.txt` sets it for.");

/// What `FactoredGrid` is, held once for the two files that ask it questions.
///
/// [`factored_lex_min.h`](factored_lex_min.h) is the module's only public header
/// and it names `Presentation` without defining it, so that nothing including it
/// inherits a vendored library and Boost. This is where it is defined, and it is
/// **not** a public header: only [`factored_lex_min.cpp`](factored_lex_min.cpp) and
/// [`factored_set_stabiliser.cpp`](factored_set_stabiliser.cpp) include it. Two
/// files because the two questions are two roles and one of them was the longest
/// file in the repository; one presentation because building the BSGS twice would
/// cost twice, and at `⟨4,4,4⟩` twice is 96 MB and a second Schreier-Sims.
namespace bilinear_rank {

/// PermLib's names for the pieces this presentation is made of, written once.
namespace axes {

using Perm = permlib::Permutation;
using Transversal = permlib::TRANSVERSAL;
using Group = permlib::PermutationGroup;
using BaseChange =
    permlib::ConjugatingBaseChange<Perm, Transversal,
                                   permlib::RandomBaseTranspose<Perm, Transversal>>;

}  // namespace axes

struct FactoredGrid::Presentation {
    std::size_t rows = 0;
    std::size_t columns = 0;
    std::vector<FactoredGenerator> generators;
    /// On `rows + columns` points. Absent for the trivial group, which is not
    /// presented at all: `least_image` then returns its input, which is the honest
    /// degenerate case and what the grid presentation did.
    boost::shared_ptr<axes::Group> group;
};

}  // namespace bilinear_rank
