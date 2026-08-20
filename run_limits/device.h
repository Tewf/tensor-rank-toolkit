#pragma once

#include <cstddef>
#include <string>
#include <vector>

/// Which processor answers a bulk question, given what this machine actually has.
///
/// The bulk questions here are all the same shape: some millions of independent
/// elements, each formed from its own index and reduced against a basis held in
/// common. That is what a card is for, and it is measured: one whole `<4,4,4>`
/// leaf, 4 294 836 225 rank-one maps, in **1.019 s** against 9.2 minutes for the
/// same leaf on one core. (It was 1.12 hours until `d85fd32` gave the host leaf
/// the kernel's own arithmetic, which is 7.3x of the gap and no card at all.)
///
/// **This is a seam, not a switch, and no GPU backend is compiled in yet.** It
/// exists so that one can be added without restructuring anything, in the shape
/// [`../integer_programme/solver_chain.h`](../integer_programme/solver_chain.h)
/// already uses for solvers: the ranking is fixed, the availability is not, a
/// machine with nothing installed still answers, and an absent backend is a state
/// reported here rather than an error discovered downstream.
///
/// **Nothing routes through it that was measured to need it.** Building the pool
/// costs 0.040 ms addressed and 96.8 ms materialised at `<3,3,3>`, and the whole
/// orbit machinery there costs 1.2 ms, which is 0.04% of a 3.12 s run: an Amdahl
/// ceiling of 1.0004x, so a kernel for either would be unmeasurable. What is
/// worth a card is the **leaf**, at 90% or more of a run, and the per-element
/// call inside it. Those are the routes this seam is for.
namespace run_limits {

/// Best first. The card leads where one is present, since every question asked
/// through here is one it is faster at; the host trails and always answers.
enum class Device { Gpu, Cpu };

const char* name_of(Device device);

/// The order in force, which is the compiled ranking until a caller changes it.
const std::vector<Device>& ranked_devices();

/// Reorder them by name, best first. False, with the offending name in
/// `unrecognised`, when one of them is not a device here.
bool set_device_order(const std::vector<std::string>& names, std::string& unrecognised);

/// Whether a backend for `device` can answer on this machine right now. The host
/// always can. The card can only once something has registered one, which is
/// what keeps this honest while nothing has.
bool available(Device device);

/// How a GPU backend announces itself. Called once, by whoever links one; the
/// probe is asked afresh on every `available` so that a card disappearing between
/// runs is noticed rather than cached.
void register_gpu_backend(bool (*probe)());

/// Below this many elements the host wins whatever else is present, because a
/// launch costs more than the work. Set from `device_launch_floor`.
std::size_t launch_floor();
void set_launch_floor(std::size_t elements);

/// The device that should answer a question of this size: the first ranked one
/// that is available, except that work below `launch_floor()` always stays here.
Device chosen_device(std::size_t elements);

}  // namespace run_limits
