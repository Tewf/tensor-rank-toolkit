/// The canonical-augmentation route, against the plain one.
///
/// Its own test because it is slow for a reason worth isolating: the parent test
/// walks the whole group at every node, and the enumerator has no early exit, so
/// it finishes the level that succeeds instead of returning from it. Both are
/// deliberate in the enumerator, which exists to *count* solution subspaces, and
/// both are wrong for a search that needs one.
///
/// What is asserted is therefore agreement and not speed: the route must return
/// the same rank as the plain one and its own receipt must check out. The
/// timing is in [`../canonical-augmentation.md`](../canonical-augmentation.md):
/// 5.1x against on the wall clock today, from 25.8x when first measured and
/// 129x at the first wiring.
#include <string>

#include "check.h"
#include "factorisation.h"
#include "tensor_file.h"

int main(int argc, char** argv) {
    const std::string directory = argc > 1 ? argv[1] : "fixtures";
    const formats::Tensor tensor =
        formats::read_tensor_file(directory + "/matmul_2x2x2.tensor");
    const linear_algebra::ModularField field(tensor.characteristic);

    canonical_factorisation::FactorisationSettings canonical;
    canonical.route = canonical_factorisation::Route::CanonicalAugmentation;
    const canonical_factorisation::Factorisation deduplicated =
        canonical_factorisation::factor_over_canonical_basis(field, tensor.slices, canonical);

    check::equal("canonical augmentation reaches Strassen's seven",
                 static_cast<long long>(deduplicated.components), 7);
    check::equal("and it engaged rather than falling back",
                 deduplicated.route == canonical_factorisation::Route::CanonicalAugmentation ? 1 : 0,
                 1);
    // Six, the closed-form generating set of <2,2,2>, and not the 216 elements
    // it generates. This asserted 216 while the parent test named an orbit by
    // walking every element, where a generating set would have made the test
    // wrong rather than slow. `PoolSetCanon` names the orbit from a base and
    // strong generating set, so the list became a cost with no purpose. What
    // the check is for is unchanged: a zero here is the silent fallback to the
    // plain route, which every other assertion in this file would still pass.
    check::equal("from the six generators of the product group",
                 static_cast<long long>(deduplicated.group_size), 6);
    check::equal("and C A gives the slices back",
                 canonical_factorisation::recovers_slices(field, tensor.slices, deduplicated) ? 1
                                                                                             : 0,
                 1);

    // Fewer nodes is what it buys, and the assertion is that it buys any: a
    // parent test that accepted everything would agree on the rank and visit the
    // same tree, which is the failure this catches.
    canonical_factorisation::FactorisationSettings plain;
    plain.route = canonical_factorisation::Route::Exhaustive;
    const canonical_factorisation::Factorisation walked =
        canonical_factorisation::factor_over_canonical_basis(field, tensor.slices, plain);

    check::equal("the plain route agrees on the rank",
                 static_cast<long long>(walked.components),
                 static_cast<long long>(deduplicated.components));
    check::equal("and visits strictly more nodes",
                 deduplicated.nodes_visited < walked.nodes_visited ? 1 : 0, 1);

    return check::report("canonical augmentation route");
}
