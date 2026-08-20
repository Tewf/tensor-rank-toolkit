// Is the canonical form a function of the orbit, on shapes where no oracle exists?
//
// [`test_orbit_separation.cpp`](test_orbit_separation.cpp) settles this completely
// at `⟨2,2,2⟩` and [`test_factored_canonisation.cpp`](test_factored_canonisation.cpp)
// settles it wherever a permutation of the grid can still be built. Past those
// shapes there is no reference left, and one half of the contract is still
// checkable without one: moving a set by a group element may not move its name.
//
// That is the weaker half — the constant function passes it, which is why it is
// never the only check here — and it is the half that reaches `⟨3,3,3⟩`, where the
// group is 4 741 632 elements, the grid presentation is a megabyte a generator, and
// neither oracle exists. What it does catch is a search that has drifted off the
// group: a base change that lost a point, a transversal read in the wrong
// direction, an orbit walked with the wrong stabiliser. All of those return
// something, and all of them return something that moves.
//
// The group elements are random words in the generators rather than the generators
// themselves, because a generator is exactly the element a wrong stabiliser chain
// is most likely to still handle.
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "candidate_pool.h"
#include "check.h"
#include "group_construction.h"
#include "pool_orbits.h"
#include "pool_set_canon.h"
#include "pool_sets.h"

namespace {

/// One shape's worth: random sets, each carried by random words and re-canonised.
void sweep(const pool_sets::MatmulShape& shape, std::size_t sets, std::size_t words,
           std::mt19937& source) {
    const bilinear_rank::Field field(2);
    const std::vector<bilinear_rank::Automorphism> generators =
        bilinear_rank::matrix_multiplication_symmetry_generators(field, shape.rows, shape.inner,
                                                                 shape.columns);
    const bilinear_rank::PoolSetCanon canon(field, generators, shape.slice_rows(),
                                            shape.slice_columns());
    // The pool is never built: a cell is an index, and where a generator sends one
    // is two lookups and a multiply.
    const bilinear_rank::PoolAction action(field, generators, shape.slice_rows(),
                                           shape.slice_columns());

    std::uniform_int_distribution<std::size_t> which_generator(0, generators.size() - 1);
    std::uniform_int_distribution<std::size_t> word_length(1, 6);

    long long moved_names = 0;
    long long moved_marked_names = 0;
    long long checked = 0;
    for (std::size_t set = 0; set < sets; ++set) {
        const std::vector<std::size_t> cells =
            pool_sets::random_set(source, canon.size(), 1 + set % 9);
        const std::size_t marked = cells[set % cells.size()];
        const std::vector<std::size_t> name = canon.canonical(cells);
        const std::vector<std::size_t> marked_name = canon.canonical_with_marked(cells, marked);

        for (std::size_t word = 0; word < words; ++word) {
            std::vector<std::size_t> carried = cells;
            std::size_t carried_mark = marked;
            const std::size_t length = word_length(source);
            for (std::size_t letter = 0; letter < length; ++letter) {
                const std::size_t generator = which_generator(source);
                for (std::size_t& cell : carried) {
                    cell = action.image(generator, static_cast<std::uint32_t>(cell));
                }
                carried_mark = action.image(generator, static_cast<std::uint32_t>(carried_mark));
            }
            if (canon.canonical(carried) != name) ++moved_names;
            if (canon.canonical_with_marked(carried, carried_mark) != marked_name) {
                ++moved_marked_names;
            }
            ++checked;
        }
    }

    std::cout << "  " << shape.name() << ": pool " << canon.size() << ", " << checked
              << " moved sets\n";
    check::equal(shape.name() + ", sets whose name moved with them", moved_names, 0);
    check::equal(shape.name() + ", marked pairs whose name moved with them", moved_marked_names,
                 0);
}

}  // namespace

int main(int argc, char** argv) {
    std::mt19937 source(20260821);

    // `⟨4,4,4⟩` on its own, because it is the claim rather than a check: the grid it
    // canonises subsets of has 4 294 836 225 cells and one permutation of that is
    // 17 GB, so no reference exists, nothing can be swept, and the only question
    // left is whether the question can be asked at all. Its own ctest entry so that
    // a failure there reads as "the largest shape" and not as "invariance".
    if (argc > 1 && std::string(argv[1]) == "--at-4x4x4") {
        sweep({4, 4, 4}, 6, 3, source);
        return check::report("the canonical form at the shape the grid cannot hold");
    }

    sweep({2, 2, 2}, 200, 8, source);
    sweep({2, 2, 3}, 150, 8, source);
    sweep({2, 3, 3}, 100, 6, source);
    // `⟨3,3,3⟩`: 261 121 cells, and the presentation is 1 022 points. This is the
    // first shape in this file that the grid presentation could not have reached at
    // all inside a test's budget.
    sweep({3, 3, 3}, 40, 4, source);

    return check::report("the canonical form under random words in the generators");
}
