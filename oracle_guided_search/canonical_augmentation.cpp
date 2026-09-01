#include "canonical_augmentation.h"

#include <optional>

#include "pool_cosets.h"
#include "pool_orbits.h"
#include "pool_set_canon.h"

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
    const formats::Tensor* tensor = nullptr;
    const std::vector<Matrix>* pool = nullptr;
    const std::vector<Automorphism>* group = nullptr;
    /// Built once for the whole walk. Presenting the group costs more than one
    /// parent test does, and every test wants the same presentation.
    const PoolSetCanon* canon = nullptr;
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
std::vector<std::size_t> plain_augmentations(const std::vector<Matrix>& pool,
                                            const ReducedBasis& span, std::size_t from) {
    std::vector<Element> scratch;
    std::vector<std::size_t> plain;
    for (std::size_t index = from; index < pool.size(); ++index) {
        if (!span.contains(pool[index], scratch)) plain.push_back(index);
    }
    return plain;
}

/// The same, quotiented, from a node's cosets rather than from its own scans.
///
/// `cosets` has already separated the pool into what lies inside this subspace and
/// what lies outside it, which is the scan this function used to make for itself.
std::vector<std::size_t> augmentations(const Walk& walk, const PoolCosets& cosets) {
    // The subgroup fixing this subspace, by backtrack from generators rather than
    // by filtering a group held as a list. That list was the last `|G|` dependency
    // here, and it is why the canonical route refused `<3,3,3>` outright: the
    // group is 4 741 632 elements and 6.2 GiB. `pool_set_canon.h` argues why the
    // setwise stabiliser of the pool content is the stabiliser of the subspace.
    const std::vector<std::vector<std::uint32_t>> action =
        walk.canon->stabiliser_generators(cosets.inside());

    std::vector<std::size_t> representatives;
    for (const std::uint32_t index : orbit_representatives(action, cosets.outside())) {
        representatives.push_back(index);
    }
    return representatives;
}


/// Every child this node offers, with the parent test already applied, handed to a
/// visitor that says whether to stop.
///
/// `descend` and `expand_one` differ only in what they do with an accepted child —
/// one recurses, the other hands back a branch — so the deciding lives here once.
/// It used to be written out in both, with a comment requiring them to stay equal
/// line for line; a shared body is the version of that requirement a compiler can
/// keep.
///
/// **One `PoolCosets` a node, not one pool scan a candidate child.** It answers
/// what is inside this subspace, what is outside it, and what is inside each child,
/// from a single reduction of every pool element. `pool_cosets.h` has the argument
/// and the measurement that asked for it.
template <class Accept>
void offer_children(Walk& walk, const std::vector<Matrix>& current, std::size_t from,
                    const Accept& accept) {
    const Field& field = *walk.field;
    const std::vector<Matrix>& pool = *walk.pool;

    if (!walk.canonical || walk.group->empty()) {
        const ReducedBasis span = linear_algebra::span_of(field, current);
        for (const std::size_t index : plain_augmentations(pool, span, from)) {
            std::vector<Matrix> child = current;
            child.push_back(pool[index]);
            if (accept(index, child)) return;
        }
        return;
    }

    const SubspaceCode current_code = subspace_code(field, current);
    const PoolCosets cosets(field, pool, current);
    // This node's own canonical name, which every one of its children's parent
    // tests would otherwise compute again: they all have the same parent, and it
    // is this node. Charged as the canonisation it is.
    const std::vector<std::size_t> current_name = walk.canon->canonical(cosets.inside());
    ++walk.report.canonisations;
    for (const std::size_t index : augmentations(walk, cosets)) {
        std::vector<Matrix> child = current;
        child.push_back(pool[index]);
        const ParentTest test = is_canonical_augmentation(
            field, walk.tensor->slices, child, current_code, current_name, index,
            cosets.extended_by(index), pool, *walk.canon);
        walk.report.group_visits += test.group_visits;
        walk.report.canonisations += test.canonisations;
        if (!test.accepted) continue;
        if (accept(index, child)) return;
    }
}

void descend(Walk& walk, const std::vector<Matrix>& current, std::size_t dimension,
             std::size_t from) {
    if (walk.stop_at_first && !walk.report.decompositions.empty()) return;
    ++walk.report.nodes;
    if (dimension == walk.target) {
        emit_if_solution(walk, current);
        return;
    }

    offer_children(walk, current, from, [&](std::size_t index, const std::vector<Matrix>& child) {
        descend(walk, child, dimension + 1, index + 1);
        return walk.stop_at_first && !walk.report.decompositions.empty();
    });
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
/// This is `descend` with the recursion replaced by a push. The node is counted
/// here, a leaf is emitted here, and the parent test's group visits are charged in
/// `offer_children`, which both walks share — so a walk split across cores counts
/// exactly what the sequential one counts, by construction rather than by a comment
/// asking two copies to stay equal.
void expand_one(Walk& walk, const std::vector<Matrix>& root, const Branch& node,
                std::vector<Branch>& children) {
    std::vector<Matrix> current = root;
    for (const std::size_t index : node.added) current.push_back((*walk.pool)[index]);

    ++walk.report.nodes;
    const std::size_t dimension = walk.base + node.added.size();
    if (dimension == walk.target) {
        emit_if_solution(walk, current);
        return;
    }

    offer_children(walk, current, node.from, [&](std::size_t index, const std::vector<Matrix>&) {
        Branch next;
        next.added = node.added;
        next.added.push_back(index);
        next.from = index + 1;
        children.push_back(std::move(next));
        return false;
    });
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
    while (taken < frontier.size() && frontier.size() - taken < run_limits::worker_count()) {
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
    run_limits::parallel_for(branches.size(), [&](std::size_t offset) {
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
                                               const formats::Tensor& tensor,
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

    // Presented once, and only when the parent test will ask. Building it walks
    // the group to work out how it permutes the two vector lists, which is the
    // last `|G|` pass left here and happens once for the whole enumeration rather
    // than once per candidate parent.
    std::optional<PoolSetCanon> canoniser;
    if (canonical && !group.empty() && !pool.empty()) {
        canoniser.emplace(field, group, pool.front().rows(), pool.front().columns());
        walk.canon = &canoniser.value();
    }

    walk.base = linear_algebra::span_of(field, tensor.slices).dimension();
    if (walk.base <= target) {
        // Spread over cores unless the counts would stop meaning the same thing.
        // `stop_at_first` is the one route where they would: it abandons a walk
        // the moment a solution appears, so which subtrees were in flight decides
        // its totals, and that is a race rather than a count.
        if (run_limits::worker_count() > 1 && !stop_at_first) {
            descend_in_parallel(walk, tensor.slices);
        } else {
            descend(walk, tensor.slices, walk.base, 0);
        }
    }

    walk.report.seconds = cli::elapsed_seconds(started);
    return walk.report;
}

}  // namespace bilinear_rank
