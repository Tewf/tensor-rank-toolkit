#pragma once

#include <csignal>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <string>

#include <unistd.h>

/// Scratch files that must not outlive a run that was cut short.
///
/// A file removed at the end of the function that wrote it survives every route
/// out of that function but one: a signal, which returns to nowhere.
/// `decide-rank-by-sat` writes a CNF of a few hundred kilobytes per question and
/// removes it when the solver answers, so a run killed between the two left the
/// file in `/tmp`, and a sweep killed twenty questions in left twenty of them.
///
/// **SIGKILL cannot be one of the signals handled here.** POSIX does not let a
/// process catch it, or SIGSTOP, so `kill -9` still leaves the file and no code
/// in this repository can change that. What is handled is the three that can be:
/// SIGINT, SIGTERM and SIGHUP, which is what a Ctrl-C, a `timeout` and a closed
/// terminal send, and which is every way one of these runs has actually died.
///
/// Nothing the handler does is unsafe in a signal context. It allocates nothing,
/// formats nothing and locks nothing: it calls `unlink` on paths copied into
/// place long before the signal arrived, then restores the default disposition
/// and re-raises, so the process still dies of the signal it was sent and a shell
/// still reports 128 + n rather than an exit code invented here.
namespace cli {

namespace interrupted {

/// One run's scratch files, held as bytes because a signal handler may not build
/// a `std::string`. Two are registered today; the table is loose rather than
/// tight so that a third caller needs no edit here.
inline constexpr std::size_t most_files = 8;
/// PATH_MAX on Linux. A longer path could not have been opened in the first
/// place, so this cannot silently truncate a name that matters.
inline constexpr std::size_t most_characters = 4096;

inline char files[most_files][most_characters] = {};
/// Written last on registration, so the handler never reads a half-copied path.
///
/// A high-water mark rather than a population: a slot is emptied by making its
/// first byte a terminator, never by compacting the table, because a signal
/// arriving mid-shuffle would read a name that had moved. So this only ever
/// grows, the handler walks to it, and an emptied slot costs one `unlink("")`
/// that fails and is ignored like every other failure in there.
inline volatile std::sig_atomic_t count = 0;

extern "C" {
inline void remove_files_and_re_raise(int signal_number) {
    for (std::sig_atomic_t at = 0; at < count; ++at) {
        // Its failure is expected and ignored: a file may never have been
        // created, and there is nothing safe to say about it from in here.
        ::unlink(files[at]);
    }
    std::signal(signal_number, SIG_DFL);
    std::raise(signal_number);
}
}  // extern "C"

}  // namespace interrupted

/// Remove `path` if this run is interrupted. Call it before the file is written,
/// once per file, and never from a handler.
///
/// It refuses rather than quietly registering nothing, because a promise of
/// cleanup that is not kept is the failure this header exists to remove.
inline void remove_when_interrupted(const std::string& path) {
    if (path.size() + 1 > interrupted::most_characters) {
        throw std::length_error("cannot arrange to remove '" + path + "': the path is too long");
    }
    if (interrupted::count >= static_cast<std::sig_atomic_t>(interrupted::most_files)) {
        throw std::length_error("cannot arrange to remove '" + path + "': already holding " +
                                std::to_string(interrupted::most_files) + " scratch files");
    }
    // An emptied slot first, so a run asking many questions reuses them instead
    // of walking off the end of the table. Registering was originally a one-way
    // door, and with three files a question that is a ceiling of two questions.
    std::sig_atomic_t at = interrupted::count;
    for (std::sig_atomic_t slot = 0; slot < interrupted::count; ++slot) {
        if (interrupted::files[slot][0] == '\0') {
            at = slot;
            break;
        }
    }
    std::memcpy(interrupted::files[at], path.c_str(), path.size() + 1);
    if (at < interrupted::count) return;  // reused, so the mark does not move
    // Assigned rather than incremented: C++20 deprecates `++` on a volatile, and
    // the two steps are the point anyway. The path is in place before the count
    // that admits it, so a signal between the two removes one file fewer rather
    // than reading a name that is half written.
    interrupted::count = at + 1;

    static bool installed = false;
    if (installed) return;
    installed = true;
    for (const int number : {SIGINT, SIGTERM, SIGHUP}) {
        std::signal(number, interrupted::remove_files_and_re_raise);
    }
}

/// Stop arranging to remove `path`, because it has been removed already.
///
/// Without this the table is a one-way door: every question registers its
/// formula and its log, so a run asking three of them would exhaust eight slots
/// and the fourth registration would throw. Emptying the slot rather than
/// compacting the table is what keeps the handler safe against a signal arriving
/// here.
inline void forget_when_interrupted(const std::string& path) {
    for (std::sig_atomic_t at = 0; at < interrupted::count; ++at) {
        if (path == interrupted::files[at]) {
            interrupted::files[at][0] = '\0';
            return;
        }
    }
}

}  // namespace cli
