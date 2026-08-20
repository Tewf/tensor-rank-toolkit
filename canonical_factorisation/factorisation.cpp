#include "factorisation.h"

#include <cmath>
#include <limits>
#include <vector>

#include "bilinear_rank_aliases.h"
#include "exit_code.h"
#include "candidate_pool.h"
#include "canonical_augmentation.h"
#include "exhaustive_search.h"
#include "group_construction.h"
#include "requested_group.h"
#include "measures.h"
#include "orbit_search.h"
#include "rank_lower_bound.h"
#include "rank_question.h"
#include "solver_process.h"
#include "solver.h"

namespace canonical_factorisation {

namespace {

/// The rank-one rows, laid into `A` in the canonical basis. A row of `A` is
/// simply the matrix read out entry by entry, which is what makes `A B` the
/// identity on content and the rank-one constraint the whole of the problem.
ModularMatrix rows_of(const std::vector<ModularMatrix>& products, std::size_t width) {
    ModularMatrix chosen(products.size(), width);
    for (std::size_t index = 0; index < products.size(); ++index) {
        for (std::size_t entry = 0; entry < width; ++entry) {
            chosen(index, entry) = products[index].data()[entry];
        }
    }
    return chosen;
}

/// `C`, by solving `slice = c A` once per slice. The rows of `A` are a basis of
/// the space the search returned, so each system has exactly one solution and a
/// failure here is a defect rather than a tensor this cannot express.
bool recovery_for(const ModularField& field, const std::vector<ModularMatrix>& slices,
                  const ModularMatrix& chosen, ModularMatrix& recovery) {
    const std::size_t width = chosen.columns();
    std::vector<std::vector<int64_t>> rows(chosen.rows(), std::vector<int64_t>(width));
    for (std::size_t index = 0; index < chosen.rows(); ++index) {
        for (std::size_t entry = 0; entry < width; ++entry) rows[index][entry] = chosen(index, entry);
    }

    recovery = ModularMatrix(slices.size(), chosen.rows());
    for (std::size_t slice = 0; slice < slices.size(); ++slice) {
        const std::vector<int64_t> target(slices[slice].data(), slices[slice].data() + width);
        std::vector<int64_t> coefficients;
        if (!linear_algebra::solve_in_row_space(field, rows, target, coefficients)) return false;
        for (std::size_t index = 0; index < coefficients.size(); ++index) {
            recovery(slice, index) = coefficients[index];
        }
    }
    return true;
}

/// How many rank-one matrices the shape has, without forming one of them.
///
/// `(p^n - 1)(p^m - 1)/(p-1)^2`, which is what the exhaustive route would have
/// to hold and what the SAT route does not.
std::size_t pool_size_for(const ModularField& field, std::size_t rows, std::size_t columns) {
    const std::size_t characteristic = static_cast<std::size_t>(field.residu());
    const std::size_t ceiling = std::numeric_limits<std::size_t>::max();

    // The count overflows long before the pool becomes formable, and a wrapped
    // size would read as small and send a hopeless shape to the pool route. So
    // saturate: anything past here is "too many to count", which is the only
    // thing the caller does with the number anyway.
    std::size_t left = 1;
    std::size_t right = 1;
    for (std::size_t step = 0; step < rows; ++step) {
        if (left > ceiling / characteristic) return ceiling;
        left *= characteristic;
    }
    for (std::size_t step = 0; step < columns; ++step) {
        if (right > ceiling / characteristic) return ceiling;
        right *= characteristic;
    }
    const std::size_t scalars = characteristic - 1;
    const std::size_t classes = (left - 1) / scalars;
    const std::size_t others = (right - 1) / scalars;
    if (classes != 0 && others > ceiling / classes) return ceiling;
    return classes * others;
}

/// The stabiliser of the slice space, narrowed from whichever ambient group was
/// asked for.
///
/// `expand_subspace_up_to_symmetry` requires a group that stabilises the span it
/// is given, and a group that does not is the one way it can report a false NO.
/// So the ambient group is narrowed here and never passed through.
///
/// A refusal to build the ambient group is not a wrong answer, only a slower
/// one, so it falls back rather than failing. What it must not do is fall back
/// **quietly**, which is what this did when it called `all_automorphisms`
/// directly: that refuses on any 4x4 map over GF(2), so every matrix
/// multiplication fixture ran unquotiented while reporting that symmetry was on.
/// The `<n,m,k>` a shape would have if it were a matrix multiplication tensor.
///
/// `nm x mk x kn` are the three dimensions, so their product is `(nmk)^2` and
/// the three sizes divide out of its square root. Returns false when they do
/// not come out whole, which is most tensors.
///
/// Guessing wrong is safe rather than merely unlikely: `stabiliser_of` keeps
/// only the elements that actually fix `span(T)`, which is the precondition
/// `expand_subspace_up_to_symmetry` needs, so a wrong guess yields a small
/// group and never a false refusal.
bool inferred_matmul_shape(std::size_t rows, std::size_t columns, std::size_t slices,
                           std::size_t shape[3]) {
    if (rows == 0 || columns == 0 || slices == 0) return false;
    const std::size_t product = rows * columns * slices;
    std::size_t root = static_cast<std::size_t>(std::sqrt(static_cast<double>(product)));
    while (root > 0 && root * root > product) --root;
    while ((root + 1) * (root + 1) <= product) ++root;
    if (root * root != product) return false;

    if (root % columns || root % slices || root % rows) return false;
    shape[0] = root / columns;   // n
    shape[1] = root / slices;    // m
    shape[2] = root / rows;      // k
    return shape[0] * shape[1] == rows && shape[1] * shape[2] == columns &&
           shape[2] * shape[0] == slices;
}

std::vector<bilinear_rank::Automorphism> stabiliser_or_nothing(
    const ModularField& field, const std::vector<ModularMatrix>& slices,
    const cli::Symmetry& symmetry, std::string& refusal) {
    if (symmetry.kind == cli::SymmetryKind::None) return {};
    try {
        return bilinear_rank::stabiliser_of(
            field, slices, bilinear_rank::requested_ambient_group(field, slices, symmetry));
    } catch (const std::exception& error) {
        refusal = error.what();
    }

    // `auto` refuses on any 4x4 map over GF(2), which is every matrix
    // multiplication fixture here, so refusing is where the useful group starts
    // rather than where it stops. The closed form needs no group enumerated and
    // works at any size, and a shape that is not really a product yields a small
    // stabiliser rather than a wrong one.
    std::size_t shape[3] = {0, 0, 0};
    if (symmetry.kind == cli::SymmetryKind::Automatic &&
        inferred_matmul_shape(slices.front().rows(), slices.front().columns(), slices.size(),
                              shape)) {
        try {
            const std::vector<bilinear_rank::Automorphism> closed =
                bilinear_rank::matrix_multiplication_symmetry_generators(field, shape[0], shape[1],
                                                                         shape[2]);
            const std::vector<bilinear_rank::Automorphism> kept =
                bilinear_rank::stabiliser_of(field, slices, closed);
            if (kept.size() > 1) refusal.clear();
            return kept;
        } catch (const std::exception&) {
        }
    }
    return {};
}

}  // namespace

namespace {

/// The SAT route: the same sweep, asked of somebody else's solver.
///
/// `find_rank` walks up from the floor exactly as the loop below does, and
/// returns a decomposition it has already checked against the tensor. What it
/// never does is enumerate the rank-one maps: the condition is clauses over the
/// operand vectors, so the space is polynomial in the shape rather than
/// exponential in it.
Factorisation by_satisfiability(const ModularField& field,
                                const std::vector<ModularMatrix>& slices, std::size_t floor,
                                std::size_t ceiling) {
    Factorisation factorisation;
    factorisation.route = Route::Satisfiability;
    factorisation.floor = floor;
    factorisation.pool_size =
        pool_size_for(field, slices.front().rows(), slices.front().columns());

    linear_algebra::Tensor tensor;
    tensor.characteristic = static_cast<int64_t>(field.residu());
    tensor.slices = slices;

    satisfiability::SolveOptions approach;
    approach.break_symmetry = true;

    const satisfiability::RankBounds bounds =
        satisfiability::find_rank(tensor, approach, floor, ceiling);
    if (bounds.decomposition.empty()) return factorisation;

    factorisation.chosen =
        rows_of(bounds.decomposition, slices.front().rows() * slices.front().columns());
    factorisation.components = bounds.decomposition.size();
    factorisation.minimal = bounds.exact;
    if (!recovery_for(field, slices, factorisation.chosen, factorisation.recovery)) {
        throw cli::CheckFailed(
            "canonical_factorisation: the solver's decomposition does not span the slices");
    }
    return factorisation;
}

/// The closed-form symmetries of a product shape as generators, or nothing.
///
/// This asked for the enumerated group while the parent test named an orbit by
/// walking every element, where generators would have made the test wrong
/// rather than slow. `PoolSetCanon` names the same orbit from a base and strong
/// generating set and never walks the group, so the list is no longer needed
/// and costs a presentation built from 216 elements instead of six: the same
/// change measured 3.04 s to 1.12 s in `oracle_guided_search/`.
///
/// It also lifts a shape limit. `matrix_multiplication_symmetries` refuses
/// above a list it can hold, which was the honest answer at `<3,3,3>` and is no
/// longer the necessary one, since 4.7 million elements have nine generators.
std::vector<bilinear_rank::Automorphism> product_group_generators(
    const ModularField& field, const std::vector<ModularMatrix>& slices, std::string& refusal) {
    std::size_t shape[3] = {0, 0, 0};
    if (!inferred_matmul_shape(slices.front().rows(), slices.front().columns(), slices.size(),
                               shape)) {
        refusal = "canonical augmentation needs a product shape, and these dimensions are not one";
        return {};
    }
    try {
        return bilinear_rank::matrix_multiplication_symmetry_generators(field, shape[0],
                                                                        shape[1], shape[2]);
    } catch (const std::exception& error) {
        refusal = error.what();
        return {};
    }
}

}  // namespace

Factorisation factor_over_canonical_basis(const ModularField& field,
                                          const std::vector<ModularMatrix>& slices,
                                          const FactorisationSettings& settings) {
    Factorisation factorisation;
    if (slices.empty()) {
        factorisation.minimal = true;
        return factorisation;
    }

    const std::size_t width = slices.front().rows() * slices.front().columns();
    factorisation.floor =
        settings.floor > 0 ? settings.floor : linear_algebra::rank_lower_bound(field, slices);

    // Decomposing each slice on its own always works, so the sum of their ranks
    // is a ceiling that needs no search to justify.
    const std::size_t ceiling = settings.ceiling > 0
                                    ? settings.ceiling
                                    : linear_algebra::multiplication_count(field, slices);

    factorisation.pool_size =
        pool_size_for(field, slices.front().rows(), slices.front().columns());

    // The route, decided once and recorded. A solver that is not installed is
    // not a reason to fail: the pool route answers the same question, and
    // saying which one ran is what keeps the two comparable.
    Route route = settings.route;
    if (route == Route::Automatic) {
        const bool solver_available =
            satisfiability::find_sat_solver(false, "", satisfiability::default_solver_order()).found;
        route = (factorisation.pool_size > settings.pool_ceiling && solver_available)
                    ? Route::Satisfiability
                    : Route::Exhaustive;
    }
    if (route == Route::Satisfiability) {
        return by_satisfiability(field, slices, factorisation.floor, ceiling);
    }

    const std::vector<ModularMatrix> pool =
        bilinear_rank::all_rank_one_maps(field, slices.front().rows(), slices.front().columns());

    // Canonical augmentation wants the whole group and the quotiented tree wants
    // the stabiliser, and the two are different objects: the first names an
    // orbit and the second only has to fix the span. Asking for the wrong one is
    // the way this reports a false refusal, so each route builds its own and
    // neither is reused. Both take generators, and the first only recently
    // could.
    std::vector<bilinear_rank::Automorphism> group;
    if (route == Route::CanonicalAugmentation) {
        group = product_group_generators(field, slices, factorisation.symmetry_refusal);
        if (group.empty()) route = Route::Exhaustive;
    }
    if (route != Route::CanonicalAugmentation) {
        group =
            stabiliser_or_nothing(field, slices, settings.symmetry, factorisation.symmetry_refusal);
    }
    factorisation.route = route;
    factorisation.group_size = group.size();

    linear_algebra::Tensor as_tensor;
    as_tensor.characteristic = static_cast<int64_t>(field.residu());
    as_tensor.slices = slices;

    bool every_refusal_complete = true;
    for (std::size_t components = factorisation.floor; components <= ceiling; ++components) {
        bilinear_rank::SearchBudget budget{settings.node_limit};
        std::vector<ModularMatrix> products;
        bool found = false;

        if (route == Route::CanonicalAugmentation) {
            // `stop_at_first`, because this is deciding and not counting: the
            // level that succeeds does not have to be finished. The levels below
            // it still are, since a level with no solution has to be exhausted
            // before it can be called empty, and those are where the node saving
            // was supposed to pay. Whether it does is measured, not assumed:
            // see `canonical-augmentation.md`.
            const bilinear_rank::EnumerationReport pass =
                bilinear_rank::enumerate_solution_subspaces(field, as_tensor, pool, group,
                                                           components, /*canonical=*/true,
                                                           /*stop_at_first=*/true);
            factorisation.nodes_visited += pass.nodes;
            if (!pass.decompositions.empty()) {
                products = pass.decompositions.front();
                found = true;
            }
        } else {
            found = group.size() > 1
                        ? bilinear_rank::expand_subspace_up_to_symmetry(field, slices, pool, group,
                                                                       components, budget, products)
                        : bilinear_rank::expand_subspace(field, slices, pool, 0, components, budget,
                                                        products);
            factorisation.nodes_visited += budget.nodes_visited.load();
        }

        if (!found) {
            if (!budget.exhausted) every_refusal_complete = false;
            continue;
        }

        factorisation.chosen = rows_of(products, width);
        factorisation.components = components;
        factorisation.minimal = every_refusal_complete;
        if (!recovery_for(field, slices, factorisation.chosen, factorisation.recovery)) {
            throw cli::CheckFailed(
                "canonical_factorisation: the rows found do not span the slices they were found "
                "for");
        }
        return factorisation;
    }
    return factorisation;
}

bool recovers_slices(const ModularField& field, const std::vector<ModularMatrix>& slices,
                     const Factorisation& factorisation) {
    if (slices.empty()) return factorisation.components == 0;
    if (factorisation.recovery.rows() != slices.size()) return false;
    if (factorisation.recovery.columns() != factorisation.chosen.rows()) return false;

    const std::size_t rows = slices.front().rows();
    const std::size_t columns = slices.front().columns();
    if (factorisation.chosen.columns() != rows * columns) return false;

    for (std::size_t index = 0; index < factorisation.chosen.rows(); ++index) {
        const ModularMatrix product = matrix_of(factorisation.chosen, index, rows, columns);
        if (linear_algebra::rank(field, product) != 1) return false;
    }

    // C A, entry by entry, against S. The one product that decides it.
    for (std::size_t slice = 0; slice < slices.size(); ++slice) {
        for (std::size_t entry = 0; entry < rows * columns; ++entry) {
            int64_t sum = 0;
            for (std::size_t index = 0; index < factorisation.chosen.rows(); ++index) {
                field.axpyin(sum, factorisation.recovery(slice, index),
                             factorisation.chosen(index, entry));
            }
            int64_t expected = 0;
            field.init(expected, slices[slice].data()[entry]);
            if (sum != expected) return false;
        }
    }
    return true;
}

}  // namespace canonical_factorisation
