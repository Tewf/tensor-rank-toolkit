#pragma once

#include "span_ranks_on_card.h"

/// The rank kernel offered to `span_element_ranks`, as the backend its seam
/// takes.
///
/// Separate from [`leaf_backend.h`](leaf_backend.h) because it answers a
/// different question for a different caller, and separate from the registration
/// for the reason [`register_the_card.cpp`](register_the_card.cpp) gives:
/// linking that object *is* the registration, and `measure-leaf` must not link
/// it or its host column would quietly become the card.
namespace gpu_leaf {

const bilinear_rank::SpanRanksOnCard& span_ranks_backend();

}  // namespace gpu_leaf
