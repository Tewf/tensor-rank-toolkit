/// The GF(2) encoding, checked without a solver anywhere in sight.
///
/// The trick is that a decomposition we already know gives the assignment the
/// formula is supposed to accept. So: build a tensor as a sum of rank-one terms,
/// write down the truth assignment those terms stand for, and assert every
/// clause and every parity holds. If the encoding says otherwise it is wrong,
/// and no solver was needed to find that out.
///
/// The negative is the same instrument: change one entry of the tensor and the
/// same assignment must now break something. An encoding that accepts anything
/// would pass the first check and fail this one.
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "binary_encoding.h"
#include "check.h"
#include "model_decomposition.h"
#include "rank_question.h"
#include "solver_process.h"

namespace {

using formats::Cnf;
using formats::Model;
using satisfiability::BinaryEncoding;
using satisfiability::Field;
using satisfiability::Matrix;

/// One rank-one term of a decomposition, as three GF(2) vectors.
struct Term {
    std::vector<bool> left;
    std::vector<bool> right;
    std::vector<bool> output;
};

formats::Tensor tensor_from(const Field& field, const std::vector<Term>& terms,
                                   std::size_t rows, std::size_t columns, std::size_t slices) {
    formats::Tensor tensor;
    tensor.characteristic = 2;
    tensor.slices.assign(slices, Matrix(rows, columns));

    for (const Term& term : terms) {
        for (std::size_t slice = 0; slice < slices; ++slice) {
            if (!term.output[slice]) continue;
            for (std::size_t row = 0; row < rows; ++row) {
                if (!term.left[row]) continue;
                for (std::size_t column = 0; column < columns; ++column) {
                    if (!term.right[column]) continue;
                    field.addin(tensor.slices[slice](row, column), field.one);
                }
            }
        }
    }
    return tensor;
}

/// The truth assignment those terms stand for, in the encoding's numbering.
///
/// Only the a, b and c variables are set here; every Tseitin variable is then
/// forced, so filling them in is mechanical and done below.
Model model_for(const BinaryEncoding& encoding, const std::vector<Term>& terms) {
    Model model;
    model.answered = true;
    model.satisfiable = true;
    model.values.assign(encoding.formula.variable_count + 1, false);

    for (std::size_t term = 0; term < terms.size(); ++term) {
        for (std::size_t index = 0; index < encoding.rows; ++index) {
            model.values[static_cast<std::size_t>(encoding.left[term * encoding.rows + index])] =
                terms[term].left[index];
        }
        for (std::size_t index = 0; index < encoding.columns; ++index) {
            model.values[static_cast<std::size_t>(encoding.right[term * encoding.columns + index])] =
                terms[term].right[index];
        }
        for (std::size_t index = 0; index < encoding.slices; ++index) {
            model.values[static_cast<std::size_t>(encoding.output[term * encoding.slices + index])] =
                terms[term].output[index];
        }
    }
    return model;
}

/// Propagate the definitional clauses so every Tseitin variable takes the value
/// its definition forces. Repeated to a fixed point, which is enough because the
/// definitions form a DAG.
void complete(const Cnf& formula, Model& model) {
    for (int pass = 0; pass < 4; ++pass) {
        for (const std::vector<int>& clause : formula.clauses) {
            if (clause.size() != 3) continue;
            // The shape written by equate_to_conjunction: (r, -l, -r').
            if (clause[0] <= 0 || clause[1] >= 0 || clause[2] >= 0) continue;
            const bool left = model.values[static_cast<std::size_t>(-clause[1])];
            const bool right = model.values[static_cast<std::size_t>(-clause[2])];
            model.values[static_cast<std::size_t>(clause[0])] = left && right;
        }
    }
}

bool literal_holds(const Model& model, int literal) {
    const bool value = model.values[static_cast<std::size_t>(std::abs(literal))];
    return literal > 0 ? value : !value;
}

/// How many clauses and parities the assignment breaks.
std::size_t violations(const Cnf& formula, const Model& model) {
    std::size_t broken = 0;
    for (const std::vector<int>& clause : formula.clauses) {
        bool any = false;
        for (int literal : clause) any = any || literal_holds(model, literal);
        if (!any) ++broken;
    }
    for (const Cnf::Parity& parity : formula.parities) {
        bool odd = false;
        for (int literal : parity.literals) odd = odd != literal_holds(model, literal);
        if (odd != parity.value) ++broken;
    }
    return broken;
}

std::vector<Term> karatsuba_terms() {
    // Two-by-two polynomial multiplication over GF(2) in three products:
    // a0b0, a1b1, and (a0+a1)(b0+b1). Slices are the three coefficients.
    return {Term{{true, false}, {true, false}, {true, false, true}},
            Term{{false, true}, {false, true}, {false, false, true}},
            Term{{true, true}, {true, true}, {false, true, false}}};
}

}  // namespace

/// The correctness condition the cube split rests on, asserted where it would
/// break rather than four minutes later inside a solver.
///
/// A cube pins term 0 to an orbit representative; the ordering demands term 0 be
/// lexicographically least. A representative need not be least, so conjoining
/// the two removes decompositions that exist and returns a false lower bound.
/// Each is sound alone. `first_term_pinned` is what keeps them apart, and the
/// only clauses in this encoding that mention two different terms at once are
/// the ordering ones, so "no clause mentions term 0 beside another term" is
/// exactly "term 0 is unordered".
void check_a_cube_leaves_term_zero_unordered(const formats::Tensor& tensor) {
    const auto ordered = satisfiability::encode_binary_rank_at_most(tensor, 3, true, false);
    const auto pinned = satisfiability::encode_binary_rank_at_most(tensor, 3, true, true);

    auto mentions_term_zero_beside_another = [](const satisfiability::BinaryEncoding& encoding) {
        std::vector<int> first;
        std::vector<int> later;
        for (std::size_t term = 0; term < encoding.products; ++term) {
            std::vector<int>& into = term == 0 ? first : later;
            for (std::size_t index = 0; index < encoding.rows; ++index) {
                into.push_back(encoding.left[term * encoding.rows + index]);
            }
            for (std::size_t index = 0; index < encoding.columns; ++index) {
                into.push_back(encoding.right[term * encoding.columns + index]);
            }
            for (std::size_t index = 0; index < encoding.slices; ++index) {
                into.push_back(encoding.output[term * encoding.slices + index]);
            }
        }
        for (const std::vector<int>& clause : encoding.formula.clauses) {
            bool has_first = false;
            bool has_later = false;
            for (const int literal : clause) {
                const int variable = literal < 0 ? -literal : literal;
                if (std::find(first.begin(), first.end(), variable) != first.end()) has_first = true;
                if (std::find(later.begin(), later.end(), variable) != later.end()) has_later = true;
            }
            if (has_first && has_later) return true;
        }
        return false;
    };

    check::equal("without a cube, term 0 is ordered against term 1",
                 mentions_term_zero_beside_another(ordered) ? 1 : 0, 1);
    check::equal("with a cube, term 0 is ordered against nothing",
                 mentions_term_zero_beside_another(pinned) ? 1 : 0, 0);
}

/// A cube is built from one encoding and reused for every `k` in a sweep, which
/// is only sound while term 0's operand variables keep the same numbers whatever
/// `products` is. True because each term's variables are allocated before the
/// next term's. Asserted here so that reordering that loop fails a test instead
/// of pinning the wrong variables in silence.
void check_term_zero_numbering_does_not_move(const formats::Tensor& tensor) {
    const auto one = satisfiability::encode_binary_rank_at_most(tensor, 1);
    const auto many = satisfiability::encode_binary_rank_at_most(tensor, 5);

    bool same = true;
    for (std::size_t index = 0; index < one.rows; ++index) {
        if (one.left[index] != many.left[index]) same = false;
    }
    for (std::size_t index = 0; index < one.columns; ++index) {
        if (one.right[index] != many.right[index]) same = false;
    }
    check::equal("term 0's operand variables do not move with the product count", same ? 1 : 0, 1);
}

int main() {
    const Field field(2);
    const std::vector<Term> terms = karatsuba_terms();
    const auto tensor = tensor_from(field, terms, 2, 2, 3);

    // Sanity: that really is 2x2-term polynomial multiplication.
    check::equal("slice 0 is a0b0", field.isOne(tensor.slices[0](0, 0)) ? 1 : 0, 1);
    check::equal("slice 1 has a0b1", field.isOne(tensor.slices[1](0, 1)) ? 1 : 0, 1);
    check::equal("slice 2 is a1b1", field.isOne(tensor.slices[2](1, 1)) ? 1 : 0, 1);

    auto encoding = satisfiability::encode_binary_rank_at_most(tensor, 3);
    check::equal("three terms of a 2x2x3 tensor use 21 literal variables",
                 static_cast<long long>(encoding.left.size() + encoding.right.size() +
                                        encoding.output.size()),
                 21);
    check::equal("one parity per tensor entry",
                 static_cast<long long>(encoding.formula.parities.size()), 12);

    Model model = model_for(encoding, terms);
    complete(encoding.formula, model);
    check::equal("Karatsuba satisfies its own encoding",
                 static_cast<long long>(violations(encoding.formula, model)), 0);
    check::equal("and the model rebuilds the tensor",
                 satisfiability::model_reconstructs(field, tensor, encoding, model) ? 1 : 0, 1);

    // The decomposition read back out is three rank-one matrices.
    const auto recovered = satisfiability::decomposition_from_model(field, encoding, model);
    check::equal("three terms recovered", static_cast<long long>(recovered.size()), 3);

    // Change one entry and the same assignment must break a parity.
    auto perturbed = tensor;
    field.addin(perturbed.slices[0](1, 1), field.one);
    auto other = satisfiability::encode_binary_rank_at_most(perturbed, 3);
    Model same = model_for(other, terms);
    complete(other.formula, same);
    check::equal("a changed tensor is no longer satisfied",
                 violations(other.formula, same) > 0 ? 1 : 0, 1);

    // The expansion of parities into ordinary clauses must agree with them.
    const Cnf expanded = formats::with_parities_expanded(encoding.formula);
    check::equal("expansion leaves no parities",
                 static_cast<long long>(expanded.parities.size()), 0);
    Model widened = model;
    widened.values.resize(expanded.variable_count + 1, false);
    // Tseitin xor variables are forced; solve them by one sweep in order.
    for (std::size_t pass = 0; pass < 2; ++pass) {
        for (const std::vector<int>& clause : expanded.clauses) {
            if (clause.size() != 3 || clause[0] >= 0) continue;
            const std::size_t result = static_cast<std::size_t>(-clause[0]);
            if (result <= encoding.formula.variable_count) continue;
            widened.values[result] = literal_holds(widened, clause[1]) != literal_holds(widened, clause[2]);
        }
    }
    check::equal("the expanded formula is satisfied by the same decomposition",
                 static_cast<long long>(violations(expanded, widened)), 0);

    // Symmetry breaking is the most dangerous constraint here: too strong and it
    // deletes a decomposition that exists, turning SAT into UNSAT and reporting
    // a lower bound that is false. Hand-propagating the ordering chain is not a
    // fair test of it, so this asks a real solver, and skips when there is none
    // rather than pretending to have checked.
    {
        const satisfiability::SatSolver solver = satisfiability::find_sat_solver(false);
        if (!solver.found) {
            std::cout << "  skip  no SAT solver on PATH, symmetry soundness unchecked\n";
        } else {
            satisfiability::SolveOptions approach;
            approach.break_symmetry = true;
            approach.plain_cnf = true;
            approach.timeout_seconds = 60;

            const auto found = satisfiability::decide_rank(tensor, 3, approach);
            check::equal("Karatsuba is still found with the ordering on",
                         found.verdict == satisfiability::Verdict::Yes ? 1 : 0, 1);
            const auto refused = satisfiability::decide_rank(tensor, 2, approach);
            check::equal("and two products are still refused",
                         refused.verdict == satisfiability::Verdict::No ? 1 : 0, 1);

            // A no is the verdict that cannot be checked after the fact, so it
            // should come with a refutation something else can check.
            // Cubes, checked without needing anybody's group. Every assignment
            // of the first term's left vector is a cube, so the set trivially
            // covers, and solving per cube must agree with solving whole. If
            // the ordering constraint were still applied to the pinned term
            // this is where it would show: a yes would become a no.
            {
                const auto pinned = satisfiability::encode_binary_rank_at_most(tensor, 3, true, true);
                const int first = pinned.left[0];
                const int second = pinned.left[1];
                satisfiability::SolveOptions cubed = approach;
                cubed.cubes = {{-first, -second}, {-first, second},
                               {first, -second},  {first, second}};

                const auto whole = satisfiability::decide_rank(tensor, 3, cubed);
                check::equal("exhaustive cubes still find Karatsuba",
                             whole.verdict == satisfiability::Verdict::Yes ? 1 : 0, 1);
                const auto none = satisfiability::decide_rank(tensor, 2, cubed);
                check::equal("and still refuse two products",
                             none.verdict == satisfiability::Verdict::No ? 1 : 0, 1);
            }

            // The same hazard where it can actually bite. An exhaustive cube
            // set cannot see it: those cubes union back to the whole formula,
            // ordering and all, so the union stays satisfiable either way. A
            // real orbit representative is one cube out of many, and it need
            // not be lexicographically least. Pin term 0 to the all-ones
            // triple and, if the ordering still starts at term 0, every later
            // term is forced to all ones as well, so the only tensor the
            // formula can build is the all-ones one and this instance comes
            // back no. That no is a false lower bound.
            {
                const std::vector<Term> pair = {Term{{true, true}, {true, true}, {true, true}},
                                                Term{{true, false}, {true, false}, {true, false}}};
                const auto sum = tensor_from(field, pair, 2, 2, 2);
                const auto layout = satisfiability::encode_binary_rank_at_most(sum, 2, true, true);

                satisfiability::SolveOptions ordered = approach;
                ordered.cubes = {{layout.left[0], layout.left[1], layout.right[0], layout.right[1],
                                  layout.output[0], layout.output[1]}};
                const auto kept = satisfiability::decide_rank(sum, 2, ordered);
                check::equal("a cube pinning a term that is not the least is still satisfiable",
                             kept.verdict == satisfiability::Verdict::Yes ? 1 : 0, 1);
            }

            // Bisection is unsound unless the question is monotone in k, so
            // that is asserted rather than assumed: a decomposition into k
            // gives one into k+1 by adding a zero term, and a refusal at k
            // refuses everything below.
            check::equal("satisfiable at 3 stays satisfiable at 4",
                         satisfiability::decide_rank(tensor, 4, approach).verdict ==
                                 satisfiability::Verdict::Yes
                             ? 1
                             : 0,
                         1);
            check::equal("refused at 2 stays refused at 1",
                         satisfiability::decide_rank(tensor, 1, approach).verdict ==
                                 satisfiability::Verdict::No
                             ? 1
                             : 0,
                         1);

            // The search itself, with and without the cheap first pass. Both
            // must reach the same exact answer; the ladder changes what it
            // spends, never what it concludes.
            {
                const auto plain = satisfiability::find_rank(tensor, approach, 1, 8);
                check::equal("the search finds the rank exactly",
                             plain.exact && plain.upper == 3 ? 1 : 0, 1);

                satisfiability::SolveOptions laddered = approach;
                laddered.probe_seconds = 1;
                const auto stepped = satisfiability::find_rank(tensor, laddered, 1, 8);
                check::equal("and the probe budget does not change the answer",
                             stepped.exact && stepped.upper == 3 ? 1 : 0, 1);
            }

            if (solver.writes_proofs) {
                satisfiability::SolveOptions certified = approach;
                certified.proof_path = "/tmp/tensor-rank-test.drat";
                const auto proved = satisfiability::decide_rank(tensor, 2, certified);
                check::equal("the refusal comes with a refutation",
                             proved.proof_bytes > 0 ? 1 : 0, 1);
            }
        }
    }

    // GF(3) must be refused rather than silently encoded as if it were GF(2).
    auto ternary = tensor;
    ternary.characteristic = 3;
    bool threw = false;
    try {
        satisfiability::encode_binary_rank_at_most(ternary, 3);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    check::equal("GF(3) is refused by the Boolean encoding", threw ? 1 : 0, 1);

    check_a_cube_leaves_term_zero_unordered(tensor);
    check_term_zero_numbering_does_not_move(tensor);

    // An encoding that cannot fit is refused before it is built.
    satisfiability::set_variable_budget(10);
    bool refused = false;
    try {
        satisfiability::encode_binary_rank_at_most(tensor, 3);
    } catch (const std::invalid_argument&) {
        refused = true;
    }
    satisfiability::set_variable_budget(10'000'000);
    check::equal("an oversized encoding is refused", refused ? 1 : 0, 1);

    // A proof asked of a solver that writes none. Refused before anything runs,
    // which is why this needs no solver installed and why the path can be
    // nonsense. The request used to be dropped instead, so the run came back a
    // no with `proof_bytes` at zero, indistinguishable from a refusal nobody
    // asked to certify.
    satisfiability::SatSolver proofless;
    proofless.found = true;
    proofless.name = "cryptominisat";
    proofless.path = "/nonexistent";
    bool proof_refused = false;
    try {
        satisfiability::run_solver(satisfiability::encode_binary_rank_at_most(tensor, 2).formula, proofless,
                              2048, 300, "/tmp/tensor-rank-unwritten.drat");
    } catch (const std::invalid_argument&) {
        proof_refused = true;
    }
    check::equal("a proof asked of a solver that writes none is refused", proof_refused ? 1 : 0, 1);

    return check::report("binary encoding");
}
