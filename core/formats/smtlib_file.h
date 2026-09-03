#pragma once

#include <cstddef>
#include <istream>
#include <map>
#include <ostream>
#include <string>
#include <vector>

namespace formats {

/// SMT-LIB in the theory of finite fields, which cvc5 decides natively.
///
/// The other file here writes DIMACS, where a field larger than GF(2) has to be
/// built out of Booleans by hand. This one does not build the field at all: the
/// solver already has it, so the encoding is a transcription of the equations
/// rather than a construction. That is the whole reason both exist.
///
/// The concrete syntax, which is small:
///
/// ```
/// (set-logic QF_FF)
/// (set-option :produce-models true)
/// (define-sort F () (_ FiniteField 3))
/// (declare-const a_0_0 F)
/// (assert (= (ff.add (ff.mul a_0_0 b_0_0) ...) (as ff1 F)))
/// (check-sat)
/// (get-model)
/// ```
///
/// Values come back as `#f4m13`, meaning 4 in GF(13).
struct SmtProblem {
    std::size_t characteristic = 0;
    std::vector<std::string> constants;
    std::vector<std::string> assertions;

    /// `(as ff<value> F)`, the way a field literal is written.
    static std::string literal(std::size_t value);
};

void write_smtlib(std::ostream& output, const SmtProblem& problem);

/// What cvc5 said. `answered` false means it printed neither verdict, which is
/// a timeout or a kill and is never unsatisfiable.
struct SmtModel {
    bool answered = false;
    bool satisfiable = false;
    std::map<std::string, std::size_t> values;
};

SmtModel read_smtlib_model(std::istream& input);

}  // namespace formats
