/// That the Gray order `span_element_ranks` walks in is invisible to what it
/// returns.
///
/// The routine visits the `p^k` elements of a span in reflected Gray order and
/// writes each rank to `ranks[index]`, carrying the index alongside the walk as
/// `± p^digit`. Two things can go wrong there and neither shows up as a crash:
/// the carried index can drift from the digit string, which silently permutes
/// the answer, and the incremental combination can drift from the element it is
/// supposed to be, which silently changes ranks. Both come back as a
/// minimum-weight basis that is merely plausible.
///
/// So the reference here is the obvious reading: rebuild element `index` from
/// `coefficient_vector(index)` and rank it, compared **entry for entry** over
/// the whole span. Sizes are never what is compared.
#include <cstdint>
#include <string>
#include <vector>

#include "check.h"
#include "measures.h"
#include "minimum_weight_basis.h"
#include "span_enumeration.h"
#include "tensor_file.h"

namespace {

using bilinear_rank::Field;
using bilinear_rank::Matrix;

std::string fixtures;

/// Every rank, rebuilt from its own index and in index order: what the routine
/// did before the walk replaced it.
std::vector<std::size_t> by_rebuilding_each_element(const Field& field,
                                                    const std::vector<Matrix>& slices) {
    const std::size_t combinations = bilinear_rank::span_size(field, slices.size());
    std::vector<std::size_t> ranks(combinations);
    std::vector<int64_t> coefficients;
    Matrix combination;
    for (std::size_t index = 0; index < combinations; ++index) {
        bilinear_rank::coefficient_vector_into(index, slices.size(), field.characteristic(),
                                               coefficients);
        bilinear_rank::linear_combination_into(field, slices, coefficients, combination);
        ranks[index] = linear_algebra::rank(field, combination);
    }
    return ranks;
}

void the_two_orders_agree(const std::string& name) {
    const formats::Tensor tensor =
        formats::read_tensor_file(fixtures + "/" + name + ".tensor");
    const Field field(tensor.characteristic);
    const std::vector<Matrix> basis = bilinear_rank::minimum_weight_basis(field, tensor.slices);

    const std::vector<std::size_t> walked = bilinear_rank::span_element_ranks(field, basis);
    const std::vector<std::size_t> rebuilt = by_rebuilding_each_element(field, basis);

    check::equal(name + ": the two orders return the same number of ranks", walked.size(),
                 static_cast<long long>(rebuilt.size()));
    std::size_t disagreements = 0;
    for (std::size_t index = 0; index < rebuilt.size() && index < walked.size(); ++index) {
        if (walked[index] != rebuilt[index]) ++disagreements;
    }
    check::equal(name + ": elements the walk ranks differently", disagreements, 0);

    // A permutation of the right multiset would pass a sum or a histogram, so
    // the check above is the one that counts; this only says the span was not
    // empty and the two are not agreeing about nothing.
    std::size_t nonzero = 0;
    for (const std::size_t rank : rebuilt) nonzero += rank != 0 ? 1 : 0;
    check::equal(name + ": elements of nonzero rank exist to disagree about", nonzero > 0, 1);
}

/// The degenerate shapes, where an off-by-one in the walk's start would hide.
void the_edges_hold(const std::string& name) {
    const formats::Tensor tensor =
        formats::read_tensor_file(fixtures + "/" + name + ".tensor");
    const Field field(tensor.characteristic);

    // No slices: one element, the empty combination, and the walk takes no step.
    const std::vector<std::size_t> nothing = bilinear_rank::span_element_ranks(field, {});
    check::equal("a span of no slices has one element", nothing.size(), 1);
    check::equal("and that element has rank zero", nothing.front(), 0);

    // One slice: `p` elements, and index zero is still the zero map.
    const std::vector<Matrix> one(tensor.slices.begin(), tensor.slices.begin() + 1);
    const std::vector<std::size_t> walked = bilinear_rank::span_element_ranks(field, one);
    const std::vector<std::size_t> rebuilt = by_rebuilding_each_element(field, one);
    check::equal(name + ": one slice, same count", walked.size(),
                 static_cast<long long>(rebuilt.size()));
    std::size_t disagreements = 0;
    for (std::size_t index = 0; index < rebuilt.size() && index < walked.size(); ++index) {
        if (walked[index] != rebuilt[index]) ++disagreements;
    }
    check::equal(name + ": one slice, elements ranked differently", disagreements, 0);
    check::equal(name + ": the zero combination still has rank zero", walked.front(), 0);
}

}  // namespace

int main(int argc, char** argv) {
    fixtures = argc > 1 ? argv[1] : "fixtures";

    // Two characteristics, because the descending step is where GF(2) is
    // degenerate: subtracting is adding there, so a walk that subtracted wrongly
    // would pass on every GF(2) fixture in the repository.
    for (const std::string& name :
         {std::string("f2_2x3"), std::string("gf4_multiplication"), std::string("matmul_2x2x2"),
          std::string("f3_3x6"), std::string("f5_3x3"), std::string("pencil_split_f3_3")}) {
        the_two_orders_agree(name);
    }
    the_edges_hold("f3_3x6");
    return check::report("span_element_ranks");
}
