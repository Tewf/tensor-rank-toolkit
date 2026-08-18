#pragma once

#include <cstddef>
#include <string>
#include <vector>

/// Step 3 of the Chudnovsky-Chudnovsky roadmap: choosing the divisor.
///
/// `[rambaud2014, Thm. 2]` says that if a curve carries a point `Q` of degree
/// `m` and an effective divisor `G = Σ uᵢPᵢ` admitting a suitable `D`, then
///
/// > `µ_sym_q(m) ≤ Σᵢ µ_sym_q(deg Pᵢ, uᵢ)`
///
/// The right-hand side depends only on the degrees and the multiplicities, so
/// choosing them well is an integer programme over
/// [the published bounds](symmetric_bound_table.h) and nothing else. This file
/// solves it by enumeration, and
/// [`interpolation_by_solver.h`](interpolation_by_solver.h) states the same
/// question as a model and hands it to a solver. The two are cross-checked
/// against each other over a sweep, which is what makes either trustworthy.
///
/// **Neither is a heuristic and the enumeration has no third-party dependency.**
/// It walks its whole reachable frontier, so its optimum is proved, and it stays
/// the fallback for that reason. Where it loses is scale: its table is quadratic
/// in `deg G` in time and in memory, while the model is at most 25 variables
/// whatever the degree. Measured on this machine, with points of degree 1 only:
/// `deg G` 500 costs the enumeration under 0.01 s and 10 MB against the chain's
/// 0.02 s, and `deg G` 4000 costs it 0.34 s and 442 MB against 0.01 s.
///
/// **What this does not do, and it is most of the method.** Steps 2 and 4 of
/// the roadmap are absent. Finding a curve with many points of low degree, and
/// checking that an admissible interpolation system `(G, D, Q)` exists at all
/// -- conditions `l(2D − G) = 0` and `i(D − Q) = 0` -- need Riemann-Roch spaces
/// and curve construction. That is Magma or Sage work, not a Givaro
/// repository. So a result here is **a lower envelope on what the method could
/// give if a curve with that supply of points exists**, and never a bound on
/// `µ_sym_q(m)` by itself. The supply is an input precisely because this cannot
/// compute it.
namespace curve_bounds {

/// How many distinct closed points of a given degree the curve is assumed to
/// carry. This is step 2's output, supplied rather than computed.
struct PointSupply {
    std::size_t degree = 0;
    std::size_t available = 0;
};

/// `count` points of degree `degree`, each taken with multiplicity
/// `multiplicity`.
struct Selection {
    std::size_t degree = 0;
    std::size_t multiplicity = 0;
    std::size_t count = 0;
};

struct BoundResult {
    bool solved = false;
    /// `Σ µ_sym_q(dᵢ, uᵢ)`, the right-hand side of Theorem 2.
    std::size_t bound = 0;
    /// `Σ uᵢ·dᵢ`, which is `deg G`.
    std::size_t degree_used = 0;
    std::vector<Selection> chosen;

    /// Which route produced this: the dynamic programme below, or the name of
    /// the backend that answered when it was asked as an integer programme
    /// ([`interpolation_by_solver.h`](interpolation_by_solver.h)). A number
    /// whose provenance is unrecorded cannot be quoted.
    std::string solved_by = "dynamic programme";

    /// Whether `bound` is the minimum or merely a value the constraints admit.
    ///
    /// Any feasible selection is already a valid bound by Theorem 2, so a
    /// weaker answer is a weaker envelope and never a wrong one. But the solver
    /// chain sets `Optimal` on any point passing `satisfies`, which checks
    /// bounds, integrality and rows and **cannot check optimality**, so only the
    /// built-in branch and bound's verdict is a proof. The dynamic programme
    /// walks its whole reachable frontier, so it proves its own.
    bool optimum_proved = true;
};

/// Minimise Theorem 2's right-hand side over effective divisors of degree
/// **exactly** `divisor_degree`, drawing points from `supply`.
///
/// Exactly, not at most. The roadmap fixes a candidate `deg G` first and then
/// asks for the best divisor of that degree, and it has to: `µ_sym_q(d, u)` is
/// positive for every point, so minimising over smaller divisors too would
/// always answer "take one rational point, cost 1", which is a bound on
/// nothing. The degree is what the existence of an interpolation system will
/// later be checked against, so it is an input, not something to economise on.
///
/// Multiplicities are capped by what the bound table knows: a point whose
/// `µ_sym_q(d, u)` is unpublished cannot be costed, so it is not offered.
BoundResult minimise_interpolation_bound(const std::vector<PointSupply>& supply,
                                       std::size_t divisor_degree);

}  // namespace curve_bounds
