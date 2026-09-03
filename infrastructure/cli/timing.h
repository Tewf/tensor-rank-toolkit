#pragma once

#include <chrono>

/// What the command-line tools share, which is almost nothing: they time a run
/// and print what it cost.
///
/// Both strands have commands, so the one place they can share a header from is
/// infrastructure/, the layer beneath both. Four copies of the same three lines
/// is what this replaces.
namespace cli {

using Clock = std::chrono::steady_clock;

/// Seconds since `started`, as the reports quote them.
///
/// A steady clock, not a wall clock: using a steady clock ensures that elapsed
/// time is always monotonic, whereas a wall clock stepping backwards would make
/// a search look faster than it actually ran.
inline double elapsed_seconds(Clock::time_point started) {
    return std::chrono::duration<double>(Clock::now() - started).count();
}

}  // namespace cli
