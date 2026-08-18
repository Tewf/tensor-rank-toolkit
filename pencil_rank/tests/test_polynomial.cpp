/// The polynomial arithmetic the pencil is diagonalised with, and the
/// factorisation the elementary divisors are counted by.
///
/// Both are small enough to check against answers written down in advance,
/// which is the only reason this module can be trusted to be exact: the rank
/// formula reads three numbers off a factorisation and would report a wrong
/// rank just as confidently as a right one.
#include <algorithm>
#include <string>
#include <vector>

#include "check.h"
#include "prime_power_factors.h"

namespace {

using pencil_rank::ModularField;
using pencil_rank::Polynomial;
using pencil_rank::PrimePower;

/// The degrees of the prime powers, ascending, so a factorisation can be
/// compared against a list written by hand.
std::vector<long long> degrees_of(const ModularField& field, const Polynomial& polynomial) {
    std::vector<long long> degrees;
    for (const PrimePower& factor : pencil_rank::prime_power_factors(field, polynomial)) {
        degrees.push_back(static_cast<long long>(factor.degree()));
    }
    std::sort(degrees.begin(), degrees.end());
    return degrees;
}

void check_degrees(const ModularField& field, const std::string& what,
                   const Polynomial& polynomial, const std::vector<long long>& expected) {
    const std::vector<long long> actual = degrees_of(field, polynomial);
    check::equal(what + ": how many prime powers",
                 static_cast<long long>(actual.size()),
                 static_cast<long long>(expected.size()));
    for (std::size_t index = 0; index < expected.size() && index < actual.size(); ++index) {
        check::equal(what + ": degree " + std::to_string(index), actual[index], expected[index]);
    }
}

}  // namespace

int main() {
    const ModularField two(2);
    const ModularField three(3);

    // Arithmetic first, because everything below is built out of it and a
    // failure here would surface as a wrong rank three files away.
    {
        const Polynomial left{1, 1};           // 1 + x
        const Polynomial right{1, 0, 1};       // 1 + x^2
        const Polynomial product = pencil_rank::multiply(two, left, right);
        check::equal("(1+x)(1+x^2) has degree 3",
                     static_cast<long long>(pencil_rank::degree(product)), 3);

        // Over GF(2) that product is (1+x)^3, so dividing by 1+x twice must be
        // exact and the third division must leave 1.
        Polynomial quotient;
        Polynomial remainder;
        pencil_rank::divide(two, product, left, quotient, remainder);
        check::equal("and divides by 1+x exactly", pencil_rank::is_zero(remainder) ? 1 : 0, 1);
        check::equal("leaving degree 2",
                     static_cast<long long>(pencil_rank::degree(quotient)), 2);

        const Polynomial common = pencil_rank::greatest_common_divisor(two, product, right);
        check::equal("gcd((1+x)^3, (1+x)^2) has degree 2",
                     static_cast<long long>(pencil_rank::degree(common)), 2);
    }

    // The derivative is zero exactly on the p-th powers, which is the fact the
    // squarefree decomposition's tail is built on rather than a case it works
    // around.
    {
        const Polynomial square{1, 0, 1};  // (1 + x)^2 over GF(2)
        check::equal("d/dx of (1+x)^2 is zero over GF(2)",
                     pencil_rank::is_zero(pencil_rank::derivative(two, square)) ? 1 : 0, 1);
        const Polynomial root = pencil_rank::characteristic_root(two, square);
        check::equal("and its square root is 1+x",
                     static_cast<long long>(pencil_rank::degree(root)), 1);
    }

    // Factorisation. Each of these is a shape the rank formula reads
    // differently, so each is a rank that would come out wrong.
    check_degrees(two, "x, one linear factor", Polynomial{0, 1}, {1});
    check_degrees(two, "x^2, one square", Polynomial{0, 0, 1}, {2});
    check_degrees(two, "x^2 + x + 1, irreducible of degree 2", Polynomial{1, 1, 1}, {2});
    check_degrees(two, "x^2 + x = x(x+1), two linear factors", Polynomial{0, 1, 1}, {1, 1});
    check_degrees(two, "x^4 + x^3 + 1, irreducible of degree 4", Polynomial{1, 0, 0, 1, 1}, {4});

    // (x^2 + x + 1)^2 = x^4 + x^2 + 1 over GF(2). One prime power of degree 4,
    // not two of degree 2: an implementation that lost the exponent would agree
    // on the total degree and disagree on the count, and the count is what the
    // rank formula uses.
    check_degrees(two, "(x^2+x+1)^2, one prime power", Polynomial{1, 0, 1, 0, 1}, {4});

    // x^3 - x = x(x-1)(x+1) over GF(3): three distinct linear factors, and the
    // case where the derivative is -1 rather than zero.
    check_degrees(three, "x^3 - x over GF(3)", Polynomial{0, 2, 0, 1}, {1, 1, 1});

    // x^2 + 1 is irreducible over GF(3) and splits over GF(5). The same
    // polynomial, two answers, which is the whole reason rank depends on the
    // field.
    check_degrees(three, "x^2 + 1 over GF(3)", Polynomial{1, 0, 1}, {2});
    const ModularField five(5);
    check_degrees(five, "x^2 + 1 over GF(5)", Polynomial{1, 0, 1}, {1, 1});

    return check::report("polynomial arithmetic and factorisation");
}
