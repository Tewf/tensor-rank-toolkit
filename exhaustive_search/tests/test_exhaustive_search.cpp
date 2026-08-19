/// The exact search, checked against answers that are known independently.
///
/// Karatsuba does 2x2 polynomial multiplication in three products and 2x3 in
/// five, and `2n-1` is a lower bound, so both are provably optimal. If the
/// complete search cannot reproduce them it is not complete.
#include <iostream>
#include <string>

#include "algorithm_recovery.h"
#include "candidate_pool.h"
#include "check.h"
#include "exhaustive_search.h"
#include "parallel.h"
#include "fewest_products.h"
#include "tensor_file.h"

namespace {

/// From nothing: the map's own slices as the starting subspace, so the answer
/// is not conditioned on any heuristic.
void check_from_scratch(const std::string& directory, const std::string& name,
                        long long expected_products) {
    const linear_algebra::Tensor tensor =
        linear_algebra::read_tensor_file(directory + "/" + name + ".tensor");
    const bilinear_rank::Field field(tensor.characteristic);
    const std::vector<bilinear_rank::Matrix> pool =
        bilinear_rank::all_rank_one_maps(field, tensor.rows(), tensor.columns());

    bilinear_rank::SearchBudget budget;
    std::vector<bilinear_rank::Matrix> products;
    const bool found =
        bilinear_rank::fewest_products_by_sweep(field, tensor.slices, pool, budget, products);

    std::cout << name << ": pool " << pool.size() << ", " << budget.nodes_visited << " nodes"
              << (budget.exhausted ? "" : " (BUDGET SPENT)") << "\n";

    if (!found) {
        std::cout << "  FAIL  " << name << ": no decomposition found at all\n";
        ++check::failure_count;
        return;
    }
    check::equal(name + " fewest products", static_cast<long long>(products.size()),
                 expected_products);

    // Every product must be rank one, and together they must compute the map.
    for (const bilinear_rank::Matrix& product : products) {
        if (linear_algebra::rank(field, product) != 1) {
            std::cout << "  FAIL  " << name << ": a product is not rank one\n";
            ++check::failure_count;
            return;
        }
    }
    bilinear_rank::Algorithm algorithm;
    if (!bilinear_rank::recover_algorithm(field, tensor.slices, products, algorithm) ||
        !linear_algebra::spans_all(field, bilinear_rank::map_computed_by(field, algorithm),
                                   tensor.slices)) {
        std::cout << "  FAIL  " << name << ": the products do not compute the map\n";
        ++check::failure_count;
    }
}

/// The two routes to the same number must agree, or the faster one is lying.
void check_routes_agree(const std::string& directory, const std::string& name) {
    const linear_algebra::Tensor tensor =
        linear_algebra::read_tensor_file(directory + "/" + name + ".tensor");
    const bilinear_rank::Field field(tensor.characteristic);
    const std::vector<bilinear_rank::Matrix> pool =
        bilinear_rank::all_rank_one_maps(field, tensor.rows(), tensor.columns());

    bilinear_rank::SearchBudget sweep_budget;
    bilinear_rank::SearchBudget bisect_budget;
    std::vector<bilinear_rank::Matrix> by_sweep;
    std::vector<bilinear_rank::Matrix> by_bisection;
    const bool swept =
        bilinear_rank::fewest_products_by_sweep(field, tensor.slices, pool, sweep_budget, by_sweep);
    const bool bisected = bilinear_rank::fewest_products_by_bisection(field, tensor.slices, pool,
                                                                     bisect_budget, by_bisection);

    check::equal(name + " sweep and bisection both succeed",
                 (swept && bisected) ? 1 : 0, 1);
    if (swept && bisected) {
        check::equal(name + " sweep and bisection agree",
                     static_cast<long long>(by_sweep.size()),
                     static_cast<long long>(by_bisection.size()));
        std::cout << "        sweep " << sweep_budget.nodes_visited << " nodes, bisection "
                  << bisect_budget.nodes_visited << "\n";
    }
}

/// A "no" that ran to exhaustion is a fact about the map rather than about the
/// searcher, and it is the only lower bound this work has. The `12 <= rank` the
/// README quotes for F2 5x5 rests entirely on these, so they are re-run rather
/// than remembered, and `exhausted` is checked alongside, because a search
/// that gave up would report the same "no".
void check_no_algorithm_with(const std::string& directory, const std::string& name,
                             std::size_t products_wanted) {
    const linear_algebra::Tensor tensor =
        linear_algebra::read_tensor_file(directory + "/" + name + ".tensor");
    const bilinear_rank::Field field(tensor.characteristic);
    const std::vector<bilinear_rank::Matrix> pool =
        bilinear_rank::all_rank_one_maps(field, tensor.rows(), tensor.columns());

    bilinear_rank::SearchBudget budget;
    std::vector<bilinear_rank::Matrix> products;
    const bool found = bilinear_rank::expand_subspace(field, tensor.slices, pool, 0,
                                                      products_wanted, budget, products);

    const std::string what = name + " has no " + std::to_string(products_wanted) + "-product algorithm";
    std::cout << what << ": " << budget.nodes_visited << " nodes\n";
    check::equal(what, found ? 1 : 0, 0);
    check::equal(what + ", exhaustively", budget.exhausted ? 1 : 0, 1);
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: test_exhaustive_search <fixtures-directory> [products-to-rule-out]\n";
        return 2;
    }
    const std::string directory = argv[1];

    // Given a size, run that one exclusion on F2 5x5 and nothing else: ruling
    // out 11 costs 77 seconds, so it is registered separately and labelled slow.
    if (argc > 2) {
        check_no_algorithm_with(directory, "f2_5x5", std::stoul(argv[2]));
        return check::report("exhaustive search, lower bound");
    }

    check_from_scratch(directory, "f2_2x2", 3);  // Karatsuba
    check_from_scratch(directory, "f2_2x3", 5);  // the write-up's worked example

    // The two tensors from outside this repository's own subject whose answers
    // are known independently of it. <2,2,2> is Strassen's seven products and
    // Winograd's proof that six are not enough, so getting 7 here and a NO at 6
    // below is the whole of a classical result, reproduced from the tensor.
    // The W state is the textbook rank-3 tensor of border rank 2.
    check_from_scratch(directory, "matmul_2x2x2", 7);
    check_from_scratch(directory, "w_state", 3);
    check_routes_agree(directory, "f2_2x2");
    check_routes_agree(directory, "f2_2x3");

    // The lower bound on F2 5x5. Ruling out 11 is the slow half of it and runs
    // as its own test; these two are a sixth of a second between them.
    check_no_algorithm_with(directory, "f2_5x5", 9);
    check_no_algorithm_with(directory, "f2_5x5", 10);
    check_no_algorithm_with(directory, "matmul_2x2x2", 6);  // Winograd's bound
    check_no_algorithm_with(directory, "w_state", 2);

    // A search that gives up must say so rather than report "no solution".
    {
        const linear_algebra::Tensor tensor =
            linear_algebra::read_tensor_file(directory + "/f2_5x5.tensor");
        const bilinear_rank::Field field(tensor.characteristic);
        const std::vector<bilinear_rank::Matrix> pool =
            bilinear_rank::all_rank_one_maps(field, tensor.rows(), tensor.columns());
        bilinear_rank::SearchBudget budget{/*node_limit=*/2000};
        std::vector<bilinear_rank::Matrix> products;
        bilinear_rank::expand_subspace(field, tensor.slices, pool, 0, 13, budget, products);
        check::equal("a spent budget reports itself", budget.exhausted ? 1 : 0, 0);
    }

    // The same, for the other limit, and the reason it exists: the node limit
    // bounds how many leaves are reached and nothing inside one, so a leaf big
    // enough was unbounded by anything the command line could say. A leaf given
    // one element must abandon that leaf, and abandoning it must withdraw the
    // answer rather than report "no solution": returning fewer maps than the
    // target reads to the caller exactly as a refutation would, and the only
    // thing keeping the two apart is `exhausted`.
    {
        const linear_algebra::Tensor tensor =
            linear_algebra::read_tensor_file(directory + "/f2_5x5.tensor");
        const bilinear_rank::Field field(tensor.characteristic);
        const std::vector<bilinear_rank::Matrix> pool =
            bilinear_rank::all_rank_one_maps(field, tensor.rows(), tensor.columns());

        bilinear_rank::SearchBudget budget{/*node_limit=*/1'000'000, /*leaf_limit=*/1};
        std::vector<bilinear_rank::Matrix> products;
        const bool found =
            bilinear_rank::expand_subspace(field, tensor.slices, pool, 0, 11, budget, products);
        check::equal("a leaf given one element finds nothing", found ? 1 : 0, 0);
        check::equal("and says the leaf limit stopped it", budget.leaf_abandoned ? 1 : 0, 1);
        check::equal("so the run is undecided, not a refutation", budget.exhausted ? 1 : 0, 0);

        // And the guard is off the moment the leaf can afford itself: 11 is
        // refuted on this fixture, and that answer has to survive the change.
        bilinear_rank::SearchBudget whole{/*node_limit=*/1'000'000, /*leaf_limit=*/1'000'000};
        std::vector<bilinear_rank::Matrix> none;
        bilinear_rank::expand_subspace(field, tensor.slices, pool, 0, 11, whole, none);
        check::equal("an affordable leaf still refutes", whole.exhausted ? 1 : 0, 1);
        check::equal("and reports no leaf abandoned", whole.leaf_abandoned ? 1 : 0, 0);
    }

    // One thread and many must answer a leaf at the root by the same route.
    //
    // The parallel branch used to call the pool scan directly there, while one
    // thread went through `rank_one_basis_of`, which picks the subspace walk
    // wherever that is cheaper and the bit-packed route over GF(2). Both routes
    // give the same answer, so the *answer* cannot catch this. The leaf budget
    // can: the walk of a 2-dimensional span over GF(2) is 4 elements and the
    // pool is 9, so a leaf limit between the two completes down one route and is
    // abandoned down the other.
    //
    // The root is a leaf exactly when the target equals the span's dimension,
    // which is the question "is this tensor's rank its own span dimension".
    {
        const linear_algebra::Tensor tensor =
            linear_algebra::read_tensor_file(directory + "/w_state.tensor");
        const bilinear_rank::Field field(tensor.characteristic);
        const std::vector<bilinear_rank::Matrix> pool =
            bilinear_rank::all_rank_one_maps(field, tensor.rows(), tensor.columns());
        const std::size_t at_the_root =
            linear_algebra::span_of(field, tensor.slices).dimension();

        std::vector<int> abandoned;
        for (std::size_t workers : {std::size_t(1), std::size_t(4)}) {
            bilinear_rank::set_worker_count(workers);
            bilinear_rank::SearchBudget budget{/*node_limit=*/1'000'000, /*leaf_limit=*/4};
            std::vector<bilinear_rank::Matrix> products;
            bilinear_rank::expand_subspace(field, tensor.slices, pool, 0, at_the_root, budget,
                                           products);
            abandoned.push_back(budget.leaf_abandoned ? 1 : 0);
        }
        bilinear_rank::set_worker_count(1);  // process-wide: leave it as it was

        check::equal("the root really is a leaf here", at_the_root, std::size_t(2));
        check::equal("one thread takes the cheap leaf route", abandoned[0], 0);
        check::equal("and four threads take the same one", abandoned[1], abandoned[0]);
    }

    return check::report("exhaustive search");
}
