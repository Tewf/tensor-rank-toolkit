/// That a file PLinOpt ships can be read here, and that what is written back
/// is a file PLinOpt would read.
///
/// `[plinopt]` is the reference toolchain for exactly these operators,
/// so SMS is the interchange and every disagreement about it is a disagreement
/// about whether the two sides can exchange work at all. Three real files,
/// committed under `fixtures/plinopt/`, each carrying one thing this reader
/// used to get wrong:
///
///   Winograd_L        integers, and the type letter is `R`, not `M`
///   DPS-smallrat_L    rationals, and its triples are not in column order
///   Karatsuba_L       over GF(2), and the type letter is `M`
///
/// All three open with a `#` comment on the first line, which the previous
/// reader could not get past: it took the header with `input >> rows`, so the
/// hash ended the read before the matrix began.
///
/// The checks below the round trip are the four places where his reader and this
/// one were compared line by line rather than assumed to agree, each one run
/// against `plinopt/bin/sms2pretty` on the same bytes before it was written
/// down: a value that does not end its line, an entry that is not a rational, a
/// file holding more than one matrix, and the field, which the file does not
/// carry at all. `../interchange/where-the-conventions-differ.md` is the
/// table, with the file and line on his side of each row.
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

#include "check.h"
#include "sms_file.h"

namespace {

using linear_algebra::ModularMatrix;
using linear_algebra::RationalMatrix;

std::string written(const linear_algebra::RationalMatrix& matrix) {
    std::ostringstream out;
    formats::write_sms(out, matrix);
    return out.str();
}

std::string written(const linear_algebra::ModularMatrix& matrix) {
    std::ostringstream out;
    formats::write_sms(out, matrix);
    return out.str();
}

std::string first_line(const std::string& text) {
    return text.substr(0, text.find('\n'));
}

/// Givaro::Integer converts to uint64_t, int64_t, float and double alike, so a
/// cast straight to the checker's type is ambiguous. Name the signed one.
long long as_integer(const Givaro::Integer& value) {
    return static_cast<long long>(static_cast<int64_t>(value));
}

/// What the reader says when it refuses `text`, or the empty string if it did
/// not refuse. The sentence is the interface: a message that does not say which
/// entry, or which line, sends the reader back to the bytes with nothing to go on.
std::string refusal(const std::string& text) {
    std::istringstream input(text);
    try {
        formats::read_sms(input);
    } catch (const std::exception& problem) {
        return problem.what();
    }
    return "";
}

std::string refusal_over(int64_t characteristic, const std::string& text) {
    const linear_algebra::ModularField field(characteristic);
    std::istringstream input(text);
    try {
        formats::read_sms(input, field);
    } catch (const std::exception& problem) {
        return problem.what();
    }
    return "";
}

/// A read reported as a number rather than as an exception, `-1` when refused.
///
/// The difference matters when a check is deliberately broken to see whether it
/// bites: a reader that throws where the test expected a matrix ends the process
/// with no output at all, which looks the same as a suite that never ran. These
/// two turn that into a `FAIL` line naming what was expected.
long long nonzeros_in(const std::string& text);

long long first_entry(const std::string& text) {
    std::istringstream input(text);
    try {
        return as_integer(formats::read_sms(input)(0, 0).nume());
    } catch (const std::exception&) {
        return -1;
    }
}

long long nonzero_count(const linear_algebra::RationalMatrix& matrix) {
    const linear_algebra::RationalField field;
    long long counted = 0;
    for (std::size_t row = 0; row < matrix.rows(); ++row) {
        for (std::size_t column = 0; column < matrix.columns(); ++column) {
            if (!field.isZero(matrix(row, column))) ++counted;
        }
    }
    return counted;
}

long long nonzeros_in(const std::string& text) {
    std::istringstream input(text);
    try {
        return nonzero_count(formats::read_sms(input));
    } catch (const std::exception&) {
        return -1;
    }
}

}  // namespace

int main(int argc, char** argv) {
    const std::string fixtures = argc > 1 ? argv[1] : "fixtures";
    const std::string plinopt = fixtures + "/plinopt/";

    // Integers with an `R` header, the case the writer used to get backwards.
    const linear_algebra::RationalMatrix winograd = formats::read_sms_file(plinopt + "2x2x2_7_Winograd_L.sms");
    check::equal("Winograd L rows", static_cast<long long>(winograd.rows()), 7);
    check::equal("Winograd L columns", static_cast<long long>(winograd.columns()), 4);
    check::equal("Winograd L nonzeros", nonzero_count(winograd), 14);

    // Rationals, and triples that arrive as 1 1, 1 3, 1 2, 1 4. hpac.imag.fr
    // asks for lexicographic order and this file does not obey; LinBox reads it
    // anyway, so nothing here may depend on the order.
    const linear_algebra::RationalMatrix small_rational =
        formats::read_sms_file(plinopt + "2x2x2_7_DPS-smallrat-12.2034_L.sms");
    check::equal("DPS smallrat rows", static_cast<long long>(small_rational.rows()), 7);
    check::equal("DPS smallrat columns", static_cast<long long>(small_rational.columns()), 4);
    check::equal("DPS smallrat entry 1,1 numerator",
                 as_integer(small_rational(0, 0).nume()), 4);
    check::equal("DPS smallrat entry 1,1 denominator",
                 as_integer(small_rational(0, 0).deno()), 9);
    check::equal("DPS smallrat out-of-order entry 1,2 numerator",
                 as_integer(small_rational(0, 1).nume()), -8);

    // A finite field, where the letter really is `M`.
    const linear_algebra::RationalMatrix karatsuba =
        formats::read_sms_file(plinopt + "1o1o2_3_Karatsuba_L.sms");
    check::equal("Karatsuba L rows", static_cast<long long>(karatsuba.rows()), 3);
    check::equal("Karatsuba L nonzeros", nonzero_count(karatsuba), 4);

    // What we write is what LinBox writes: cardinality zero is `R`, a finite
    // field is `M`. Emitting `M` for an integer matrix, as this once did, tells
    // a reader the entries live somewhere they do not.
    check::equal("rationals are written R", first_line(written(winograd)) == "7 4 R", 1);
    check::equal("rationals with fractions are written R",
                 first_line(written(small_rational)) == "7 4 R", 1);

    linear_algebra::ModularMatrix over_gf2(3, 2);
    over_gf2(0, 0) = 1;
    over_gf2(1, 0) = 1;
    over_gf2(1, 1) = 1;
    over_gf2(2, 1) = 1;
    check::equal("GF(p) is written M", first_line(written(over_gf2)) == "3 2 M", 1);

    // A round trip changes nothing, comments and ordering aside.
    const linear_algebra::RationalMatrix reread = [&] {
        std::istringstream input(written(small_rational));
        return formats::read_sms(input);
    }();
    check::equal("round trip keeps the shape",
                 static_cast<long long>(reread.rows() * reread.columns()), 28);
    check::equal("round trip keeps every nonzero", nonzero_count(reread),
                 nonzero_count(small_rational));
    check::equal("round trip keeps 4/9", as_integer(reread(0, 0).nume()), 4);

    // Two triples on one line, which the reader used to take as two triples and
    // LinBox takes as one number. `plinopt/bin/sms2pretty` on exactly these
    // bytes prints `[ 5327 0]` and reports one nonzero, with no warning, so a
    // reader that accepted them would hold a different matrix from his out of
    // the same file. Splitting a triple across lines stays legal, because his
    // reader accepts that: only the value is greedy, and only to end of line.
    check::text("two triples on one line are refused", refusal("3 2 R\n1 1 5 3 2 7\n0 0 0\n"),
                "SMS value '5' is not the last word on its line, and LinBox would read it "
                "together with what follows as a single number");
    check::equal("a triple split across lines is read",
                 first_entry("3 2 R\n1 1\n5\n2 2 7\n0 0 0\n"), 5);

    // Entries are not always rationals. `sms2pretty` opens a stream over a
    // polynomial ring, and three of his 153 matrices use it: the `-X` family
    // from the accuracy paper, checked with `-P "X^2-3"`. Refusing them by
    // name beats reporting a digit that was not found.
    check::text("an indeterminate is named rather than reported as a bad digit",
                refusal("3 2 R\n1 1 2X\n0 0 0\n"),
                "SMS value '2X' is a polynomial in an indeterminate, not a rational. Entries "
                "here live in Q or in GF(p); PLinOpt's -P family does not");

    // One file may hold several matrices, and `4o4o4_F32_Montgomery_P.sms` holds
    // four. His checkers read the first and stop, and so does this; only
    // `sms2pretty` loops with `newmatrix()`. What must not happen is reading on
    // past the terminator and mixing two matrices into one.
    check::equal("a file of several matrices yields the first and none of the second",
                 nonzeros_in("2 2 R\n1 1 1\n0 0 0\n\n# the second\n2 2 R\n2 2 9\n0 0 0\n"), 1);

    // The reading half of `write_sms(ostream, linear_algebra::ModularMatrix)`, which had none.
    // SMS carries no field, so the field is a parameter, exactly as PLinOpt
    // takes it on the command line with `-q`.
    {
        const linear_algebra::ModularField gf7(7);
        std::istringstream input("2 2 R\n1 1 4/9\n2 2 -8\n0 0 0\n");
        const linear_algebra::ModularMatrix reduced = formats::read_sms(input, gf7);
        check::equal("4/9 modulo 7 is 2", reduced(0, 0), 2);
        check::equal("-8 modulo 7 is 6", reduced(1, 1), 6);
    }
    // A denominator that vanishes has no residue to stand for. PLinOpt reaches
    // the same verdict on the same file by a different route: `MMchecker
    // data/2x2x2_7_DPS-smallrat-12.2034_{L,R,P}.sms -q 2` reports "not a 2x2x2
    // MM algorithm", which is true but does not say that a denominator was the
    // reason.
    check::text("a vanishing denominator is refused, and says which entry",
                refusal_over(2, "2 2 R\n1 1 1/2\n0 0 0\n"),
                "SMS entry 1 1 is 1/2, whose denominator vanishes modulo 2");

    // The comment-writing path, which is the one that actually leaves here.
    // `--emit-operators` attaches a two-line provenance block, and every line of
    // it has to carry its own `#` or the second becomes a malformed line ahead of
    // the header. PLinOpt's tools skipped the block without a word, and nothing
    // on this side checked that they could.
    {
        linear_algebra::ModularMatrix operand(2, 3);
        operand(0, 1) = 1;
        operand(1, 2) = 1;
        const std::string path = "sms_provenance_round_trip.sms";
        formats::write_sms_file(path, "Recovered from a fixture\nby minimise-rank.",
                                       operand);

        std::ifstream back(path);
        std::string first, second;
        std::getline(back, first);
        std::getline(back, second);
        check::equal("every provenance line is commented",
                     first.starts_with("# ") && second.starts_with("# "), 1);

        const linear_algebra::RationalMatrix commented = formats::read_sms_file(path);
        check::equal("a commented file reads back to the same shape",
                     static_cast<long long>(commented.rows() * commented.columns()), 6);
        check::equal("a commented file keeps its nonzeros", nonzero_count(commented), 2);
        std::remove(path.c_str());
    }

    return check::report("sms interoperability");
}
