#include "pool_set_canon.h"

#include <mutex>
#include <optional>
#include <utility>

#include "candidate_pool.h"
#include "factored_lex_min.h"
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
    std::size_t lefts = 0;
    std::size_t rights = 0;
    /// `lefts * rights`: the pool size, which is what a caller indexes by and is no
    /// longer a permutation domain.
    std::size_t points = 0;
    std::vector<FactoredGenerator> generators;
    std::optional<FactoredGrid> grid;
    /// The same group on a **doubled left vector list**, so a marked pair can be
    /// canonised as a set. Built on demand, because a search that never asks for a
    /// distinguished element never pays for it, and built once even when the
    /// enumeration's workers ask together: `canonical_augmentation.cpp` shares one
    /// of these across threads.
    mutable std::once_flag doubling;
    mutable std::optional<FactoredGrid> doubled;
};

namespace {

/// The generators again with the left vector list doubled: left vector `l` and left
/// vector `lefts + l` move alike.
///
/// A pair `(set, mark)` becomes a set on a doubled ground set, and on a grid the
/// cheapest way to double the ground set is to double one axis. Cell `(lefts + l,
/// r)` has flat index `(lefts + l) * rights + r`, which is
/// `l * rights + r + lefts * rights`: exactly the point `p + size()` the doubled
/// grid presentation used for the mark, with the same arithmetic and the same
/// order, so the two forms are the same function and not merely alike. Doubling the
/// axis costs `lefts` extra points where doubling the grid cost `lefts * rights`.
std::vector<FactoredGenerator> with_doubled_left(
    const std::vector<FactoredGenerator>& generators, std::size_t lefts) {
    std::vector<FactoredGenerator> doubled;
    doubled.reserve(generators.size());
    for (const FactoredGenerator& generator : generators) {
        FactoredGenerator both;
        both.left.resize(2 * lefts);
        for (std::size_t left = 0; left < lefts; ++left) {
            both.left[left] = generator.left[left];
            both.left[lefts + left] = static_cast<std::uint32_t>(lefts + generator.left[left]);
        }
        both.right = generator.right;
        doubled.push_back(std::move(both));
    }
    return doubled;
}

}  // namespace

/// One factored generator per automorphism, from the factored action.
///
/// `[covanov2019, Thm. 17]`'s stabiliser acts on a rank-one map `left ⊗ right` by
/// moving each factor on its own side, so a pool permutation is a pair of
/// permutations of the two vector lists and never a table of pool size squared.
/// Nothing here expands that pair: [`factored_lex_min.h`](factored_lex_min.h) takes
/// the group in exactly this form and presents it on `lefts + rights` points, which
/// is what makes a 10^14-element group affordable at `⟨4,4,4⟩` rather than merely
/// at `f2_5x5`.
PoolSetCanon::PoolSetCanon(const Field& field, const std::vector<Automorphism>& generators,
                           std::size_t rows, std::size_t columns)
    : presentation_(std::make_unique<Presentation>()) {
    presentation_->lefts = normalised_vectors(field, rows).size();
    presentation_->rights = normalised_vectors(field, columns).size();
    presentation_->points = presentation_->lefts * presentation_->rights;

    const FactoredAction action = factored_action(field, generators, rows, columns);
    for (std::size_t which = 0; which < generators.size(); ++which) {
        presentation_->generators.push_back({action.left[which], action.right[which]});
    }
    presentation_->grid.emplace(presentation_->lefts, presentation_->rights,
                                presentation_->generators);
}

PoolSetCanon::~PoolSetCanon() = default;
PoolSetCanon::PoolSetCanon(PoolSetCanon&&) noexcept = default;
PoolSetCanon& PoolSetCanon::operator=(PoolSetCanon&&) noexcept = default;

std::size_t PoolSetCanon::size() const { return presentation_->points; }

std::vector<std::size_t> PoolSetCanon::canonical(const std::vector<std::size_t>& indices) const {
    return presentation_->grid->least_image(indices);
}

std::vector<std::size_t> PoolSetCanon::canonical_with_marked(
    const std::vector<std::size_t>& indices, std::size_t marked) const {
    std::call_once(presentation_->doubling, [this] {
        presentation_->doubled.emplace(
            2 * presentation_->lefts, presentation_->rights,
            with_doubled_left(presentation_->generators, presentation_->lefts));
    });
    std::vector<std::size_t> cells = indices;
    cells.push_back(presentation_->points + marked);
    return presentation_->doubled->least_image(cells);
}

std::vector<std::vector<std::uint32_t>> PoolSetCanon::stabiliser_generators(
    const std::vector<std::size_t>& indices) const {
    const std::size_t rights = presentation_->rights;
    std::vector<std::vector<std::uint32_t>> generators;
    for (const FactoredGenerator& fixing : presentation_->grid->setwise_stabiliser(indices)) {
        std::vector<std::uint32_t> images(presentation_->points);
        for (std::size_t point = 0; point < presentation_->points; ++point) {
            images[point] = static_cast<std::uint32_t>(fixing.left[point / rights]) *
                                static_cast<std::uint32_t>(rights) +
                            fixing.right[point % rights];
        }
        generators.push_back(std::move(images));
    }
    return generators;
}

}  // namespace bilinear_rank
