#include "streamlining.h"

#include <algorithm>
#include <fstream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace satisfiability {

namespace {

/// A scheme file as the bits of its coefficients: every integer in the file in
/// order, |value| mod 2. The .m layout is nested braces around exactly
/// products x 3 x (rows + columns + slices)... integers, so reading the
/// integers in order is the whole parse and the count is the format check.
std::vector<int> scheme_bits(const std::string& path, std::size_t expected) {
    std::ifstream input(path);
    if (!input) throw std::invalid_argument("cannot read the scheme file " + path);
    std::vector<int> bits;
    char c;
    std::string number;
    auto flush = [&] {
        if (!number.empty() && number != "-") bits.push_back(std::abs(std::stoi(number)) % 2);
        number.clear();
    };
    while (input.get(c)) {
        if (c == '-' || (c >= '0' && c <= '9')) number.push_back(c);
        else flush();
    }
    flush();
    if (bits.size() != expected) {
        throw std::invalid_argument("scheme file holds " + std::to_string(bits.size()) +
                                    " coefficients where this encoding wants " +
                                    std::to_string(expected));
    }
    return bits;
}

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

    // A product's owned entries force alpha, beta and gamma pieces, and through
    // the Tseitin chain every cross combination (a, b, c) of forced pieces is a
    // forced-true triple term. A draw whose exclusivity unit denies a forced
    // term - or a zeroing that hits one - is refuted by propagation before any
    // search: the cross-term channel that killed 197 of 250 instances in
    // [campaign-2026-09-01.md](shaped-encodings/campaign-2026-09-01.md). So the
    // pairing redraws until consistent, and the forced terms leave the zeroing
    // pool.
    std::vector<int> forced_terms;   // triple literals the pairing forces true
    if (devices.pair_type3) {
        const std::size_t type3 = odd_entries.size(), products = encoding.products;
        if (type3 < products || type3 > 2 * products) {
            throw std::invalid_argument("pairing quota needs r <= T3 <= 2r, got T3 = " +
                                        std::to_string(type3) + " over r = " +
                                        std::to_string(products));
        }
        const std::size_t area = encoding.columns * encoding.slices;
        std::vector<std::size_t> owner_of(formula.parities.size(), products);
        bool consistent = false;
        std::vector<std::size_t> owners;
        for (std::size_t attempt = 0; attempt < 500 && !consistent; ++attempt) {
            // T3 - r products carry two odd terms, the rest one: Heule's quota
            // with the 19 and the 4 derived instead of hardcoded.
            owners.clear();
            std::vector<std::size_t> order(products);
            for (std::size_t term = 0; term < products; ++term) order[term] = term;
            std::shuffle(order.begin(), order.end(), draw);
            for (std::size_t index = 0; index < products; ++index) {
                owners.push_back(order[index]);
                if (index < type3 - products) owners.push_back(order[index]);
            }
            std::shuffle(owners.begin(), owners.end(), draw);
            std::shuffle(odd_entries.begin(), odd_entries.end(), draw);
            owner_of.assign(formula.parities.size(), products);
            for (std::size_t index = 0; index < odd_entries.size(); ++index) {
                owner_of[odd_entries[index]] = owners[index];
            }
            consistent = true;
            forced_terms.clear();
            for (std::size_t product = 0; consistent && product < products; ++product) {
                std::vector<std::size_t> lefts, rights, outputs;
                for (std::size_t index = 0; index < odd_entries.size(); ++index) {
                    if (owners[index] != product) continue;
                    const std::size_t entry = odd_entries[index];
                    lefts.push_back(entry / area);
                    rights.push_back(entry % area / encoding.slices);
                    outputs.push_back(entry % encoding.slices);
                }
                for (std::size_t left : lefts) {
                    for (std::size_t right : rights) {
                        for (std::size_t output : outputs) {
                            const std::size_t entry =
                                (left * encoding.columns + right) * encoding.slices + output;
                            if (owner_of[entry] == product) continue;   // its own unit
                            if (owner_of[entry] < products) {           // odd, owned elsewhere
                                consistent = false;
                            } else {                                    // even: protect it
                                forced_terms.push_back(formula.parities[entry].literals[product]);
                            }
                        }
                    }
                }
            }
        }
        if (!consistent) {
            throw std::runtime_error("no consistent pairing in 500 draws for seed " +
                                     std::to_string(devices.seed));
        }
        for (std::size_t index = 0; index < odd_entries.size(); ++index) {
            const linear_algebra::Cnf::Parity& parity = formula.parities[odd_entries[index]];
            for (std::size_t term = 0; term < parity.literals.size(); ++term) {
                formula.add_clause({term == owners[index] ? parity.literals[term]
                                                          : -parity.literals[term]});
            }
        }
    }

    if (!devices.fixing_scheme.empty() && devices.fixing_fraction > 0.0) {
        // The scheme's bits in the file's order: per product, the A matrix, the
        // B matrix, the C matrix, row-major - which is exactly this encoding's
        // left/right/output coordinate order.
        const std::size_t per_product = encoding.rows + encoding.columns + encoding.slices;
        const std::vector<int> bits =
            scheme_bits(devices.fixing_scheme, encoding.products * per_product);
        // Scheme files disagree on the third matrix's orientation - the matmul
        // tensor's own third index is (result column, result row), so many
        // formats store C transposed, the same convention split the two
        // Algorithm types document. The 729 Brent equations are the arbiter:
        // whichever orientation satisfies them all is the file's, and a file
        // satisfying neither is refused before a single unit is emitted, since
        // fixing a wrong solution would make every no downstream meaningless.
        const std::size_t result_columns = full.columns;
        auto slice_offset = [&](std::size_t slice, bool transposed) {
            if (!transposed) return slice;
            return slice % result_columns * full.rows + slice / result_columns;
        };
        auto bit = [&](std::size_t product, std::size_t offset) {
            return bits[product * per_product + offset];
        };
        auto satisfies = [&](bool transposed) {
            std::size_t index = 0;
            for (std::size_t row = 0; row < encoding.rows; ++row) {
                for (std::size_t column = 0; column < encoding.columns; ++column) {
                    for (std::size_t slice = 0; slice < encoding.slices; ++slice, ++index) {
                        int parity = 0;
                        const std::size_t offset = encoding.rows + encoding.columns +
                                                   slice_offset(slice, transposed);
                        for (std::size_t product = 0; product < encoding.products; ++product) {
                            parity ^= bit(product, row) & bit(product, encoding.rows + column) &
                                      bit(product, offset);
                        }
                        if ((parity != 0) != formula.parities[index].value) return false;
                    }
                }
            }
            return true;
        };
        const bool direct = satisfies(false);
        if (!direct && !satisfies(true)) {
            throw std::invalid_argument(
                "the scheme does not satisfy this tensor's Brent equations in either "
                "orientation of its third matrix");
        }
        const bool transposed = !direct;
        // Fix the drawn share of the operator variables to the scheme's values.
        std::vector<std::pair<int, int>> assignments;   // (variable, value)
        for (std::size_t product = 0; product < encoding.products; ++product) {
            for (std::size_t offset = 0; offset < encoding.rows; ++offset) {
                assignments.push_back({encoding.left[product * encoding.rows + offset],
                                       bit(product, offset)});
            }
            for (std::size_t offset = 0; offset < encoding.columns; ++offset) {
                assignments.push_back({encoding.right[product * encoding.columns + offset],
                                       bit(product, encoding.rows + offset)});
            }
            for (std::size_t offset = 0; offset < encoding.slices; ++offset) {
                assignments.push_back(
                    {encoding.output[product * encoding.slices + offset],
                     bit(product, encoding.rows + encoding.columns +
                                      slice_offset(offset, transposed))});
            }
        }
        std::shuffle(assignments.begin(), assignments.end(), draw);
        const std::size_t fixed = static_cast<std::size_t>(
            devices.fixing_fraction * static_cast<double>(assignments.size()));
        for (std::size_t position = 0; position < fixed && position < assignments.size();
             ++position) {
            formula.add_clause(
                {assignments[position].second ? assignments[position].first
                                              : -assignments[position].first});
        }
    }

    if (devices.zero_fraction > 0.0) {
        std::sort(forced_terms.begin(), forced_terms.end());
        std::vector<int> pool;
        for (int literal : light_terms) {
            if (!std::binary_search(forced_terms.begin(), forced_terms.end(), literal)) {
                pool.push_back(literal);
            }
        }
        std::shuffle(pool.begin(), pool.end(), draw);
        const std::size_t zeroed =
            static_cast<std::size_t>(devices.zero_fraction * static_cast<double>(pool.size()));
        for (std::size_t index = 0; index < zeroed && index < pool.size(); ++index) {
            formula.add_clause({-pool[index]});
        }
    }
}

}  // namespace satisfiability
