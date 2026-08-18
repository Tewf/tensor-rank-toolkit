#include "canonical_parent.h"

#include <algorithm>

#include "candidate_pool.h"
#include "span_basis.h"

namespace bilinear_rank {

namespace {

/// The pool elements lying inside `span(generators)`.
std::vector<std::size_t> pool_inside(const Field& field, const std::vector<Matrix>& pool,
                                     const std::vector<Matrix>& generators) {
    const ReducedBasis span = linear_algebra::span_of(field, generators);
    std::vector<std::size_t> inside;
    std::vector<Element> scratch;
    for (std::size_t index = 0; index < pool.size(); ++index) {
        if (span.contains(pool[index], scratch)) inside.push_back(index);
    }
    return inside;
}

/// `Σ coefficients[i] · basis[i]`.
Matrix combine(const Field& field, const std::vector<Matrix>& basis,
               const std::vector<int64_t>& coefficients) {
    Matrix total(basis.front().rows(), basis.front().columns());
    for (std::size_t which = 0; which < coefficients.size(); ++which) {
        for (std::size_t entry = 0; entry < total.entry_count(); ++entry) {
            field.axpyin(total.data()[entry], coefficients[which], basis[which].data()[entry]);
        }
    }
    return total;
}

/// Every reachable subspace one dimension below `span(base) + span(added)` and above
/// `span(base)`.
///
/// These are the hyperplanes of the quotient, one per nonzero linear functional up to
/// scalar, which `normalised_vectors` enumerates exactly. `added` is a basis of that
/// quotient; which basis does not matter, because the set of hyperplanes does not
/// depend on it, and that independence is what makes the canonical parent invariant.
///
/// Reachable means spanned by `base` plus pool elements, since the walk can add
/// nothing else. A hyperplane that is not is no parent of anything.
///
/// An earlier version dropped one contained pool element at a time instead. It never
/// produced a single parent: a solution subspace of `⟨2,2,2⟩` contains seven pool
/// elements spanning a three-dimensional quotient, and any six of the seven still span
/// it, so every candidate had the wrong dimension and every augmentation was rejected.
std::vector<std::vector<Matrix>> candidate_parents(const Field& field,
                                                   const std::vector<Matrix>& base,
                                                   const std::vector<Matrix>& added,
                                                   const std::vector<Matrix>& pool,
                                                   const std::vector<std::size_t>& inside) {
    const std::size_t quotient = added.size();
    const std::size_t wanted = linear_algebra::span_of(field, base).dimension() + quotient - 1;

    std::vector<std::vector<Matrix>> parents;
    for (const std::vector<int64_t>& functional : normalised_vectors(field, quotient)) {
        std::size_t pivot = 0;
        while (pivot < quotient && field.isZero(functional[pivot])) ++pivot;

        std::vector<Matrix> candidate = base;
        for (std::size_t index = 0; index < quotient; ++index) {
            if (index == pivot) continue;
            std::vector<int64_t> coefficients(quotient, field.zero);
            field.assign(coefficients[index], field.one);
            Element ratio;
            field.div(ratio, functional[index], functional[pivot]);
            field.neg(coefficients[pivot], ratio);
            candidate.push_back(combine(field, added, coefficients));
        }
        if (linear_algebra::span_of(field, candidate).dimension() != wanted) continue;

        std::vector<Matrix> reachable = base;
        const ReducedBasis span = linear_algebra::span_of(field, candidate);
        std::vector<Element> scratch;
        for (const std::size_t index : inside) {
            if (span.contains(pool[index], scratch)) reachable.push_back(pool[index]);
        }
        if (linear_algebra::span_of(field, reachable).dimension() != wanted) continue;
        parents.push_back(std::move(candidate));
    }
    return parents;
}

}  // namespace

ParentTest is_canonical_augmentation(const Field& field, const std::vector<Matrix>& base,
                                    const std::vector<Matrix>& child,
                                    const SubspaceCode& parent_code, const Matrix& added,
                                    const std::vector<Matrix>& pool,
                                    const std::vector<Automorphism>& group) {
    ParentTest test;
    const std::vector<std::size_t> inside = pool_inside(field, pool, child);
    const std::vector<Matrix> chosen(child.begin() + base.size(), child.end());
    const std::vector<std::vector<Matrix>> parents =
        candidate_parents(field, base, chosen, pool, inside);

    SubspaceCode least;
    std::vector<SubspaceCode> least_codes;
    for (const std::vector<Matrix>& parent : parents) {
        const CanonicalSubspace canonical = canonical_subspace(field, group, parent);
        test.group_visits += group.size();
        if (least.empty() || canonical.code < least) {
            least = canonical.code;
            least_codes.assign(1, subspace_code(field, parent));
        } else if (canonical.code == least) {
            least_codes.push_back(subspace_code(field, parent));
        }
    }
    if (std::find(least_codes.begin(), least_codes.end(), parent_code) == least_codes.end()) {
        return test;
    }

    const CanonicalSubspace canonical_child = canonical_subspace(field, group, child);
    test.group_visits += group.size();
    const SubspaceCode added_code = subspace_code(field, {added});

    SubspaceCode best_key;
    SubspaceCode added_key;
    for (const std::size_t index : inside) {
        SubspaceCode key;
        for (const std::size_t which : canonical_child.attaining) {
            const SubspaceCode image = image_code(field, group[which], pool[index]);
            if (key.empty() || image < key) key = image;
        }
        if (subspace_code(field, {pool[index]}) == added_code) added_key = key;
        if (best_key.empty() || key < best_key) best_key = key;
    }
    test.accepted = added_key == best_key;
    return test;
}

}  // namespace bilinear_rank
