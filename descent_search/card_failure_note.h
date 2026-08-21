#pragma once

#include "card_failure.h"
#include "report.h"

/// Saying, once and at the end, that a card fell over and the host answered.
///
/// **Separate from [`card_failure.h`](card_failure.h), which records and never
/// prints.** A library that writes to a stream cannot be used by a command that
/// pipes its results, so the record is kept there and the sentence is written
/// here, in the one place every command that can reach a card includes.
///
/// One file rather than the same four lines in four `*_main.cpp`s, because the
/// sentence is not the point — the *reason* is, and a reason copied four times is
/// a reason that will be corrected in one of them. The reason: a card that
/// silently stopped being used is a run that got mysteriously slower, and the
/// answer it produced is the same answer, so the only thing to report is the
/// clock.
namespace bilinear_rank {

/// Nothing at all when the card never failed, which is the ordinary case and
/// includes every machine that has no card.
inline void note_if_the_card_failed() {
    if (card_failure().empty()) return;
    cli::note() << "the card failed and the host answered instead: " << card_failure();
}

}  // namespace bilinear_rank
