#include "factorisation.h"

#include <vector>

#include "bilinear_rank_aliases.h"
#include "exit_code.h"
#include "candidate_pool.h"
#include "exhaustive_search.h"
#include "group_construction.h"
#include "measures.h"
#include "orbit_search.h"
#include "rank_lower_bound.h"
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

/// The stabiliser of the slice space, or nothing when no ambient group can be
/// built for the shape. Falling back is not a wrong answer, only a slower one.
std::vector<bilinear_rank::Automorphism> stabiliser_or_nothing(
    const ModularField& field, const std::vector<ModularMatrix>& slices) {
    try {
        const std::vector<bilinear_rank::Automorphism> ambient = bilinear_rank::all_automorphisms(
            field, slices.front().rows(), slices.front().columns());
        return bilinear_rank::stabiliser_of(field, slices, ambient);
    } catch (const std::exception&) {
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

    const std::vector<ModularMatrix> pool =
        bilinear_rank::all_rank_one_maps(field, slices.front().rows(), slices.front().columns());
    const std::vector<bilinear_rank::Automorphism> group =
        settings.use_symmetry ? stabiliser_or_nothing(field, slices)
                              : std::vector<bilinear_rank::Automorphism>{};

    bool every_refusal_complete = true;
    for (std::size_t components = factorisation.floor; components <= ceiling; ++components) {
        bilinear_rank::SearchBudget budget{settings.node_limit};
        std::vector<ModularMatrix> products;
        const bool found =
            group.size() > 1
                ? bilinear_rank::expand_subspace_up_to_symmetry(field, slices, pool, group,
                                                                components, budget, products)
                : bilinear_rank::expand_subspace(field, slices, pool, 0, components, budget,
                                                 products);
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
