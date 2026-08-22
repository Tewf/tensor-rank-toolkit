/// That the defaults are derived from the machine and not fitted to this one.
///
/// `adapting-to-the-machine/fitted-or-genuine.md` sorted every constant here
/// into genuine, policy and fitted, and said outright that **a fitted number
/// that ships as a constant is a bug on someone else's hardware**. The memory
/// ceiling was one of the four: 2 GiB, chosen because this laptop has 16 GB.
/// It is now an eighth of what the machine reports, which is the same 2 GiB
/// here and a different number elsewhere.
///
/// **The rule is tested as a pure function of the scale**, because a test that
/// asserted 2 GiB would be a test of this laptop, which is the defect rather
/// than the check. What is asserted against the real machine is only what stays
/// true on any of them: a power of two, not below the physical figure, and a
/// budget inside its own clamps.
#include <cstddef>

#include "check.h"
#include "machine.h"

int main() {
    using bilinear_rank::core_count;
    using bilinear_rank::memory_scale_bytes;
    using bilinear_rank::physical_memory_bytes;
    using bilinear_rank::suggested_memory_budget;
    using bilinear_rank::suggested_memory_budget_for;

    const long long gibibyte = 1LL << 30;
    const long long mebibyte = 1LL << 20;

    // The rule, at the five scales worth naming. The first row is this
    // repository's whole published history: a 16 GB laptop keeps its 2 GiB.
    check::equal("16 GiB machine keeps the shipped budget",
                 static_cast<long long>(suggested_memory_budget_for(16 * gibibyte)), 2 * gibibyte);
    check::equal("4 GiB machine gets an eighth",
                 static_cast<long long>(suggested_memory_budget_for(4 * gibibyte)), 512 * mebibyte);
    check::equal("512 GiB server stops at the ceiling",
                 static_cast<long long>(suggested_memory_budget_for(512 * gibibyte)), 64 * gibibyte);
    check::equal("1 GiB machine stops at the floor",
                 static_cast<long long>(suggested_memory_budget_for(gibibyte)), 256 * mebibyte);
    check::equal("an unreadable machine falls back to the shipped number",
                 static_cast<long long>(suggested_memory_budget_for(0)), 2 * gibibyte);

    // And against whatever this machine actually is, only what is true anywhere.
    const long long physical = static_cast<long long>(physical_memory_bytes());
    const long long scale = static_cast<long long>(memory_scale_bytes());
    if (physical == 0) {
        check::equal("an unreadable machine has no scale either", scale, 0);
    } else {
        check::equal("the scale is at least the physical memory", scale >= physical, 1);
        check::equal("the scale is a power of two", (scale & (scale - 1)) == 0, 1);
        check::equal("the scale is under twice the physical memory", scale < 2 * physical, 1);
    }
    const long long budget = static_cast<long long>(suggested_memory_budget());
    check::equal("the budget is at or above the floor", budget >= 256 * mebibyte, 1);
    check::equal("the budget is at or below the ceiling", budget <= 64 * gibibyte, 1);
    check::equal("there is at least one core", core_count() >= 1, 1);
    return check::report("machine");
}
