#pragma once

#include <cstddef>
#include <functional>

/// Running independent work on the machine's cores.
///
/// Both searches spend their time on work that shares nothing: one subtree per
/// pool element in the exact search, one trial basis per candidate in the
/// heuristic. Neither was ever given more than one core.
///
/// **The default is one worker**, so a run reproduces exactly what this
/// repository has always published, and `--threads` opts in. What changes with
/// more threads is wall-clock time, never a count: the exact search still
/// visits the same nodes and the heuristic still adopts the same candidates in
/// the same order. The one thing that can differ is *which* decomposition a
/// successful exact search hands back, since threads race to find one, and any
/// of them computes the map.
namespace bilinear_rank {

/// Workers a search may use. One by default; `set_worker_count(0)` asks for as
/// many as the machine has.
std::size_t worker_count();
void set_worker_count(std::size_t workers);

/// Run `body(index)` for every index below `count`.
///
/// Dynamically scheduled, because subtrees differ by orders of magnitude in
/// size and a static split would leave eleven cores waiting for the twelfth.
void parallel_for(std::size_t count, const std::function<void(std::size_t)>& body);

}  // namespace bilinear_rank
