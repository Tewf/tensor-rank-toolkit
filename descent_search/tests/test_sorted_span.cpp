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

    // The same filtration, handed the ranks of the span without the last slice,
    // which is the form the descent's cost query would call it in. Half the
    // enumeration is then a rank it copies rather than computes, and a wrong
    // index range there is invisible in the answer for exactly the fixtures
    // where the two halves happen to agree.
    if (slices.size() > 1) {
        const std::vector<Matrix> without_last(slices.begin(), slices.end() - 1);
        const bilinear_rank::SortedSpan reusing(
            field, slices, bilinear_rank::span_element_ranks(field, without_last));
        check::equal(label + ": cost given known ranks",
                     static_cast<long long>(reusing.cost()),
                     static_cast<long long>(filtration.cost()));
        check::equal(label + ": rank-one basis given known ranks",
                     static_cast<long long>(reusing.has_rank_one_basis()),
                     static_cast<long long>(filtration.has_rank_one_basis()));
    }

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

/// `cost(V)` is an upper bound on the fewest rank-one maps covering `V`, and can
/// be strictly larger. Pinned because the header said they were equal until
/// 2026-08-20, and that reading makes `cost(V) > k` look like a sound prune when
/// it would throw away a live solution.
void cost_is_not_the_covering_number() {
    const Field field(2);
    auto diagonal = [&field](int a, int b, int c) {
        Matrix m(3, 3);
        for (std::size_t row = 0; row < 3; ++row) {
            for (std::size_t column = 0; column < 3; ++column) m(row, column) = field.zero;
        }
        m(0, 0) = a;
        m(1, 1) = b;
        m(2, 2) = c;
        return m;
    };
    // Every nonzero element of this plane has rank two, so no basis is cheaper.
    const std::vector<Matrix> plane{diagonal(1, 1, 0), diagonal(0, 1, 1)};
    const bilinear_rank::SortedSpan filtration(field, plane);
    check::equal("counterexample: dimension", static_cast<long long>(filtration.dimension()), 2LL);
    check::equal("counterexample: cost", static_cast<long long>(filtration.cost()), 4LL);
    check::equal("counterexample: no rank-one basis",
                 static_cast<long long>(filtration.has_rank_one_basis()), 0LL);

    // And it is not monotone: adjoining one rank-one map raises the dimension and
    // *lowers* the cost, which is the only way this quantity could ever end a
    // branch early. Pinned because the search's own page denied it until
    // 2026-08-20 while its measurements showed it.
    std::vector<Matrix> extended = plane;
    extended.push_back(diagonal(1, 0, 0));
    const bilinear_rank::SortedSpan grown(field, extended);
    check::equal("counterexample: dimension rose",
                 static_cast<long long>(grown.dimension()), 3LL);
    check::equal("counterexample: cost fell", static_cast<long long>(grown.cost()), 3LL);
    check::equal("counterexample: now a rank-one basis",
                 static_cast<long long>(grown.has_rank_one_basis()), 1LL);

    // And yet it lies inside the span of three rank-one maps, so a search asking
    // k = 3 has a solution here that `cost(V) > k` would have refused.
    linear_algebra::SpanBasis<Field> cover(field, 9);
    for (const Matrix& unit : {diagonal(1, 0, 0), diagonal(0, 1, 0), diagonal(0, 0, 1)}) {
        cover.try_add(unit);
    }
    check::equal("counterexample: cover dimension",
                 static_cast<long long>(cover.dimension()), 3LL);
    for (const Matrix& element : plane) {
        check::equal("counterexample: covered by three rank-one maps",
                     static_cast<long long>(cover.contains(element)), 1LL);
    }
}

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

    cost_is_not_the_covering_number();

    return check::report("sorted span");
}
