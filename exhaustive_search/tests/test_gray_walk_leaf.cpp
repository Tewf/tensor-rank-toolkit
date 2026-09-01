/// The Gray walk and the rebuild answer the same leaf, on the same span.
///
/// [`subspace_walk.h`](../subspace_walk.h) visits the elements of a subspace in
/// reflected Gray code order so that each one costs a row addition instead of a
/// rebuild from its base-`p` digits. **The order is different from the order the
/// rebuild used**, so the two can hand back different rank-one bases and that is
/// allowed; what is not allowed is for them to disagree about how many
/// independent rank-one maps the subspace holds, because that number *is* the
/// leaf's verdict and every rank this repository publishes is a stack of them.
///
/// A disagreement would not crash. A walk that skipped elements would refute a
/// decomposition that exists, and a walk that formed an element wrongly would
/// find rank-one maps that are not in the span at all. So the two routes are run
/// against each other here, on the same span, and three things are asserted: the
/// counts match, the two answers span the same space, and every map returned is
/// rank one and inside the subspace it came from.
///
/// **Over odd characteristic**, which is the whole of what this file is for: the
/// GF(2) leaf is a separate path in [`gf2_leaf.h`](../gf2_leaf.h) and does not
/// go through here, so `p = 2` would exercise the digits at the one radix where
/// a reflected code is the same thing as an alternating one.
#include <cstddef>
#include <string>
#include <vector>

#include "candidate_pool.h"
#include "check.h"
#include "exhaustive_search.h"
#include "measures.h"
#include "span_basis.h"
#include "subspace_walk.h"
#include "tensor_file.h"

namespace {

using bilinear_rank::Field;
using bilinear_rank::Matrix;
using bilinear_rank::ReducedBasis;

std::size_t power(std::size_t base, std::size_t exponent) {
    std::size_t total = 1;
    for (std::size_t step = 0; step < exponent; ++step) total *= base;
    return total;
}

/// Whether every map of `inner` lies in the span of `outer`.
bool spanned_by(const Field& field, std::size_t width, const std::vector<Matrix>& outer,
                const std::vector<Matrix>& inner) {
    ReducedBasis reach(field, width);
    for (const Matrix& map : outer) reach.try_add(map);
    for (const Matrix& map : inner) {
        if (!reach.contains(map)) return false;
    }
    return true;
}

/// Both routes on one span, with a target no walk can reach early, so each one
/// visits every element and returns a maximal independent set of the rank-one
/// maps inside.
///
/// That set has an order-independent *size* — it is a basis of the span of all
/// the rank-one elements, and a greedy over a matroid gives one whatever order
/// it sees the elements in — which is why the counts may be compared for
/// equality rather than merely for both being enough.
void check_one_span(const std::string& label, const Field& field, const ReducedBasis& span,
                    std::size_t rows, std::size_t columns) {
    const std::size_t width = rows * columns;
    const auto characteristic = static_cast<std::size_t>(field.characteristic());
    const std::size_t elements = power(characteristic, span.dimension());
    const std::size_t unreachable = width + 1;

    const std::vector<Matrix> walked = bilinear_rank::by_walking_the_subspace(
        field, span, rows, columns, unreachable, elements, nullptr);
    const std::vector<Matrix> rebuilt = bilinear_rank::by_rebuilding_each_element(
        field, span, rows, columns, unreachable, elements, nullptr);

    check::equal(label + ": the two routes find as many rank-one maps as each other",
                 static_cast<long long>(walked.size()),
                 static_cast<long long>(rebuilt.size()));
    check::equal(label + ": and the two answers span the same space",
                 (spanned_by(field, width, rebuilt, walked)
                  && spanned_by(field, width, walked, rebuilt)) ? 1 : 0,
                 1);

    long long not_rank_one = 0;
    long long outside = 0;
    for (const Matrix& map : walked) {
        if (linear_algebra::rank(field, map) != 1) ++not_rank_one;
        if (!span.contains(map)) ++outside;
    }
    check::equal(label + ": every map the walk returns is rank one", not_rank_one, 0);
    check::equal(label + ": and is inside the subspace it walked", outside, 0);

    // The leaf's real question, which stops as soon as it is answered: the two
    // must still agree, and the early break is a different code path.
    if (span.dimension() > 0) {
        const std::vector<Matrix> walked_leaf = bilinear_rank::by_walking_the_subspace(
            field, span, rows, columns, span.dimension(), elements, nullptr);
        const std::vector<Matrix> rebuilt_leaf = bilinear_rank::by_rebuilding_each_element(
            field, span, rows, columns, span.dimension(), elements, nullptr);
        check::equal(label + ": and agree when asked the leaf's own question",
                     static_cast<long long>(walked_leaf.size()),
                     static_cast<long long>(rebuilt_leaf.size()));
    }

    // The budget is spent per element examined and not per element formed, so a
    // limit below the count has to stop both routes and a limit at the count has
    // to stop neither, whichever order the elements arrive in.
    if (elements > 2) {
        std::vector<int> abandoned_below;
        std::vector<int> abandoned_at;
        for (const int route : {0, 1}) {
            bilinear_rank::SearchBudget tight{/*node_limit=*/1'000'000,
                                              /*leaf_limit=*/elements / 2};
            bilinear_rank::SearchBudget whole{/*node_limit=*/1'000'000, /*leaf_limit=*/elements};
            if (route == 0) {
                bilinear_rank::by_walking_the_subspace(field, span, rows, columns, unreachable,
                                                       elements, &tight);
                bilinear_rank::by_walking_the_subspace(field, span, rows, columns, unreachable,
                                                       elements, &whole);
            } else {
                bilinear_rank::by_rebuilding_each_element(field, span, rows, columns, unreachable,
                                                          elements, &tight);
                bilinear_rank::by_rebuilding_each_element(field, span, rows, columns, unreachable,
                                                          elements, &whole);
            }
            abandoned_below.push_back(tight.leaf_abandoned ? 1 : 0);
            abandoned_at.push_back(whole.leaf_abandoned ? 1 : 0);
        }
        check::equal(label + ": a limit under the count stops the walk", abandoned_below[0], 1);
        check::equal(label + ": and stops the rebuild the same way", abandoned_below[1],
                     abandoned_below[0]);
        check::equal(label + ": a limit at the count stops the walk not at all", abandoned_at[0], 0);
        check::equal(label + ": nor the rebuild", abandoned_at[1], abandoned_at[0]);
    }
}

}  // namespace

int main(int argc, char** argv) {
    if (argc <= 1) {
        std::cout << "usage: test_gray_walk_leaf <fixtures>\n";
        return 1;
    }
    const std::string directory = argv[1];

    // The three fixtures over odd characteristic. `f5_3x3` was written for this
    // file: GF(3) was the only odd prime the directory carried, and a base-p
    // walk that happened to be right at p = 3 and wrong at p = 5 would have had
    // nothing to fail against.
    for (const char* name : {"f3_3x6", "pencil_split_f3_3", "f5_3x3"}) {
        const formats::Tensor tensor =
            formats::read_tensor_file(directory + "/" + name + ".tensor");
        const Field field(tensor.characteristic);
        const std::size_t rows = tensor.rows();
        const std::size_t columns = tensor.columns();
        const std::size_t width = rows * columns;

        // The tensor's own span, which is the leaf at the root of a search.
        ReducedBasis span(field, width);
        for (const Matrix& slice : tensor.slices) span.try_add(slice);
        check_one_span(std::string(name) + ", span(T)", field, span, rows, columns);

        // And a span built out of rank-one maps, which has a rank-one basis by
        // construction. span(T) of a polynomial multiplication holds few
        // rank-one elements or none, so on its own it would let a walk that
        // found nothing at all look correct.
        const bilinear_rank::RankOnePool pool(field, rows, columns);
        ReducedBasis from_pool(field, width);
        std::size_t chosen = 0;
        for (std::size_t index = 0; index < pool.size() && chosen < 4; ++index) {
            if (from_pool.try_add(pool.at(index))) ++chosen;
        }
        check::equal(std::string(name) + ": built a 4-dimensional rank-one span",
                     static_cast<long long>(chosen), 4);
        check_one_span(std::string(name) + ", span of 4 rank-one maps", field, from_pool, rows,
                       columns);
    }

    return check::report("gray walk leaf");
}
