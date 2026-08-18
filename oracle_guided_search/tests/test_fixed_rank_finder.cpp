#include <string>

#include "check.h"
#include "descending_sweep.h"
#include "tensor_file.h"
#include "tensor_flattening.h"

/// What the finder must get right, split by whether it needs a solver.
///
/// The checks that need none run always. The ones that do are behind
/// `--with-solver`, following `test_orbit_cubes.cpp`, because a suite that spends
/// minutes is a suite nobody runs.
namespace {

std::string fixture_directory(int argc, char** argv) {
    for (int argument = 1; argument < argc; ++argument) {
        const std::string given = argv[argument];
        if (given.rfind("--", 0) != 0) return given;
    }
    return "fixtures";
}

bilinear_rank::FinderSettings settings_for(const linear_algebra::Tensor& tensor,
                                           const std::vector<std::size_t>& shape) {
    const bilinear_rank::Field field(tensor.characteristic);
    bilinear_rank::FinderSettings settings;
    settings.floor = linear_algebra::flattening_lower_bound(field, tensor.slices);
    settings.matmul_shape = shape;
    settings.candidate_seconds = 60;
    return settings;
}

void without_a_solver(const std::string& directory) {
    const linear_algebra::Tensor strassen =
        linear_algebra::read_tensor_file(directory + "/matmul_2x2x2.tensor");
    check::equal("naive ceiling of <2,2,2>", bilinear_rank::naive_ceiling(strassen), 16);

    const bilinear_rank::FinderSettings settings = settings_for(strassen, {2, 2, 2});
    check::equal("flattening floor of <2,2,2>", settings.floor, 4);

    // Below the floor is the one no this finder is entitled to, and it must cost
    // nothing: no solver is on the path of this call at all.
    const bilinear_rank::FoundAtRank refused = bilinear_rank::find_at_rank(strassen, 3, settings);
    check::equal("k = 3 is below the floor",
                 refused.outcome == bilinear_rank::Outcome::BelowFloor, 1);
    check::equal("and asked nothing", refused.candidates_asked, 0);
}

void with_a_solver(const std::string& directory) {
    const linear_algebra::Tensor strassen =
        linear_algebra::read_tensor_file(directory + "/matmul_2x2x2.tensor");
    const bilinear_rank::FinderSettings settings = settings_for(strassen, {2, 2, 2});

    // Strassen's seven, the answer everything here is calibrated against. The
    // decomposition is multiplied out inside `find_at_rank`, so reaching this line
    // at all is most of the check.
    const bilinear_rank::FoundAtRank found = bilinear_rank::find_at_rank(strassen, 7, settings);
    check::equal("<2,2,2> found at 7", bilinear_rank::was_found(found.outcome), 1);
    check::equal("with seven products", found.algorithm.product_count(), 7);
    check::equal("out of five orbit candidates", found.candidates, 5);

    // The descent must stop above the rank rather than below it. Starting at 8
    // keeps the run short and still crosses the boundary that matters.
    const bilinear_rank::SweepResult sweep =
        bilinear_rank::descend_from_ceiling(strassen, 8, settings);
    check::equal("descending from 8 reaches 7", sweep.upper, 7);
    check::equal("and stops there", sweep.algorithm.product_count(), 7);
}

}  // namespace

int main(int argc, char** argv) {
    const std::string directory = fixture_directory(argc, argv);
    bool with_solver = false;
    for (int argument = 1; argument < argc; ++argument) {
        if (std::string(argv[argument]) == "--with-solver") with_solver = true;
    }

    without_a_solver(directory);
    if (with_solver) with_a_solver(directory);
    return check::report("fixed_rank_finder");
}
