#include "orbit_moves.h"

#include <cstdint>
#include <numeric>

namespace bilinear_rank {

std::vector<Matrix> moves_up_to_symmetry(const Field& field, const std::vector<Matrix>& slices,
                                         const std::vector<Automorphism>& ambient,
                                         const std::vector<Matrix>& moves,
                                         std::size_t* stabiliser_size) {
    if (stabiliser_size != nullptr) *stabiliser_size = 0;
    if (ambient.empty() || moves.empty()) return moves;

    // The group of this node's span, not of the map the run started from.
    const std::vector<Automorphism> stabiliser = stabiliser_of(field, slices, ambient);
    if (stabiliser_size != nullptr) *stabiliser_size = stabiliser.size();
    if (stabiliser.empty()) return moves;

    // Scaled to one representative per scalar class, because that is the list
    // `permutation_action_on` looks images up in and it scales every image
    // before looking one up. The generated moves are outer products of vectors
    // nothing normalised, so over GF(3) and up this list really does contain
    // entries whose leading term is not one, which
    // [`tests/test_orbit_moves.cpp`](tests/test_orbit_moves.cpp) asserts rather
    // than assumes.
    //
    // **Removing this line breaks nothing today, and it is kept anyway.** The
    // move set happens to be closed under scaling — the span contains `λv` with
    // `v`, and the summands of `λv` are `λ` times the summands of `v` — so every
    // class is present in full and every lookup would land. That is an accident
    // of what generates the list, not the contract `permutation_action_on`
    // states, and the routine here takes any list of moves.
    std::vector<Matrix> classes;
    classes.reserve(moves.size());
    for (const Matrix& move : moves) {
        classes.push_back(scalar_class_representative(field, move));
    }

    std::vector<std::uint32_t> everything(moves.size());
    std::iota(everything.begin(), everything.end(), std::uint32_t(0));
    const std::vector<std::uint32_t> representatives =
        orbit_representatives(permutation_action_on(field, stabiliser, classes), everything);

    // The moves as they were offered, not the scaled copies: `V + <g>` does not
    // depend on the scale, but the basis `minimum_weight_basis_with` hands back
    // holds whichever matrix it was given, and a run should return the maps its
    // move generator produced.
    std::vector<Matrix> kept;
    kept.reserve(representatives.size());
    for (const std::uint32_t index : representatives) kept.push_back(moves[index]);
    return kept;
}

}  // namespace bilinear_rank
