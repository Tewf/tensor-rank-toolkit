/// The GF(2) span walk in bits against the same walk in field elements, on the
/// same questions.
///
/// [`../gf2_span_walk.h`](../gf2_span_walk.h) replaces the representation of a
/// matrix inside step 1 and nothing else: the same span, the same order, the
/// same ceiling and the same greedy. So the only thing worth asserting is that
/// it did not change the answer, and the only assertion worth making is entry
/// for entry. A count or a sum would pass on a permuted span, and a permuted
/// span is exactly what a drifting index gives.
///
/// **Three things are held against the general path**, because the search calls
/// all three and a wrong answer from any of them is a published number that
/// nobody would question: every rank of a span, the minimum-weight basis, and
/// the basis with one candidate adjoined, which is the call
/// [`../../incumbent_search/cost_first_search.cpp`](../../incumbent_search/cost_first_search.cpp)
/// makes once per child and the only one that exercises the floor under the
/// unranked half.
///
/// **And the general field path is asserted to be untouched.** GF(3) and GF(5)
/// reach none of this, which is a claim about `gf2_span_walk_applies` and is
/// checked as one, rather than by trusting that a field nobody tested still
/// works.
///
/// `gf2_rank` itself is checked separately and exhaustively, over every matrix
/// of the small shapes, the way `is_rank_one` is: it is the one new primitive
/// under all of this, and neither direction of an error in it announces itself.
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "candidate_pool.h"
#include "check.h"
#include "gf2_bits.h"
#include "gf2_span_walk.h"
#include "measures.h"
#include "minimum_weight_basis.h"
#include "span_enumeration.h"
#include "tensor_file.h"

namespace {

using bilinear_rank::Field;
using bilinear_rank::Matrix;

std::string fixtures;

/// Entries where two lists of maps differ, shape included: a basis that came
/// back the right length and the wrong maps is the failure this is looking for.
std::size_t differences(const std::vector<Matrix>& left, const std::vector<Matrix>& right) {
    if (left.size() != right.size()) return left.size() + right.size();
    std::size_t differing = 0;
    for (std::size_t index = 0; index < left.size(); ++index) {
        if (left[index].rows() != right[index].rows() ||
            left[index].columns() != right[index].columns()) {
            ++differing;
            continue;
        }
        for (std::size_t entry = 0; entry < left[index].entry_count(); ++entry) {
            if (left[index].data()[entry] != right[index].data()[entry]) ++differing;
        }
    }
    return differing;
}

std::size_t differences(const std::vector<std::size_t>& left,
                        const std::vector<std::size_t>& right) {
    if (left.size() != right.size()) return left.size() + right.size();
    std::size_t differing = 0;
    for (std::size_t index = 0; index < left.size(); ++index) {
        if (left[index] != right[index]) ++differing;
    }
    return differing;
}

/// The three answers, taken over one representation.
///
/// The switch is process-wide and read once per call, so it is set around the
/// call and put back straight away: a test that left it false would report on
/// the general path twice and pass.
struct Answers {
    std::vector<std::size_t> ranks;
    std::vector<Matrix> basis;
    std::size_t cost = 0;
    std::vector<Matrix> adjoined;
    std::size_t adjoined_cost = 0;
};

Answers answers_over(const Field& field, const std::vector<Matrix>& slices,
                     const std::vector<Matrix>& candidates, bool packed) {
    bilinear_rank::set_gf2_span_walk_offered(packed);

    Answers taken;
    taken.basis = bilinear_rank::minimum_weight_basis(field, slices, {}, &taken.cost);
    taken.ranks = bilinear_rank::span_element_ranks(field, taken.basis);
    for (const Matrix& candidate : candidates) {
        std::size_t cost = 0;
        const std::vector<Matrix> grown =
            bilinear_rank::minimum_weight_basis_with(field, taken.basis, candidate, taken.ranks, &cost);
        taken.adjoined.insert(taken.adjoined.end(), grown.begin(), grown.end());
        taken.adjoined_cost += cost;
    }

    bilinear_rank::set_gf2_span_walk_offered(true);
    return taken;
}

/// The first few rank-one pieces of the map, which is what a move offers: the
/// candidate has to be outside the basis often enough for the greedy to have
/// something to do, and a piece of the map itself is.
std::vector<Matrix> some_candidates(const Field& field, const std::vector<Matrix>& slices) {
    const std::vector<Matrix> pieces = bilinear_rank::rank_one_candidates(field, slices);
    const std::size_t wanted = pieces.size() < 4 ? pieces.size() : 4;
    return std::vector<Matrix>(pieces.begin(), pieces.begin() + static_cast<std::ptrdiff_t>(wanted));
}

void the_two_representations_agree(const std::string& name) {
    const linear_algebra::Tensor tensor =
        linear_algebra::read_tensor_file(fixtures + "/" + name + ".tensor");
    const Field field(tensor.characteristic);
    const std::vector<Matrix> candidates = some_candidates(field, tensor.slices);

    const Answers packed = answers_over(field, tensor.slices, candidates, true);
    const Answers general = answers_over(field, tensor.slices, candidates, false);

    check::equal(name + ": the packed walk is the one that answered",
                 bilinear_rank::gf2_span_walk_applies(field, tensor.slices) ? 1 : 0, 1);
    check::equal(name + ": elements of the span the two rank differently",
                 differences(packed.ranks, general.ranks), 0);
    check::equal(name + ": ranks in the span", static_cast<long long>(general.ranks.size()),
                 static_cast<long long>(packed.ranks.size()));
    check::equal(name + ": entries where the two minimum-weight bases differ",
                 differences(packed.basis, general.basis), 0);
    check::equal(name + ": the cost the two report", static_cast<long long>(packed.cost),
                 static_cast<long long>(general.cost));
    check::equal(name + ": entries where the two differ with a candidate adjoined",
                 differences(packed.adjoined, general.adjoined), 0);
    check::equal(name + ": the cost the two report with a candidate adjoined",
                 static_cast<long long>(packed.adjoined_cost),
                 static_cast<long long>(general.adjoined_cost));

    // A span of one rank each way would agree about nothing. This says there was
    // something to disagree about, which the checks above cannot say for
    // themselves.
    std::size_t nonzero = 0;
    for (const std::size_t rank : general.ranks) nonzero += rank != 0 ? 1 : 0;
    check::equal(name + ": elements of nonzero rank exist to disagree about", nonzero > 0, 1);
}

/// The fields the packed walk must never answer for, and does not.
void the_general_field_is_untouched(const std::string& name) {
    const linear_algebra::Tensor tensor =
        linear_algebra::read_tensor_file(fixtures + "/" + name + ".tensor");
    const Field field(tensor.characteristic);
    const std::vector<Matrix> candidates = some_candidates(field, tensor.slices);

    check::equal(name + ": the packed walk declines this field",
                 bilinear_rank::gf2_span_walk_applies(field, tensor.slices) ? 1 : 0, 0);

    // And the answers do not move when the switch does, which is the same
    // statement taken from the other end.
    const Answers offered = answers_over(field, tensor.slices, candidates, true);
    const Answers withheld = answers_over(field, tensor.slices, candidates, false);
    check::equal(name + ": elements of the span ranked differently",
                 differences(offered.ranks, withheld.ranks), 0);
    check::equal(name + ": entries where the two bases differ",
                 differences(offered.basis, withheld.basis), 0);
    check::equal(name + ": the cost reported", static_cast<long long>(offered.cost),
                 static_cast<long long>(withheld.cost));
    check::equal(name + ": entries where the two differ with a candidate adjoined",
                 differences(offered.adjoined, withheld.adjoined), 0);
}

/// Every matrix of one shape over GF(2), packed and ranked both ways.
///
/// Exhaustive because at these shapes it can be, which is the right test for a
/// primitive that the whole file rests on.
void the_packed_rank_is_the_rank(std::size_t rows, std::size_t columns) {
    const Field field(2);
    const std::string shape = std::to_string(rows) + "x" + std::to_string(columns);
    const std::size_t width = rows * columns;
    const std::size_t words = linear_algebra::gf2_word_count(width);

    Matrix matrix(rows, columns);
    std::vector<std::uint64_t> packed(words);
    std::size_t disagreements = 0;
    for (std::size_t index = 0; index < (std::size_t(1) << width); ++index) {
        for (std::size_t entry = 0; entry < width; ++entry) {
            field.assign(matrix.data()[entry], static_cast<int64_t>((index >> entry) & 1));
        }
        linear_algebra::gf2_pack(matrix.data(), width, packed.data());
        if (linear_algebra::gf2_rank(packed.data(), rows, columns) !=
            linear_algebra::rank(field, matrix)) {
            ++disagreements;
        }
    }
    check::equal("GF(2) " + shape + ": matrices the packed rank gets wrong", disagreements, 0);
}

}  // namespace

int main(int argc, char** argv) {
    fixtures = argc > 1 ? argv[1] : "fixtures";

    // Every shape small enough to enumerate whole, and then one shape and its
    // transpose: 5x2 and 2x5 hold the same ten bits and are the same ten bits
    // read as different rows, which is what a row reader that swapped its two
    // indices would get away with on a square.
    for (const std::size_t rows : {std::size_t(1), std::size_t(2), std::size_t(3)}) {
        for (const std::size_t columns : {std::size_t(1), std::size_t(2), std::size_t(3),
                                          std::size_t(4)}) {
            the_packed_rank_is_the_rank(rows, columns);
        }
    }
    the_packed_rank_is_the_rank(5, 2);
    the_packed_rank_is_the_rank(2, 5);

    for (const std::string& name :
         {std::string("f2_2x3"), std::string("gf4_multiplication"), std::string("matmul_2x2x2"),
          std::string("cyclic_f2_5"), std::string("gf8_multiplication")}) {
        the_two_representations_agree(name);
    }

    for (const std::string& name :
         {std::string("f3_3x6"), std::string("f5_3x3"), std::string("pencil_split_f3_3")}) {
        the_general_field_is_untouched(name);
    }
    return check::report("gf2_span_walk");
}
