#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "leaf_question.h"

/// The two leaf routes on the card, as two calls a host program can make.
///
/// Declared away from the `.cu` files so [`measure_leaf.cpp`](measure_leaf.cpp)
/// is an ordinary C++ translation unit and only the kernels go through `nvcc`.
namespace gpu_leaf {

/// What one launch found, and what it cost.
///
/// `indices` is sorted, because the threads that wrote it were not ordered and
/// the greedy the host runs afterwards depends on the order: see
/// [`README.md`](README.md) on why sorting is enough to make the answer
/// bit-identical to the sequential loop.
///
/// `overflowed` true means the survivor buffer was too small and some survivors
/// were never written, so `indices` is not an answer to anything. Nothing here
/// truncates silently.
struct GpuSurvivors {
    std::vector<std::uint64_t> indices;
    bool overflowed = false;
    /// Seconds inside the kernels, from CUDA events, summed over the launches.
    double kernel_seconds = 0.0;
    /// Seconds from before the first launch to after the last copy back, which
    /// is what the work actually cost the host that asked for it.
    double wall_seconds = 0.0;
};

/// Pool elements in `[left_begin, left_end) x every right` that lie in the span.
///
/// The range is whole rows of the outer-product grid, matching
/// [`scan_pool_on_host`](host_reference.h).
GpuSurvivors scan_pool_on_gpu(const LeafQuestion& question, std::size_t left_begin,
                              std::size_t left_end, std::size_t capacity);

/// Subspace elements in `[begin, end)` that have rank one.
GpuSurvivors walk_subspace_on_gpu(const LeafQuestion& question, std::uint64_t begin,
                                  std::uint64_t end, std::size_t capacity);

/// The card the two calls above will run on, for the record a measurement keeps.
std::string device_description();

/// Whether a card this build can launch on is present right now.
///
/// **It never throws**, where everything else here does: it is the probe
/// `run_limits::available` asks on every question, and a driver that has gone
/// away between leaves is an answer of no rather than a run that stops.
bool card_present();

}  // namespace gpu_leaf
