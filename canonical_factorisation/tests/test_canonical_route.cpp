/// The canonical-augmentation route, against the plain one.
///
/// Its own test because it is slow for a reason worth isolating: the parent test
/// walks the whole group at every node, and the enumerator has no early exit, so
/// it finishes the level that succeeds instead of returning from it. Both are
/// deliberate in the enumerator, which exists to *count* solution subspaces, and
/// both are wrong for a search that needs one.
///
/// What is asserted is therefore agreement and not speed: the route must return
/// the same rank as the plain one and its own receipt must check out. The timing
/// is in [`../narrowing-the-search.md`](../narrowing-the-search.md), where it
/// loses by two orders of magnitude and says so.
#include <string>

#include "check.h"
#include "factorisation.h"
#include "tensor_file.h"

int main(int argc, char** argv) {
    const std::string directory = argc > 1 ? argv[1] : "fixtures";
    const linear_algebra::Tensor tensor =
        linear_algebra::read_tensor_file(directory + "/matmul_2x2x2.tensor");
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
    check::equal("with the full 216-element group of the product",
                 static_cast<long long>(deduplicated.group_size), 216);
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
