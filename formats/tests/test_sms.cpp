/// SMS format is stable on round-trip for the fixture operators.
#include <iostream>
#include <sstream>
#include <string>

#include "check.h"
#include "dense_matrix_file.h"
#include "linear_algebra.h"
#include "sms_file.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: test_sms <fixtures-directory>\n";
        return 2;
    }
    const std::string directory = argv[1];
    const linear_algebra::RationalField field;

    for (const char* name : {"strassen_u", "strassen_v", "alternative_basis_u"}) {
        const linear_algebra::RationalMatrix original =
            linear_algebra::read_rational_matrix_file(directory + "/" + name + ".matrix");

        std::ostringstream written;
        linear_algebra::write_sms(written, original);
        std::istringstream reading(written.str());
        const linear_algebra::RationalMatrix returned = linear_algebra::read_sms(reading);

        check::equal(std::string(name) + " rows", static_cast<long long>(returned.rows()),
                     static_cast<long long>(original.rows()));
        check::equal(std::string(name) + " columns", static_cast<long long>(returned.columns()),
                     static_cast<long long>(original.columns()));
        check::equal(std::string(name) + " nonzeros survive the round trip",
                     static_cast<long long>(linear_algebra::nonzero_count(field, returned)),
                     static_cast<long long>(linear_algebra::nonzero_count(field, original)));

        bool identical = true;
        for (std::size_t entry = 0; entry < original.entry_count(); ++entry) {
            if (!(original.data()[entry] == returned.data()[entry])) identical = false;
        }
        check::equal(std::string(name) + " entries identical", identical ? 1 : 0, 1);
    }

    // Both operators are tagged R. The letter says which ring, not whether the
    // entries happen to be whole: LinBox writes R for cardinality zero, which
    // the integers and the rationals both are, and M only for a finite field.
    // Strassen's operator is integers and PLinOpt ships it as `7 4 R`.
    {
        std::ostringstream integers;
        linear_algebra::write_sms(
            integers, linear_algebra::read_rational_matrix_file(directory + "/strassen_u.matrix"));
        check::equal("integer operator tagged R", integers.str().substr(0, 6) == "7 4 R\n" ? 1 : 0,
                     1);
        std::ostringstream fractions;
        linear_algebra::write_sms(fractions, linear_algebra::read_rational_matrix_file(
                                                 directory + "/alternative_basis_u.matrix"));
        check::equal("rational operator tagged R",
                     fractions.str().substr(0, 6) == "7 4 R\n" ? 1 : 0, 1);
    }

    return check::report("SMS format");
}
