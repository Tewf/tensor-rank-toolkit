#include "pool_cosets.h"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <limits>
#include <unordered_map>

#include "span_basis.h"

namespace bilinear_rank {

namespace {

constexpr std::size_t no_coset = std::numeric_limits<std::size_t>::max();

/// The residue's scalar class, so that two pool elements land together exactly
/// when they lie in one line of the quotient.
///
/// Over GF(2) this only ever divides by one, and doing it anyway is what keeps the
/// class correct over every other characteristic: `span(current) + <added>` holds
/// `x` when `x`'s residue is **any** nonzero multiple of `added`'s, not only when
/// it is equal to it.
void normalise(const Field& field, std::vector<Element>& residue) {
    for (Element& entry : residue) {
        if (field.isZero(entry)) continue;
        Element scale;
        field.inv(scale, entry);
        for (Element& other : residue) field.mulin(other, scale);
        return;
    }
}

/// FNV-1a over the residue, to find the coset's bucket without comparing every
/// representative. A collision costs a comparison and never an answer: the
/// representatives themselves decide.
std::uint64_t hash_of(const std::vector<Element>& residue) {
    std::uint64_t value = 1469598103934665603ULL;
    for (const Element& entry : residue) {
        value ^= static_cast<std::uint64_t>(entry);
        value *= 1099511628211ULL;
    }
    return value;
}

}  // namespace

PoolCosets::PoolCosets(const Field& field, const std::vector<Matrix>& pool,
                       const std::vector<Matrix>& generators) {
    const ReducedBasis span = linear_algebra::span_of(field, generators);
    coset_of_.assign(pool.size(), no_coset);

    std::vector<std::vector<Element>> representatives;
    std::unordered_map<std::uint64_t, std::vector<std::size_t>> buckets;
    std::vector<Element> residue;
    for (std::size_t index = 0; index < pool.size(); ++index) {
        residue.assign(pool[index].data(), pool[index].data() + pool[index].entry_count());
        span.reduce(residue);
        if (std::all_of(residue.begin(), residue.end(),
                        [&](const Element& entry) { return field.isZero(entry); })) {
            inside_.push_back(index);
            continue;
        }
        outside_.push_back(static_cast<std::uint32_t>(index));
        normalise(field, residue);

        std::vector<std::size_t>& bucket = buckets[hash_of(residue)];
        std::size_t found = no_coset;
        for (const std::size_t candidate : bucket) {
            if (representatives[candidate] == residue) {
                found = candidate;
                break;
            }
        }
        if (found == no_coset) {
            found = representatives.size();
            representatives.push_back(residue);
            cosets_.emplace_back();
            bucket.push_back(found);
        }
        coset_of_[index] = found;
        cosets_[found].push_back(index);
    }
}

std::vector<std::size_t> PoolCosets::extended_by(std::size_t added) const {
    const std::size_t coset = coset_of_[added];
    if (coset == no_coset) return inside_;

    // Both lists are increasing, so the union is one merge and the result is the
    // order `pool_inside` would have produced, which is the contract.
    const std::vector<std::size_t>& joining = cosets_[coset];
    std::vector<std::size_t> content;
    content.reserve(inside_.size() + joining.size());
    std::merge(inside_.begin(), inside_.end(), joining.begin(), joining.end(),
               std::back_inserter(content));
    return content;
}

}  // namespace bilinear_rank
