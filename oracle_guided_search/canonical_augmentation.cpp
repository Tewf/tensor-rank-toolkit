#include "canonical_augmentation.h"

#include <algorithm>
#include <cstdint>
#include <exception>
#include <mutex>

#include "algorithm_recovery.h"
#include "canonical_parent.h"
#include "exhaustive_search.h"
#include "exit_code.h"
#include "parallel.h"
#include "span_basis.h"
#include "timing.h"

namespace bilinear_rank {

namespace {

/// One walk of the tree.
struct Walk {
    const Field* field = nullptr;
    const linear_algebra::Tensor* tensor = nullptr;
    const std::vector<Matrix>* pool = nullptr;
    const std::vector<Automorphism>* group = nullptr;
    /// The span dimension the walk starts from, so a branch named by the pool
    /// elements it added knows what dimension it sits at.
    std::size_t base = 0;
    std::size_t target = 0;
    bool canonical = false;
    /// Stop as soon as one solution is in hand, for a caller that is deciding
    /// rather than counting. Off by default, because a count that stopped early
    /// is not a count.
    bool stop_at_first = false;
    /// Every distinct solution subspace's code, kept only so the plain route can
    /// report the true count. It is exactly the storage the canonical route does
    /// without, which is the point of using a parent test instead of a seen-set.
    std::vector<SubspaceCode> seen;
    EnumerationReport report;
};

/// Emit `child` if its rank-one content really is a decomposition, after multiplying
/// it out against the map.
void emit_if_solution(Walk& walk, const std::vector<Matrix>& child) {
    const Field& field = *walk.field;
    std::vector<Element> scratch;
    const std::vector<Matrix> products = independent_rank_one_maps_in(
        field, linear_algebra::span_of(field, child),
        linear_algebra::flattened_width<Field>(child), *walk.pool, walk.target, scratch);
    if (products.size() != walk.target) return;

    Algorithm algorithm;
    if (!recovers_map(field, walk.tensor->slices, products, algorithm)) {
        throw cli::CheckFailed("an enumerated subspace's rank-one basis does not compute the map");
    }
    ++walk.report.emitted;

    const SubspaceCode code = subspace_code(field, child);
    if (std::find(walk.seen.begin(), walk.seen.end(), code) != walk.seen.end()) return;
    walk.seen.push_back(code);
    ++walk.report.distinct;
    walk.report.decompositions.push_back(products);
}

/// The pool indices worth trying from this node.
///
/// Without the group, the ordering constraint is all there is, which is the existing
/// behaviour. With it, one candidate per orbit of the current subspace's stabiliser,
/// recomputed here because the object has moved and a quotient taken earlier is stale.
std::vector<std::size_t> augmentations(const Walk& walk, const std::vector<Matrix>& current,
                                       const ReducedBasis& span, std::size_t from) {
    const Field& field = *walk.field;
    const std::vector<Matrix>& pool = *walk.pool;
    std::vector<Element> scratch;

    if (!walk.canonical || walk.group->empty()) {
        std::vector<std::size_t> plain;
        for (std::size_t index = from; index < pool.size(); ++index) {
            if (!span.contains(pool[index], scratch)) plain.push_back(index);
        }
        return plain;
    }

    std::vector<std::uint32_t> outside;
    for (std::uint32_t index = 0; index < pool.size(); ++index) {
        if (!span.contains(pool[index], scratch)) outside.push_back(index);
    }
    const std::vector<Automorphism> stabiliser = stabiliser_of(field, current, *walk.group);
    const std::vector<std::vector<std::uint32_t>> action = permutation_action_on(field, stabiliser, pool);

    std::vector<std::size_t> representatives;
    for (const std::uint32_t index : orbit_representatives(action, outside)) {
        representatives.push_back(index);
    }
    return representatives;
}

void descend(Walk& walk, const std::vector<Matrix>& current, std::size_t dimension,
             std::size_t from) {
    if (walk.stop_at_first && !walk.report.decompositions.empty()) return;
    ++walk.report.nodes;
    if (dimension == walk.target) {
        emit_if_solution(walk, current);
        return;
    }

    const Field& field = *walk.field;
    const std::vector<Matrix>& pool = *walk.pool;
    const ReducedBasis span = linear_algebra::span_of(field, current);
    const SubspaceCode current_code = subspace_code(field, current);

    for (const std::size_t index : augmentations(walk, current, span, from)) {
        std::vector<Matrix> child = current;
        child.push_back(pool[index]);
        if (walk.canonical && !walk.group->empty()) {
            const ParentTest test = is_canonical_augmentation(
                field, walk.tensor->slices, child, current_code, pool[index], pool, *walk.group);
            walk.report.group_visits += test.group_visits;
            if (!test.accepted) continue;
        }
        descend(walk, child, dimension + 1, index + 1);
        if (walk.stop_at_first && !walk.report.decompositions.empty()) return;
    }
}

/// Fold one branch's walk into the total.
///
/// Every number here is a sum except `distinct`, which is the size of a union, so
/// none of them depends on how the branches were divided up or on the order they
/// finished in. That is what makes this enumeration safe to spread over cores: a
/// branch reports a count of a subtree it walked out, not a race to a witness.
/// `seen` and `report.decompositions` are pushed together in `emit_if_solution`
/// and so stay index for index alongside each other, which is what lets a
/// duplicate be dropped together with its decomposition.
void absorb(Walk& total, const Walk& branch) {
    total.report.emitted += branch.report.emitted;
    total.report.nodes += branch.report.nodes;
    total.report.group_visits += branch.report.group_visits;
    for (std::size_t which = 0; which < branch.seen.size(); ++which) {
        const SubspaceCode& code = branch.seen[which];
        if (std::find(total.seen.begin(), total.seen.end(), code) != total.seen.end()) continue;
        total.seen.push_back(code);
        ++total.report.distinct;
        total.report.decompositions.push_back(branch.report.decompositions[which]);
    }
}

/// A subtree, named by the pool elements added to the root rather than by the
/// subspace they build.
///
/// Indices and not matrices, because many of them are held at once: at `⟨3,3,3⟩`
/// one node has 261 121 children, which is 2 MB of indices and 1.5 GB of
/// subspaces.
struct Branch {
    std::vector<std::size_t> added;
    std::size_t from = 0;
};

/// Do one node's own work and hand back its accepted children, instead of
/// recursing into them.
///
/// This is `descend` with the recursive call removed, and it has to stay that way
/// line for line: the node is counted here, a leaf is emitted here, and the parent
/// test's group visits are charged here, so that a walk split across cores counts
/// exactly what the sequential one counts.
void expand_one(Walk& walk, const std::vector<Matrix>& root, const Branch& node,
                std::vector<Branch>& children) {
    const Field& field = *walk.field;
    const std::vector<Matrix>& pool = *walk.pool;

    std::vector<Matrix> current = root;
    for (const std::size_t index : node.added) current.push_back(pool[index]);

    ++walk.report.nodes;
    const std::size_t dimension = walk.base + node.added.size();
    if (dimension == walk.target) {
        emit_if_solution(walk, current);
        return;
    }

    const ReducedBasis span = linear_algebra::span_of(field, current);
    const SubspaceCode current_code = subspace_code(field, current);
    for (const std::size_t index : augmentations(walk, current, span, node.from)) {
        std::vector<Matrix> child = current;
        child.push_back(pool[index]);
        if (walk.canonical && !walk.group->empty()) {
            const ParentTest test = is_canonical_augmentation(
                field, walk.tensor->slices, child, current_code, pool[index], pool, *walk.group);
            walk.report.group_visits += test.group_visits;
            if (!test.accepted) continue;
        }
        Branch next;
        next.added = node.added;
        next.added.push_back(index);
        next.from = index + 1;
        children.push_back(std::move(next));
    }
}

/// The same walk, its subtrees spread over cores.
///
/// **This counts rather than stops, which is what makes it the safe one.** The
/// plain exact search shares a `SearchBudget` and races to a witness, so workers
/// dispatch subtrees a sequential walk never reaches and a tight `--node-limit`
/// can turn a proof into an undecided
/// ([`what-threads-change.md`](../exhaustive_search/what-threads-change.md)).
/// Here there is no witness, no early exit and no shared budget: every subtree is
/// visited whatever the thread count, so `emitted`, `distinct`, `nodes` and
/// `group_visits` are identical at one thread and at twelve, which
/// `tests/test_canonical_augmentation.cpp` asserts rather than assumes.
///
/// **The split goes deeper than the root when the root is too narrow**, which the
/// canonical route is: it augments by one pool element per orbit, and `⟨2,2,2⟩` has
/// five, so a root-only split would leave seven cores of twelve with nothing to do.
/// The prefix above the frontier is walked here, sequentially, and counted exactly
/// as `descend` counts it.
///
/// A branch keeps its own `Walk`, so the only things shared are the field, the
/// pool, the group and the tensor, all read-only, and the exception slot. A check
/// that fails inside a worker has to be caught there: an exception crossing a
/// `std::thread` boundary is a terminate and not a diagnostic.
void descend_in_parallel(Walk& walk, const std::vector<Matrix>& root) {
    // Widen the frontier one node at a time, oldest first, until it holds at least
    // as many independent subtrees as there are workers. One node at a time and
    // not one level, so that the live frontier is bounded by one node's children
    // plus the worker count rather than by the widest level of the tree.
    std::vector<Branch> frontier(1);
    std::size_t taken = 0;
    while (taken < frontier.size() && frontier.size() - taken < worker_count()) {
        const Branch node = std::move(frontier[taken]);
        ++taken;
        expand_one(walk, root, node, frontier);
    }
    if (taken == frontier.size()) return;  // the prefix was the whole walk

    // Copied with the counts cleared, so a branch starts from the walk's
    // configuration and none of its totals.
    Walk fresh = walk;
    fresh.report = EnumerationReport();
    fresh.seen.clear();
    std::vector<Walk> branches(frontier.size() - taken, fresh);

    std::exception_ptr failure;
    std::mutex handover;
    parallel_for(branches.size(), [&](std::size_t offset) {
        try {
            const Branch& node = frontier[taken + offset];
            std::vector<Matrix> subspace = root;
            for (const std::size_t index : node.added) subspace.push_back((*walk.pool)[index]);
            descend(branches[offset], subspace, walk.base + node.added.size(), node.from);
        } catch (...) {
            const std::lock_guard<std::mutex> keep(handover);
            if (!failure) failure = std::current_exception();
        }
    });
    if (failure) std::rethrow_exception(failure);

    for (const Walk& branch : branches) absorb(walk, branch);
}

}  // namespace

EnumerationReport enumerate_solution_subspaces(const Field& field,
                                               const linear_algebra::Tensor& tensor,
                                               const std::vector<Matrix>& pool,
                                               const std::vector<Automorphism>& group,
                                               std::size_t target, bool canonical,
                                               bool stop_at_first) {
    const cli::Clock::time_point started = cli::Clock::now();
    Walk walk;
    walk.field = &field;
    walk.tensor = &tensor;
    walk.pool = &pool;
    walk.stop_at_first = stop_at_first;
    walk.group = &group;
    walk.target = target;
    walk.canonical = canonical;

    walk.base = linear_algebra::span_of(field, tensor.slices).dimension();
    if (walk.base <= target) {
        // Spread over cores unless the counts would stop meaning the same thing.
        // `stop_at_first` is the one route where they would: it abandons a walk
        // the moment a solution appears, so which subtrees were in flight decides
        // its totals, and that is a race rather than a count.
        if (worker_count() > 1 && !stop_at_first) {
            descend_in_parallel(walk, tensor.slices);
        } else {
            descend(walk, tensor.slices, walk.base, 0);
        }
    }

    walk.report.seconds = cli::elapsed_seconds(started);
    return walk.report;
}

}  // namespace bilinear_rank
