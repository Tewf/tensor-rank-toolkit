#include "span_ranks_backend.h"

#include <exception>
#include <mutex>
#include <vector>

#include "card_failure.h"
#include "span_ranks.h"

/// The rank kernel offered to `span_element_ranks`, behind the seam it takes.
///
/// [`span_ranks.h`](span_ranks.h) is the call a measurement makes; this is the
/// same kernel behind the question a *search* asks, which is not the same
/// question. A measurement hands over a range and takes seconds. A search hands
/// over a span and takes every rank, and needs two things a measurement does not:
/// a shape with no kernel has to be declined rather than thrown at, and a CUDA
/// call that failed has to become the host answering rather than the run
/// stopping.
namespace gpu_leaf {

namespace {

/// One span on the card at a time, whatever `--threads` says.
///
/// **The slices live in `__constant__` memory and there is one of them.** Two
/// workers uploading two spans to the same symbol would each rank the other's,
/// which is a wrong rank in every slot with nothing downstream to catch it. A
/// card is one resource, so serialising costs nothing that was ever available.
std::mutex the_card;

/// The span as the kernel reads it. Copied because it changes at every node and
/// is a few kilobytes; there is no per-shape table here to cache, which is the
/// one way this is simpler than the leaf backend.
SpanQuestion question_from(const bilinear_rank::PackedSpan& span) {
    SpanQuestion question;
    question.rows = span.rows;
    question.columns = span.columns;
    question.words = span.words;
    question.slices = span.slices;
    question.slice_rows.assign(span.slice_rows, span.slice_rows + span.slices * span.words);
    return question;
}

/// Everything below may throw, and none of it may throw at the search.
///
/// `cuda_guard.cuh` turns every failed call into an exception naming the call,
/// which is what a measurement wants. A search wants the host to answer and the
/// run to say so afterwards, because a card that failed is a slower run and never
/// a wrong one.
template <typename Answer>
bool answered_or_the_host(const Answer& answer) try {
    return answer();
} catch (const std::exception& failure) {
    bilinear_rank::note_card_failure(failure.what());
    return false;
}

bool ranks_on_the_card(const bilinear_rank::PackedSpan& span, std::uint64_t elements,
                       std::vector<std::size_t>& ranks) {
    if (span.slice_rows == nullptr || span.slices == 0) return false;
    // The caller sized it and reads every slot, so a range that does not cover it
    // is declined rather than half filled.
    if (ranks.size() != elements) return false;
    if (elements != (std::uint64_t(1) << span.slices)) return false;

    const std::lock_guard<std::mutex> only_one(the_card);
    return answered_or_the_host([&] {
        const GpuRanks answered = rank_span_on_gpu(question_from(span), 0, elements);
        if (answered.ranks.size() != ranks.size()) return false;
        // Widened only on the way out, so a caller handed false never sees a
        // vector half in one representation.
        for (std::size_t index = 0; index < ranks.size(); ++index) {
            ranks[index] = answered.ranks[index];
        }
        return true;
    });
}

const bilinear_rank::SpanRanksOnCard offered{&span_ranks_handle, &ranks_on_the_card};

}  // namespace

const bilinear_rank::SpanRanksOnCard& span_ranks_backend() { return offered; }

}  // namespace gpu_leaf
