#pragma once

#include <cstddef>
#include <string>

/// How much memory one run may ask for, in the one place that decides it.
///
/// Named memory_budget.h and not limits.h: a header called limits.h on this
/// include path is found by `#include <limits.h>` inside GMP, and the build
/// fails somewhere else entirely, complaining that UINT_MAX does not exist.
///
/// Both bulk allocations here grow exponentially in the shape of the tensor,
/// and neither used to be bounded by anything a machine could survive. The pool
/// of rank-one maps for 4x4 matrix multiplication is `(2^16 - 1)^2` matrices of
/// 256 entries each, which `bytes_per_matrix` prices at **8.2 TiB**: as a
/// `reserve` it is an instant kill, and the caller has no way to find that out
/// other than by dying. The span in step 1 grows the same way, `p^slices`.
///
/// This comment said 720 GB until 2026-08-17, which is its own formula out by a
/// factor of eleven and had been quoted onward as if measured. `decide-rank` on a
/// `<4,4,4>` tensor prints the real number, and printing it is the whole point of
/// the file, so the number in the prose has no excuse for disagreeing.
///
/// So the size is computed first and compared against a budget, and a run that
/// does not fit says so and stops. A refusal naming the number is a result; an
/// out-of-memory kill is not.
namespace run_limits {

/// The ceiling on a single bulk allocation. **An eighth of what the machine
/// has** by default ([`machine.h`](machine.h)), which is the two gibibytes this
/// repository has always used on the 16 GB laptop it was written on, and moves
/// on its own to 512 MiB on a 4 GB box and 64 GiB on a 512 GB server. That
/// leaves room for a browser and an editor to survive the run in the same
/// proportion whatever the machine is. `--max-memory` moves it per run and
/// `memory_budget_bytes` in `tunables.conf` moves it per checkout.
std::size_t memory_budget();
void set_memory_budget(std::size_t bytes);

/// What `count` matrices of `entries` field elements cost, near enough: each
/// carries its own heap vector of `int64_t` plus the object and allocator
/// headers.
std::size_t bytes_per_matrix(std::size_t entries);

/// Throw unless `count` items of `bytes_each` fit the budget, naming what was
/// asked for and what is allowed. The comparison is a division, so nothing
/// overflows on the way to reporting a number that would have.
void require_room(const std::string& what, std::size_t count, std::size_t bytes_each);

}  // namespace run_limits
