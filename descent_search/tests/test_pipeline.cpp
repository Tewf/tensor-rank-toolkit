/// Steps 1 and 2 of Table 1, and the property the table does not state: that
/// what comes out still generates the map that went in.
#include <chrono>
#include <iostream>
#include <string>

#include "candidate_pool.h"
#include "check.h"
#include "fewest_products.h"
#include "minimise_rank.h"
#include "minimum_weight_basis.h"
#include "span_basis.h"
#include "tensor_file.h"

namespace {

struct Expectation {
    const char* name;
    long long naive;
    long long after_step_1;
    long long after_step_2;
    long long rank_bound;
};

/// The bound column is `flattening_floor`, which is the maximum over the flattening
/// ranks and the two rank sums of `[yang2025]`. Every one of the four rose: 9 to
/// 10, 10 to 14, 10 to 14, 8 to 9. On `f2_3x8` that leaves a gap of one against
/// the known rank of 15, where it used to be five.
constexpr Expectation kExpectations[] = {
    {"f2_5x5", 25, 16, 14, 12},  // 12, not 10, since Griesmer joined the floor
    {"f2_3x8", 24, 19, 16, 14},
    {"f2_4x7", 28, 19, 16, 14},
    {"f3_3x6", 18, 12, 11, 9},
};

double seconds_since(std::chrono::steady_clock::time_point started) {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
}

/// The whole point of a rewrite: it must still compute the original map.
bool generates(const linear_algebra::ModularField& field, const std::vector<linear_algebra::ModularMatrix>& rewritten,
               const std::vector<linear_algebra::ModularMatrix>& original) {
    if (rewritten.empty()) return original.empty();
    linear_algebra::SpanBasis<linear_algebra::ModularField> span(field, rewritten.front().entry_count());
    for (const linear_algebra::ModularMatrix& slice : rewritten) span.try_add(slice);
    for (const linear_algebra::ModularMatrix& slice : original) {
        if (!span.contains(slice)) return false;
    }
    return true;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: test_pipeline <fixtures-directory>\n";
        return 2;
    }
    const std::string directory = argv[1];

    for (const Expectation& expected : kExpectations) {
        const std::string name = expected.name;
        const formats::Tensor tensor = formats::read_tensor_file(directory + "/" + name + ".tensor");
        const linear_algebra::ModularField field(tensor.characteristic);
        std::cout << name << "\n";

        check::equal(name + " naive", static_cast<long long>(
                                          linear_algebra::multiplication_count(field, tensor.slices)),
                     expected.naive);

        auto started = std::chrono::steady_clock::now();
        const std::vector<linear_algebra::ModularMatrix> step_1 = bilinear_rank::minimum_weight_basis(field, tensor.slices);
        const double step_1_seconds = seconds_since(started);

        check::equal(name + " after step 1",
                     static_cast<long long>(linear_algebra::multiplication_count(field, step_1)),
                     expected.after_step_1);

        started = std::chrono::steady_clock::now();
        const std::vector<linear_algebra::ModularMatrix> own_products =
            bilinear_rank::rank_one_candidates(field, step_1);
        const std::vector<linear_algebra::ModularMatrix> shortlist =
            bilinear_rank::improving_candidates(field, step_1, own_products);
        const std::vector<linear_algebra::ModularMatrix> step_2 =
            bilinear_rank::minimise_rank(field, step_1, shortlist);
        const double step_2_seconds = step_1_seconds + seconds_since(started);

        check::equal(name + " after step 2",
                     static_cast<long long>(linear_algebra::multiplication_count(field, step_2)),
                     expected.after_step_2);

        // A lower bound sitting above a decomposition somebody reached is a false
        // lower bound, and nothing downstream catches one: `decide-rank` refuses
        // every target under this number without searching at all.
        const std::size_t bound = bilinear_rank::flattening_floor(field, tensor.slices);
        check::equal(name + " rank bound", static_cast<long long>(bound),
                     expected.rank_bound);
        if (bound > linear_algebra::multiplication_count(field, step_2)) {
            std::cout << "  FAIL  " << name << ": the bound is above a decomposition that exists\n";
            ++check::failure_count;
        }

        if (!generates(field, step_1, tensor.slices) || !generates(field, step_2, tensor.slices)) {
            std::cout << "  FAIL  " << name << ": the rewrite no longer generates the map\n";
            ++check::failure_count;
        }

        std::cout << "        step 1 " << step_1_seconds << " s, step 2 " << step_2_seconds
                  << " s cumulative, " << step_1.size() << " then " << step_2.size()
                  << " slices\n";
    }

    return check::report("pipeline");
}
