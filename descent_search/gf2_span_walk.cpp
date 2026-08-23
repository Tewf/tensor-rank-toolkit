#include "gf2_span_walk.h"

namespace bilinear_rank {

namespace {
bool offered = true;
}  // namespace

void set_gf2_span_walk_offered(bool wanted) { offered = wanted; }
bool gf2_span_walk_offered() { return offered; }

bool gf2_span_walk_applies(const Field& field, const std::vector<Matrix>& slices) {
    // An empty set of slices spans one element, the walk takes no step and
    // there is nothing to pack; the general path answers that in the time it
    // would take to decide anything here.
    if (!offered || slices.empty()) return false;
    if (static_cast<std::size_t>(field.characteristic()) != 2) return false;

    // One bit of the index per slice, so the shifts the walk and the greedy
    // take are defined. Nothing reaches this: `span_size` refuses a span of
    // more than 40 slices over GF(2) long before the walk takes a step, and a
    // set this file declines is a set the general path throws on.
    if (slices.size() >= 64) return false;

    const std::size_t rows = slices.front().rows();
    const std::size_t columns = slices.front().columns();
    if (rows == 0 || columns == 0 || columns > 64) return false;

    // **Every slice in the front slice's shape, and checked rather than
    // assumed.** This file ranks each slice in that one shape, where the
    // general path ranks each in its own, so two slices of the same entry count
    // and different shapes would give a different rank ceiling here and a
    // different answer with it. Every caller in this repository hands over the
    // slices of one tensor and a candidate of that shape, which is why the
    // check has never fired; a predicate that rests on a caller's invariant
    // rather than on what it can see is how it would fire quietly.
    for (const Matrix& slice : slices) {
        if (slice.rows() != rows || slice.columns() != columns) return false;
    }
    return true;
}

}  // namespace bilinear_rank
