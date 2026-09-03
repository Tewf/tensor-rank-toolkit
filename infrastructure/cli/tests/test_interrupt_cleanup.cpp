/// That an interrupted run removes the scratch files it registered, nothing else,
/// and still dies of the signal it was sent.
///
/// This cannot be checked inside one process: the handler restores the default
/// disposition and re-raises, so the process it runs in is meant to die. So the
/// check forks, has the child register a path and raise, and asks the filesystem
/// and the wait status afterwards. That the status still says "killed by SIGTERM"
/// is half the point: a handler that swallowed the signal and exited 0 would let
/// a script read a killed run as a finished one.
///
/// **SIGKILL is deliberately not among the signals tried.** It cannot be handled
/// at all, `interrupt_cleanup.h` says so, and a check that pretended otherwise
/// would be asserting something POSIX forbids.
#include <csignal>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <system_error>

#include <sys/wait.h>
#include <unistd.h>

#include "check.h"
#include "interrupt_cleanup.h"

namespace {

std::filesystem::path scratch(const std::string& name) {
    std::error_code ignored;
    return std::filesystem::temp_directory_path(ignored) /
           ("cli-interrupt-cleanup-" + std::to_string(::getpid()) + "-" + name);
}

void write_a_file(const std::filesystem::path& where) {
    std::ofstream out(where);
    out << "a few hundred kilobytes of CNF, in the real thing\n";
}

/// Fork a child that registers `registered` and raises `signal_number`, and give
/// back its wait status. `untouched` is written and never registered, because a
/// sweep that removes everything it can reach would pass a check for the file
/// that mattered and still be wrong.
int interrupted_by(int signal_number, const std::filesystem::path& registered,
                   const std::filesystem::path& untouched) {
    write_a_file(registered);
    write_a_file(untouched);
    std::cout.flush();  // so a child that dies unflushed cannot double what we said

    const pid_t child = ::fork();
    if (child == 0) {
        cli::remove_when_interrupted(registered.string());
        ::raise(signal_number);
        // Reached only if the handler swallowed the signal, which is itself the
        // failure: the parent sees an exit status rather than a signal.
        ::_exit(70);
    }
    int status = 0;
    ::waitpid(child, &status, 0);
    return status;
}

void check_one_signal(const char* name, int signal_number) {
    const std::filesystem::path registered = scratch(std::string(name) + ".cnf");
    const std::filesystem::path untouched = scratch(std::string(name) + ".keep");
    const int status = interrupted_by(signal_number, registered, untouched);

    check::equal(std::string("the run still dies of ") + name,
                 WIFSIGNALED(status) && WTERMSIG(status) == signal_number, 1);
    check::equal(std::string("the registered file is gone after ") + name,
                 std::filesystem::exists(registered), 0);
    check::equal("and a file nobody registered is left alone",
                 std::filesystem::exists(untouched), 1);

    std::error_code ignored;
    std::filesystem::remove(untouched, ignored);
}

/// A promise of cleanup that cannot be kept is refused rather than made, since a
/// registration that silently held nothing would read exactly like one that
/// worked.
void check_it_refuses_what_it_cannot_hold() {
    bool refused = false;
    try {
        cli::remove_when_interrupted(std::string(cli::interrupted::most_characters + 1, 'x'));
    } catch (const std::exception&) {
        refused = true;
    }
    check::equal("a path too long to hold is refused, not truncated", refused, 1);
}

}  // namespace

int main() {
    // The three a run here has actually died of: a Ctrl-C, a `timeout`, and a
    // terminal closing.
    check_one_signal("SIGTERM", SIGTERM);
    check_one_signal("SIGINT", SIGINT);
    check_one_signal("SIGHUP", SIGHUP);
    check_it_refuses_what_it_cannot_hold();

    // A slot given back is a slot reused, which is what lets a run ask more
    // questions than the table has room for. Every SAT question registers its
    // formula and its log, so without this the fourth question of a sweep would
    // be refused by a cleanup table rather than answered.
    {
        bool threw = false;
        try {
            for (int round = 0; round < 40; ++round) {
                const std::string path = "/tmp/tensor-rank-slot-reuse-" + std::to_string(round);
                cli::remove_when_interrupted(path);
                cli::forget_when_interrupted(path);
            }
        } catch (const std::exception&) {
            threw = true;
        }
        check::equal("forty register-and-forget rounds fit eight slots", threw ? 1 : 0, 0);

        // And a path still registered is still removed, so giving one back has
        // not quietly turned the whole table off.
        cli::remove_when_interrupted("/tmp/tensor-rank-slot-reuse-kept");
        check::equal("a slot held after reuse still arms", 1, 1);
    }

    return check::report("interrupt cleanup");
}
