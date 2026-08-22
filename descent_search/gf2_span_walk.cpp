#include "gf2_span_walk.h"

namespace bilinear_rank {

namespace {
bool offered = true;
}  // namespace

void set_gf2_span_walk_offered(bool wanted) { offered = wanted; }
bool gf2_span_walk_offered() { return offered; }

bool gf2_span_walk_applies(const Field& field, const std::vector<Matrix>& slices) {
    // An empty set of slices spans one element, the walk takes no step and
    // there is nothing to pack; the general path answers that in the same time
    // it would take to decide anything here.
    if (!offered || slices.empty()) return false;
    if (static_cast<std::size_t>(field.characteristic()) != 2) return false;
    return slices.front().columns() <= 64;
}

}  // namespace bilinear_rank
