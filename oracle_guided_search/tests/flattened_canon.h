#pragma once

// Before any PermLib header, and for the reason `vendor/permlib/README.md` gives.
#include <boost/next_prior.hpp>

#define BOOST_ALLOW_DEPRECATED_HEADERS

#include <permlib/permlib_api.h>

#include <algorithm>
#include <cstddef>
#include <list>
#include <vector>

#include "automorphism.h"
#include "bilinear_rank_aliases.h"
#include "candidate_pool.h"
#include "pool_orbits.h"

/// The canonical form as it was: the group expanded to one permutation **of the
/// pool** per generator and handed to `[permlib]`'s `smallestSetImage`.
///
/// This is not a second implementation written to have something to compare
/// against. It is the code [`../pool_set_canon.cpp`](../pool_set_canon.cpp) ran
/// until the presentation moved to the two axes, moved here rather than deleted,
/// because the claim the move makes is not "the new form is as good" but "the new
/// form is the same answer". A differential test is the only thing that can hold it
/// to that, and it needs the old answer to still be computable.
///
/// It lives in `tests/` and nothing outside links it, which is the honest place for
/// it: at `⟨4,4,4⟩` one of these permutations is 17 GB, so this is a reference that
/// exists only where the shapes are small enough for it to exist.
///
/// `copies` is how many times the pool is laid end to end with the group acting
/// alike on each copy. One copy is `canonical`; two is the doubled ground set
/// `canonical_with_marked` puts a mark on.
namespace flattened_canon {

class OnThePool {
   public:
    OnThePool(const bilinear_rank::Field& field,
              const std::vector<bilinear_rank::Automorphism>& generators, std::size_t rows,
              std::size_t columns, std::size_t copies = 1)
        : copies_(copies) {
        const std::size_t right_count = bilinear_rank::normalised_vectors(field, columns).size();
        points_ = bilinear_rank::normalised_vectors(field, rows).size() * right_count;

        const bilinear_rank::FactoredAction action =
            bilinear_rank::factored_action(field, generators, rows, columns);
        std::list<permlib::Permutation::ptr> permutations;
        for (std::size_t which = 0; which < generators.size(); ++which) {
            permlib::Permutation::perm image(copies_ * points_);
            for (std::size_t point = 0; point < points_; ++point) {
                const std::size_t moved =
                    static_cast<std::size_t>(action.left[which][point / right_count]) *
                        right_count +
                    action.right[which][point % right_count];
                for (std::size_t copy = 0; copy < copies_; ++copy) {
                    image[copy * points_ + point] =
                        static_cast<permlib::dom_int>(copy * points_ + moved);
                }
            }
            permutations.push_back(permlib::Permutation::ptr(new permlib::Permutation(image)));
        }
        if (permutations.empty()) return;
        group_ = permlib::construct(static_cast<permlib::dom_int>(copies_ * points_),
                                    permutations.begin(), permutations.end());
    }

    /// The pool size, whatever `copies` is.
    std::size_t size() const { return points_; }

    std::vector<std::size_t> least_image(const std::vector<std::size_t>& cells) const {
        if (!group_) {
            std::vector<std::size_t> sorted = cells;
            std::sort(sorted.begin(), sorted.end());
            sorted.erase(std::unique(sorted.begin(), sorted.end()), sorted.end());
            return sorted;
        }
        permlib::dset set(copies_ * points_);
        for (const std::size_t cell : cells) set.set(cell);

        const permlib::dset least = permlib::smallestSetImage(*group_, set);
        std::vector<std::size_t> image;
        for (std::size_t point = 0; point < least.size(); ++point) {
            if (least[point]) image.push_back(point);
        }
        return image;
    }

   private:
    std::size_t points_ = 0;
    std::size_t copies_ = 1;
    boost::shared_ptr<permlib::PermutationGroup> group_;
};

}  // namespace flattened_canon
