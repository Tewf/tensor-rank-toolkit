#include "span_ranks_on_card.h"

namespace bilinear_rank {

namespace {

const SpanRanksOnCard* registered = nullptr;

}  // namespace

void register_span_ranks_on_card(const SpanRanksOnCard* backend) { registered = backend; }
const SpanRanksOnCard* span_ranks_on_card() { return registered; }

}  // namespace bilinear_rank
