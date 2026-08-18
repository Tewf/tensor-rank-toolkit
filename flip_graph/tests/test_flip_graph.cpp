/// Walking a decomposition: that a walk never changes what the scheme computes,
/// and that over GF(p) it can see the flips that only exist up to a scalar.
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "algorithm_recovery.h"
#include "candidate_pool.h"
#include "check.h"
#include "flip_graph.h"
#include "tensor_file.h"

namespace {

using bilinear_rank::Element;
using bilinear_rank::Field;
using bilinear_rank::Matrix;
using bilinear_rank::Scheme;
using bilinear_rank::Term;

/// The map a scheme actually computes, which is the only thing a walk is
/// allowed to leave alone.
std::vector<Matrix> map_of(const Field& field, const Scheme& scheme) {
    return bilinear_rank::map_computed_by(field, bilinear_rank::algorithm_of(scheme));
}

bool same_map(const Field& field, const std::vector<Matrix>& left,
              const std::vector<Matrix>& right) {
    if (left.size() != right.size()) return false;
    for (std::size_t slice = 0; slice < left.size(); ++slice) {
        if (left[slice].rows() != right[slice].rows()) return false;
        if (left[slice].columns() != right[slice].columns()) return false;
        for (std::size_t row = 0; row < left[slice].rows(); ++row) {
            for (std::size_t column = 0; column < left[slice].columns(); ++column) {
                if (!field.areEqual(left[slice](row, column), right[slice](row, column))) {
                    return false;
                }
            }
        }
    }
    return true;
}

std::vector<Element> vector_of(const Field& field, const std::vector<std::int64_t>& entries) {
    std::vector<Element> result;
    for (const std::int64_t entry : entries) {
        Element value;
        field.init(value, entry);
        result.push_back(value);
    }
    return result;
}

Term term_of(const Field& field, const std::vector<std::int64_t>& left,
             const std::vector<std::int64_t>& right, const std::vector<std::int64_t>& output) {
    return Term{vector_of(field, left), vector_of(field, right), vector_of(field, output)};
}

/// Two terms whose left factors are `(1,0)` and `(2,0)`: the same direction,
/// different spellings. Over GF(3) this pair is one flip; over GF(2) it cannot
/// arise, because 2 is 0 there.
void check_scalar_sharing() {
    const Field field(3);
    const Scheme scheme = {term_of(field, {1, 0}, {1, 0}, {1, 0, 0}),
                           term_of(field, {2, 0}, {0, 1}, {0, 1, 0})};

    bilinear_rank::FlipReport report;
    const std::vector<Matrix> before = map_of(field, scheme);
    const Scheme walked = bilinear_rank::walk(field, scheme, 1, 1, &report);

    check::equal("GF(3): a flip that exists only up to a scalar is found",
                 static_cast<long long>(report.flips), 1);
    check::equal("GF(3): flipping it leaves the map alone",
                 same_map(field, before, map_of(field, walked)), 1);
}

/// The same two terms with the scalar removed: over GF(2) the pair still shares
/// its left factor, so the walk must behave exactly as it always did.
void check_plain_sharing() {
    const Field field(2);
    const Scheme scheme = {term_of(field, {1, 0}, {1, 0}, {1, 0, 0}),
                           term_of(field, {1, 0}, {0, 1}, {0, 1, 0})};

    bilinear_rank::FlipReport report;
    const std::vector<Matrix> before = map_of(field, scheme);
    const Scheme walked = bilinear_rank::walk(field, scheme, 1, 1, &report);

    check::equal("GF(2): a shared factor is still a flip", static_cast<long long>(report.flips), 1);
    check::equal("GF(2): flipping it leaves the map alone",
                 same_map(field, before, map_of(field, walked)), 1);
}

void check_reductions() {
    const Field field(3);

    // A term with a zero factor computes nothing and must go.
    Scheme with_nothing = {term_of(field, {1, 0}, {1, 0}, {1, 0}),
                           term_of(field, {0, 0}, {1, 0}, {0, 1})};
    check::equal("a zero factor removes its term",
                 static_cast<long long>(bilinear_rank::reduce(field, with_nothing)), 1);
    check::equal("and leaves the rest", static_cast<long long>(with_nothing.size()), 1);

    // Two terms agreeing in two modes up to scalars: `(1,0)⊗(1,0)⊗(1,0)` and
    // `(2,0)⊗(2,0)⊗(1,0)` are `A⊗B⊗(1,0)` and `A⊗B⊗(4,0)`, so they merge.
    Scheme mergeable = {term_of(field, {1, 0}, {1, 0}, {1, 0}),
                        term_of(field, {2, 0}, {2, 0}, {1, 0})};
    const std::vector<Matrix> before = map_of(field, mergeable);
    check::equal("two terms alike up to scalars merge",
                 static_cast<long long>(bilinear_rank::reduce(field, mergeable)), 1);
    check::equal("merging leaves the map alone", same_map(field, before, map_of(field, mergeable)),
                 1);
}

struct Expectation {
    const char* name;
    long long naive;
    long long reached;  // with these seeds and this many steps
};

/// The walk on real maps. The counts are what these seeds reach in this many
/// steps, not a claim about the rank: this is a heuristic and every number it
/// gives is an upper bound that has to compute the map to count at all.
///
/// `f3_3x6` is deliberately in the list and deliberately unflattering. On a
/// budget this small the walk gets 18 down to 16; given 60 000 flips a seed it
/// reaches 12, and `minimise-rank` reaches 10 in seven seconds. Over GF(3) this
/// method is behind the heuristic it was meant to rescue, and a test that only
/// held GF(2) fixtures would not say so.
void check_fixtures(const std::string& directory) {
    constexpr Expectation kExpectations[] = {
        {"matmul_2x2x2", 8, 7},
        {"f2_2x2", 4, 3},
        {"f3_3x6", 18, 16},
    };

    for (const Expectation& expectation : kExpectations) {
        const linear_algebra::Tensor tensor =
            linear_algebra::read_tensor_file(directory + "/" + expectation.name + ".tensor");
        const Field field(tensor.characteristic);

        bilinear_rank::Algorithm naive;
        if (!bilinear_rank::recovers_map(
                field, tensor.slices, bilinear_rank::rank_one_candidates(field, tensor.slices),
                naive)) {
            std::cout << "  FAIL  " << expectation.name << ": no naive scheme\n";
            ++check::failure_count;
            continue;
        }

        const Scheme start = bilinear_rank::scheme_of(naive);
        check::equal(std::string(expectation.name) + " naive",
                     static_cast<long long>(start.size()), expectation.naive);

        Scheme best = start;
        for (std::uint64_t seed = 1; seed <= 4; ++seed) {
            const Scheme walked = bilinear_rank::walk(field, start, 4000, seed, nullptr);
            if (walked.size() < best.size()) best = walked;
        }

        check::equal(std::string(expectation.name) + " walked to",
                     static_cast<long long>(best.size()), expectation.reached);
        // The only claim that matters: it still computes the map it started from.
        check::equal(std::string(expectation.name) + " still computes its map",
                     same_map(field, map_of(field, start), map_of(field, best)), 1);
    }
}

}  // namespace

int main(int argc, char** argv) {
    check_scalar_sharing();
    check_plain_sharing();
    check_reductions();
    if (argc > 1) check_fixtures(argv[1]);
    return check::report("flip graph");
}
