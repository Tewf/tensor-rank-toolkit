#pragma once

#include <cstddef>
#include <istream>
#include <ostream>
#include <vector>

namespace linear_algebra {

/// A formula in conjunctive normal form, and the DIMACS files it is written to
/// and its answer read back from.
///
/// Variables are numbered from 1, and a literal is that number, negated by a
/// minus sign, which is the DIMACS convention and not worth translating.
///
/// Parity constraints are kept apart from ordinary clauses because they are
/// the whole reason a solver choice matters here. A GF(2) tensor equation *is*
/// a parity constraint, and a solver with native XOR takes it as one line;
/// without that it has to be expanded into four clauses per pair and a chain of
/// fresh variables. Keeping them separate lets the same formula be written
/// either way rather than committing at the point it is built.
struct Cnf {
    std::size_t variable_count = 0;
    std::vector<std::vector<int>> clauses;

    /// `literals` xored together must equal `value`.
    struct Parity {
        std::vector<int> literals;
        bool value = false;
    };
    std::vector<Parity> parities;

    int new_variable() { return static_cast<int>(++variable_count); }
    void add_clause(std::vector<int> literals) { clauses.push_back(std::move(literals)); }
    void add_parity(std::vector<int> literals, bool value) {
        parities.push_back(Parity{std::move(literals), value});
    }

    /// Clauses after expansion, which is what the header line must count.
    std::size_t total_clause_count(bool native_xor) const;

    /// Variables after expansion. Expanding a parity introduces fresh ones, so
    /// this differs from `variable_count` exactly when the parities are being
    /// written out as clauses, and reporting the wrong one describes a file
    /// that was not written.
    std::size_t total_variable_count(bool native_xor) const;
};

/// The same formula with every parity constraint turned into ordinary clauses.
///
/// Tseitin: a chain of fresh variables, each the xor of the running total and
/// the next literal, four clauses apiece, and a unit clause fixing the end.
Cnf with_parities_expanded(const Cnf& formula, std::size_t cutting_number = 3,
                           bool pooled = false);

/// Write DIMACS. With `native_xor` the parities become `x`-prefixed lines, which
/// CryptoMiniSat reads directly; without it they are expanded first.
void write_dimacs(std::ostream& output, const Cnf& formula, bool native_xor);

/// What a solver said: whether it was satisfiable and, if so, the value of every
/// variable, indexed from 1 so that `values[v]` is variable `v`.
struct Model {
    bool answered = false;
    bool satisfiable = false;
    std::vector<bool> values;
};

/// Read a solver's stdout: `s SATISFIABLE` or `s UNSATISFIABLE`, then `v` lines.
///
/// `answered` false means neither verdict was printed, which is what a timeout
/// or a memory kill looks like from here, and it must never be read as
/// unsatisfiable, because that would turn a run that gave up into a proof.
Model read_dimacs_model(std::istream& input);

}  // namespace linear_algebra
