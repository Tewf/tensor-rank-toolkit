// Is a child's pool content, read off the cosets, the list the pool scan returns?
//
// `PoolCosets` replaces one pool scan per candidate child with one per node, which
// is worth 7 to 14 scans a node and was 98% of the canonical route's time at
// `<2,3,3>`. The whole of its correctness is that `extended_by(i)` is exactly
// `pool_inside(field, pool, current + pool[i])`, same elements and same order, so
// that is what is checked — on every child of every subspace tried, not on a
// sample of them.
//
// **Over GF(2) the residue never needs normalising and a broken normalisation
// would pass.** A pool element lies in `span(current) + <added>` when its residue
// is any nonzero *multiple* of `added`'s, and over GF(2) there is only one. So the
// larger characteristics are here on purpose: `f3_3x6` and `f5_3x3` are the rows
// that fail if the scalar class is dropped.
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

#include "candidate_pool.h"
#include "check.h"
#include "pool_cosets.h"
#include "pool_set_canon.h"
#include "span_basis.h"
#include "tensor_file.h"

namespace {

struct Tally {
    long long children = 0;
    long long mismatches = 0;
};

/// Every child of `current`, content against content.
void check_children(const bilinear_rank::Field& field,
                    const std::vector<bilinear_rank::Matrix>& pool,
                    const std::vector<bilinear_rank::Matrix>& current, Tally& tally) {
    const bilinear_rank::PoolCosets cosets(field, pool, current);
    if (cosets.inside() != bilinear_rank::pool_inside(field, pool, current)) ++tally.mismatches;

    for (const std::uint32_t index : cosets.outside()) {
        std::vector<bilinear_rank::Matrix> child = current;
        child.push_back(pool[index]);
        ++tally.children;
        if (cosets.extended_by(index) != bilinear_rank::pool_inside(field, pool, child)) {
            ++tally.mismatches;
        }
    }
}

/// The slice space, then the slice space plus one pool element, then plus two:
/// the three depths a node of the enumeration sits at before the counts stop being
/// walkable in a default-suite test.
void check_tensor(const std::string& path, std::size_t subspaces, Tally& tally) {
    const formats::Tensor tensor = formats::read_tensor_file(path);
    const bilinear_rank::Field field(tensor.characteristic);
    const std::vector<bilinear_rank::Matrix> pool =
        bilinear_rank::all_rank_one_maps(field, tensor.rows(), tensor.columns());

    check_children(field, pool, tensor.slices, tally);

    bilinear_rank::ReducedBasis span = linear_algebra::span_of(field, tensor.slices);
    std::vector<bilinear_rank::Matrix> current = tensor.slices;
    std::vector<bilinear_rank::Element> scratch;
    std::size_t taken = 0;
    for (std::size_t index = 0; index < pool.size() && taken < subspaces; ++index) {
        if (span.contains(pool[index], scratch)) continue;
        span.try_add(pool[index]);
        current.push_back(pool[index]);
        ++taken;
        check_children(field, pool, current, tally);
    }
}

}  // namespace

int main(int argc, char** argv) {
    const std::string directory = argc > 1 ? argv[1] : "fixtures";

    Tally tally;
    check_tensor(directory + "/matmul_2x2x2.tensor", 2, tally);
    std::cout << "  after GF(2), 4x4: " << tally.children << " children\n";
    check_tensor(directory + "/f3_3x6.tensor", 2, tally);
    check_tensor(directory + "/f5_3x3.tensor", 2, tally);

    check::equal("children checked", tally.children > 2000 ? 1 : 0, 1);
    check::equal("contents differing from the pool scan",
                 static_cast<long long>(tally.mismatches), 0);
    return check::report("pool_cosets");
}
