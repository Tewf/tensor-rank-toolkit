/// `is_rank_one` and `rank(...) == 1`, held against each other on every matrix
/// small enough to enumerate.
///
/// The general leaf in [`../subspace_walk.h`](../subspace_walk.h) puts every
/// element of every subspace it walks through
/// [`is_rank_one`](../../../../core/linear_algebra/measures.h) and through nothing else, so
/// that predicate *is* the leaf's verdict and a wrong answer from it is a wrong
/// rank. Neither direction of error announces itself: a false no drops a
/// rank-one map and refutes a decomposition that exists, and a false yes offers
/// the search a product it cannot build with. Both come back as a number,
/// published, with nothing downstream to catch them.
///
/// So the predicate is not argued from its derivation here, it is checked
/// against the elimination it replaced, over **all** matrices of the small
/// shapes (which is affordable and therefore the right test to run) and over a
/// sample where it is not. The named cases below are the ones where an
/// implementation goes wrong quietly: a zero row, a first row that is zero, and
/// a rank-two matrix that agrees with a rank-one one for exactly as long as a
/// careless test looks.
#include <cstddef>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

#include "check.h"
#include "measures.h"

namespace {

using linear_algebra::ModularField;
using linear_algebra::ModularMatrix;

std::string shape_label(int64_t characteristic, std::size_t rows, std::size_t columns) {
    return "GF(" + std::to_string(characteristic) + ") " + std::to_string(rows) + "x" +
           std::to_string(columns);
}

/// Both answers for one matrix, and whether they are the same answer.
bool agrees(const ModularField& field, const ModularMatrix& matrix) {
    const bool predicate =
        linear_algebra::is_rank_one(field, matrix.data(), matrix.rows(), matrix.columns());
    const bool by_rank = linear_algebra::rank(field, matrix) == 1;
    return predicate == by_rank;
}

/// Write `index` into the matrix as a base-`characteristic` string, which walks
/// every matrix of the shape exactly once as `index` runs over its range.
void fill_from_index(const ModularField& field, ModularMatrix& matrix, std::size_t index,
                     int64_t characteristic) {
    const auto radix = static_cast<std::size_t>(characteristic);
    std::size_t remaining = index;
    for (std::size_t entry = 0; entry < matrix.entry_count(); ++entry) {
        field.assign(matrix.data()[entry], static_cast<int64_t>(remaining % radix));
        remaining /= radix;
    }
}

/// Every matrix of one shape over one field. This is the test that matters, and
/// it is exhaustive because at these shapes it can be.
void check_every_matrix(int64_t characteristic, std::size_t rows, std::size_t columns) {
    const ModularField field(characteristic);
    const std::string label = shape_label(characteristic, rows, columns);

    std::size_t total = 1;
    for (std::size_t entry = 0; entry < rows * columns; ++entry) {
        total *= static_cast<std::size_t>(characteristic);
    }

    ModularMatrix matrix(rows, columns);
    long long disagreements = 0;
    long long rank_one_count = 0;
    for (std::size_t index = 0; index < total; ++index) {
        fill_from_index(field, matrix, index, characteristic);
        if (!agrees(field, matrix)) ++disagreements;
        if (linear_algebra::rank(field, matrix) == 1) ++rank_one_count;
    }

    check::equal(label + ": every matrix gets the same verdict from both", disagreements, 0);
    // A sweep where nothing is rank one would pass while testing nothing, so the
    // number found is asserted to be a number and not zero.
    check::equal(label + ": and the sweep found rank-one matrices to disagree about",
                 rank_one_count > 0 ? 1 : 0, 1);
}

/// A shape too large to enumerate, sampled instead.
void check_sampled_matrices(int64_t characteristic, std::size_t rows, std::size_t columns,
                            std::size_t samples) {
    const ModularField field(characteristic);
    const std::string label = shape_label(characteristic, rows, columns) + " sampled";

    std::mt19937 generator(20250820);
    std::uniform_int_distribution<int64_t> entries(0, characteristic - 1);

    ModularMatrix matrix(rows, columns);
    long long disagreements = 0;
    long long rank_one_count = 0;
    for (std::size_t sample = 0; sample < samples; ++sample) {
        for (std::size_t entry = 0; entry < matrix.entry_count(); ++entry) {
            field.assign(matrix.data()[entry], entries(generator));
        }
        // A uniform matrix is almost never rank one at 3x3, so half the samples
        // are built rank one on purpose: an outer product of two random vectors,
        // which is rank one exactly when neither vector is zero. Without this the
        // sweep would only ever exercise the no branch.
        if (sample % 2 == 1) {
            std::vector<int64_t> left(rows);
            std::vector<int64_t> right(columns);
            for (int64_t& entry : left) entry = entries(generator);
            for (int64_t& entry : right) entry = entries(generator);
            for (std::size_t row = 0; row < rows; ++row) {
                for (std::size_t column = 0; column < columns; ++column) {
                    field.mul(matrix(row, column), left[row], right[column]);
                }
            }
        }
        if (!agrees(field, matrix)) ++disagreements;
        if (linear_algebra::rank(field, matrix) == 1) ++rank_one_count;
    }

    check::equal(label + ": every sample gets the same verdict from both", disagreements, 0);
    check::equal(label + ": and the samples included rank-one matrices",
                 rank_one_count > 0 ? 1 : 0, 1);
}

ModularMatrix matrix_of(const ModularField& field,
                        const std::vector<std::vector<int64_t>>& entries) {
    const std::size_t rows = entries.size();
    const std::size_t columns = rows == 0 ? 0 : entries.front().size();
    ModularMatrix matrix(rows, columns);
    for (std::size_t row = 0; row < rows; ++row) {
        for (std::size_t column = 0; column < columns; ++column) {
            field.assign(matrix(row, column), entries[row][column]);
        }
    }
    return matrix;
}

/// One named matrix, with the verdict written out rather than computed, and
/// `rank` asked as well so the case pins both.
void check_named(const ModularField& field, const std::string& what,
                 const std::vector<std::vector<int64_t>>& entries, int expected) {
    const ModularMatrix matrix = matrix_of(field, entries);
    check::equal(
        what,
        linear_algebra::is_rank_one(field, matrix.data(), matrix.rows(), matrix.columns()) ? 1 : 0,
        expected);
    check::equal(what + ", as rank says too",
                 linear_algebra::rank(field, matrix) == 1 ? 1 : 0, expected);
}

}  // namespace

int main() {
    for (const int64_t characteristic : {int64_t(2), int64_t(3), int64_t(5)}) {
        check_every_matrix(characteristic, 2, 2);
        check_every_matrix(characteristic, 2, 3);
        // The transpose shape as well: a zero row can sit in the middle only
        // when there are more rows than the pivot search reaches at once, and
        // 3x2 is the smallest shape where that happens.
        check_every_matrix(characteristic, 3, 2);
    }
    check_sampled_matrices(5, 3, 3, 20000);

    const ModularField field(5);

    // Rank zero is not rank one, which is the case a test of "all rows are
    // multiples of the first" passes by accident if it forgets to require one.
    check_named(field, "the zero matrix is not rank one", {{0, 0, 0}, {0, 0, 0}}, 0);
    check_named(field, "nor is the 1x1 zero", {{0}}, 0);
    check_named(field, "a single nonzero entry is", {{3}}, 1);

    // A zero first row: the pivot is not at row zero, and the row it is at is
    // the one every later row must be a multiple of.
    check_named(field, "a first row of zeros over a rank-one matrix", {{0, 0}, {1, 2}}, 1);
    check_named(field, "and over a rank-two one", {{0, 0}, {1, 2}, {2, 3}}, 0);

    // A zero row in the middle, which the cross-multiplication has to pass with
    // no case of its own: `a[p]*0 == 0*a[j]` on both sides.
    check_named(field, "a zero row between two multiples", {{1, 2}, {0, 0}, {2, 4}}, 1);
    check_named(field, "and a zero row last", {{1, 2}, {2, 4}, {0, 0}}, 1);

    // Agrees with a rank-one matrix on its first row and nowhere else. A test
    // that only looked at row zero, or that took the first column as the pivot,
    // would call this one rank one.
    check_named(field, "a rank-two matrix sharing a rank-one first row", {{1, 2}, {1, 3}}, 0);
    check_named(field, "a rank-two matrix whose first column is zero", {{0, 1, 2}, {0, 1, 3}}, 0);
    check_named(field, "a rank-one matrix whose first column is zero", {{0, 1, 2}, {0, 2, 4}}, 1);
    check_named(field, "a rank-two matrix whose pivot column agrees", {{1, 1}, {1, 2}}, 0);

    // The pivot is not column zero and a later row is zero exactly where the
    // first one is, which is the shape that made the scalar look right.
    check_named(field, "a multiple that is zero at the pivot", {{0, 1, 1}, {0, 0, 1}}, 0);

    return check::report("rank one predicate");
}
