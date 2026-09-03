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

}  // namespace

/// Hyperplanes of the quotient, and not the route that looks obvious: an earlier
/// version dropped one contained pool element at a time, and it never produced a
/// single parent. A solution subspace of `⟨2,2,2⟩` contains seven pool elements
/// spanning a three-dimensional quotient, and any six of the seven still span it,
/// so every candidate had the wrong dimension and every augmentation was rejected.
std::vector<CandidateParent> candidate_parents(const Field& field,
                                               const std::vector<Matrix>& base,
                                               const std::vector<Matrix>& added,
                                               const std::vector<Matrix>& pool,
                                               const std::vector<std::size_t>& inside) {
    const std::size_t quotient = added.size();
    const std::size_t wanted = linear_algebra::span_of(field, base).dimension() + quotient - 1;

    std::vector<CandidateParent> parents;
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

        // The child's pool content and not the pool: `span(candidate)` sits inside
        // `span(base) + span(added)`, so a pool element it contains is one the
        // child contains too and nothing outside `inside` can qualify. The list is
        // kept rather than thrown away, since it is what names the parent's orbit.
        std::vector<Matrix> reachable = base;
        std::vector<std::size_t> content;
        const ReducedBasis span = linear_algebra::span_of(field, candidate);
        std::vector<Element> scratch;
        for (const std::size_t index : inside) {
            if (!span.contains(pool[index], scratch)) continue;
            reachable.push_back(pool[index]);
            content.push_back(index);
        }
        if (linear_algebra::span_of(field, reachable).dimension() != wanted) continue;
        parents.push_back({std::move(candidate), std::move(content)});
    }
    return parents;
}

bool is_distinguished_cell(const PoolSetCanon& canon, const std::vector<std::size_t>& indices,
                           std::size_t marked) {
    const std::vector<std::size_t> key = canon.canonical_with_marked(indices, marked);
    return key.back() - canon.size() == key.front();
}

ParentTest is_canonical_augmentation(const Field& field, const std::vector<Matrix>& base,
                                    const std::vector<Matrix>& child,
                                    const SubspaceCode& parent_code,
                                    const std::vector<std::size_t>& parent_name,
                                    std::size_t added, const std::vector<std::size_t>& inside,
                                    const std::vector<Matrix>& pool,
                                    const PoolSetCanon& canon) {
    ParentTest test;
    const std::vector<Matrix> chosen(child.begin() + base.size(), child.end());
    const std::vector<CandidateParent> parents =
        candidate_parents(field, base, chosen, pool, inside);

    // The canonical name of each candidate parent's orbit, by least image under a
    // prescribed group rather than by walking it. `canonical_subspace` took the
    // least code over every element, which is what made this route lose: one
    // test cost `|G|` reductions, and the wall clock paid for every node saved.
    //
    // **Asked as an early exit rather than as a minimum.** The condition is that
    // the parent's class is least, and one candidate strictly below it settles that
    // in the negative; the minimum itself is never wanted. Two hyperplanes of one
    // quotient are distinct subspaces with distinct codes, so "the parent attains
    // the least name" and "the parent's code is among those attaining it" are the
    // same statement, which is what lets the loop stop where it does.
    bool parent_seen = false;
    for (const CandidateParent& parent : parents) {
        if (subspace_code(field, parent.generators) == parent_code) {
            parent_seen = true;
            continue;
        }
        const std::vector<std::size_t> name = canon.canonical(parent.content);
        ++test.canonisations;
        if (name < parent_name) return test;
    }
    if (!parent_seen) return test;

    // The distinguished pool element of the child, by canonising the **pair**
    // rather than by minimising over the group elements that attain the child's
    // canonical form. Linton's algorithm returns an image and not the element that
    // got there, so the attaining coset is not available; a marked pair on a
    // doubled ground set asks the same question with the same primitive, and a
    // minimum of orbit invariants is an orbit invariant.
    //
    // Only the added element's pair is canonised. Its key carries the answer on
    // its own, because the least mark entry any cell can reach is the canonical
    // set's least point and that point is the key's first entry, so the other
    // cells' keys were a minimum whose value was already in hand.
    // `is_distinguished_cell` sets the argument out in full.
    //
    // `added` is the mark, directly. It used to be found by scanning `inside` for
    // the element whose single-map code matched, which is a search for something
    // the caller had just chosen by index: `all_rank_one_maps` holds each rank-one
    // map once, so exactly one index could ever match and it is this one.
    ++test.canonisations;
    test.accepted = is_distinguished_cell(canon, inside, added);
    return test;
}

}  // namespace bilinear_rank
