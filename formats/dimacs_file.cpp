#include "dimacs_file.h"

#include <algorithm>
#include <deque>
#include <cstdlib>
#include <sstream>
#include <string>

namespace linear_algebra {

namespace {

/// Clauses forcing the xor of `slice` to be false: one clause forbidding each
/// odd-parity sign pattern, 2^(n-1) clauses of length n. The n = 3 case is the
/// classic xor gadget; larger slices are what a cutting number above 3 buys.
void forbid_odd_parity(Cnf& formula, const std::vector<int>& slice) {
    const std::size_t patterns = std::size_t{1} << slice.size();
    for (std::size_t pattern = 0; pattern < patterns; ++pattern) {
        if (__builtin_parityll(pattern) == 0) continue;   // even patterns satisfy xor = 0
        // The slice's output (its last element) leads each clause: the model
        // sweep in test_binary_encoding derives forced aux values from clauses
        // shaped `-output, inputs...`, the old fixed gadget's order.
        std::vector<int> clause(slice.size());
        for (std::size_t bit = 0; bit < slice.size(); ++bit) {
            clause[bit] = (pattern >> bit) & 1 ? -slice[bit] : slice[bit];
        }
        std::rotate(clause.begin(), clause.end() - 1, clause.end());
        formula.add_clause(std::move(clause));
    }
}

/// Heule's zero-or-two streamliner on one slice: forbid the even patterns with
/// four or more true literals, so each slice may hold zero or two. Sufficient,
/// not necessary - a solution may be lost, which is the streamlining contract.
void forbid_heavy_even_patterns(Cnf& formula, const std::vector<int>& slice) {
    const std::size_t patterns = std::size_t{1} << slice.size();
    for (std::size_t pattern = 0; pattern < patterns; ++pattern) {
        if (__builtin_parityll(pattern) != 0) continue;
        if (__builtin_popcountll(pattern) < 4) continue;
        std::vector<int> clause(slice.size());
        for (std::size_t bit = 0; bit < slice.size(); ++bit) {
            clause[bit] = (pattern >> bit) & 1 ? -slice[bit] : slice[bit];
        }
        formula.add_clause(std::move(clause));
    }
}

}  // namespace

Cnf with_parities_expanded(const Cnf& formula, ParityExpansion shape) {
    const std::size_t cutting_number = shape.cutting_number;
    const bool pooled = shape.pooled;
    if (cutting_number < 3) std::abort();   // a slice needs one output and two inputs
    Cnf expanded;
    expanded.variable_count = formula.variable_count;
    expanded.clauses = formula.clauses;

    for (const Cnf::Parity& parity : formula.parities) {
        if (parity.literals.empty()) {
            // Nothing xored is false. Wanting it true is a contradiction, and
            // the empty clause is how DIMACS says so.
            if (parity.value) expanded.add_clause({});
            continue;
        }

        // Slices of up to cutting_number - 1 inputs each get a fresh output
        // carrying their xor. Linear chaining consumes the carried output first,
        // so the chain a walk must traverse is as long as the parity; pooled
        // chaining takes inputs first-in-first-out and pushes each output at the
        // back, which makes the chains logarithmic - the SAT 2021 measurement
        // behind this knob found cut 6 pooled the best CNF of a length-23 parity
        // and this function's old fixed shape, cut 3 linear, the worst.
        std::deque<int> pending(parity.literals.begin(), parity.literals.end());
        while (pending.size() > 1) {
            std::vector<int> slice;
            const std::size_t inputs =
                std::min(cutting_number - 1, pending.size());
            if (pooled) {
                for (std::size_t taken = 0; taken < inputs; ++taken) {
                    slice.push_back(pending.front());
                    pending.pop_front();
                }
            } else {
                slice.push_back(pending.front());   // the carried output stays first
                pending.pop_front();
                for (std::size_t taken = 1; taken < inputs; ++taken) {
                    slice.push_back(pending.front());
                    pending.pop_front();
                }
            }
            const int output = expanded.new_variable();
            slice.push_back(output);                // xor(inputs) ^ output = 0, so output carries the xor
            forbid_odd_parity(expanded, slice);
            if (shape.zero_or_two) forbid_heavy_even_patterns(expanded, slice);
            if (pooled) pending.push_back(output);
            else pending.push_front(output);
        }
        expanded.add_clause({parity.value ? pending.front() : -pending.front()});
    }
    return expanded;
}

std::size_t Cnf::total_clause_count(bool native_xor) const {
    if (native_xor) return clauses.size() + parities.size();
    return with_parities_expanded(*this).clauses.size();
}

std::size_t Cnf::total_variable_count(bool native_xor) const {
    if (native_xor) return variable_count;
    return with_parities_expanded(*this).variable_count;
}

void write_dimacs(std::ostream& output, const Cnf& formula, bool native_xor,
                  ParityExpansion shape) {
    const Cnf written = native_xor ? formula : with_parities_expanded(formula, shape);

    output << "p cnf " << written.variable_count << ' '
           << (written.clauses.size() + written.parities.size()) << '\n';
    for (const std::vector<int>& clause : written.clauses) {
        for (int literal : clause) output << literal << ' ';
        output << "0\n";
    }
    for (const Cnf::Parity& parity : written.parities) {
        output << 'x';
        // A negated literal flips the parity a solver requires, so an even
        // constraint is written by negating one of its literals.
        bool flipped = parity.value;
        for (int literal : parity.literals) {
            output << (flipped ? literal : -literal) << ' ';
            flipped = true;
        }
        output << "0\n";
    }
}

Model read_dimacs_model(std::istream& input) {
    Model model;
    std::vector<int> assigned;

    std::string line;
    while (std::getline(input, line)) {
        if (line.rfind("s ", 0) == 0) {
            // Only the two verdicts are answers. A solver interrupted by its
            // deadline can still print an s line - kissat's alarm handler says
            // `s UNKNOWN` on the way out - and reading any s line as an answer
            // turned a timeout into a refutation on 2026-09-01: cyclic_f2_7
            // "refuted" at 601 s under a 600 s cap and at 901 s under a 900 s
            // one, the verdict tracking the cap.
            const bool unsatisfiable = line.find("UNSATISFIABLE") != std::string::npos;
            const bool satisfiable = !unsatisfiable && line.find("SATISFIABLE") != std::string::npos;
            model.answered = satisfiable || unsatisfiable;
            model.satisfiable = satisfiable;
            continue;
        }
        if (line.rfind("v ", 0) != 0) continue;

        std::istringstream values(line.substr(2));
        int literal = 0;
        while (values >> literal) {
            if (literal != 0) assigned.push_back(literal);
        }
    }

    if (!model.satisfiable) return model;
    std::size_t highest = 0;
    for (int literal : assigned) {
        highest = std::max(highest, static_cast<std::size_t>(std::abs(literal)));
    }
    model.values.assign(highest + 1, false);
    for (int literal : assigned) {
        if (literal > 0) model.values[static_cast<std::size_t>(literal)] = true;
    }
    return model;
}

}  // namespace linear_algebra
