#include "child_process.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

namespace bilinear_rank {

bool run_to_completion(const std::vector<std::string>& command,
                       const std::filesystem::path& log, const ChildLimits& limits) {
    if (command.empty()) return false;

    std::vector<char*> argv;
    argv.reserve(command.size() + 1);
    for (const std::string& word : command) argv.push_back(const_cast<char*>(word.c_str()));
    argv.push_back(nullptr);

    const std::string log_path = log.string();
    const pid_t child = ::fork();
    if (child < 0) return false;

    if (child == 0) {
        // Its own group, set on both sides of the fork because whichever runs
        // first wins and neither ordering is guaranteed.
        ::setpgid(0, 0);

        // `setrlimit` rather than a shell's `ulimit -v`, which is the same call
        // with a shell in the way. Both halves are set: leaving the hard limit
        // alone would let the child raise its own.
        if (limits.megabytes > 0) {
            const rlim_t bytes = static_cast<rlim_t>(limits.megabytes) * 1024 * 1024;
            const rlimit space{bytes, bytes};
            ::setrlimit(RLIMIT_AS, &space);
        }

        const int null_in = ::open("/dev/null", O_RDONLY);
        const int output = ::open(log_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
        if (null_in >= 0) ::dup2(null_in, STDIN_FILENO);
        if (output >= 0) {
            ::dup2(output, STDOUT_FILENO);
            if (limits.merge_stderr) ::dup2(output, STDERR_FILENO);
        }
        if (!limits.merge_stderr) {
            const int null_err = ::open("/dev/null", O_WRONLY);
            if (null_err >= 0) ::dup2(null_err, STDERR_FILENO);
        }

        // One second of grace, so the parent's kill is what normally ends a run
        // and this only fires when there is no parent left to do it.
        ::alarm(static_cast<unsigned int>(limits.seconds) + 1);
        ::execvp(argv[0], argv.data());
        // Only reached when the binary vanished between the availability probe
        // and here. `_exit` and not `exit`, so no parent's destructors run twice.
        ::_exit(127);
    }

    ::setpgid(child, child);

    // Polled rather than alarmed, because a SIGCHLD handler is process-wide state
    // for a library to be setting and this is called in a loop over backends.
    //
    // The interval doubles from 50 us to 20 ms. A flat interval has to choose
    // between adding it to every millisecond-long solve and burning syscalls for
    // five minutes on a long one; doubling pays neither.
    long pause_nanoseconds = 50'000;
    double waited = 0;
    int status = 0;
    while (waited < limits.seconds) {
        const pid_t done = ::waitpid(child, &status, WNOHANG);
        if (done == child) return true;
        // EINTR is not an answer about the child, and returning here would walk
        // away from a solver still holding a core, which is the whole defect.
        if (done < 0 && errno == EINTR) continue;
        if (done < 0) {
            ::killpg(child, SIGKILL);
            ::waitpid(child, &status, 0);
            return false;
        }
        const timespec pause{0, pause_nanoseconds};
        ::nanosleep(&pause, nullptr);
        waited += static_cast<double>(pause_nanoseconds) / 1e9;
        if (pause_nanoseconds < 20'000'000) pause_nanoseconds *= 2;
    }

    ::killpg(child, SIGKILL);
    // Blocking, so the child is reaped rather than left a zombie, and the caller
    // returns to a machine with nothing of ours running on it.
    ::waitpid(child, &status, 0);
    return true;
}

}  // namespace bilinear_rank
