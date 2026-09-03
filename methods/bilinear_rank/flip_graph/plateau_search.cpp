#include "plateau_search.h"

#include <numeric>
#include <string>
#include <unordered_set>

#include "measures.h"
#include "span_queries.h"
#include "minimum_weight_basis.h"
#include "span_basis.h"

namespace bilinear_rank {

namespace {

/// A state is its subspace, and `SpanBasis` keeps that in reduced row echelon
/// form, so its rows are the same bytes for every basis of the same space. Two
/// paths that arrive at one subspace are one state.
/// Hashed rather than kept: a subspace of `⟨3,3,3⟩` is 17 KB written out, and a
/// search that visits two hundred thousand of them would spend gigabytes
/// remembering where it had been. A collision costs one skipped state in a
/// method that guarantees nothing anyway, and at 64 bits it will not happen.
std::uint64_t fingerprint(const Field& field, const std::vector<Matrix>& slices) {
    const ReducedBasis span = linear_algebra::span_of(field, slices);
    std::string bytes;
    for (const std::vector<Element>& row : span.rows()) {
        bytes.append(reinterpret_cast<const char*>(row.data()), row.size() * sizeof(Element));
        bytes.push_back('\n');
    }
    return std::hash<std::string>{}(bytes);
}

/// The candidates worth trying: one per orbit when there is a group, all of them
/// when there is not.
///
/// Which generators count is recomputed per state, because the group that makes
/// two candidates interchangeable is the group of the map as it stands now. What
/// each generator *does* to the pool is not: that is fixed, and computing it per
/// state is seconds of matrix multiplication at `⟨3,3,3⟩` against a table lookup.
std::vector<Matrix> worth_trying(const Field& field, const std::vector<Matrix>& slices,
                                 const std::vector<Matrix>& pool,
                                 const std::vector<Automorphism>& ambient,
                                 const std::vector<std::vector<std::uint32_t>>& ambient_action,
                                 const std::vector<std::uint32_t>& everything) {
    if (ambient.empty()) return pool;

    std::vector<std::vector<std::uint32_t>> here;
    for (std::size_t index = 0; index < ambient.size(); ++index) {
        std::vector<Matrix> moved;
        moved.reserve(slices.size());
        for (const Matrix& slice : slices) moved.push_back(act_on(field, ambient[index], slice));
        if (linear_algebra::spans_all(field, moved, slices)) here.push_back(ambient_action[index]);
    }

    std::vector<Matrix> quotiented;
    for (const std::uint32_t index : orbit_representatives(here, everything)) {
        quotiented.push_back(pool[index]);
    }
    return quotiented;
}

/// Everything reachable from here without ever paying more.
struct Plateau {
    const Field& field;
    const std::vector<Matrix>& pool;
    const std::vector<Automorphism>& ambient;
    std::vector<std::vector<std::uint32_t>> ambient_action;  // once, not per state
    std::vector<std::uint32_t> everything;
    std::size_t state_budget;

    std::unordered_set<std::uint64_t> visited;
    std::vector<Matrix> best;
    std::size_t best_cost = 0;
    std::size_t improvements = 0;
    std::size_t sideways = 0;

    void descend(const std::vector<Matrix>& slices, std::size_t depth) {
        if (visited.size() >= state_budget) return;

        const std::size_t cost = linear_algebra::multiplication_count(field, slices);
        if (cost < best_cost) {
            best_cost = cost;
            best = slices;
        }

        const std::vector<Matrix> candidates =
            worth_trying(field, slices, pool, ambient, ambient_action, everything);
        const std::vector<std::size_t> known = span_element_ranks(field, slices);
        const ReducedBasis span = linear_algebra::span_of(field, slices);

        // Downhill first, and a strict improvement earns a fresh depth: the
        // plateau it may now be standing on is a different one.
        std::vector<std::vector<Matrix>> level;
        std::vector<Element> scratch;
        for (const Matrix& candidate : candidates) {
            if (span.contains(candidate, scratch)) continue;

            std::vector<Matrix> attempt = minimum_weight_basis_with(field, slices, candidate, known);
            const std::size_t reached = linear_algebra::multiplication_count(field, attempt);
            if (reached > cost) continue;

            const std::uint64_t mark = fingerprint(field, attempt);
            if (!visited.insert(mark).second) continue;

            if (reached < cost) {
                ++improvements;
                descend(attempt, depth);
                if (visited.size() >= state_budget) return;
            } else if (depth > 0) {
                level.push_back(std::move(attempt));
            }
        }

        for (const std::vector<Matrix>& across : level) {
            ++sideways;
            descend(across, depth - 1);
            if (visited.size() >= state_budget) return;
        }
    }
};

}  // namespace

std::vector<Matrix> cross_plateaus(const Field& field, const std::vector<Matrix>& slices,
                                   const std::vector<Matrix>& pool,
                                   const std::vector<Automorphism>& ambient, std::size_t depth,
                                   std::size_t state_budget, PlateauReport* report) {
    std::vector<std::uint32_t> everything(pool.size());
    std::iota(everything.begin(), everything.end(), std::uint32_t(0));

    Plateau plateau{field,
                    pool,
                    ambient,
                    ambient.empty() ? std::vector<std::vector<std::uint32_t>>{}
                                    : permutation_action_on(field, ambient, pool),
                    std::move(everything),
                    state_budget,
                    {},
                    slices,
                    linear_algebra::multiplication_count(field, slices),
                    0,
                    0};
    plateau.visited.insert(fingerprint(field, slices));
    plateau.descend(slices, depth);

    if (report != nullptr) {
        report->improvements = plateau.improvements;
        report->sideways = plateau.sideways;
        report->states = plateau.visited.size();
        report->best = plateau.best_cost;
    }
    return plateau.best;
}

}  // namespace bilinear_rank
