/// The closed form has to be right about the orbits, not merely produce the
/// right number of them.
///
/// A cube pins the first term of a decomposition. If the representatives missed
/// an orbit, every cube would refuse the decompositions living there and the
/// search would report a lower bound that is false, with nothing downstream able
/// to catch it. So three things are checked here, and the third is the one that
/// would actually catch it: their orbits must be disjoint, together they must be
/// the whole pool, and **a formula split into cubes must give the same verdict as
/// the same formula whole**, on a map whose rank is known.
#include <iostream>
#include <numeric>
#include <string>

#include "automorphism.h"
#include "binary_encoding.h"
#include "candidate_pool.h"
#include "check.h"
#include "group_construction.h"
#include "map_construction.h"
#include "orbit_cubes.h"
#include "pool_orbits.h"
#include "solver_process.h"
#include "tensor_file.h"

namespace {

void check_representatives_partition_the_pool(const bilinear_rank::Field& field, std::size_t rows,
                                              std::size_t inner, std::size_t columns) {
    const std::string what = "<" + std::to_string(rows) + "," + std::to_string(inner) + "," +
                             std::to_string(columns) + ">";
    const std::vector<bilinear_rank::Matrix> pool =
        bilinear_rank::all_rank_one_maps(field, rows * inner, inner * columns);
    const std::vector<bilinear_rank::Automorphism> generators =
        bilinear_rank::matrix_multiplication_symmetry_generators(field, rows, inner, columns);
    const std::vector<std::vector<std::uint32_t>> action =
        bilinear_rank::permutation_action_on(field, generators, pool);

    // Where each pool element sits, so a representative can be looked up.
    std::vector<std::uint32_t> everything(pool.size());
    std::iota(everything.begin(), everything.end(), std::uint32_t(0));
    const std::vector<std::uint32_t> computed = bilinear_rank::orbit_representatives(action, everything);

    const std::vector<bilinear_rank::Matrix> closed =
        bilinear_rank::matrix_multiplication_orbit_representatives(field, rows, inner, columns);
    check::equal(what + " closed form counts the orbits",
                 static_cast<long long>(closed.size()),
                 static_cast<long long>(computed.size()));

    // Every representative's orbit, marked. Disjoint means no element is marked
    // twice; covering means none is left unmarked.
    std::vector<int> marked_by(pool.size(), -1);
    for (std::size_t index = 0; index < closed.size(); ++index) {
        std::size_t start = pool.size();
        for (std::size_t position = 0; position < pool.size(); ++position) {
            bool same = true;
            for (std::size_t entry = 0; entry < pool[position].entry_count() && same; ++entry) {
                same = field.areEqual(pool[position].data()[entry], closed[index].data()[entry]);
            }
            if (same) { start = position; break; }
        }
        if (start == pool.size()) {
            std::cout << "  FAIL  " << what << ": a representative is not in the pool\n";
            ++check::failure_count;
            return;
        }

        std::vector<std::uint32_t> frontier{static_cast<std::uint32_t>(start)};
        marked_by[start] = static_cast<int>(index);
        while (!frontier.empty()) {
            const std::uint32_t reached = frontier.back();
            frontier.pop_back();
            for (const std::vector<std::uint32_t>& permutation : action) {
                const std::uint32_t image = permutation[reached];
                if (marked_by[image] == static_cast<int>(index)) continue;
                if (marked_by[image] != -1) {
                    std::cout << "  FAIL  " << what << ": two representatives share an orbit\n";
                    ++check::failure_count;
                    return;
                }
                marked_by[image] = static_cast<int>(index);
                frontier.push_back(image);
            }
        }
    }

    std::size_t uncovered = 0;
    for (const int owner : marked_by) {
        if (owner == -1) ++uncovered;
    }
    check::equal(what + " orbits cover the pool", static_cast<long long>(uncovered), 0);
}

linear_algebra::Tensor matrix_multiplication(std::size_t rows, std::size_t inner,
                                             std::size_t columns) {
    linear_algebra::Tensor tensor;
    tensor.characteristic = 2;
    tensor.slices = bilinear_rank::matrix_multiplication_tensor(rows, inner, columns);
    return tensor;
}

/// A cube must pin every coordinate of both operands, once each.
void check_cubes_are_well_formed(const bilinear_rank::Field& field) {
    const std::size_t rows = 2, inner = 2, columns = 2;
    std::vector<int> left(rows * inner), right(inner * columns);
    std::iota(left.begin(), left.end(), 1);
    std::iota(right.begin(), right.end(), static_cast<int>(left.size()) + 1);

    const std::vector<std::vector<int>> cubes = bilinear_rank::orbit_cubes(
        field, matrix_multiplication(rows, inner, columns).slices, rows, inner, columns, left,
        right);
    check::equal("<2,2,2> one cube per orbit", static_cast<long long>(cubes.size()), 5);
    for (const std::vector<int>& cube : cubes) {
        check::equal("a cube pins every coordinate", static_cast<long long>(cube.size()),
                     static_cast<long long>(left.size() + right.size()));
    }
}

/// A shape the caller merely asserts is the failure mode this refuses.
void check_a_wrong_shape_is_refused(const bilinear_rank::Field& field) {
    std::vector<int> left(4), right(4);
    std::iota(left.begin(), left.end(), 1);
    std::iota(right.begin(), right.end(), 5);

    bool refused = false;
    try {
        // The polynomial product of two quadratics is 4x4x... and is not <2,2,2>.
        bilinear_rank::orbit_cubes(field, bilinear_rank::polynomial_multiplication_tensor(2, 2), 2,
                                  2, 2, left, right);
    } catch (const std::exception&) {
        refused = true;
    }
    check::equal("a tensor that is not the named product is refused", refused ? 1 : 0, 1);
}

enum class Verdict { Yes, No, Unknown };

const char* spelled(Verdict verdict) {
    return verdict == Verdict::Yes ? "yes" : (verdict == Verdict::No ? "no" : "unknown");
}

/// Solve one formula, with `cube` asserted as unit clauses when it is not empty.
Verdict solve_with(const satisfiability::SatSolver& solver,
                   const satisfiability::BinaryEncoding& encoding, const std::vector<int>& cube) {
    linear_algebra::Cnf formula = encoding.formula;
    for (const int literal : cube) formula.add_clause({literal});

    const satisfiability::SolverRun run = satisfiability::solve(formula, solver, 2048, 120);
    if (!run.answered) return Verdict::Unknown;
    return run.satisfiable ? Verdict::Yes : Verdict::No;
}

/// The union over the cubes, which is what solving them all and taking the
/// disjunction means. An unanswered cube leaves the union unknown rather than
/// no, because a cube that gave up has refuted nothing.
Verdict solve_over_cubes(const satisfiability::SatSolver& solver,
                         const satisfiability::BinaryEncoding& encoding,
                         const std::vector<std::vector<int>>& cubes) {
    bool anything_unknown = false;
    for (const std::vector<int>& cube : cubes) {
        const Verdict verdict = solve_with(solver, encoding, cube);
        if (verdict == Verdict::Yes) return Verdict::Yes;
        if (verdict == Verdict::Unknown) anything_unknown = true;
    }
    return anything_unknown ? Verdict::Unknown : Verdict::No;
}

/// The check that would catch a wrong symmetry, and the only one that would.
///
/// `<2,2,2>` has rank exactly 7, so the whole formula is satisfiable at 7 and
/// unsatisfiable at 6. Splitting it into cubes must not move either answer. A
/// cube set that missed an orbit would still say no at 6, which is why the yes at
/// 7 is the one that matters: it is where an over-strong constraint shows up.
///
/// `break_symmetry` is the case this test was missing and the one the bug lived
/// in. Cubes alone are sound and the term ordering alone is sound; their
/// conjunction is not, because a cube pins term 0 to an orbit representative and
/// the ordering demands term 0 be lexicographically least, which a representative
/// need not be. So the cube runs get `first_term_pinned`, and the numbering still
/// matches the unpinned encoding because that flag only skips an ordering whose
/// auxiliary variables come after every operand variable.
void check_cubes_do_not_change_the_answer(const satisfiability::SatSolver& solver,
                                          const bilinear_rank::Field& field, std::size_t rows,
                                          std::size_t inner, std::size_t columns,
                                          std::size_t products, bool break_symmetry,
                                          Verdict expected) {
    const linear_algebra::Tensor tensor = matrix_multiplication(rows, inner, columns);
    const satisfiability::BinaryEncoding whole_form =
        satisfiability::encode_binary_rank_at_most(tensor, products, break_symmetry, false);
    const satisfiability::BinaryEncoding cube_form =
        satisfiability::encode_binary_rank_at_most(tensor, products, break_symmetry, true);

    const std::vector<std::vector<int>> cubes = bilinear_rank::orbit_cubes(
        field, tensor.slices, rows, inner, columns, cube_form.left, cube_form.right);

    const Verdict whole = solve_with(solver, whole_form, {});
    const Verdict split = solve_over_cubes(solver, cube_form, cubes);

    const std::string shape = "<" + std::to_string(rows) + "," + std::to_string(inner) + "," +
                              std::to_string(columns) + ">";
    const std::string what = shape + " at " + std::to_string(products) + " products" +
                             (break_symmetry ? " with the term ordering on" : "");
    if (whole == Verdict::Unknown || split == Verdict::Unknown) {
        std::cout << "  skip  " << what << ": the solver gave no verdict\n";
        return;
    }
    if (whole != expected) {
        std::cout << "  FAIL  " << what << " whole = " << spelled(whole) << ", expected "
                  << spelled(expected) << "\n";
        ++check::failure_count;
        return;
    }
    check::equal(what + ": cubes agree with the whole formula", split == whole ? 1 : 0, 1);
}

}  // namespace

/// The structural checks are seconds and the solver ones are minutes, so the
/// second half is asked for rather than assumed: `--with-solver` runs it, and
/// CTest registers that as the slow test.
int main(int argc, char** argv) {
    const bool with_solver = argc > 1 && std::string(argv[1]) == "--with-solver";
    const bilinear_rank::Field over_two(2);

    if (!with_solver) {
        check_representatives_partition_the_pool(over_two, 2, 2, 2);
        check_representatives_partition_the_pool(over_two, 2, 2, 3);
        check_cubes_are_well_formed(over_two);
        check_a_wrong_shape_is_refused(over_two);
        return check::report("orbit cubes");
    }

    const satisfiability::SatSolver solver = satisfiability::find_sat_solver(true);
    if (!solver.found) {
        std::cout << "  skip  cubes against the whole formula: no solver on PATH\n";
    } else {
        check_cubes_do_not_change_the_answer(solver, over_two, 2, 2, 2, 7, false, Verdict::Yes);
        check_cubes_do_not_change_the_answer(solver, over_two, 2, 2, 2, 6, false, Verdict::No);
        // The conjunction, which is what the two earlier lines never reached.
        check_cubes_do_not_change_the_answer(solver, over_two, 2, 2, 2, 7, true, Verdict::Yes);
        check_cubes_do_not_change_the_answer(solver, over_two, 2, 2, 2, 6, true, Verdict::No);
        // A second shape, because a cube set right for one <n,m,k> and wrong for
        // another would pass everything above. <2,2,3> has rank 11, so 7 is a
        // refutation and a cheap one, far below the rank.
        check_cubes_do_not_change_the_answer(solver, over_two, 2, 2, 3, 7, true, Verdict::No);
    }
    return check::report("orbit cubes against the whole formula");
}
