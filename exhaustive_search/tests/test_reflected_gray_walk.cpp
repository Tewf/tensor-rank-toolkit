/// The base-`p` reflected Gray order is an order on every string, and a ±1 one.
///
/// The subspace walk in [`subspace_walk.h`](../subspace_walk.h) rests entirely
/// on two properties of this successor, and neither is visible in the answers it
/// produces: a leaf that skipped an element would report a subspace has no
/// rank-one basis when it has one, and a leaf that moved two digits at a step
/// would carry a combination that is not the element it thinks it is. Both come
/// back as a wrong rank rather than as a crash, so both are asserted here
/// directly rather than inferred from a verdict downstream.
///
/// Asserted for `p` in {2, 3, 5} and lengths 1 to 4, which is 780 strings and a
/// few microseconds. `p = 2` is the ordinary binary reflected code, and the odd
/// radices are the ones where "reflected" is doing work: a digit there runs up
/// to `p - 1`, turns round and runs back down, rather than alternating.
#include <cstddef>
#include <string>
#include <vector>

#include "check.h"
#include "reflected_gray_walk.h"

namespace {

using bilinear_rank::ReflectedGrayWalk;

/// The string read as a base-`radix` number, which is a distinct index per
/// string and so the seat in the register of what has been visited.
std::size_t seat_of(const std::vector<std::size_t>& string, std::size_t radix) {
    std::size_t seat = 0;
    std::size_t place = 1;
    for (const std::size_t digit : string) {
        seat += digit * place;
        place *= radix;
    }
    return seat;
}

std::size_t power(std::size_t base, std::size_t exponent) {
    std::size_t total = 1;
    for (std::size_t step = 0; step < exponent; ++step) total *= base;
    return total;
}

/// One radix and one length: walk it and report what went wrong, or nothing.
void check_one_shape(std::size_t radix, std::size_t length) {
    const std::string label =
        "p=" + std::to_string(radix) + " dim=" + std::to_string(length);
    const std::size_t expected = power(radix, length);

    ReflectedGrayWalk walk(length, radix);
    std::vector<char> visited(expected, 0);

    // The walk starts on the all-zero string, before any step, which is the
    // element the subspace walk never tests.
    std::size_t zeros = 0;
    for (const std::size_t digit : walk.string()) zeros += digit;
    check::equal(label + ": starts on the all-zero string", static_cast<long long>(zeros), 0);

    std::vector<std::size_t> previous = walk.string();
    visited[seat_of(previous, radix)] = 1;
    std::size_t seen = 1;

    long long revisits = 0;
    long long out_of_range = 0;
    long long not_one_digit = 0;
    long long not_one_step = 0;
    long long misreported = 0;

    ReflectedGrayWalk::Step step;
    while (walk.advance(step)) {
        const std::vector<std::size_t>& current = walk.string();

        std::size_t moved = length;
        std::size_t changes = 0;
        for (std::size_t place = 0; place < length; ++place) {
            if (current[place] >= radix) ++out_of_range;
            if (current[place] == previous[place]) continue;
            ++changes;
            moved = place;
        }
        if (changes != 1) {
            ++not_one_digit;
        } else {
            const std::size_t before = previous[moved];
            const std::size_t after = current[moved];
            const bool upward = after == before + 1;
            const bool downward = before == after + 1;
            if (!upward && !downward) ++not_one_step;
            // The step the walk reports is what the subspace walk acts on, so
            // it has to be the change that actually happened and not merely a
            // change of the right size.
            if (moved != step.digit || upward != step.upward) ++misreported;
        }

        const std::size_t seat = seat_of(current, radix);
        if (visited[seat] != 0) ++revisits;
        visited[seat] = 1;
        ++seen;
        previous = current;
    }

    check::equal(label + ": visits every string", static_cast<long long>(seen),
                 static_cast<long long>(expected));
    check::equal(label + ": and none of them twice", revisits, 0);
    check::equal(label + ": every digit stays below the radix", out_of_range, 0);
    check::equal(label + ": one digit changes a step", not_one_digit, 0);
    check::equal(label + ": and it changes by exactly one", not_one_step, 0);
    check::equal(label + ": the step reported is the step taken", misreported, 0);

    long long unvisited = 0;
    for (const char seat : visited) {
        if (seat == 0) ++unvisited;
    }
    check::equal(label + ": no string left out", unvisited, 0);
}

}  // namespace

int main() {
    for (const std::size_t radix : {std::size_t(2), std::size_t(3), std::size_t(5)}) {
        for (std::size_t length = 1; length <= 4; ++length) check_one_shape(radix, length);
    }

    // A span of dimension zero is a leaf the search does reach, and the walk of
    // it is the zero map alone: one string, no steps, nothing to test.
    {
        ReflectedGrayWalk walk(0, 3);
        ReflectedGrayWalk::Step step;
        check::equal("dimension zero takes no step", walk.advance(step) ? 1 : 0, 0);
    }

    return check::report("reflected gray walk");
}
