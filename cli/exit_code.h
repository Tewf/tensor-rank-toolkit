#pragma once

#include <stdexcept>
#include <string>

/// How a command tells a script what happened, in one vocabulary for all three.
///
/// The three commands here answer the same question by different machinery, so
/// a caller must be able to read their answers the same way. The distinction
/// that earns most of these codes: **a question that was not answered is not a
/// question answered no.** A budget running out proves nothing, and folding it
/// into a refusal turns giving up into a lower bound. That is a mistake this
/// repository has already published once, in a table rather than in a code.
namespace cli {

enum class ExitCode {
    /// An algorithm with that many products exists, and it was verified.
    Yes = 0,
    /// Proved that none exists.
    No = 1,
    /// The arguments did not parse.
    Usage = 2,
    /// A budget was exhausted: timeout, memory cap, node limit. Nothing is
    /// proved, in either direction.
    Undecided = 3,
    /// An answer was produced and failed its own check: a decomposition that
    /// does not rebuild the map, or a refutation the checker rejects. This is
    /// worse than an error, because it means a component is wrong.
    Unverified = 4,
    /// Could not run at all: unreadable file, no solver, an exception.
    Error = 5,
};

inline int exit_status(ExitCode code) { return static_cast<int>(code); }

/// Thrown when an answer fails its own verification, so that the check can live
/// deep in a library and still reach the exit code without every layer between
/// having to return it.
///
/// It exists to keep `Unverified` off `Error`: both are failures, but one says
/// the machine could not run and the other says the machine ran and lied.
class CheckFailed : public std::runtime_error {
   public:
    explicit CheckFailed(const std::string& what) : std::runtime_error(what) {}
};

}  // namespace cli
