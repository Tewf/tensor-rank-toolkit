#include "memory_budget.h"

#include <stdexcept>

#include "machine.h"

namespace bilinear_rank {

namespace {

/// Derived from the machine on first use, so it is asked once and only by a run
/// that has a use for the answer. `--max-memory` and `memory_budget_bytes`
/// overwrite it before any allocation is priced. The flag is a separate `bool`
/// rather than a zero sentinel: `--max-memory 0` is a legal thing to type, it
/// means refuse everything, and a sentinel would silently re-derive instead.
std::size_t budget = 0;
bool budget_chosen = false;

/// Bytes as a human reads them, so a refusal is legible at a glance.
std::string readable(long double bytes) {
    const char* units[] = {"bytes", "KiB", "MiB", "GiB", "TiB", "PiB"};
    int unit = 0;
    while (bytes >= 1024.0L && unit < 5) {
        bytes /= 1024.0L;
        ++unit;
    }
    std::string digits = std::to_string(static_cast<double>(bytes));
    digits.resize(digits.find('.') + 2);  // one decimal is plenty
    return digits + " " + units[unit];
}

}  // namespace

std::size_t memory_budget() {
    if (!budget_chosen) {
        budget = suggested_memory_budget();
        budget_chosen = true;
    }
    return budget;
}

void set_memory_budget(std::size_t bytes) {
    budget = bytes;
    budget_chosen = true;
}

std::size_t bytes_per_matrix(std::size_t entries) {
    // sizeof(Matrix) is two sizes and a vector header; the entries are int64_t
    // on the heap, behind an allocator header of about sixteen bytes.
    return 56 + 8 * entries;
}

void require_room(const std::string& what, std::size_t count, std::size_t bytes_each) {
    if (count == 0 || bytes_each == 0) return;
    const std::size_t allowed = memory_budget();
    if (bytes_each <= allowed / count) return;

    const long double wanted = static_cast<long double>(count) * static_cast<long double>(bytes_each);
    throw std::runtime_error(what + " needs " + readable(wanted) + " (" + std::to_string(count) +
                             " items), over the " + readable(static_cast<long double>(allowed)) +
                             " budget. Raise it with --max-memory if the machine has the room.");
}

}  // namespace bilinear_rank
