#include "canonical_augmentation.h"

#include <algorithm>
#include <cstdint>

#include "algorithm_recovery.h"
#include "canonical_parent.h"
#include "exhaustive_search.h"
#include "exit_code.h"
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
    std::size_t target = 0;
    bool canonical = false;
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
                                       const Span& span, std::size_t from) {
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
    const std::vector<std::vector<std::uint32_t>> action = action_on(field, stabiliser, pool);

    std::vector<std::size_t> representatives;
    for (const std::uint32_t index : one_per_orbit(action, outside)) {
        representatives.push_back(index);
    }
    return representatives;
}

void descend(Walk& walk, const std::vector<Matrix>& current, std::size_t dimension,
             std::size_t from) {
    ++walk.report.nodes;
    if (dimension == walk.target) {
        emit_if_solution(walk, current);
        return;
    }

    const Field& field = *walk.field;
    const std::vector<Matrix>& pool = *walk.pool;
    const Span span = linear_algebra::span_of(field, current);
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
    }
}

}  // namespace

EnumerationReport enumerate_solution_subspaces(const Field& field,
                                               const linear_algebra::Tensor& tensor,
                                               const std::vector<Matrix>& pool,
                                               const std::vector<Automorphism>& group,
                                               std::size_t target, bool canonical) {
    const cli::Clock::time_point started = cli::Clock::now();
    Walk walk;
    walk.field = &field;
    walk.tensor = &tensor;
    walk.pool = &pool;
    walk.group = &group;
    walk.target = target;
    walk.canonical = canonical;

    const std::size_t base = linear_algebra::span_of(field, tensor.slices).dimension();
    if (base <= target) descend(walk, tensor.slices, base, 0);

    walk.report.seconds = cli::elapsed_seconds(started);
    return walk.report;
}

}  // namespace bilinear_rank
