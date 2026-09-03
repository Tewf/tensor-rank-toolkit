/// What bounds a run on this machine, and where each number came from.
///
/// Three surfaces decide what a run may spend — a flag, `tunables.conf`, and a
/// compiled default that is now sometimes a reading of the machine — and until
/// this instrument there was no way to see the result of the three without
/// running a search and reading its plan. That is the wrong order: the numbers
/// should be legible before the hour is spent, not inferred from how the hour
/// went.
///
/// It is an instrument and not a tool. It asks nothing about a tensor, takes no
/// map, and answers no question in
/// [`OPTIONS/one-question-per-command.md`](../OPTIONS/one-question-per-command.md).
/// What it prints is this machine and this working directory.
///
/// **A flag beats the file beats the default**, and the `source` column says
/// which of the three each value ended at. Flags are per run and cannot be seen
/// from here, so the column stops at the file; `OPTIONS.md` is the table of what
/// each flag moves.
#include <iostream>
#include <string>
#include <vector>

#include "arguments.h"
#include "device.h"
#include "exit_code.h"
#include "machine.h"
#include "memory_budget.h"
#include "parallel.h"
#include "report.h"
#include "tunables.h"

namespace {

std::string readable_bytes(std::size_t bytes) {
    const char* units[] = {"bytes", "KiB", "MiB", "GiB", "TiB"};
    long double left = static_cast<long double>(bytes);
    int unit = 0;
    while (left >= 1024.0L && unit < 4) {
        left /= 1024.0L;
        ++unit;
    }
    std::string digits = std::to_string(static_cast<double>(left));
    digits.resize(digits.find('.') + 2);
    return digits + " " + units[unit];
}

void row(const std::string& name, const std::string& value, const std::string& note) {
    std::cout << "  " << name;
    for (std::size_t pad = name.size(); pad < 24; ++pad) std::cout << ' ';
    std::cout << value;
    // At least one space, however long the value was: a column that runs into
    // the next one is a table nobody can read, and some of these are lists.
    std::size_t pad = value.size();
    do {
        std::cout << ' ';
    } while (++pad < 22);
    std::cout << note << "\n";
}

}  // namespace

int main(int argc, char** argv) try {
    cli::Arguments arguments(argc, argv);
    while (arguments.next_flag()) {
        if (arguments.is("--help", "-h")) {
            // Through note(), as report.h's rule has it: a usage block is not
            // a result, so it is commented, on stderr, and stdout is untouched.
            // Exit Usage, as every command here does for --help: a line asking
            // for help asked no question, and
            // `cli/tests/check_argument_grammar.sh` asserts it on every binary.
            cli::note()
                << "usage: show-limits [--help]\n"
                   "\n"
                   "  Prints what bounds a run here: the machine, the memory and core\n"
                   "  ceilings derived from it, and every tunable with the value in\n"
                   "  force and where it came from. Reads tunables.conf the same way\n"
                   "  every command does. Runs nothing and measures nothing.";
            return cli::exit_status(cli::ExitCode::Usage);
        }
        arguments.refuse();
    }

    std::cout << "# show-limits: what a run is bounded by here\n\n";

    std::cout << "machine, as the kernel reports it\n";
    row("cores", std::to_string(run_limits::core_count()), "hardware_concurrency()");
    const std::size_t physical = run_limits::physical_memory_bytes();
    row("physical memory", physical == 0 ? "unknown" : readable_bytes(physical),
        physical == 0 ? "no /proc/meminfo and no sysconf: defaults fall back" : "MemTotal");
    row("memory scale", run_limits::memory_scale_bytes() == 0
                            ? "unknown"
                            : readable_bytes(run_limits::memory_scale_bytes()),
        "rounded up to a power of two");

    std::cout << "\nderived from it, and what moves each\n";
    row("allocation ceiling", readable_bytes(run_limits::memory_budget()),
        "an eighth of the scale; --max-memory");
    row("workers", std::to_string(run_limits::worker_count()),
        "one until asked; --threads N, 0 for every core");
    std::string devices;
    for (const run_limits::Device device : run_limits::ranked_devices()) {
        devices += (devices.empty() ? "" : " ");
        devices += run_limits::name_of(device);
        devices += run_limits::available(device) ? "" : "(absent)";
    }
    row("device order", devices, "first available answers; --device cpu|gpu|auto");
    row("launch floor", std::to_string(run_limits::launch_floor()) + " elements",
        "below it the host answers; measure-leaf floor re-fits it");

    const cli::Tunables shipped;
    const cli::Tunables& running = cli::tunables();
    std::cout << "\ntunables, in force here\n";
    for (const auto& [name, field] : cli::counted_tunables()) {
        const std::size_t value = running.*field;
        const bool moved = value != shipped.*field;
        std::string source = moved ? "tunables.conf" : "default";
        for (const auto& [named, resolve] : cli::machine_read_tunables()) {
            if (named == name && value == resolve()) source = moved ? "auto" : "default (auto)";
        }
        row(name, std::to_string(value), source);
    }
    for (const auto& [name, field] : cli::listed_tunables()) {
        std::string joined;
        for (const std::string& word : running.*field) joined += (joined.empty() ? "" : " ") + word;
        row(name, joined, (running.*field == shipped.*field) ? "default" : "tunables.conf");
    }

    std::cout << "\n# A flag beats this file beats the default; flags are per run and are in\n"
                 "# OPTIONS.md. `auto` in the file asks the machine, and is accepted only\n"
                 "# where a machine reading exists.\n";
    return 0;
} catch (const cli::ArgumentError& problem) {
    // A word that could not be read: the run never started, so Usage, the way
    // every command leaves. Until 2026-09-03 refuse() escaped to
    // std::terminate here - the one binary that aborted on a typo, and
    // web_interface/limits.py shells out to this one.
    cli::note() << "show-limits: " << problem.what();
    return cli::exit_status(cli::ExitCode::Usage);
}
