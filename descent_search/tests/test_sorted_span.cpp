/// The filtration answers what the greedy answers, on every fixture.
///
/// `SortedSpan::cost` reads the dimensions of the rank filtration;
/// `multiplication_count(minimum_weight_basis(...))` builds a minimum-weight
/// basis and adds up its ranks. The header argues they are the same number. This
/// asserts it, because an argument that nothing checks is how a rewrite that is
/// nearly right ships.
///
/// The leaf identity is asserted beside it: a span has a rank-one basis exactly
/// when its rank-one elements already span it, which is the question every leaf
/// of the exact search asks and the one the filtration answers for free.
///
/// Every `.tensor` that ships is walked, and the cheap ones are walked twice: a
/// tensor whose span is small enough is also checked one dimension at a time, by
/// adjoining rank-one pool maps, because a node of the search is a span that grew
/// rather than a tensor that was read, and those are the spans the identity has
/// to hold on.
#include <string>
#include <vector>

#include "candidate_pool.h"
#include "check.h"
#include "measures.h"
#include "minimum_weight_basis.h"
#include "sorted_span.h"
#include "span_basis.h"
#include "tensor_file.h"

namespace {

using bilinear_rank::Field;
using bilinear_rank::Matrix;

constexpr const char* kFixtures[] = {
    "cyclic_f2_5", "f2_2x2", "f2_2x3", "f2_5x5", "gf16_multiplication",
    "gf4_multiplication", "gf8_multiplication", "matmul_2x2x2", "matmul_2x2x3",
    "nonconcise_matmul_2x2x2", "pencil_nilpotent_f2_3", "pencil_split_f3_3", "w_state",
};

void agrees(const std::string& label, const Field& field, const std::vector<Matrix>& slices) {
    const bilinear_rank::SortedSpan filtration(field, slices);
    const std::vector<Matrix> basis = bilinear_rank::minimum_weight_basis(field, slices);

    check::equal(label + ": cost", static_cast<long long>(filtration.cost()),
                 static_cast<long long>(linear_algebra::multiplication_count(field, basis)));
    check::equal(label + ": dimension", static_cast<long long>(filtration.dimension()),
                 static_cast<long long>(basis.size()));

    // A minimum-weight basis whose ranks sum to its own size is one made of
    // rank-one maps, and there is no cheaper way for the sum to reach the size.
    const bool by_greedy =
        linear_algebra::multiplication_count(field, basis) == basis.size();
    check::equal(label + ": rank-one basis",
                 static_cast<long long>(filtration.has_rank_one_basis()),
                 static_cast<long long>(by_greedy));
}

}  // namespace

int main(int argc, char** argv) {
    if (argc <= 1) {
        std::cout << "usage: test_sorted_span <fixtures>\n";
        return 1;
    }
    const std::string directory = argv[1];

    for (const char* name : kFixtures) {
        const auto tensor = linear_algebra::read_tensor_file(directory + "/" + name + ".tensor");
        const Field field(tensor.characteristic);
        agrees(name, field, tensor.slices);
    }

    // And on spans the search actually builds, which are not the span of a file:
    // adjoin rank-one maps one at a time and re-ask at every dimension.
    for (const char* name : {"matmul_2x2x2", "f2_2x3", "gf4_multiplication"}) {
        const auto tensor = linear_algebra::read_tensor_file(directory + "/" + name + ".tensor");
        const Field field(tensor.characteristic);
        const std::vector<Matrix> pool = bilinear_rank::all_rank_one_maps(
            field, tensor.slices.front().rows(), tensor.slices.front().columns());

        std::vector<Matrix> grown = tensor.slices;
        linear_algebra::SpanBasis<Field> reached(
            field, tensor.slices.front().rows() * tensor.slices.front().columns());
        for (const Matrix& slice : tensor.slices) reached.try_add(slice);

        std::size_t taken = 0;
        for (int step = 0; step < 3; ++step) {
            while (taken < pool.size() && !reached.try_add(pool[taken])) ++taken;
            if (taken >= pool.size()) break;
            grown.push_back(pool[taken]);
            ++taken;
            agrees(std::string(name) + " grown by " + std::to_string(step + 1), field, grown);
        }
    }

    return check::report("sorted span");
}
