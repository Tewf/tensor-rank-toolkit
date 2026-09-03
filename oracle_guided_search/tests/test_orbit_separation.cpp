// Does the canonical form separate the orbits, or only respect them?
//
// Respecting them is easy and worthless. Any function constant on orbits passes an
// invariance check (the constant function does), and the failure that costs
// something is the other direction: a form that gives **two inequivalent sets the
// same name** merges their classes, so the enumeration reaches one of them and
// skips the other, and a search that was going to refute a rank refutes it on fewer
// subspaces than it thinks. There is no downstream check for that. The
// enumeration's own totals do not catch it, because a coarser form gives a smaller
// node count, which is what a better form gives too.
//
// So this is not a spot check on the sets a span happens to produce. At `⟨2,2,2⟩`
// the group is 216 elements, so the orbit partition of **every** k-subset of the
// 225-element pool is available by definition rather than by algorithm, and the
// partition the canonical form induces is held against it entry for entry, for
// k = 2 and k = 3. Merging and splitting are counted apart, because they are
// different faults and only one of them is dangerous.
//
// The packed key here is the sorted subset read as a number in base `pool.size()`,
// which orders subsets of one size exactly as their sorted index sequences do. So
// the whole-group minimum is the same object `canonical` returns, and the two can
// be compared for equality as well as for inducing the same partition.
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "candidate_pool.h"
#include "check.h"
#include "group_construction.h"
#include "map_construction.h"
#include "pool_action_table.h"
#include "pool_set_canon.h"
#include "pool_sets.h"

namespace {

using Key = std::uint64_t;

Key packed(const std::vector<std::size_t>& subset, std::size_t pool_size) {
    Key key = 0;
    for (const std::size_t index : subset) key = key * pool_size + index;
    return key;
}

/// The least image of `subset` over **every** element of the group.
Key least_over_the_group(const std::vector<std::size_t>& subset,
                         const std::vector<std::vector<std::uint32_t>>& table,
                         std::size_t pool_size, std::vector<std::size_t>& image) {
    Key least = 0;
    bool started = false;
    for (const std::vector<std::uint32_t>& moves : table) {
        for (std::size_t which = 0; which < subset.size(); ++which) {
            image[which] = moves[subset[which]];
        }
        std::sort(image.begin(), image.end());
        const Key key = packed(image, pool_size);
        if (!started || key < least) {
            least = key;
            started = true;
        }
    }
    return least;
}

struct Verdict {
    long long subsets = 0;
    long long orbits = 0;
    long long names = 0;
    /// Subsets whose canonical form is not the whole-group minimum itself.
    long long differing_keys = 0;
    /// Subsets whose name is shared with a set in another orbit: the dangerous one.
    long long merged = 0;
    /// Subsets in an orbit that the name splits in two.
    long long split = 0;
};

/// Every k-subset of `pool_size`, both partitions, compared.
Verdict partitions_of_subsets(std::size_t width, std::size_t pool_size,
                              const bilinear_rank::PoolSetCanon& canon,
                              const std::vector<std::vector<std::uint32_t>>& table) {
    Verdict verdict;
    std::unordered_map<Key, Key> orbit_to_name;
    std::unordered_map<Key, Key> name_to_orbit;

    std::vector<std::size_t> subset(width);
    std::vector<std::size_t> image(width);
    // An odometer over strictly increasing indices, so every subset is visited once
    // and no list of them is ever held: there are 1 873 200 at width three.
    for (std::size_t which = 0; which < width; ++which) subset[which] = which;
    while (true) {
        const Key orbit = least_over_the_group(subset, table, pool_size, image);
        const Key name = packed(canon.canonical(subset), pool_size);
        ++verdict.subsets;
        if (orbit != name) ++verdict.differing_keys;

        const auto named = orbit_to_name.emplace(orbit, name);
        if (!named.second && named.first->second != name) ++verdict.split;
        const auto orbited = name_to_orbit.emplace(name, orbit);
        if (!orbited.second && orbited.first->second != orbit) ++verdict.merged;

        std::size_t position = width;
        while (position > 0 && subset[position - 1] == pool_size - width + position - 1) {
            --position;
        }
        if (position == 0) break;
        ++subset[position - 1];
        for (std::size_t which = position; which < width; ++which) {
            subset[which] = subset[which - 1] + 1;
        }
    }
    verdict.orbits = static_cast<long long>(orbit_to_name.size());
    verdict.names = static_cast<long long>(name_to_orbit.size());
    return verdict;
}

void report_on(const std::string& what, const Verdict& verdict) {
    std::cout << "  " << what << ": " << verdict.subsets << " subsets, " << verdict.orbits
              << " orbits, " << verdict.names << " names\n";
    check::equal(what + ", orbits found by walking all 216 elements", verdict.orbits,
                 verdict.names);
    check::equal(what + ", subsets the canonical form merges into another orbit",
                 verdict.merged, 0);
    check::equal(what + ", subsets whose orbit the canonical form splits", verdict.split, 0);
    check::equal(what + ", subsets whose name is not the whole-group minimum",
                 verdict.differing_keys, 0);
}

}  // namespace

int main() {
    const pool_sets::MatmulShape shape{2, 2, 2};
    const bilinear_rank::Field field(2);
    const std::vector<bilinear_rank::Matrix> pool =
        bilinear_rank::all_rank_one_maps(field, shape.slice_rows(), shape.slice_columns());
    const std::vector<bilinear_rank::Automorphism> whole =
        bilinear_rank::matrix_multiplication_symmetries(field, shape.rows, shape.inner,
                                                        shape.columns);
    const bilinear_rank::PoolSetCanon canon(
        field,
        bilinear_rank::matrix_multiplication_symmetry_generators(field, shape.rows, shape.inner,
                                                                 shape.columns),
        shape.slice_rows(), shape.slice_columns());

    check::equal("the pool of <2,2,2>", static_cast<long long>(pool.size()), 225);
    check::equal("the group the oracle walks", static_cast<long long>(whole.size()), 216);

    const std::vector<std::vector<std::uint32_t>> table =
        pool_action_table::of(field, whole, shape.slice_rows(), shape.slice_columns());

    report_on("pairs", partitions_of_subsets(2, pool.size(), canon, table));
    report_on("triples", partitions_of_subsets(3, pool.size(), canon, table));

    return check::report("the orbit partition of every small subset");
}
