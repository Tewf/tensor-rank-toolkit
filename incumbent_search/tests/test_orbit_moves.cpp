/// That quotienting a node's moves changes what the search costs and not what it
/// answers.
///
/// The quotient is the one kind of change nothing downstream catches. A group
/// that does not stabilise the node's span, or a representative rule that drops
/// a whole orbit, removes the move that would have paid; the run then reports a
/// worse cost with no sign that anything went wrong, exactly as
/// [`../../orbit_reduction/README.md`](../../orbit_reduction/README.md) says of
/// the exact search. So the quotiented run is held against the unquotiented one
/// on every fixture where both finish:
///
/// 1. **the same cost**, and a decomposition that rebuilds the map;
/// 2. **no more nodes**, which is the only thing the quotient is for;
/// 3. **over GF(3)**, that it runs at all. The moves are outer products nothing
///    normalised, and `permutation_action_on` looks images up after scaling them
///    to their scalar class, so an unscaled list is one the group provably leaves
///    and the run stops with "the pool is not closed under this group". Over
///    GF(2) that branch is invisible, which is why the case here is over GF(3).
#include <cstddef>
#include <string>
#include <vector>

#include "algorithm_recovery.h"
#include "candidate_pool.h"
#include "check.h"
#include "cost_first_search.h"
#include "group_construction.h"
#include "level_lowering_moves.h"
#include "measures.h"
#include "minimum_weight_basis.h"
#include "orbit_moves.h"
#include "tensor_file.h"

namespace {

using bilinear_rank::Automorphism;
using bilinear_rank::Field;
using bilinear_rank::Matrix;

std::string fixtures;

bilinear_rank::IncumbentLimits branch_and_bound(const std::vector<Matrix>& slices) {
    bilinear_rank::IncumbentLimits limits;
    limits.width = 0;  // every child, so the two trees are comparable rather than two beams
    limits.node_limit = 200'000;
    limits.summand_rank = std::min(slices.front().rows(), slices.front().columns());
    return limits;
}

/// The two runs of one question, and the three things asked of the pair.
void the_quotient_keeps_the_answer(const std::string& name,
                                   const std::vector<Matrix>& slices, const Field& field,
                                   const std::vector<Automorphism>& ambient) {
    const std::vector<Matrix> start = bilinear_rank::minimum_weight_basis(field, slices);
    bilinear_rank::IncumbentLimits limits = branch_and_bound(slices);

    bilinear_rank::IncumbentReport whole;
    bilinear_rank::search_from_above(field, start, {}, limits, &whole);

    limits.quotient_moves = true;
    bilinear_rank::IncumbentReport quotiented;
    const std::vector<Matrix> found =
        bilinear_rank::search_from_above(field, start, {}, limits, &quotiented, ambient);

    check::equal(name + ": the quotiented search reaches the same cost", quotiented.best,
                 static_cast<long long>(whole.best));
    check::equal(name + ": and it costs what its answer costs",
                 linear_algebra::multiplication_count(field, found),
                 static_cast<long long>(quotiented.best));
    bilinear_rank::Algorithm algorithm;
    check::equal(name + ": the quotiented answer rebuilds the map",
                 bilinear_rank::recovers_map(
                     field, slices, bilinear_rank::rank_one_candidates(field, found), algorithm),
                 1);
    check::equal(name + ": the quotient enters no more nodes", quotiented.nodes <= whole.nodes, 1);
    check::equal(name + ": and it did offer fewer moves than it generated",
                 quotiented.moves_entered <= quotiented.moves_offered, 1);
}

/// A pencil of 2x2 matrices over GF(3), built here rather than added as a
/// fixture: the whole point is a field where scaling is not the identity, and no
/// fixture is both over GF(3) and small enough for `all_automorphisms`.
std::vector<Matrix> gf3_pencil() {
    Matrix first(2, 2);
    first(0, 0) = 1;
    first(1, 1) = 1;
    Matrix second(2, 2);
    second(0, 1) = 1;
    second(1, 0) = 2;
    return {first, second};
}

/// Claim 3 on its own, at the level of the routine rather than of a search: the
/// generated moves of a GF(3) span survive being quotiented.
void the_move_set_is_closed_over_gf3() {
    const Field field(3);
    const std::vector<Matrix> slices = gf3_pencil();
    const std::vector<Matrix> basis = bilinear_rank::minimum_weight_basis(field, slices);
    const std::vector<std::size_t> known = bilinear_rank::span_element_ranks(field, basis);
    const std::vector<Matrix> moves = bilinear_rank::level_lowering_moves(field, basis, known, 2);
    const std::vector<Automorphism> ambient = bilinear_rank::all_automorphisms(field, 2, 2);

    check::equal("GF(3): there are moves to quotient", moves.size() > 0, 1);
    check::equal("GF(3): there is a group to quotient by", ambient.size() > 0, 1);

    // The case has to actually reach the branch it exists for. A move set that
    // happened to be normalised already would pass everything below whether the
    // scaling were there or not, and the check would be theatre.
    std::size_t unnormalised = 0;
    for (const Matrix& move : moves) {
        for (std::size_t entry = 0; entry < move.entry_count(); ++entry) {
            if (field.isZero(move.data()[entry])) continue;
            unnormalised += field.isOne(move.data()[entry]) ? 0 : 1;
            break;
        }
    }
    check::equal("GF(3): moves whose leading entry is not one", unnormalised > 0, 1);

    std::size_t stabiliser = 0;
    const std::vector<Matrix> kept =
        bilinear_rank::moves_up_to_symmetry(field, basis, ambient, moves, &stabiliser);
    check::equal("GF(3): the stabiliser is not empty", stabiliser > 0, 1);
    check::equal("GF(3): the quotient keeps no more than it was given",
                 kept.size() <= moves.size(), 1);
    check::equal("GF(3): and it keeps at least one", kept.size() > 0, 1);

    // Every kept move is one of the moves it was given, entry for entry. A
    // routine that returned the scaled copies would pass every count above.
    std::size_t strangers = 0;
    for (const Matrix& keeper : kept) {
        bool found = false;
        for (const Matrix& move : moves) {
            if (move.entry_count() != keeper.entry_count()) continue;
            bool same = true;
            for (std::size_t entry = 0; entry < move.entry_count(); ++entry) {
                same = same && move.data()[entry] == keeper.data()[entry];
            }
            found = found || same;
        }
        strangers += found ? 0 : 1;
    }
    check::equal("GF(3): kept moves that were never offered", strangers, 0);

    // **The covering property itself**, which is what the search rests on and
    // what every count above would survive losing: no orbit may be dropped. The
    // ambient group here is a whole group rather than a generating set, so its
    // stabiliser is a subgroup and one application of one element reaches the
    // whole orbit — this is the property and not an approximation of it.
    const std::vector<Automorphism> elements =
        bilinear_rank::stabiliser_of(field, basis, ambient);
    std::size_t uncovered = 0;
    for (const Matrix& move : moves) {
        const Matrix wanted = bilinear_rank::scalar_class_representative(field, move);
        bool covered = false;
        for (const Matrix& keeper : kept) {
            for (const Automorphism& sigma : elements) {
                const Matrix image = bilinear_rank::scalar_class_representative(
                    field, bilinear_rank::act_on(field, sigma, keeper));
                bool same = image.entry_count() == wanted.entry_count();
                for (std::size_t entry = 0; same && entry < wanted.entry_count(); ++entry) {
                    same = image.data()[entry] == wanted.data()[entry];
                }
                covered = covered || same;
            }
        }
        uncovered += covered ? 0 : 1;
    }
    check::equal("GF(3): moves no kept move reaches", uncovered, 0);
}

}  // namespace

int main(int argc, char** argv) {
    fixtures = argc > 1 ? argv[1] : "fixtures";

    {
        const linear_algebra::Tensor tensor =
            linear_algebra::read_tensor_file(fixtures + "/matmul_2x2x2.tensor");
        const Field field(tensor.characteristic);
        the_quotient_keeps_the_answer(
            "matmul_2x2x2", tensor.slices, field,
            bilinear_rank::matrix_multiplication_symmetry_generators(field, 2, 2, 2));
    }
    {
        const linear_algebra::Tensor tensor =
            linear_algebra::read_tensor_file(fixtures + "/gf4_multiplication.tensor");
        const Field field(tensor.characteristic);
        the_quotient_keeps_the_answer("gf4_multiplication", tensor.slices, field,
                                      bilinear_rank::all_automorphisms(field, 2, 2));
    }
    {
        const Field field(3);
        the_quotient_keeps_the_answer("gf3_pencil", gf3_pencil(), field,
                                      bilinear_rank::all_automorphisms(field, 2, 2));
    }
    the_move_set_is_closed_over_gf3();

    // A group of nothing is what a run that never asked for one passes, and it
    // has to leave the search exactly as it was rather than quotient by the
    // trivial group and cost a stabiliser scan a node.
    {
        const linear_algebra::Tensor tensor =
            linear_algebra::read_tensor_file(fixtures + "/matmul_2x2x2.tensor");
        const Field field(tensor.characteristic);
        const std::vector<Matrix> start =
            bilinear_rank::minimum_weight_basis(field, tensor.slices);
        bilinear_rank::IncumbentLimits limits = branch_and_bound(tensor.slices);
        bilinear_rank::IncumbentReport whole;
        bilinear_rank::search_from_above(field, start, {}, limits, &whole);

        limits.quotient_moves = true;
        bilinear_rank::IncumbentReport asked;
        bilinear_rank::search_from_above(field, start, {}, limits, &asked, {});
        check::equal("no group: the same nodes", asked.nodes, static_cast<long long>(whole.nodes));
        check::equal("no group: the same children", asked.children,
                     static_cast<long long>(whole.children));
        check::equal("no group: every move still entered", asked.moves_entered,
                     static_cast<long long>(asked.moves_offered));
    }
    return check::report("orbit_moves");
}
