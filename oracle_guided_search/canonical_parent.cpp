#include "canonical_parent.h"

#include <algorithm>

#include "candidate_pool.h"
#include "pool_set_canon.h"
#include "span_basis.h"

namespace bilinear_rank {

namespace {

/// `Σ coefficients[i] · basis[i]`.
Matrix linear_combination(const Field& field, const std::vector<Matrix>& basis,
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
            candidate.push_back(linear_combination(field, added, coefficients));
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
                                    const std::vector<Automorphism>& group,
                                    const PoolSetCanon& canon) {
    ParentTest test;
    const std::vector<std::size_t> inside = pool_inside(field, pool, child);
    const std::vector<Matrix> chosen(child.begin() + base.size(), child.end());
    const std::vector<std::vector<Matrix>> parents =
        candidate_parents(field, base, chosen, pool, inside);

    // The canonical name of each candidate parent's orbit, by least image under a
    // prescribed group rather than by walking it. `canonical_subspace` took the
    // least code over every element, which is what made this route lose: 16.2x
    // fewer nodes for 15x the wall clock, because one test cost `|G|` reductions.
    std::vector<std::size_t> least;
    std::vector<SubspaceCode> least_codes;
    for (const std::vector<Matrix>& parent : parents) {
        const std::vector<std::size_t> name = canon.canonical(pool_inside(field, pool, parent));
        ++test.canonisations;
        if (least.empty() || name < least) {
            least = name;
            least_codes.assign(1, subspace_code(field, parent));
        } else if (name == least) {
            least_codes.push_back(subspace_code(field, parent));
        }
    }
    if (std::find(least_codes.begin(), least_codes.end(), parent_code) == least_codes.end()) {
        return test;
    }

    // The distinguished pool element of the child, by canonising the **pair**
    // rather than by minimising over the group elements that attain the child's
    // canonical form. Linton's algorithm returns an image and not the element that
    // got there, so the attaining coset is not available; a marked pair on a
    // doubled ground set asks the same question with the same primitive, and a
    // minimum of orbit invariants is an orbit invariant.
    const SubspaceCode added_code = subspace_code(field, {added});

    std::vector<std::size_t> best_key;
    std::vector<std::size_t> added_key;
    bool added_seen = false;
    for (const std::size_t index : inside) {
        const std::vector<std::size_t> key = canon.canonical_with_marked(inside, index);
        ++test.canonisations;
        if (subspace_code(field, {pool[index]}) == added_code) {
            added_key = key;
            added_seen = true;
        }
        if (best_key.empty() || key < best_key) best_key = key;
    }
    test.accepted = added_seen && added_key == best_key;
    return test;
}

}  // namespace bilinear_rank
