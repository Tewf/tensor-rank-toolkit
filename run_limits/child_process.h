#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

/// Starting an outside program so that nothing of ours is left running.
///
/// This module already owns how much memory and how many cores one run may take,
/// and a child process is both, which is why the one way to start one lives here
/// rather than in whichever module happened to need it first.
///
/// **It used to live in two places and only one of them was right.**
/// `integer_programme` learned the hard way that `std::system` under a `timeout`
/// prefix "was not a handle on anything": the prefix bounded an orphan's duration
/// and not its effect, it held a core for those seconds, and every measurement
/// taken afterwards read slow. One ILP cell read 6.62 s against a stray where it
/// reads 3.34 s clean. `satisfiability` still used that abandoned pattern, through
/// `popen("sh -c 'ulimit -v N; exec timeout T ...'")`, and one leaked solver per
/// worker is what a parallel cube split would have multiplied.
namespace run_limits {

/// What one child may take, and where its output goes.
///
/// `megabytes` of 0 means no address-space cap, which is what the integer
/// programme route has always run with; the SAT route caps because a solver
/// asked for an unsatisfiable instance will grow until something stops it.
/// `merge_stderr` false sends the child's stderr to `/dev/null`, which is what
/// the SAT route wants because it parses the log and a solver's progress chatter
/// is not an answer.
struct ChildLimits {
    double seconds = 0.0;
    std::size_t megabytes = 0;
    bool merge_stderr = true;
};

/// Start `command` in a process group of its own, wait out the clock, and kill
/// the **group** if the clock runs out. False when it could not be started.
///
/// The group is what must be killed, not the child. A backend that forks a worker
/// leaves it behind when only its parent is signalled, which is the same leak with
/// an extra step. `setpgid` on both sides of the fork and `killpg` in the parent
/// covers both, and it never matches on a process name, so it cannot reach a
/// solver somebody else is running or, worse, the process doing the killing.
///
/// **The child also caps itself, and dropping that would be a regression.** A
/// parent killed mid-solve cannot kill anything, and what the old `timeout` prefix
/// genuinely bought was that the orphan died within the cap rather than at reboot.
/// `alarm` survives `execvp` while SIGALRM's disposition resets to terminate, so
/// the child carries its own deadline and needs nobody alive to enforce it. The
/// parent's kill is what makes the *call* leave nothing running; the alarm is what
/// makes a *dead parent* leave nothing running. Both are wanted and neither
/// replaces the other.
///
/// **What that leaves, measured rather than assumed.** Send SIGTERM to a run
/// mid-solve and the solver outlives it: its own alarm ends it, so it holds a core
/// for up to `seconds + 1`, which is 301 s at the SAT default. The scratch files
/// go immediately, through `cli/interrupt_cleanup.h`, and the solver does not.
/// Closing that window would mean killing the group from a signal handler, which
/// is safe to do but would put a second handler on the same three signals as the
/// file cleanup, and two handlers on one signal is one clobbering the other. It is
/// bounded and it is one process, so it is written down here rather than fixed
/// with a handler chain nobody asked for.
bool run_to_completion(const std::vector<std::string>& command,
                       const std::filesystem::path& log, const ChildLimits& limits);

}  // namespace run_limits
