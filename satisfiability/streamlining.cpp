#include "streamlining.h"

#include <algorithm>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace satisfiability {

namespace {

/// How many of the three matmul index agreements an entry satisfies. Entry
/// (row, column, slice) decodes as A-entry (i1, i2), B-entry (j1, j2) and
/// C-entry (k1, k2) row-major; type 3 - all three agreements - is exactly the
/// tensor's odd entries, which `streamline_matmul` asserts entry by entry.
int term_type(const MatmulShape& shape, std::size_t row, std::size_t column, std::size_t slice) {
    const std::size_t i2 = row % shape.inner, j1 = column / shape.columns;
    const std::size_t j2 = column % shape.columns, k2 = slice % shape.columns;
    const std::size_t k1 = slice / shape.columns, i1 = row / shape.inner;
    return static_cast<int>(i2 == j1) + static_cast<int>(j2 == k2) + static_cast<int>(k1 == i1);
}

}  // namespace

void streamline_matmul(BinaryEncoding& encoding, const MatmulShape& shape,
                       const Streamliners& devices) {
    const MatmulShape full{encoding.rows / shape.inner, shape.inner,
                           encoding.columns / shape.inner};
    if (full.rows * shape.inner != encoding.rows || full.columns * shape.inner != encoding.columns ||
        full.rows * full.columns != encoding.slices) {
        throw std::invalid_argument("inner dimension " + std::to_string(shape.inner) +
                                    " does not divide this tensor's modes");
    }

    // The parities were added in (row, column, slice) order with one summand
    // per product, so parities[entry].literals[term] is the triple variable
    // t[entry][term]; the type test doubling as a tensor check keeps that
    // coupling honest, since a reordering would scramble the types too.
    linear_algebra::Cnf& formula = encoding.formula;
    std::vector<std::size_t> odd_entries;
    std::vector<int> light_terms;   // triple variables of type-0/1/2 entries
    std::size_t entry = 0;
    for (std::size_t row = 0; row < encoding.rows; ++row) {
        for (std::size_t column = 0; column < encoding.columns; ++column) {
            for (std::size_t slice = 0; slice < encoding.slices; ++slice, ++entry) {
                const int type = term_type(full, row, column, slice);
                const linear_algebra::Cnf::Parity& parity = formula.parities[entry];
                if ((type == 3) != parity.value) {
                    throw std::invalid_argument(
                        "entry types contradict the tensor: not a matmul tensor of this shape");
                }
                if (type == 3) {
                    odd_entries.push_back(entry);
                } else {
                    light_terms.insert(light_terms.end(), parity.literals.begin(),
                                       parity.literals.end());
                }
            }
        }
    }

    std::mt19937_64 draw(devices.seed);

    if (devices.pair_type3) {
        const std::size_t type3 = odd_entries.size(), products = encoding.products;
        if (type3 < products || type3 > 2 * products) {
            throw std::invalid_argument("pairing quota needs r <= T3 <= 2r, got T3 = " +
                                        std::to_string(type3) + " over r = " +
                                        std::to_string(products));
        }
        // T3 - r products carry two odd terms, the rest one: Heule's quota with
        // the 19 and the 4 derived instead of hardcoded.
        std::vector<std::size_t> owners;
        std::vector<std::size_t> order(products);
        for (std::size_t term = 0; term < products; ++term) order[term] = term;
        std::shuffle(order.begin(), order.end(), draw);
        for (std::size_t index = 0; index < products; ++index) {
            owners.push_back(order[index]);
            if (index < type3 - products) owners.push_back(order[index]);
        }
        std::shuffle(owners.begin(), owners.end(), draw);
        std::shuffle(odd_entries.begin(), odd_entries.end(), draw);
        for (std::size_t index = 0; index < odd_entries.size(); ++index) {
            const linear_algebra::Cnf::Parity& parity = formula.parities[odd_entries[index]];
            for (std::size_t term = 0; term < parity.literals.size(); ++term) {
                formula.add_clause({term == owners[index] ? parity.literals[term]
                                                          : -parity.literals[term]});
            }
        }
    }

    if (devices.zero_fraction > 0.0) {
        std::shuffle(light_terms.begin(), light_terms.end(), draw);
        const std::size_t zeroed =
            static_cast<std::size_t>(devices.zero_fraction * static_cast<double>(light_terms.size()));
        for (std::size_t index = 0; index < zeroed && index < light_terms.size(); ++index) {
            formula.add_clause({-light_terms[index]});
        }
    }
}

}  // namespace satisfiability
