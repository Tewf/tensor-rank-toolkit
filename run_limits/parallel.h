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
/// more threads is wall-clock time, and **on a satisfiable question also a
/// count**, which this comment used to deny. Ruling a `k` out visits the whole
/// tree whoever visits it, so a refutation's node total is exact at any thread
/// count; finding a witness stops early, and workers dispatch subtrees a
/// depth-first walk would never have reached, so that total is an upper bound
/// that grows with the workers.
/// [`what-threads-change.md`](../exhaustive_search/what-threads-change.md) carries
/// the measurement and the consequence, which is that a tight `--node-limit` can
/// turn a proof into an undecided. The heuristic still adopts the same candidates in the same order.
/// Which decomposition a successful search hands back also differs, since
/// threads race to find one, and any of them computes the map.
namespace run_limits {

/// Workers a search may use. One by default; `set_worker_count(0)` asks for as
/// many as the machine has.
std::size_t worker_count();
void set_worker_count(std::size_t workers);

/// Run `body(index)` for every index below `count`.
///
/// Dynamically scheduled, because subtrees differ by orders of magnitude in
/// size and a static split would leave eleven cores waiting for the twelfth.
///
/// **What `body` throws is rethrown here, once, on the calling thread.** The
/// first exception wins; the workers still running give up their remaining
/// indices rather than finish work whose result nobody will read. Without this a
/// `require_room` refusal — the mechanism that exists so a machine smaller than
/// the one a run was written on says so instead of being killed — left a
/// `std::thread` and so called `std::terminate`, which turned the one graceful
/// failure here into the one abrupt one, and only above `--threads 1`.
void parallel_for(std::size_t count, const std::function<void(std::size_t)>& body);

}  // namespace run_limits
