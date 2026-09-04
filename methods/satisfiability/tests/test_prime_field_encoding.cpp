/// The one-hot GF(p) encoding, checked the same way as the Boolean one and for
/// the same reason: this is the backend whose field arithmetic is hand-written,
/// so it is the one that can be confidently wrong.
///
/// A decomposition we already know gives the assignment the formula must
/// accept. Propagating the definitional clauses fills in every derived group,
/// and then nothing may be violated. If the tensor is altered, the value forced
/// by the definitions and the value demanded by the final clause disagree, two
/// members of one one-hot group are true at once, and the at-most-one clause
/// catches it, which is how a wrong answer becomes a failing test rather than
/// a plausible number.
#include <cstdlib>
#include <iostream>
#include <vector>

#include "binary_encoding.h"
#include "check.h"
#include "model_decomposition.h"
#include "prime_field_encoding.h"
#include "rank_question.h"
#include "solver_process.h"

namespace {

using formats::Cnf;
using formats::Model;
using satisfiability::Field;
using satisfiability::Matrix;
using satisfiability::PrimeFieldEncoding;

/// A rank-one term over GF(p), as three vectors of field values.
struct Term {
    std::vector<std::size_t> left;
    std::vector<std::size_t> right;
    std::vector<std::size_t> output;
};

/// Add a plain integer into a field element.
void Element_add(const Field& field, satisfiability::Element& target, std::size_t value) {
    satisfiability::Element addend;
    field.init(addend, static_cast<int64_t>(value));
    field.addin(target, addend);
}

formats::Tensor tensor_from(const Field& field, std::size_t characteristic,
                                   const std::vector<Term>& terms, std::size_t rows,
                                   std::size_t columns, std::size_t slices) {
    formats::Tensor tensor;
    tensor.characteristic = static_cast<int64_t>(characteristic);
    tensor.slices.assign(slices, Matrix(rows, columns));

    for (const Term& term : terms) {
        for (std::size_t slice = 0; slice < slices; ++slice) {
            for (std::size_t row = 0; row < rows; ++row) {
                for (std::size_t column = 0; column < columns; ++column) {
                    const std::size_t value =
                        (term.left[row] * term.right[column] * term.output[slice]) % characteristic;
                    Element_add(field, tensor.slices[slice](row, column), value);
                }
            }
        }
    }
    return tensor;
}

/// Set the variables standing for the known field values, then let the
/// definitional clauses force everything else.
Model model_for(const PrimeFieldEncoding& encoding, const std::vector<Term>& terms) {
    Model model;
    model.answered = true;
    model.satisfiable = true;
    model.values.assign(encoding.formula.variable_count + 1, false);

    const std::size_t p = encoding.characteristic;
    for (std::size_t term = 0; term < terms.size(); ++term) {
        for (std::size_t index = 0; index < encoding.rows; ++index) {
            model.values[static_cast<std::size_t>(
                encoding.left[(term * encoding.rows + index) * p + terms[term].left[index]])] = true;
        }
        for (std::size_t index = 0; index < encoding.columns; ++index) {
            model.values[static_cast<std::size_t>(
                encoding.right[(term * encoding.columns + index) * p +
                               terms[term].right[index]])] = true;
        }
        for (std::size_t index = 0; index < encoding.slices; ++index) {
            model.values[static_cast<std::size_t>(
                encoding.output[(term * encoding.slices + index) * p +
                                terms[term].output[index]])] = true;
        }
    }
    return model;
}

/// Propagate unit clauses and the `(-x, -y, z)` definitions to a fixed point.
void complete(const Cnf& formula, Model& model, std::size_t passes) {
    for (std::size_t pass = 0; pass < passes; ++pass) {
        for (const std::vector<int>& clause : formula.clauses) {
            if (clause.size() == 1 && clause[0] > 0) {
                model.values[static_cast<std::size_t>(clause[0])] = true;
            } else if (clause.size() == 3 && clause[0] < 0 && clause[1] < 0 && clause[2] > 0) {
                const bool first = model.values[static_cast<std::size_t>(-clause[0])];
                const bool second = model.values[static_cast<std::size_t>(-clause[1])];
                if (first && second) model.values[static_cast<std::size_t>(clause[2])] = true;
            }
        }
    }
}

bool literal_holds(const Model& model, int literal) {
    const bool value = model.values[static_cast<std::size_t>(std::abs(literal))];
    return literal > 0 ? value : !value;
}

/// How many clauses the assignment breaks. No parities here, unlike the
/// binary encoding's version of this same check: the one-hot encoding needs
/// none.
std::size_t violations(const Cnf& formula, const Model& model) {
    std::size_t broken = 0;
    for (const std::vector<int>& clause : formula.clauses) {
        bool any = false;
        for (int literal : clause) any = any || literal_holds(model, literal);
        if (!any) ++broken;
    }
    return broken;
}

}  // namespace

int main() {
    // GF(3), two rank-one terms of a 2x2 map with two slices.
    const std::size_t characteristic = 3;
    const Field field(static_cast<int64_t>(characteristic));
    const std::vector<Term> terms = {Term{{1, 0}, {1, 0}, {1, 0}}, Term{{0, 2}, {0, 1}, {0, 1}}};
    const auto tensor = tensor_from(field, characteristic, terms, 2, 2, 2);

    auto encoding = satisfiability::encode_prime_rank_at_most(tensor, 2);
    check::equal("one-hot groups for every unknown",
                 static_cast<long long>(encoding.left.size() + encoding.right.size() +
                                        encoding.output.size()),
                 static_cast<long long>(2 * (2 + 2 + 2) * characteristic));

    Model model = model_for(encoding, terms);
    complete(encoding.formula, model, 8);
    check::equal("the known GF(3) decomposition satisfies its encoding",
                 static_cast<long long>(violations(encoding.formula, model)), 0);
    check::equal("and the model rebuilds the tensor",
                 satisfiability::model_reconstructs(field, tensor, encoding, model) ? 1 : 0, 1);

    // Alter one entry: the definitions force one value and the final clause
    // demands another, so a one-hot group has two members true.
    auto perturbed = tensor;
    satisfiability::Element one;
    field.init(one, 1);
    field.addin(perturbed.slices[0](0, 0), one);
    auto other = satisfiability::encode_prime_rank_at_most(perturbed, 2);
    Model same = model_for(other, terms);
    complete(other.formula, same, 8);
    check::equal("a changed tensor is no longer satisfied",
                 violations(other.formula, same) > 0 ? 1 : 0, 1);

    // GF(2) through the general encoder must accept what the Boolean one does.
    // The two agreeing there is the cheapest evidence the tables are right.
    const Field binary(2);
    const std::vector<Term> karatsuba = {Term{{1, 0}, {1, 0}, {1, 0, 1}},
                                         Term{{0, 1}, {0, 1}, {0, 0, 1}},
                                         Term{{1, 1}, {1, 1}, {0, 1, 0}}};
    const auto binary_tensor = tensor_from(binary, 2, karatsuba, 2, 2, 3);
    auto general = satisfiability::encode_prime_rank_at_most(binary_tensor, 3);
    Model shared = model_for(general, karatsuba);
    complete(general.formula, shared, 10);
    check::equal("Karatsuba satisfies the general encoder too",
                 static_cast<long long>(violations(general.formula, shared)), 0);
    check::equal("and it rebuilds the same tensor",
                 satisfiability::model_reconstructs(binary, binary_tensor, general, shared) ? 1 : 0,
                 1);

    // Over GF(p) symmetry breaking quotients by two things, and the scaling one
    // is the dangerous half: (la) x (mb) x (nc) is the same term as a x b x c
    // whenever lmn = 1, so the constraint normalises two operand vectors to a
    // first nonzero of 1. If that is written wrong it deletes decompositions
    // and every UNSAT above it becomes a false lower bound. So: normalise a
    // decomposition of known rank by hand, order it, and it must survive.
    {
        const auto inverse = [&](std::size_t value) {
            for (std::size_t candidate = 1; candidate < characteristic; ++candidate) {
                if ((value * candidate) % characteristic == 1) return candidate;
            }
            return std::size_t{1};
        };
        const auto leading = [&](const std::vector<std::size_t>& vector) {
            for (std::size_t value : vector) {
                if (value != 0) return value;
            }
            return std::size_t{1};
        };

        std::vector<Term> normalised;
        for (const Term& term : terms) {
            const std::size_t left_scale = inverse(leading(term.left));
            const std::size_t right_scale = inverse(leading(term.right));
            const std::size_t absorbed = (leading(term.left) * leading(term.right)) % characteristic;

            Term fixed;
            for (std::size_t value : term.left) fixed.left.push_back((value * left_scale) % characteristic);
            for (std::size_t value : term.right) fixed.right.push_back((value * right_scale) % characteristic);
            for (std::size_t value : term.output) fixed.output.push_back((value * absorbed) % characteristic);
            normalised.push_back(fixed);
        }

        // The normalised terms must still be the same tensor.
        const auto rebuilt = tensor_from(field, characteristic, normalised, 2, 2, 2);
        bool same = true;
        for (std::size_t slice = 0; slice < 2; ++slice) {
            for (std::size_t entry = 0; entry < rebuilt.slices[slice].entry_count(); ++entry) {
                same = same && field.areEqual(rebuilt.slices[slice].data()[entry],
                                              tensor.slices[slice].data()[entry]);
            }
        }
        check::equal("normalising the scalars preserves the tensor", same ? 1 : 0, 1);

        const satisfiability::SatSolver solver = satisfiability::find_sat_solver(false);
        if (!solver.found) {
            std::cout << "  skip  no SAT solver on PATH, GF(3) symmetry soundness unchecked\n";
        } else {
            satisfiability::SolveOptions approach;
            approach.break_symmetry = true;
            approach.plain_cnf = true;
            approach.timeout_seconds = 60;

            const auto found = satisfiability::decide_rank(tensor, 2, approach);
            check::equal("the GF(3) rank is still found with both symmetries broken",
                         found.verdict == satisfiability::Verdict::Yes ? 1 : 0, 1);
            const auto refused = satisfiability::decide_rank(tensor, 1, approach);
            check::equal("and one product is still refused",
                         refused.verdict == satisfiability::Verdict::No ? 1 : 0, 1);
        }
    }

    // A composite characteristic has no field to write a table for.
    auto composite = tensor;
    composite.characteristic = 4;
    bool threw = false;
    try {
        satisfiability::encode_prime_rank_at_most(composite, 2);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    check::equal("GF(4) is refused, it is not a prime field", threw ? 1 : 0, 1);

    // `SolveOptions::cubes` has always said GF(2) only, and nothing said so at
    // runtime. Over a larger prime the literals index a different encoding, and
    // this encoder orders term 0 against term 1 and normalises its first nonzero
    // entry to 1, neither of which the orbit representative a cube pins need
    // satisfy. The answer could be a no that is not a lower bound, so the
    // question is refused before any solver runs.
    bool cubes_refused = false;
    try {
        satisfiability::SolveOptions cubed;
        cubed.break_symmetry = true;
        cubed.cubes = {{1}, {-1}};
        satisfiability::decide_rank(tensor, 2, cubed);
    } catch (const std::invalid_argument&) {
        cubes_refused = true;
    }
    check::equal("a cube split over GF(3) is refused", cubes_refused ? 1 : 0, 1);

    return check::report("prime field encoding");
}
