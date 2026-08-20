#include "minimise_rank.h"

#include <numeric>

#include "candidate_pool.h"
#include "measures.h"
#include "parallel.h"
#include "minimum_weight_basis.h"
#include "span_basis.h"

namespace bilinear_rank {

namespace {

/// The trial bases the walk has not reached yet, computed on the other cores.
///
/// This is the whole of the parallelism here, and it is safe for one reason:
/// `minimum_weight_basis_with` reads only the map, which does not move while candidates are
/// being tried, and the candidate itself. So the walk stays exactly sequential,
/// with the same order, the same adoptions, the same pruning and the same
/// answer, while the answers it is about to need are prepared behind it.
struct Trials {
    std::vector<std::vector<Matrix>> basis;
    std::vector<char> ready;

    explicit Trials(std::size_t candidates) : basis(candidates), ready(candidates, 0) {}

    void forget(std::size_t candidate) {
        basis[candidate].clear();
        basis[candidate].shrink_to_fit();
        ready[candidate] = 0;
    }
};

template <typename Candidates>
void prefetch(const Field& field, const std::vector<Matrix>& slices,
              const std::vector<std::size_t>& known, const Candidates& candidates,
              const std::vector<std::size_t>& queue, std::size_t from, Trials& trials) {
    const std::size_t ahead = worker_count() * 4;

    std::vector<std::size_t> wanted;
    for (std::size_t step = from; step < queue.size() && wanted.size() < ahead; ++step) {
        if (!trials.ready[queue[step]]) wanted.push_back(queue[step]);
    }
    parallel_for(wanted.size(), [&](std::size_t slot) {
        const std::size_t candidate = wanted[slot];
        trials.basis[candidate] = minimum_weight_basis_with(field, slices, candidates[candidate], known);
        trials.ready[candidate] = 1;
    });
}

/// The trial for one candidate, prepared with its neighbours if there are cores
/// to prepare them on.
template <typename Candidates>
const std::vector<Matrix>& trial_for(const Field& field, const std::vector<Matrix>& slices,
                                     const std::vector<std::size_t>& known,
                                     const Candidates& candidates,
                                     const std::vector<std::size_t>& queue, std::size_t step,
                                     Trials& trials) {
    const std::size_t candidate = queue[step];
    if (!trials.ready[candidate] && worker_count() > 1) {
        prefetch(field, slices, known, candidates, queue, step, trials);
    }
    if (!trials.ready[candidate]) {
        trials.basis[candidate] = minimum_weight_basis_with(field, slices, candidates[candidate], known);
        trials.ready[candidate] = 1;
    }
    return trials.basis[candidate];
}

/// What is left worth trying after a candidate failed to pay.
///
/// The basis that attempt produced spans everything the candidate could have
/// reached, so any later candidate already inside it would fail the same way and
/// is dropped unexamined. This is the irreversible pruning that makes the step a
/// heuristic: a candidate discarded here is never reconsidered, even though a
/// different order might have adopted it.
///
/// Anything dropped also has its trial forgotten, which is what keeps the
/// prepared work bounded by the number of cores rather than by the pool.
template <typename Candidates>
std::vector<std::size_t> survivors_after(const Field& field, const std::vector<Matrix>& attempt,
                                         const std::vector<Matrix>& also_reached,
                                         const Candidates& candidates,
                                         const std::vector<std::size_t>& queue, std::size_t step,
                                         Trials& trials) {
    ReducedBasis reached = linear_algebra::span_of(field, attempt);
    for (const Matrix& kept : also_reached) reached.try_add(kept);

    for (std::size_t gone = 0; gone <= step; ++gone) trials.forget(queue[gone]);

    std::vector<std::size_t> survivors;
    std::vector<Element> scratch;
    for (std::size_t later = step + 1; later < queue.size(); ++later) {
        if (reached.contains(candidates[queue[later]], scratch)) {
            trials.forget(queue[later]);
        } else {
            survivors.push_back(queue[later]);
        }
    }
    return survivors;
}

/// Every candidate, in order, as the walk starts.
std::vector<std::size_t> all_of(std::size_t count) {
    std::vector<std::size_t> queue(count);
    std::iota(queue.begin(), queue.end(), std::size_t(0));
    return queue;
}

/// The shortlist, whichever way the candidates are supplied.
template <typename Candidates>
std::vector<Matrix> improving_over(const Field& field, const std::vector<Matrix>& slices,
                                   const Candidates& candidates) {
    const std::size_t baseline = linear_algebra::multiplication_count(field, slices);
    const std::vector<std::size_t> known = span_element_ranks(field, slices);

    Trials trials(candidates.size());
    std::vector<Matrix> selected;
    std::vector<std::size_t> remaining = all_of(candidates.size());

    for (;;) {
        ReducedBasis span = linear_algebra::span_of(field, slices);
        for (const Matrix& kept : selected) span.try_add(kept);

        bool pruned = false;
        std::vector<Element> scratch;
        for (std::size_t step = 0; step < remaining.size(); ++step) {
            const Matrix& candidate = candidates[remaining[step]];
            if (span.contains(candidate, scratch)) continue;

            const std::vector<Matrix>& attempt =
                trial_for(field, slices, known, candidates, remaining, step, trials);
            if (linear_algebra::multiplication_count(field, attempt) < baseline) {
                span.try_add(candidate);
                selected.push_back(candidate);
                trials.forget(remaining[step]);
                continue;
            }

            remaining =
                survivors_after(field, attempt, selected, candidates, remaining, step, trials);
            pruned = true;
            break;
        }
        if (!pruned) return selected;
    }
}

}  // namespace

std::vector<Matrix> improving_candidates(const Field& field, const std::vector<Matrix>& slices,
                                         const std::vector<Matrix>& candidates) {
    return improving_over(field, slices, Materialised{candidates});
}

std::vector<Matrix> improving_candidates(const Field& field, const std::vector<Matrix>& slices,
                                         const RankOnePool& pool) {
    return improving_over(field, slices, Addressed{pool});
}

std::vector<Matrix> minimise_rank(const Field& field, std::vector<Matrix> slices,
                                  std::vector<Matrix> candidates) {
    Trials trials(candidates.size());
    std::vector<std::size_t> remaining = all_of(candidates.size());

    for (;;) {
        // Recomputed whenever the map moves, which is once per improvement
        // adopted, against once per candidate tried.
        std::vector<std::size_t> known = span_element_ranks(field, slices);
        ReducedBasis span = linear_algebra::span_of(field, slices);
        bool pruned = false;
        std::vector<Element> scratch;

        for (std::size_t step = 0; step < remaining.size(); ++step) {
            const Matrix& candidate = candidates[remaining[step]];
            if (span.contains(candidate, scratch)) continue;

            const std::vector<Matrix>& attempt =
                trial_for(field, slices, known, candidates, remaining, step, trials);
            if (linear_algebra::multiplication_count(field, attempt) <
                linear_algebra::multiplication_count(field, slices)) {
                slices = attempt;  // copied before the trials that hold it are dropped
                known = span_element_ranks(field, slices);
                span = linear_algebra::span_of(field, slices);
                // The map moved, so every prepared answer was about a different
                // question. The walk itself carries on from here, exactly as it
                // did before any of this was parallel.
                trials = Trials(candidates.size());
                continue;
            }

            remaining = survivors_after(field, attempt, {}, candidates, remaining, step, trials);
            pruned = true;
            break;
        }
        if (!pruned) return slices;
    }
}

std::vector<Matrix> descend_from_own_basis(const Field& field, const std::vector<Matrix>& slices) {
    const std::vector<Matrix> basis = minimum_weight_basis(field, slices);
    return minimise_rank(field, basis,
                         improving_candidates(field, basis, rank_one_candidates(field, basis)));
}

}  // namespace bilinear_rank
