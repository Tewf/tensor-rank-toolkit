// Does naming the orbit from the two axes give the same name as naming it from the
// grid?
//
// `PoolSetCanon` used to present the group on the pool, `left_count * right_count`
// points, and ask `[permlib]` for the least image of a subset of it. It now
// presents the group on `left_count + right_count` points and runs
// `[linton2004]`'s search against cells. The claim that move makes is not that the
// new form is as good a canonical form as the old (that would be a new definition
// and would need its own proof) but that it is **the same function**: same group,
// same flat index `left * right_count + right`, same order on images, therefore the
// same set of pool indices out.
//
// So this asks nothing else. `flattened_canon.h` still computes the old answer, on
// every shape where a permutation of the grid can be held at all, and the two are
// held against each other on the sets the search itself meets and on random ones
// besides. A disagreement here is the reduction being wrong; there is no reading of
// it where both are right.
#include <algorithm>
#include <cstddef>
#include <iostream>
#include <optional>
#include <random>
#include <string>
#include <vector>

#include "candidate_pool.h"
#include "check.h"
#include "flattened_canon.h"
#include "group_construction.h"
#include "map_construction.h"
#include "pool_set_canon.h"
#include "pool_sets.h"
#include "search_children.h"
#include "span_basis.h"

namespace {

struct Tally {
    long long sets = 0;
    long long disagreements = 0;
    long long marked_pairs = 0;
    long long marked_disagreements = 0;
};

/// How much of a shape to cover. The grid presentation is what limits this: it
/// costs a permutation of `left_count * right_count` points per generator, so
/// `⟨2,3,3⟩`'s 32 193 is affordable and `⟨2,2,4⟩`'s 3 825 is cheap, while the two
/// small shapes can be swept exhaustively.
struct Sample {
    /// Stride over the pool when taking the depth-one children of the enumerator.
    std::size_t stride = 1;
    std::size_t random_sets = 0;
    /// Levels of the search's own tree to walk, or zero to walk none. The sets a
    /// span produces are not random subsets, and the parent test only ever sees
    /// those, so both kinds are asked for.
    std::size_t search_depth = 0;
    /// Also check the doubled ground set `canonical_with_marked` uses. Off at the
    /// largest shape, where the reference would want a permutation of twice the
    /// grid.
    bool doubled = true;
};

/// One shape, with both canonical forms built over the same generators.
class Differential {
   public:
    Differential(const pool_sets::MatmulShape& shape, bool doubled)
        : field_(2),
          slices_(bilinear_rank::matrix_multiplication_tensor(shape.rows, shape.inner,
                                                              shape.columns)),
          pool_(bilinear_rank::all_rank_one_maps(field_, shape.slice_rows(),
                                                 shape.slice_columns())),
          generators_(bilinear_rank::matrix_multiplication_symmetry_generators(
              field_, shape.rows, shape.inner, shape.columns)),
          canon_(field_, generators_, shape.slice_rows(), shape.slice_columns()),
          on_the_grid_(field_, generators_, shape.slice_rows(), shape.slice_columns(), 1) {
        if (doubled) {
            on_the_doubled_grid_.emplace(field_, generators_, shape.slice_rows(),
                                         shape.slice_columns(), 2);
        }
    }

    const bilinear_rank::Field& field() const { return field_; }
    const std::vector<bilinear_rank::Matrix>& slices() const { return slices_; }
    const std::vector<bilinear_rank::Matrix>& pool() const { return pool_; }
    const bilinear_rank::PoolSetCanon& canon() const { return canon_; }
    const Tally& tally() const { return tally_; }

    void compare_on(const std::vector<std::size_t>& indices) {
        std::vector<std::size_t> sorted = indices;
        std::sort(sorted.begin(), sorted.end());
        sorted.erase(std::unique(sorted.begin(), sorted.end()), sorted.end());

        ++tally_.sets;
        if (canon_.canonical(sorted) != on_the_grid_.least_image(sorted)) {
            ++tally_.disagreements;
        }
        if (!on_the_doubled_grid_ || sorted.empty()) return;

        // Three marks rather than all of them: the mark's own key is the only part
        // of a pair's key that depends on which cell is marked, and the first, a
        // middle and the last cell are three different ones. Sweeping every cell is
        // what `test_distinguished_cell.cpp` does, on a different question.
        for (const std::size_t marked :
             {sorted.front(), sorted[sorted.size() / 2], sorted.back()}) {
            ++tally_.marked_pairs;
            std::vector<std::size_t> cells = sorted;
            cells.push_back(canon_.size() + marked);
            if (canon_.canonical_with_marked(sorted, marked) !=
                on_the_doubled_grid_->least_image(cells)) {
                ++tally_.marked_disagreements;
            }
        }
    }

   private:
    bilinear_rank::Field field_;
    std::vector<bilinear_rank::Matrix> slices_;
    std::vector<bilinear_rank::Matrix> pool_;
    std::vector<bilinear_rank::Automorphism> generators_;
    bilinear_rank::PoolSetCanon canon_;
    flattened_canon::OnThePool on_the_grid_;
    std::optional<flattened_canon::OnThePool> on_the_doubled_grid_;
    Tally tally_;
};

void sweep(const pool_sets::MatmulShape& shape, const Sample& sample, std::mt19937& source) {
    Differential shapes_worth(shape, sample.doubled);
    const bilinear_rank::Field& field = shapes_worth.field();
    const std::vector<bilinear_rank::Matrix>& pool = shapes_worth.pool();

    // The depth-one children of the enumerator: the base plus one pool element,
    // which is the first thing the parent test is ever asked about.
    for (std::size_t index = 0; index < pool.size(); index += sample.stride) {
        std::vector<bilinear_rank::Matrix> child = shapes_worth.slices();
        child.push_back(pool[index]);
        shapes_worth.compare_on(bilinear_rank::pool_inside(field, pool, child));
    }

    if (sample.search_depth > 0) {
        const std::size_t base =
            linear_algebra::span_of(field, shapes_worth.slices()).dimension();
        search_children::below(field, shapes_worth.slices(), pool, shapes_worth.canon(),
                               shapes_worth.slices(), base, base + sample.search_depth,
                               [&](const std::vector<bilinear_rank::Matrix>& child) {
                                   shapes_worth.compare_on(
                                       bilinear_rank::pool_inside(field, pool, child));
                               });
    }

    for (std::size_t which = 0; which < sample.random_sets; ++which) {
        shapes_worth.compare_on(pool_sets::random_set(source, pool.size(), 1 + which % 12));
    }

    const Tally& tally = shapes_worth.tally();
    std::cout << "  " << shape.name() << ": " << tally.sets << " sets, " << tally.marked_pairs
              << " marked pairs\n";
    check::equal(shape.name() + ", enough sets to be a sample", tally.sets > 100 ? 1 : 0, 1);
    check::equal(shape.name() + ", sets where the two canonical forms differ",
                 tally.disagreements, 0);
    check::equal(shape.name() + ", marked pairs where the two canonical forms differ",
                 tally.marked_disagreements, 0);
}

}  // namespace

int main() {
    std::mt19937 source(20260820);

    // `⟨2,2,2⟩` swept whole: every depth-one child, the search's own tree three
    // levels down, and two thousand random subsets.
    sweep({2, 2, 2}, {1, 2000, 3, true}, source);

    // `⟨2,2,3⟩`, a second group on a second pool, two levels of the tree.
    sweep({2, 2, 3}, {1, 1000, 2, true}, source);

    // `⟨2,2,4⟩` and `⟨2,3,3⟩` are the last two shapes where a permutation of the
    // grid can be built at all: 3 825 and 32 193 points. No tree walk here: one
    // node of it scans the whole pool for its content, and what is being checked is
    // the canonical form rather than the walk.
    sweep({2, 2, 4}, {5, 500, 0, true}, source);
    sweep({2, 3, 3}, {400, 200, 0, false}, source);

    return check::report("the factored canonical form against the grid one");
}
