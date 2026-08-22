/// That the greedy over `Q` really returns the minimum, checked against two
/// authorities that share no line with it.
///
/// The claim is a theorem, Rado-Edmonds on the linear matroid, but a theorem is
/// about the algorithm and a test is about the code. So the answer is squeezed
/// from both sides on cases small enough to settle by hand:
///
/// - **From below**, a brute force that walks *every* column subset, including
///   the ones above the `n - r + 1` ceiling the implementation stops at, builds
///   the codewords supported inside each one by its own elimination, and runs
///   the greedy on them. If the ceiling or the pivot bookkeeping were wrong,
///   this walk would find something lighter.
/// - **From above**, every basis drawn from the codewords with coefficients in
///   a small integer box. That is an upper bound on the optimum by construction,
///   so an answer heavier than it would be a defeat by a set anybody can write
///   down.
#include <cstddef>
#include <functional>
#include <string>
#include <vector>

#include "check.h"
#include "combinations.h"
#include "linear_algebra.h"
#include "rational_sparsifier.h"
#include "solver.h"

namespace {

using matrix_sparsification::Element;
using matrix_sparsification::Field;
using matrix_sparsification::Matrix;

Matrix matrix_of(const Field& field, std::size_t rows, std::size_t columns,
                 const std::vector<int64_t>& entries) {
    Matrix built(rows, columns);
    for (std::size_t row = 0; row < rows; ++row) {
        for (std::size_t column = 0; column < columns; ++column) {
            field.init(built(row, column), entries[row * columns + column]);
        }
    }
    return built;
}

/// A basis of the codewords supported inside `support`, by elimination on the
/// rows themselves rather than on an echelon form of them.
std::vector<std::vector<Element>> supported_inside(const Field& field, const Matrix& rows,
                                                   const std::vector<std::size_t>& support) {
    std::vector<std::vector<Element>> outside;
    std::vector<bool> inside(rows.columns(), false);
    for (std::size_t column : support) inside[column] = true;
    for (std::size_t row = 0; row < rows.rows(); ++row) {
        std::vector<Element> entries;
        for (std::size_t column = 0; column < rows.columns(); ++column) {
            if (!inside[column]) entries.push_back(rows(row, column));
        }
        outside.push_back(std::move(entries));
    }

    std::vector<std::vector<Element>> codewords;
    for (const std::vector<Element>& coefficients :
         linear_algebra::vanishing_combinations(field, outside)) {
        std::vector<Element> codeword(rows.columns(), Element());
        for (std::size_t row = 0; row < rows.rows(); ++row) {
            if (field.isZero(coefficients[row])) continue;
            for (std::size_t column = 0; column < rows.columns(); ++column) {
                field.axpyin(codeword[column], coefficients[row], rows(row, column));
            }
        }
        codewords.push_back(std::move(codeword));
    }
    return codewords;
}

std::size_t weight_of(const Field& field, const std::vector<Element>& entries) {
    std::size_t weight = 0;
    for (const Element& entry : entries) {
        if (!field.isZero(entry)) ++weight;
    }
    return weight;
}

/// The minimum, by walking every column subset in ascending size and keeping
/// whatever raises the rank. No ceiling, no echelon shortcut.
std::size_t minimum_by_walking_every_subset(const Field& field, const Matrix& rows) {
    linear_algebra::SpanBasis<Field> held(field, rows.columns());
    const std::size_t dimension = linear_algebra::rank(field, rows);
    std::size_t total = 0;
    for (std::size_t size = 1; size <= rows.columns() && held.dimension() < dimension; ++size) {
        for (const std::vector<std::size_t>& support :
             matrix_sparsification::combinations(rows.columns(), size)) {
            for (const std::vector<Element>& codeword : supported_inside(field, rows, support)) {
                if (held.try_add(codeword)) total += weight_of(field, codeword);
            }
        }
    }
    return total;
}

/// The lightest basis among the codewords whose coefficients lie in
/// `{-1, 0, 1}`, which is an upper bound on the optimum and nothing more.
std::size_t lightest_basis_in_a_small_box(const Field& field, const Matrix& rows) {
    std::vector<std::vector<Element>> box;
    std::vector<int> coefficients(rows.rows(), -1);
    for (;;) {
        std::vector<Element> codeword(rows.columns(), Element());
        for (std::size_t row = 0; row < rows.rows(); ++row) {
            if (coefficients[row] == 0) continue;
            Element scalar;
            field.init(scalar, coefficients[row]);
            for (std::size_t column = 0; column < rows.columns(); ++column) {
                field.axpyin(codeword[column], scalar, rows(row, column));
            }
        }
        if (weight_of(field, codeword) != 0) box.push_back(std::move(codeword));
        std::size_t at = 0;
        while (at < rows.rows() && coefficients[at] == 1) coefficients[at++] = -1;
        if (at == rows.rows()) break;
        ++coefficients[at];
    }

    const std::size_t dimension = linear_algebra::rank(field, rows);
    std::size_t best = static_cast<std::size_t>(-1);
    std::vector<std::size_t> chosen(dimension);
    const std::function<void(std::size_t, std::size_t)> walk = [&](std::size_t from,
                                                                  std::size_t taken) {
        if (taken == dimension) {
            linear_algebra::SpanBasis<Field> span(field, rows.columns());
            std::size_t weight = 0;
            for (std::size_t index = 0; index < dimension; ++index) {
                span.try_add(box[chosen[index]]);
                weight += weight_of(field, box[chosen[index]]);
            }
            if (span.dimension() == dimension && weight < best) best = weight;
            return;
        }
        for (std::size_t index = from; index < box.size(); ++index) {
            chosen[taken] = index;
            walk(index + 1, taken + 1);
        }
    };
    walk(0, 0);
    return best;
}

void check_case(const std::string& name, const Field& field, const Matrix& given) {
    const Matrix answer = matrix_sparsification::sparsest_basis_over_the_rationals(field, given);
    const std::size_t reached = linear_algebra::nonzero_count(field, answer);

    // The row space, not the column space: the answer is a basis of the rows,
    // and a rank-deficient one carries a zero row that changes the columns.
    check::equal(name + ": spans what it was given",
                 linear_algebra::same_row_space(field, given, answer), 1);
    check::equal(name + ": matches the walk over every subset", static_cast<long long>(reached),
                 static_cast<long long>(minimum_by_walking_every_subset(field, given)));
    check::equal(name + ": is no heavier than the best basis in the box",
                 reached <= lightest_basis_in_a_small_box(field, given), 1);
    check::equal(name + ": is no heavier than the input",
                 reached <= linear_algebra::nonzero_count(field, given), 1);
}

}  // namespace

int main() {
    const Field field;

    // A pair of weight-three rows whose difference has weight two. The greedy
    // has to take the difference first, which is the whole algorithm in one case.
    check_case("a pair whose difference is lighter", field,
               matrix_of(field, 2, 4, {1, 1, 1, 0, 1, 1, 0, 1}));

    // Two rows sharing a zero set, so the space of codewords vanishing there has
    // dimension two. This is the shape a search for column subsets of corank
    // exactly one cannot see, and it is why that filter is not what proves the
    // answer minimal.
    check_case("a corank-two zero set", field,
               matrix_of(field, 3, 5, {1, 1, 0, 0, 0, 1, -1, 0, 0, 0, 1, 0, 1, 1, 1}));

    // Entries that are not 0 or +-1, so the answer is a genuinely rational one
    // and not a relabelled GF(2) case.
    check_case("rational entries", field,
               matrix_of(field, 3, 5, {2, 3, 0, 1, 0, 4, 6, 1, 0, 0, 1, 0, 0, 2, 3}));

    // Wider than it is tall by a margin, where the ceiling on the greedy weights
    // is loose and the scan has room to stop early or late.
    check_case("a wide operator", field,
               matrix_of(field, 3, 7, {1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1}));

    // Rank deficient: the third row is the sum of the first two, so a basis has
    // two vectors and the answer carries a zero row.
    const Matrix deficient = matrix_of(field, 3, 4, {1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1});
    const Matrix answer = matrix_sparsification::sparsest_basis_over_the_rationals(field, deficient);
    check::equal("rank deficient: the basis is as small as the rank",
                 static_cast<long long>(linear_algebra::rank(field, answer)), 2);
    check::equal("rank deficient: and spans the same rows",
                 linear_algebra::same_row_space(field, deficient, answer), 1);

    return check::report("rational_sparsifier");
}
