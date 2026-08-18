#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "field.h"

/// A linear objective under linear constraints, with some variables required to
/// be whole.
///
/// Several things in this repository minimise something subject to constraints
/// and used to do it by enumeration. The curve strand's interpolation bound is
/// stated here now and enumerated only as a fallback and a cross-check; the two
/// sparsification oracles still walk column subsets.
/// [`solver_chain.h`](solver_chain.h) is what solves a model once it is written.
///
/// Everything is exact rationals, for the reason the rest of the repository is:
/// the quantities being minimised are counts, and a count that is nearly right
/// is a different count.
namespace optimisation {

using Number = linear_algebra::RationalField::Element;

enum class Sense { Minimise, Maximise };
enum class Relation { LessOrEqual, GreaterOrEqual, Equal };

/// A variable is continuous and non-negative unless said otherwise.
///
/// Both bounds are carried explicitly rather than by a sentinel, because "no
/// upper bound" is a state the file formats disagree about and it has to survive
/// being written down. See [`mps_format.h`](mps_format.h).
struct Variable {
    std::string name;
    bool bounded_below = true;
    Number lower = Number(0);
    bool bounded_above = false;
    Number upper = Number(0);
    bool integral = false;
};

/// One variable's coefficient in one constraint.
struct Coefficient {
    std::size_t variable = 0;
    Number value = Number(0);
};

/// Sparse, because the programmes that matter are. Encoding a rank question
/// writes rows of four nonzeros among two thousand variables, and carrying the
/// zeros costs hundreds of megabytes for nothing.
struct Constraint {
    std::string name;
    std::vector<Coefficient> terms;
    Relation relation = Relation::LessOrEqual;
    Number bound = Number(0);
};

/// A constraint from a dense row, dropping the zeros. For the small programmes
/// that are written out by hand.
Constraint constraint_of(const std::vector<Number>& coefficients, Relation relation, Number bound);

struct IntegerProgramme {
    Sense sense = Sense::Minimise;
    std::vector<Variable> variables;
    std::vector<Number> objective;  // one per variable, in order
    std::vector<Constraint> constraints;
};

/// Whether there is an answer at all, and how much of one.
///
/// `Exhausted` is the honest outcome when a search ran out of budget or a solver
/// gave nothing usable: it is not a claim that no answer exists. Only `Optimal`
/// and `Infeasible` are claims, and only the built-in solver makes the second.
enum class Status { Optimal, Infeasible, Unbounded, Exhausted };

struct Solution {
    Status status = Status::Exhausted;
    Number objective = Number(0);
    std::vector<Number> values;
    std::string solved_by;
};

/// Does this point satisfy every constraint, every bound and every integrality
/// demand, exactly?
///
/// The external solvers answer in decimal, so what comes back from them is a
/// claim rather than a result. This is how the claim is checked, and nothing
/// leaves [`solver_chain.h`](solver_chain.h) that has not passed it.
bool satisfies(const IntegerProgramme& programme, const std::vector<Number>& values);

/// The objective at a point, from the model's own coefficients rather than from
/// whatever the solver printed.
Number objective_at(const IntegerProgramme& programme, const std::vector<Number>& values);

/// Is `candidate` the better of the two values, in this programme's direction?
bool improves(Sense sense, const Number& candidate, const Number& incumbent);

}  // namespace optimisation
