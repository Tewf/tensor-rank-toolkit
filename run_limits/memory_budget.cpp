#include "memory_budget.h"

#include <stdexcept>

namespace bilinear_rank {

namespace {

std::size_t budget = std::size_t(2) << 30;  // 2 GiB

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

std::size_t memory_budget() { return budget; }

void set_memory_budget(std::size_t bytes) { budget = bytes; }

std::size_t bytes_per_matrix(std::size_t entries) {
    // sizeof(Matrix) is two sizes and a vector header; the entries are int64_t
    // on the heap, behind an allocator header of about sixteen bytes.
    return 56 + 8 * entries;
}

void require_room(const std::string& what, std::size_t count, std::size_t bytes_each) {
    if (count == 0 || bytes_each == 0) return;
    if (bytes_each <= budget / count) return;

    const long double wanted = static_cast<long double>(count) * static_cast<long double>(bytes_each);
    throw std::runtime_error(what + " needs " + readable(wanted) + " (" + std::to_string(count) +
                             " items), over the " + readable(static_cast<long double>(budget)) +
                             " budget. Raise it with --max-memory if the machine has the room.");
}

}  // namespace bilinear_rank
