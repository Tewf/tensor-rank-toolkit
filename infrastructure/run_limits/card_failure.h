#pragma once

#include <string>

/// The first thing that went wrong on the card, if anything has.
///
/// **Every seam that offers work to a card needs this, and there is one card**,
/// so it is one record rather than one per seam:
/// [`../../methods/bilinear_rank/exhaustive/gf2_leaf_on_card.h`](../../methods/bilinear_rank/exhaustive/gf2_leaf_on_card.h)
/// for a leaf and
/// [`../../methods/bilinear_rank/greedy_heuristic/span_ranks_on_card.h`](../../methods/bilinear_rank/greedy_heuristic/span_ranks_on_card.h)
/// for a span's ranks both write here. It sits in this module because that is
/// the one both can reach: `exhaustive` links `greedy_heuristic` and not the
/// other way round.
///
/// **A failed CUDA call is never an answer that changed.** Every backend catches
/// it, notes it here and returns false, and the host answers the same question
/// the same way; what changed is the clock. So it is kept rather than printed
/// (a library that writes to a stream cannot be used by a command that pipes its
/// results), and a command prints it once at the end, because a card that
/// silently stopped being used is a run that got mysteriously slower.
namespace bilinear_rank {

/// Record a failure, if none has been recorded yet.
///
/// **The first and not the last**, because the first is the one with a cause a
/// reader can act on: everything after a card falls over tends to be the same
/// sticky error repeated once per launch.
void note_card_failure(const std::string& what);

/// What that was, in the words the runtime used. Empty while nothing has gone
/// wrong.
const std::string& card_failure();

}  // namespace bilinear_rank
