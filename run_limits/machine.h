#pragma once

#include <cstddef>

/// What this machine has, read once, in the one place that asks the operating
/// system anything.
///
/// Every other number in `run_limits/` is a policy or a measurement. These four
/// are neither: they are properties of whatever hardware the binary happens to
/// be running on, and they exist so that a default can be *derived* rather than
/// fitted. `adapting-to-the-machine/fitted-or-genuine.md` sorted this
/// repository's constants into genuine, policy and fitted, and named the model
/// the fitted ones should follow: `set_worker_count(0)` asks
/// `hardware_concurrency()` and no number from this chassis appears in it.
/// This header is that model, made available to memory as well as to cores.
///
/// **Nothing here is measured and nothing here is timed.** A probe that took a
/// stopwatch to the machine would make every run's defaults depend on what else
/// was running, which is the opposite of reproducible. These are counts the
/// kernel already knows.
namespace bilinear_rank {

/// Physical memory in bytes, or **0 when it cannot be read**. Callers must
/// handle the zero rather than dividing by it: a container with a hidden
/// `/proc` and a platform this was never built for both land there.
std::size_t physical_memory_bytes();

/// `physical_memory_bytes()` rounded **up** to a power of two, or 0 when that is
/// unknown. Firmware and the kernel keep some of the memory a machine is sold
/// with, so a 16 GB laptop reports 15.3 GiB and a fraction of the raw figure is
/// a fraction of an arbitrary number. Rounding up recovers the figure a person
/// would say out loud, which is the one a budget should be a fraction of.
std::size_t memory_scale_bytes();

/// Cores this machine will run threads on, never 0. `worker_count()` is the
/// policy built on top of it, and stays at one until a run asks for more.
std::size_t core_count();

/// The budget a machine of that scale gets, as a pure function of the scale so
/// the rule can be tested without owning the hardware it describes. `0` means
/// the machine could not be read and returns the shipped 2 GiB.
std::size_t suggested_memory_budget_for(std::size_t scale_bytes);

/// The ceiling one bulk allocation gets when nothing overrides it: an eighth of
/// `memory_scale_bytes()`, which is **exactly the 2 GiB this repository has
/// always shipped on the 16 GB laptop it was written on**, and is 512 MiB on a
/// 4 GB box and 64 GiB on a 512 GB server without anybody re-measuring anything.
///
/// Clamped to [256 MiB, 64 GiB]. The floor keeps a tiny machine from refusing
/// work it could do; the ceiling keeps a large one from letting a single
/// exponential allocation swallow the host before `require_room` speaks. When
/// the machine cannot be read at all it falls back to 2 GiB, the shipped
/// number, so an unreadable `/proc` changes nothing rather than something.
std::size_t suggested_memory_budget();

}  // namespace bilinear_rank
