#include "pool_set_canon.h"

// Before any PermLib header: PermLib calls `boost::next` in three places and
// includes nothing that declares it, and a qualified name inside a template is
// looked up where the template is defined. `vendor/permlib/README.md` says so.
#include <boost/next_prior.hpp>

// PermLib reaches a Boost header that announces its own deprecation with a
// `#pragma message`. A SYSTEM include directory silences warnings and not
// pragmas, so this is the only way to keep the build quiet, and a build that
// prints one line nobody can act on is how a build comes to print twenty.
#define BOOST_ALLOW_DEPRECATED_HEADERS

#include <permlib/permlib_api.h>

#include <algorithm>

#include "candidate_pool.h"
#include "pool_orbits.h"
#include "span_basis.h"

namespace bilinear_rank {

std::vector<std::size_t> pool_inside(const Field& field, const std::vector<Matrix>& pool,
                                     const std::vector<Matrix>& generators) {
    const ReducedBasis span = linear_algebra::span_of(field, generators);
    std::vector<std::size_t> inside;
    std::vector<Element> scratch;
    for (std::size_t index = 0; index < pool.size(); ++index) {
        if (span.contains(pool[index], scratch)) inside.push_back(index);
    }
    return inside;
}

struct PoolSetCanon::Presentation {
    std::size_t points = 0;
    boost::shared_ptr<permlib::PermutationGroup> group;
    /// The same group on `2 * points`, acting alike on both halves, so that a
    /// marked pair can be canonised as a set. Built on demand: a search that
    /// never asks for a distinguished element never pays for it.
    mutable boost::shared_ptr<permlib::PermutationGroup> doubled;
    std::vector<std::vector<permlib::dom_int>> images;  // one per generator
};

/// One PermLib permutation per generator, from the factored action.
///
/// `[covanov2019, Thm. 17]`'s stabiliser acts on a rank-one map `left ⊗ right` by
/// moving each factor on its own side, so a pool permutation is a pair of
/// permutations of the two vector lists and never a table of pool size squared.
/// That is what makes presenting a 10^14-element group on 961 points affordable.
PoolSetCanon::PoolSetCanon(const Field& field, const std::vector<Automorphism>& generators,
                           std::size_t rows, std::size_t columns)
    : presentation_(std::make_unique<Presentation>()) {
    const std::size_t left_count = normalised_vectors(field, rows).size();
    const std::size_t right_count = normalised_vectors(field, columns).size();
    presentation_->points = left_count * right_count;

    const FactoredAction action = factored_action(field, generators, rows, columns);

    std::list<permlib::Permutation::ptr> permutations;
    for (std::size_t which = 0; which < generators.size(); ++which) {
        permlib::Permutation::perm image(presentation_->points);
        for (std::size_t point = 0; point < presentation_->points; ++point) {
            const std::size_t left = action.left[which][point / right_count];
            const std::size_t right = action.right[which][point % right_count];
            image[point] = static_cast<permlib::dom_int>(left * right_count + right);
        }
        permutations.push_back(permlib::Permutation::ptr(new permlib::Permutation(image)));
        presentation_->images.push_back(image);
    }

    // An empty generator list is the trivial group, and PermLib is not asked to
    // construct one: `canonical` below then returns its input, which is the
    // honest degenerate case and matches `canonical_subspace`'s empty-group arm.
    if (permutations.empty()) return;
    presentation_->group = permlib::construct(
        static_cast<permlib::dom_int>(presentation_->points), permutations.begin(),
        permutations.end());
}

PoolSetCanon::~PoolSetCanon() = default;
PoolSetCanon::PoolSetCanon(PoolSetCanon&&) noexcept = default;
PoolSetCanon& PoolSetCanon::operator=(PoolSetCanon&&) noexcept = default;

std::size_t PoolSetCanon::size() const { return presentation_->points; }

/// The doubled presentation, built the first time a marked pair is asked for.
static boost::shared_ptr<permlib::PermutationGroup> doubled_group(
    std::size_t points, const std::vector<std::vector<permlib::dom_int>>& images) {
    std::list<permlib::Permutation::ptr> permutations;
    for (const std::vector<permlib::dom_int>& image : images) {
        permlib::Permutation::perm both(2 * points);
        for (std::size_t point = 0; point < points; ++point) {
            both[point] = image[point];
            both[points + point] = static_cast<permlib::dom_int>(points + image[point]);
        }
        permutations.push_back(permlib::Permutation::ptr(new permlib::Permutation(both)));
    }
    return permlib::construct(static_cast<permlib::dom_int>(2 * points), permutations.begin(),
                              permutations.end());
}

std::vector<std::size_t> PoolSetCanon::canonical_with_marked(
    const std::vector<std::size_t>& indices, std::size_t marked) const {
    if (presentation_->images.empty()) {
        std::vector<std::size_t> sorted = indices;
        std::sort(sorted.begin(), sorted.end());
        sorted.push_back(presentation_->points + marked);
        return sorted;
    }
    if (!presentation_->doubled) {
        presentation_->doubled = doubled_group(presentation_->points, presentation_->images);
    }

    permlib::dset set(2 * presentation_->points);
    for (const std::size_t index : indices) set.set(index);
    set.set(presentation_->points + marked);

    const permlib::dset least = permlib::smallestSetImage(*presentation_->doubled, set);

    std::vector<std::size_t> canonical;
    for (std::size_t point = 0; point < least.size(); ++point) {
        if (least[point]) canonical.push_back(point);
    }
    return canonical;
}

std::vector<std::size_t> PoolSetCanon::canonical(
    const std::vector<std::size_t>& indices) const {
    if (!presentation_->group) {
        std::vector<std::size_t> sorted = indices;
        std::sort(sorted.begin(), sorted.end());
        return sorted;
    }

    permlib::dset set(presentation_->points);
    for (const std::size_t index : indices) set.set(index);

    const permlib::dset least = permlib::smallestSetImage(*presentation_->group, set);

    std::vector<std::size_t> canonical;
    for (std::size_t point = 0; point < least.size(); ++point) {
        if (least[point]) canonical.push_back(point);
    }
    return canonical;
}

}  // namespace bilinear_rank
