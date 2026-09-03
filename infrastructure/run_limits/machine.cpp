#include "machine.h"

#include <cstdio>
#include <string>
#include <thread>

#if defined(__unix__) || defined(__APPLE__)
#include <unistd.h>
#endif

namespace run_limits {

namespace {

/// `MemTotal` in bytes, or 0. Preferred over `sysconf` because it is the figure
/// every other tool on a Linux box quotes, so a refusal here and `free -h`
/// disagree by rounding rather than by kind.
std::size_t meminfo_total_bytes() {
    std::FILE* file = std::fopen("/proc/meminfo", "r");
    if (file == nullptr) return 0;
    char label[64] = {};
    unsigned long long value = 0;
    char unit[16] = {};
    std::size_t bytes = 0;
    while (std::fscanf(file, "%63s %llu %15s", label, &value, unit) == 3) {
        if (std::string(label) == "MemTotal:") {
            bytes = static_cast<std::size_t>(value) * 1024;  // meminfo is in kB
            break;
        }
    }
    std::fclose(file);
    return bytes;
}

std::size_t sysconf_total_bytes() {
#if defined(_SC_PHYS_PAGES) && defined(_SC_PAGE_SIZE)
    const long pages = sysconf(_SC_PHYS_PAGES);
    const long page = sysconf(_SC_PAGE_SIZE);
    if (pages > 0 && page > 0) return static_cast<std::size_t>(pages) * static_cast<std::size_t>(page);
#endif
    return 0;
}

}  // namespace

std::size_t physical_memory_bytes() {
    static const std::size_t bytes = [] {
        const std::size_t from_meminfo = meminfo_total_bytes();
        return from_meminfo != 0 ? from_meminfo : sysconf_total_bytes();
    }();
    return bytes;
}

std::size_t memory_scale_bytes() {
    const std::size_t physical = physical_memory_bytes();
    if (physical == 0) return 0;
    std::size_t rounded = 1;
    while (rounded < physical) {
        if (rounded > (std::size_t(1) << 62)) return physical;  // absurd, and no headroom to double
        rounded <<= 1;
    }
    return rounded;
}

std::size_t core_count() {
    const unsigned int detected = std::thread::hardware_concurrency();
    return detected == 0 ? 1 : static_cast<std::size_t>(detected);
}

std::size_t suggested_memory_budget_for(std::size_t scale_bytes) {
    if (scale_bytes == 0) return std::size_t(2) << 30;  // the shipped number, unchanged
    const std::size_t eighth = scale_bytes / 8;
    const std::size_t floor_bytes = std::size_t(256) << 20;
    const std::size_t ceiling_bytes = std::size_t(64) << 30;
    if (eighth < floor_bytes) return floor_bytes;
    if (eighth > ceiling_bytes) return ceiling_bytes;
    return eighth;
}

std::size_t suggested_memory_budget() { return suggested_memory_budget_for(memory_scale_bytes()); }

}  // namespace run_limits
