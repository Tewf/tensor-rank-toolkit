#include "combinations.h"

#include <limits>
#include <numeric>
#include <string>

#include "memory_budget.h"

namespace matrix_sparsification {

namespace {

/// `C(total, size)`, saturating rather than wrapping.
///
/// Multiply-then-divide in this order keeps every partial product an exact
/// binomial coefficient, so the only thing that can go wrong is the one
/// multiplication, and it is checked. Saturating is enough: anything at the
/// ceiling is past any budget, and `require_room` prints the ceiling rather than
/// a number that wrapped into looking affordable.
std::size_t how_many_subsets(std::size_t total, std::size_t size) {
    if (size > total) return 0;
    const std::size_t ceiling = std::numeric_limits<std::size_t>::max();
    const std::size_t shorter = size < total - size ? size : total - size;
    std::size_t count = 1;
    for (std::size_t step = 1; step <= shorter; ++step) {
        if (count > ceiling / (total - shorter + step)) return ceiling;
        count = count * (total - shorter + step) / step;
    }
    return count;
}

}  // namespace

/// **Priced, because the count is `C(total, size)` and the operators this is
/// pointed at are not always the 7x4 ones shipped.** `minimise-rank
/// --emit-operators` writes the operator of whatever was searched, and the README
/// invites feeding those back in: the top-down sparsifier on a `⟨4,4,4⟩`
/// operator asks for `C(47, 23)`, about 1.6e13 subsets, which was a `reserve`-free
/// `push_back` loop that grew until the kernel stopped it. A refusal naming the
/// number is a result; an out-of-memory kill is not.
std::vector<std::vector<std::size_t>> combinations(std::size_t total, std::size_t size) {
    std::vector<std::vector<std::size_t>> result;
    if (size > total) return result;

    bilinear_rank::require_room(
        "the " + std::to_string(size) + "-subsets of " + std::to_string(total) + " columns",
        how_many_subsets(total, size), sizeof(std::vector<std::size_t>) + sizeof(std::size_t) * size);
    result.reserve(how_many_subsets(total, size));

    std::vector<std::size_t> chosen(size);
    std::iota(chosen.begin(), chosen.end(), std::size_t(0));
    for (;;) {
        result.push_back(chosen);
        std::size_t position = size;
        while (position > 0 && chosen[position - 1] == total - size + position - 1) --position;
        if (position == 0) return result;
        ++chosen[position - 1];
        for (std::size_t later = position; later < size; ++later) {
            chosen[later] = chosen[later - 1] + 1;
        }
    }
}

}  // namespace matrix_sparsification
