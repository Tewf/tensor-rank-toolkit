#include "combinations.h"

#include <numeric>

namespace matrix_sparsification {

std::vector<std::vector<std::size_t>> combinations(std::size_t total, std::size_t size) {
    std::vector<std::vector<std::size_t>> result;
    if (size > total) return result;

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
