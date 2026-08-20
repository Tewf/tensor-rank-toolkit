/// What `rank_lower_bound` is worth *inside* the search, which is nothing.
///
/// The obvious pruning rule for a branch and bound is: at every node compute a
/// lower bound on the rank of the span in hand, and if it already meets the best
/// answer known, kill the subtree. It is sound. It is also, on every question
/// here, exactly the free test `dim V >= best` that costs nothing, and this
/// pins why.
///
/// **The bound is `max(dim V, the bound at the root)`.** Every term degrades:
///
/// - the flattening of `V` along the axis the search grows is its dimension, so
///   that term *is* `dim V`;
/// - Griesmer reads `(k, d)` off the space, `d` being the least rank in it, and
///   the search's whole job is putting **rank-one** maps into `V`, so `d` is 1
///   from the first choice onward and `sum_j ceil(1/p^j)` over `k` terms is `k`,
///   which is `dim V` again;
/// - the rank sums are bounded by the contraction shapes and not by the depth,
///   so they stop moving.
///
/// The consequence is arithmetic. The root bound is the floor a search already
/// starts from, and any live question has `best > floor`, so
/// `bound(V) >= best` holds exactly when `dim V >= best` does. A per-node bound
/// prunes nothing a free comparison does not, and costs a rank table to say so.
///
/// This is pinned rather than written down because it is the kind of claim that
/// stops being true the day somebody adds a bound that does not read the least
/// rank in the space. On that day this test fails, and the failure is the news.
#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

#include "candidate_pool.h"
#include "check.h"
#include "rank_lower_bound.h"
#include "span_basis.h"
#include "tensor_file.h"

namespace {

using linear_algebra::ModularField;
using linear_algebra::ModularMatrix;

/// Walk the first `levels` choices the search would make, adding one rank-one
/// pool map per level in index order, and assert the bound is the maximum of the
/// dimension and the root's value at every one of them.
void follow_a_branch(const std::string& directory, const char* name, std::size_t levels) {
    const auto tensor = linear_algebra::read_tensor_file(directory + "/" + name + ".tensor");
    const ModularField field(tensor.characteristic);
    const std::size_t rows = tensor.slices.front().rows();
    const std::size_t columns = tensor.slices.front().columns();
    const std::vector<ModularMatrix> pool = bilinear_rank::all_rank_one_maps(field, rows, columns);

    std::vector<ModularMatrix> basis = tensor.slices;
    linear_algebra::SpanBasis<ModularField> span(field, rows * columns);
    for (const ModularMatrix& slice : tensor.slices) span.try_add(slice);

    const auto at_root = static_cast<long long>(linear_algebra::rank_lower_bound(field, basis));

    std::size_t taken = 0;
    for (std::size_t level = 0; level <= levels; ++level) {
        const auto dimension = static_cast<long long>(span.dimension());
        const auto bound = static_cast<long long>(linear_algebra::rank_lower_bound(field, basis));
        check::equal(std::string(name) + " at depth " + std::to_string(level), bound,
                     std::max(dimension, at_root));

        while (taken < pool.size() && !span.try_add(pool[taken])) ++taken;
        if (taken >= pool.size()) return;
        basis.push_back(pool[taken]);
        ++taken;
    }
}

}  // namespace

int main(int argc, char** argv) {
    const std::string directory = argc > 1 ? argv[1] : "fixtures";

    // Three shapes, three root bounds, and six levels each: enough that the
    // first term to stop moving has stopped, and cheap enough to run always.
    follow_a_branch(directory, "f2_5x5", 6);
    follow_a_branch(directory, "matmul_2x2x2", 6);
    follow_a_branch(directory, "matmul_2x2x3", 6);

    return check::report("bound at a node");
}
