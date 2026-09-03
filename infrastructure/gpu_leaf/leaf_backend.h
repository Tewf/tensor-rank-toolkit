#pragma once

#include "gf2_leaf_on_card.h"
#include "gpu_leaf.h"

/// The kernels offered to the exact search, as the backend its seam takes.
///
/// [`gpu_leaf.h`](gpu_leaf.h) is the pair of calls a measurement makes; this is
/// the same two kernels behind the questions a *search* asks, which are not the
/// same questions. A measurement hands over a range and takes seconds. A search
/// hands over a leaf and takes survivors, and needs three things a measurement
/// does not: a shape it has no kernel for has to be declined rather than thrown
/// at, an overflowed survivor buffer has to be re-run rather than reported, and
/// a CUDA call that failed has to become the host answering rather than the run
/// stopping.
///
/// **Registration is separate on purpose** and lives in
/// [`register_the_card.cpp`](register_the_card.cpp), which only `decide-rank`
/// links. `measure-leaf` links these kernels and must not register them: a
/// harness whose host column had quietly become the card would compare the card
/// with itself and report 1.0x, or worse, report nothing wrong at all.
namespace gpu_leaf {

const bilinear_rank::LeafOnCard& card_backend();

/// Survivors one launch may report before the range is cut, for the one test
/// that has to make it too small.
///
/// **Overflow is the branch nothing here reaches.** A 47-dimensional span keeps
/// a handful of the four billion maps tested against it, so a real leaf never
/// fills a 65 536-entry buffer, and a recovery nothing exercises is a recovery
/// that does not work. `tests/test_survivor_overflow.cpp` shrinks it until the
/// buffer overflows on purpose, and asserts that the survivors that come back
/// are still the host's, element for element, and that a leaf the chunking
/// cannot rescue is declined rather than truncated.
void set_survivor_capacity(std::size_t capacity);

}  // namespace gpu_leaf
