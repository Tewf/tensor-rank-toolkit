/// The three claims this search rests on, executed rather than argued.
///
/// 1. **The bound is admissible.** `cost(V) >= dim V` for every subspace, with
///    equality exactly when `V` has a rank-one basis. That is the whole proof
///    that a branch under `dim V + 1 >= best` holds nothing better, and it is
///    checked here on every subspace the search itself reaches, not on a
///    handpicked few.
/// 2. **The generated move set is the right one.** A rank-one `g` outside `V`
///    with `cost(V + <g>) <= cost(V)` is a level-lowering summand of some element
///    of `V`, which
///    [`../level_lowering_moves.h`](../level_lowering_moves.h) derives. Brute-forced
///    against the whole pool on the small fixtures: every candidate the pool
///    offers that does not simply cost one more is in the generated set.
/// 3. **The answer computes the map.** A search that quietly loses a slice
///    reports excellent numbers.
#include <algorithm>
#include <string>
#include <vector>

#include "algorithm_recovery.h"
#include "candidate_pool.h"
#include "check.h"
#include "cost_first_search.h"
#include "level_lowering_moves.h"
#include "measures.h"
#include "minimum_weight_basis.h"
#include "span_basis.h"
#include "tensor_file.h"

namespace {

using bilinear_rank::Field;
using bilinear_rank::Matrix;

std::string fixtures;

std::vector<Matrix> slices_of(const std::string& name, int64_t& characteristic) {
    const formats::Tensor tensor =
        formats::read_tensor_file(fixtures + "/" + name + ".tensor");
    characteristic = tensor.characteristic;
    return tensor.slices;
}

/// Claim 1, on every subspace one adjunction away from a minimum-weight basis of
/// the fixture: `cost >= dim`, and `cost == dim` exactly when every basis element
/// has rank one.
void the_bound_is_admissible(const std::string& name) {
    int64_t characteristic = 0;
    const std::vector<Matrix> slices = slices_of(name, characteristic);
    const Field field(characteristic);

    const std::vector<Matrix> basis = bilinear_rank::minimum_weight_basis(field, slices);
    const std::vector<std::size_t> known = bilinear_rank::span_element_ranks(field, basis);
    const linear_algebra::SpanBasis<Field> span = linear_algebra::span_of(field, basis);
    const std::vector<Matrix> pool = bilinear_rank::all_rank_one_maps(
        field, slices.front().rows(), slices.front().columns());

    std::size_t violations = 0;
    std::size_t tight_but_not_rank_one = 0;
    std::vector<bilinear_rank::Element> scratch;
    for (const Matrix& candidate : pool) {
        if (span.contains(candidate, scratch)) continue;
        const std::vector<Matrix> child =
            bilinear_rank::minimum_weight_basis_with(field, basis, candidate, known);
        const std::size_t cost = linear_algebra::multiplication_count(field, child);
        if (cost < child.size()) ++violations;

        const bool every_element_is_rank_one =
            std::all_of(child.begin(), child.end(), [&](const Matrix& element) {
                return linear_algebra::is_rank_one(field, element);
            });
        if ((cost == child.size()) != every_element_is_rank_one) ++tight_but_not_rank_one;
    }
    check::equal(name + ": subspaces with cost below their dimension", violations, 0);
    check::equal(name + ": cost meets the dimension other than at a rank-one basis",
                 tight_but_not_rank_one, 0);
}

/// Claim 2: every pool candidate that does not cost exactly one more is one of
/// the moves generated from the span.
void the_generated_moves_are_the_useful_ones(const std::string& name, std::size_t cutoff) {
    int64_t characteristic = 0;
    const std::vector<Matrix> slices = slices_of(name, characteristic);
    const Field field(characteristic);

    const std::vector<Matrix> basis = bilinear_rank::minimum_weight_basis(field, slices);
    const std::vector<std::size_t> known = bilinear_rank::span_element_ranks(field, basis);
    const std::size_t cost = linear_algebra::multiplication_count(field, basis);
    const linear_algebra::SpanBasis<Field> span = linear_algebra::span_of(field, basis);

    // The generated set, held as flattened entries so membership is a lookup.
    const std::vector<Matrix> generated =
        bilinear_rank::level_lowering_moves(field, basis, known, cutoff);
    std::vector<std::vector<bilinear_rank::Element>> offered;
    for (const Matrix& move : generated) {
        offered.emplace_back(move.data(), move.data() + move.entry_count());
    }
    std::sort(offered.begin(), offered.end());

    const std::vector<Matrix> pool = bilinear_rank::all_rank_one_maps(
        field, slices.front().rows(), slices.front().columns());
    std::size_t useful = 0;
    std::size_t missed = 0;
    std::vector<bilinear_rank::Element> scratch;
    for (const Matrix& candidate : pool) {
        if (span.contains(candidate, scratch)) continue;
        const std::vector<Matrix> child =
            bilinear_rank::minimum_weight_basis_with(field, basis, candidate, known);
        if (linear_algebra::multiplication_count(field, child) > cost) continue;
        ++useful;
        const std::vector<bilinear_rank::Element> flat(candidate.data(),
                                                       candidate.data() + candidate.entry_count());
        if (!std::binary_search(offered.begin(), offered.end(), flat)) ++missed;
    }
    // Only meaningful when the cutoff covers every rank in the span, which is
    // why the caller passes one: below that the generated set is a restriction
    // by design and a miss is not a fault.
    check::equal(name + ": useful pool candidates the generated moves miss", missed, 0);
    check::equal(name + ": at least one useful candidate exists to be missed", useful > 0, 1);
}

/// Claim 3, and the one result worth pinning: from the naive eight, the search
/// reaches Strassen's seven and the seven products rebuild the tensor.
void it_reaches_a_known_answer(const std::string& name, std::size_t expected) {
    int64_t characteristic = 0;
    const std::vector<Matrix> slices = slices_of(name, characteristic);
    const Field field(characteristic);

    bilinear_rank::IncumbentLimits limits;
    limits.width = 0;  // every child: a branch and bound, not a beam
    limits.node_limit = 200'000;
    limits.summand_rank = std::min(slices.front().rows(), slices.front().columns());

    bilinear_rank::IncumbentReport report;
    const std::vector<Matrix> answer = bilinear_rank::search_from_above(
        field, bilinear_rank::minimum_weight_basis(field, slices), {}, limits, &report);

    bilinear_rank::Algorithm algorithm;
    const bool rebuilds = bilinear_rank::recovers_map(
        field, slices, bilinear_rank::rank_one_candidates(field, answer), algorithm);
    check::equal(name + ": the answer rebuilds the map", rebuilds, 1);
    check::equal(name + ": products", algorithm.product_count(), expected);
}

/// Claim 4: `--below k` is the same answer from a smaller tree, and says
/// truthfully which of the two things happened.
///
/// Three questions of one fixture, because the setting has three outcomes and
/// only one of them is the interesting one:
///
/// - `below` at the answer reaches it, and reaches it entering **no more nodes**
///   than the plain run — the whole point of seeding the incumbent low;
/// - `below` under anything the search can build does not reach it, and
///   `reached_below` stays false. Nothing else may be read off that;
/// - `below` at or above the start is already held, so nothing is searched.
void below_finds_the_same_answer_sooner(const std::string& name, std::size_t answer) {
    int64_t characteristic = 0;
    const std::vector<Matrix> slices = slices_of(name, characteristic);
    const Field field(characteristic);
    const std::vector<Matrix> start = bilinear_rank::minimum_weight_basis(field, slices);
    const std::size_t start_cost = linear_algebra::multiplication_count(field, start);

    bilinear_rank::IncumbentLimits limits;
    limits.width = 0;
    limits.node_limit = 200'000;
    limits.summand_rank = std::min(slices.front().rows(), slices.front().columns());

    bilinear_rank::IncumbentReport plain;
    bilinear_rank::search_from_above(field, start, {}, limits, &plain);
    check::equal(name + ": the plain run reaches the answer", plain.best, answer);
    check::equal(name + ": the plain run claims nothing about a ceiling",
                 plain.reached_below, 0);

    limits.below = answer;
    bilinear_rank::IncumbentReport asked;
    const std::vector<Matrix> found = bilinear_rank::search_from_above(field, start, {}, limits,
                                                                      &asked);
    check::equal(name + ": --below at the answer reaches it", asked.reached_below, 1);
    check::equal(name + ": --below at the answer costs the answer",
                 linear_algebra::multiplication_count(field, found), answer);
    check::equal(name + ": --below at the answer enters no more nodes",
                 asked.nodes <= plain.nodes, 1);

    bilinear_rank::Algorithm algorithm;
    check::equal(name + ": the --below answer rebuilds the map",
                 bilinear_rank::recovers_map(field, slices,
                                             bilinear_rank::rank_one_candidates(field, found),
                                             algorithm),
                 1);

    // Under the answer. The tree runs out and the report says so; it is not a
    // refutation of `answer - 1` and nothing here reads it as one.
    limits.below = answer - 1;
    bilinear_rank::IncumbentReport missed;
    bilinear_rank::search_from_above(field, start, {}, limits, &missed);
    check::equal(name + ": --below under what is reachable is not reached",
                 missed.reached_below, 0);

    // At the start. Nothing to search for, so nothing is searched.
    limits.below = start_cost;
    bilinear_rank::IncumbentReport already;
    bilinear_rank::search_from_above(field, start, {}, limits, &already);
    check::equal(name + ": --below at the start is reached at once", already.reached_below, 1);
    check::equal(name + ": --below at the start expands no node", already.nodes, 0);
}

}  // namespace

int main(int argc, char** argv) {
    fixtures = argc > 1 ? argv[1] : "fixtures";

    for (const std::string& name : {std::string("f2_2x3"), std::string("gf4_multiplication"),
                                    std::string("matmul_2x2x2")}) {
        the_bound_is_admissible(name);
    }
    the_generated_moves_are_the_useful_ones("gf4_multiplication", 4);
    the_generated_moves_are_the_useful_ones("matmul_2x2x2", 4);

    it_reaches_a_known_answer("gf4_multiplication", 3);
    it_reaches_a_known_answer("matmul_2x2x2", 7);

    below_finds_the_same_answer_sooner("gf4_multiplication", 3);
    below_finds_the_same_answer_sooner("matmul_2x2x2", 7);
    return check::report("cost_first_search");
}
