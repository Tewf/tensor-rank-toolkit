#include "fixed_rank_finder.h"

#include "binary_encoding.h"
#include "candidate_pool.h"
#include "exhaustive_search.h"
#include "exit_code.h"
#include "measures.h"
#include "orbit_cubes.h"
#include "span_basis.h"
#include "timing.h"

namespace bilinear_rank {

namespace {

/// How many rank-one maps of the tensor's shape there are, without building them.
///
/// `normalised_vectors` is one vector per scalar class and the pool is the grid of
/// outer products of two such lists, so the count is the product of the two
/// lengths. Counted by building the two lists, which are hundreds of short
/// vectors, rather than by exponentiating and hoping the product fits.
std::size_t pool_size(const Field& field, const linear_algebra::Tensor& tensor) {
    const std::size_t left = normalised_vectors(field, tensor.rows()).size();
    const std::size_t right = normalised_vectors(field, tensor.columns()).size();
    return left * right;
}

/// A rank-one basis of the map's own span, when it has one, and empty otherwise.
///
/// This is `independent_rank_one_maps_in` over the span of the slices, which is
/// Gaussian elimination and nothing else, so it is polynomial in the pool. When it
/// returns as many maps as the span has dimensions those maps *are* an algorithm,
/// and it answers every `k` from that dimension up at once with no solver.
///
/// It cannot fire on a matrix multiplication tensor, where the span has one
/// dimension per output entry and the rank is strictly larger. It is here for the
/// shapes where it can, and because a free test that sometimes ends the search is
/// worth asking before a test that costs half a minute a candidate.
std::vector<Matrix> rank_one_basis_of_span(const Field& field,
                                           const linear_algebra::Tensor& tensor,
                                           std::size_t pool_limit) {
    if (tensor.slices.empty()) return {};
    if (pool_size(field, tensor) > pool_limit) return {};

    const std::size_t dimension = linear_algebra::span_of(field, tensor.slices).dimension();
    const std::vector<Matrix> pool =
        all_rank_one_maps(field, tensor.rows(), tensor.columns());
    std::vector<Matrix> found = rank_one_maps_within(field, tensor.slices, pool);
    if (found.size() != dimension) return {};
    return found;
}

/// One cube per orbit of the first term, or none.
///
/// None is the honest answer in three cases and each is a real limit rather than
/// an oversight. No shape named: nothing says which orbits to take. Not GF(2): a
/// cube's literals are numbered for the Boolean encoding and the prime encoder
/// orders and normalises the very term a cube pins, so `decide_rank` refuses it,
/// and it is right to. Not the named product: `orbit_cubes` throws, which is the
/// guard working.
///
/// With no cubes the finder still works and asks one unrestricted question per
/// `k`. That is the whole method minus its commitment, and worth having as the
/// control it is measured against.
std::vector<std::vector<int>> commitment_cubes(const linear_algebra::Tensor& tensor,
                                               std::size_t products,
                                               const FinderSettings& settings) {
    if (settings.matmul_shape.size() != 3) return {};
    if (tensor.characteristic != 2) return {};

    // The encoder allocates term 0's operand variables before any other term's,
    // so this numbering is good for every `products` in a sweep. Asserted in
    // `satisfiability/tests/test_binary_encoding.cpp` rather than read off the
    // loop.
    const satisfiability::BinaryEncoding numbering =
        satisfiability::encode_binary_rank_at_most(tensor, products);
    const Field field(tensor.characteristic);
    return orbit_cubes(field, tensor.slices, settings.matmul_shape[0], settings.matmul_shape[1],
                       settings.matmul_shape[2], numbering.left, numbering.right);
}

/// Drop the terms that are zero.
///
/// A question asked at `k` above the rank is satisfied with terms to spare, and a
/// solver spends them by setting operand vectors to zero. Those terms compute
/// nothing, so dropping them leaves a decomposition of the same map with strictly
/// fewer products, which is a better answer and not merely a tidier one.
///
/// It is also **required**, not cosmetic. `recover_operands` reads each slice as an
/// outer product and refuses anything that is not rank one; a zero matrix is not
/// rank one, so a model with a spare term fails `recovers_map` and the whole find
/// is thrown away as unverified. `find_rank` never meets this because it walks
/// upward and stops at the rank, where nothing is spare. A descending sweep starts
/// at the ceiling, where almost everything is.
std::vector<Matrix> without_zero_terms(const Field& field, const std::vector<Matrix>& terms) {
    std::vector<Matrix> kept;
    kept.reserve(terms.size());
    for (const Matrix& term : terms) {
        if (linear_algebra::nonzero_count(field, term) > 0) kept.push_back(term);
    }
    return kept;
}

/// Multiply the decomposition out against the map, and refuse to report it
/// otherwise.
///
/// The oracle already checked that the model rebuilds the tensor. This checks the
/// stronger and more useful thing: that the three recovered operators compute the
/// map. A decomposition that does not is not a cheaper algorithm, it is a wrong
/// one, and it is cheaper to disbelieve every yes than to publish one.
void verify_or_throw(const Field& field, const linear_algebra::Tensor& tensor,
                     FoundAtRank& result) {
    if (!recovers_map(field, tensor.slices, result.decomposition, result.algorithm)) {
        throw cli::CheckFailed(
            "a certified decomposition did not rebuild the map through recovers_map");
    }
}

}  // namespace

FoundAtRank find_at_rank(const linear_algebra::Tensor& tensor, std::size_t products,
                         const FinderSettings& settings) {
    const cli::Clock::time_point started = cli::Clock::now();
    const Field field(tensor.characteristic);

    FoundAtRank result;
    result.products = products;

    // The one place a no is a no. A bound proved in polynomial time elsewhere
    // makes every smaller `k` impossible, and saying so costs nothing.
    if (settings.floor > products) {
        result.outcome = Outcome::BelowFloor;
        result.seconds = cli::elapsed_seconds(started);
        return result;
    }

    std::vector<Matrix> free_answer = rank_one_basis_of_span(field, tensor, settings.pool_limit);
    if (!free_answer.empty() && free_answer.size() <= products) {
        result.outcome = Outcome::FoundWithoutSolver;
        result.decomposition = std::move(free_answer);
        verify_or_throw(field, tensor, result);
        result.seconds = cli::elapsed_seconds(started);
        return result;
    }

    satisfiability::SolveOptions approach = settings.approach;
    approach.timeout_seconds = settings.candidate_seconds;
    approach.probe_seconds = 0;
    // Never asked for, because a refutation is exactly what this does not wait
    // for, and asking would price every passed-over candidate at proof-writing
    // speed.
    approach.proof_path.clear();
    approach.cubes = commitment_cubes(tensor, products, settings);
    result.candidates = approach.cubes.empty() ? 1 : approach.cubes.size();

    // `decide_rank` stops at the first yes and treats an unknown as neither
    // answer, which is the behaviour wanted here. What it does not do on its own
    // is say which candidate paid, hence the report.
    const satisfiability::Answer answer =
        satisfiability::decide_rank(tensor, products, approach, &result.per_candidate);
    result.candidates_asked =
        approach.cubes.empty() ? 1 : result.per_candidate.verdict.size();
    result.solver_name = answer.solver_name;

    if (answer.verdict == satisfiability::Verdict::Yes) {
        result.outcome = Outcome::Found;
        result.accepted_candidate = result.candidates_asked == 0 ? 0 : result.candidates_asked - 1;
        result.decomposition = without_zero_terms(field, answer.decomposition);
        verify_or_throw(field, tensor, result);
    } else {
        result.outcome = Outcome::NotFound;
    }
    // Kept rather than collapsed into the outcome. A `No` here means every
    // candidate refused with none timing out, and since the cubes cover every
    // first term up to the group that is a genuine refutation, on the solver's
    // word and with no DRAT proof asked for. `Unknown` means at least one
    // candidate was passed over and nothing is proved in either direction. Both
    // arrive as `NotFound`, and only this says which.
    result.oracle_verdict = answer.verdict;

    result.seconds = cli::elapsed_seconds(started);
    return result;
}

}  // namespace bilinear_rank
