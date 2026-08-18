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
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

#include "check.h"
#include "sms_file.h"

namespace {

using linear_algebra::ModularMatrix;
using linear_algebra::RationalMatrix;

std::string written(const RationalMatrix& matrix) {
    std::ostringstream out;
    linear_algebra::write_sms(out, matrix);
    return out.str();
}

std::string written(const ModularMatrix& matrix) {
    std::ostringstream out;
    linear_algebra::write_sms(out, matrix);
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

long long nonzero_count(const RationalMatrix& matrix) {
    const linear_algebra::RationalField field;
    long long counted = 0;
    for (std::size_t row = 0; row < matrix.rows(); ++row) {
        for (std::size_t column = 0; column < matrix.columns(); ++column) {
            if (!field.isZero(matrix(row, column))) ++counted;
        }
    }
    return counted;
}

}  // namespace

int main(int argc, char** argv) {
    const std::string fixtures = argc > 1 ? argv[1] : "fixtures";
    const std::string plinopt = fixtures + "/plinopt/";

    // Integers with an `R` header, the case the writer used to get backwards.
    const RationalMatrix winograd = linear_algebra::read_sms_file(plinopt + "2x2x2_7_Winograd_L.sms");
    check::equal("Winograd L rows", static_cast<long long>(winograd.rows()), 7);
    check::equal("Winograd L columns", static_cast<long long>(winograd.columns()), 4);
    check::equal("Winograd L nonzeros", nonzero_count(winograd), 14);

    // Rationals, and triples that arrive as 1 1, 1 3, 1 2, 1 4. hpac.imag.fr
    // asks for lexicographic order and this file does not obey; LinBox reads it
    // anyway, so nothing here may depend on the order.
    const RationalMatrix small_rational =
        linear_algebra::read_sms_file(plinopt + "2x2x2_7_DPS-smallrat-12.2034_L.sms");
    check::equal("DPS smallrat rows", static_cast<long long>(small_rational.rows()), 7);
    check::equal("DPS smallrat columns", static_cast<long long>(small_rational.columns()), 4);
    check::equal("DPS smallrat entry 1,1 numerator",
                 as_integer(small_rational(0, 0).nume()), 4);
    check::equal("DPS smallrat entry 1,1 denominator",
                 as_integer(small_rational(0, 0).deno()), 9);
    check::equal("DPS smallrat out-of-order entry 1,2 numerator",
                 as_integer(small_rational(0, 1).nume()), -8);

    // A finite field, where the letter really is `M`.
    const RationalMatrix karatsuba =
        linear_algebra::read_sms_file(plinopt + "1o1o2_3_Karatsuba_L.sms");
    check::equal("Karatsuba L rows", static_cast<long long>(karatsuba.rows()), 3);
    check::equal("Karatsuba L nonzeros", nonzero_count(karatsuba), 4);

    // What we write is what LinBox writes: cardinality zero is `R`, a finite
    // field is `M`. Emitting `M` for an integer matrix, as this once did, tells
    // a reader the entries live somewhere they do not.
    check::equal("rationals are written R", first_line(written(winograd)) == "7 4 R", 1);
    check::equal("rationals with fractions are written R",
                 first_line(written(small_rational)) == "7 4 R", 1);

    ModularMatrix over_gf2(3, 2);
    over_gf2(0, 0) = 1;
    over_gf2(1, 0) = 1;
    over_gf2(1, 1) = 1;
    over_gf2(2, 1) = 1;
    check::equal("GF(p) is written M", first_line(written(over_gf2)) == "3 2 M", 1);

    // A round trip changes nothing, comments and ordering aside.
    const RationalMatrix reread = [&] {
        std::istringstream input(written(small_rational));
        return linear_algebra::read_sms(input);
    }();
    check::equal("round trip keeps the shape",
                 static_cast<long long>(reread.rows() * reread.columns()), 28);
    check::equal("round trip keeps every nonzero", nonzero_count(reread),
                 nonzero_count(small_rational));
    check::equal("round trip keeps 4/9", as_integer(reread(0, 0).nume()), 4);

    // The comment-writing path, which is the one that actually leaves here.
    // `--emit-operators` attaches a two-line provenance block, and every line of
    // it has to carry its own `#` or the second becomes a malformed line ahead of
    // the header. PLinOpt's tools skipped the block without a word, and nothing
    // on this side checked that they could.
    {
        ModularMatrix operand(2, 3);
        operand(0, 1) = 1;
        operand(1, 2) = 1;
        const std::string path = "sms_provenance_round_trip.sms";
        linear_algebra::write_sms_file(path, "Recovered from a fixture\nby minimise-rank.",
                                       operand);

        std::ifstream back(path);
        std::string first, second;
        std::getline(back, first);
        std::getline(back, second);
        check::equal("every provenance line is commented",
                     first.starts_with("# ") && second.starts_with("# "), 1);

        const RationalMatrix commented = linear_algebra::read_sms_file(path);
        check::equal("a commented file reads back to the same shape",
                     static_cast<long long>(commented.rows() * commented.columns()), 6);
        check::equal("a commented file keeps its nonzeros", nonzero_count(commented), 2);
        std::remove(path.c_str());
    }

    return check::report("sms interoperability");
}
