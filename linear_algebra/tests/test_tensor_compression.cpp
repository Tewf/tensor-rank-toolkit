/// The round trip, which is the property that makes compression usable: compress
/// a tensor to its concise core, decompose the core, expand the decomposition,
/// and what comes back must decompose the *original* tensor, exactly, with the
/// same number of terms.
///
/// **Every tensor fixture in this repository except one is already concise**, so
/// over them compression is the identity and a green test over them alone would
/// prove nothing while reading as coverage. `nonconcise_matmul_2x2x2` is the
/// exception and exists for this test: `matmul_2x2x2` with one dependent row, two
/// dependent columns and three dependent slices appended, so its core is that
/// fixture back byte for byte and the known rank 7 is the core's rank. The random
/// section below is non-concise by construction as well, over three fields and
/// with the kept positions scattered rather than leading.
#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include "check.h"
#include "tensor_compression.h"
#include "tensor_contraction.h"
#include "tensor_file.h"

namespace {

using linear_algebra::ModularField;
using linear_algebra::ModularMatrix;
using Decomposition = linear_algebra::RankDecomposition<ModularField>;

/// Every `.tensor` fixture, named rather than globbed, so a fixture that stops
/// parsing fails the test instead of silently leaving it checking less.
constexpr const char* kFixtures[] = {
    "cyclic_f2_5",          "cyclic_f2_7",             "f2_2x2",
    "f2_2x3",               "f2_3x8",                  "f2_4x7",
    "f2_5x5",               "f3_3x6",                  "gf16_multiplication",
    "gf32_multiplication",  "gf4_multiplication",      "gf64_multiplication",
    "gf8_multiplication",   "matmul_2x2x2",            "matmul_2x2x3",
    "matmul_2x3x4",         "matmul_3x3x3",            "matmul_3x3x4",
    "matmul_3x4x4",         "nonconcise_matmul_2x2x2", "pencil_irreducible_f2_4",
    "pencil_nilpotent_f2_3", "pencil_singular_f2_2x3", "pencil_split_f3_3",
    "w_state",
};

bool same_tensor(const std::vector<ModularMatrix>& left,
                 const std::vector<ModularMatrix>& right) {
    if (left.size() != right.size()) return false;
    for (std::size_t slice = 0; slice < left.size(); ++slice) {
        if (left[slice].rows() != right[slice].rows() ||
            left[slice].columns() != right[slice].columns()) {
            return false;
        }
        for (std::size_t entry = 0; entry < left[slice].entry_count(); ++entry) {
            if (left[slice].data()[entry] != right[slice].data()[entry]) return false;
        }
    }
    return true;
}

/// A decomposition every tensor has without a search: one term per nonzero row of
/// a slice, that row against the two standard basis vectors picking the row and
/// the slice it came from.
///
/// Not minimal and not meant to be. What the expansion has to carry is *any*
/// decomposition, and this is the one that exists for every fixture here without
/// running something exponential over it. Its factors are as unstructured as the
/// fixture is once expanded, so an expansion that mixed two axes up, or applied
/// one transposed, does not survive them.
Decomposition rows_decomposition(const ModularField& field,
                                 const std::vector<ModularMatrix>& slices) {
    const std::size_t rows = slices.empty() ? 0 : slices.front().rows();
    const std::size_t columns = slices.empty() ? 0 : slices.front().columns();

    std::vector<std::pair<std::size_t, std::size_t>> carrying;  // (slice, row)
    for (std::size_t slice = 0; slice < slices.size(); ++slice) {
        for (std::size_t row = 0; row < rows; ++row) {
            bool nonzero = false;
            for (std::size_t column = 0; column < columns; ++column) {
                if (!field.isZero(slices[slice](row, column))) nonzero = true;
            }
            if (nonzero) carrying.emplace_back(slice, row);
        }
    }

    Decomposition decomposition;
    decomposition.factor_by_axis = {ModularMatrix(carrying.size(), rows),
                                    ModularMatrix(carrying.size(), columns),
                                    ModularMatrix(carrying.size(), slices.size())};
    for (std::size_t term = 0; term < carrying.size(); ++term) {
        const std::size_t slice = carrying[term].first;
        const std::size_t row = carrying[term].second;
        field.assign(decomposition.factor_by_axis[0](term, row), field.one);
        field.assign(decomposition.factor_by_axis[2](term, slice), field.one);
        for (std::size_t column = 0; column < columns; ++column) {
            decomposition.factor_by_axis[1](term, column) = slices[slice](row, column);
        }
    }
    return decomposition;
}

/// Compress, decompose the core, expand, compare. Returns whether the tensor was
/// concise, so the caller can count how many fixtures the identity was all this
/// exercised on.
bool check_round_trip(const ModularField& field, const std::vector<ModularMatrix>& slices,
                      const std::string& label) {
    const std::vector<std::size_t> ranks = linear_algebra::flattening_ranks(field, slices);
    const linear_algebra::ConciseCompression<ModularField> compression =
        linear_algebra::compress_to_concise(field, slices);

    // The core has the shape the flattening ranks predict, and is concise, which
    // is the whole claim about the compressed tensor.
    check::equal(label + " core slices", static_cast<long long>(compression.slices.size()),
                 static_cast<long long>(ranks[2]));
    check::equal(label + " core rows",
                 static_cast<long long>(compression.slices.empty()
                                            ? 0
                                            : compression.slices.front().rows()),
                 static_cast<long long>(ranks[0]));
    check::equal(label + " core columns",
                 static_cast<long long>(compression.slices.empty()
                                            ? 0
                                            : compression.slices.front().columns()),
                 static_cast<long long>(ranks[1]));
    check::equal(label + " core is concise",
                 linear_algebra::is_concise(field, compression.slices) ? 1 : 0, 1);
    check::equal(label + " core keeps the flattening ranks",
                 linear_algebra::flattening_ranks(field, compression.slices) == ranks ? 1 : 0, 1);

    for (std::size_t axis = 0; axis < 3; ++axis) {
        check::equal(label + " expansion " + std::to_string(axis) + " maps the core's axis",
                     static_cast<long long>(compression.expansion_by_axis[axis].columns()),
                     static_cast<long long>(ranks[axis]));
        check::equal(label + " expansion " + std::to_string(axis) + " reaches the original's axis",
                     static_cast<long long>(compression.expansion_by_axis[axis].rows()),
                     static_cast<long long>(linear_algebra::axis_dimension<ModularField>(slices, axis)));
    }

    // The round trip. The core decomposition is checked against the core first,
    // so a failure downstream is the expansion's and not the test helper's.
    const Decomposition core = rows_decomposition(field, compression.slices);
    check::equal(label + " the core decomposition rebuilds the core",
                 same_tensor(linear_algebra::tensor_from_decomposition(field, core),
                             compression.slices)
                     ? 1
                     : 0,
                 1);

    const Decomposition expanded = linear_algebra::expand_decomposition(field, compression, core);
    check::equal(label + " expansion keeps the term count",
                 static_cast<long long>(expanded.term_count()),
                 static_cast<long long>(core.term_count()));
    check::equal(label + " expansion decomposes the original",
                 same_tensor(linear_algebra::tensor_from_decomposition(field, expanded), slices)
                     ? 1
                     : 0,
                 1);

    const bool concise = linear_algebra::is_concise(field, slices);
    if (concise) {
        // Nothing to compress, so compression must be the identity rather than a
        // reshuffle that happens to have the right rank.
        check::equal(label + " concise: the core is the tensor itself",
                     same_tensor(compression.slices, slices) ? 1 : 0, 1);
    }
    return concise;
}

/// Strassen's seven products as a decomposition of `matmul_2x2x2` over GF(2): the
/// rows of `fixtures/strassen_u.matrix`, `strassen_v.matrix` and
/// `strassen_w.matrix` read modulo 2, where their -1 entries are 1.
///
/// The digits are here rather than converted at runtime because those are
/// `.matrix` files of rationals and this is a GF(2) test. The first thing done
/// with them is to check they rebuild the fixture, so a typo fails on its own line
/// instead of quietly weakening the check that follows.
constexpr std::int64_t kStrassenRowFactor[7][4] = {{1, 0, 0, 1}, {0, 0, 1, 1}, {1, 0, 0, 0},
                                                   {0, 0, 0, 1}, {1, 1, 0, 0}, {1, 0, 1, 0},
                                                   {0, 1, 0, 1}};
constexpr std::int64_t kStrassenColumnFactor[7][4] = {{1, 0, 0, 1}, {1, 0, 0, 0}, {0, 1, 0, 1},
                                                      {1, 0, 1, 0}, {0, 0, 0, 1}, {1, 1, 0, 0},
                                                      {0, 0, 1, 1}};
constexpr std::int64_t kStrassenSliceFactor[7][4] = {{1, 0, 0, 1}, {0, 0, 1, 1}, {0, 1, 0, 1},
                                                     {1, 0, 1, 0}, {1, 1, 0, 0}, {0, 0, 0, 1},
                                                     {1, 0, 0, 0}};

ModularMatrix factor_from(const std::int64_t entries[7][4]) {
    ModularMatrix factor(7, 4);
    for (std::size_t term = 0; term < 7; ++term) {
        for (std::size_t position = 0; position < 4; ++position) {
            factor(term, position) = entries[term][position];
        }
    }
    return factor;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: test_tensor_compression <fixtures-directory>\n";
        return 2;
    }
    const std::string fixtures = argv[1];

    std::size_t not_concise = 0;
    for (const char* name : kFixtures) {
        const formats::Tensor tensor =
            formats::read_tensor_file(fixtures + "/" + std::string(name) + ".tensor");
        const ModularField field(tensor.characteristic);
        if (!check_round_trip(field, tensor.slices, name)) ++not_concise;
    }

    // Pinned, because it is the reason the fixture below had to be built: if a
    // second non-concise fixture is ever added this line says so, and until then
    // it says that the twenty-four others exercise the identity only.
    check::equal("fixtures that are not concise", static_cast<long long>(not_concise), 1);

    // The one that is not concise, where compression does something and the rank
    // of the core is known rather than inferred.
    {
        const ModularField field(2);
        const formats::Tensor padded =
            formats::read_tensor_file(fixtures + "/nonconcise_matmul_2x2x2.tensor");
        const formats::Tensor product =
            formats::read_tensor_file(fixtures + "/matmul_2x2x2.tensor");

        check::equal("the padded tensor is not concise",
                     linear_algebra::is_concise(field, padded.slices) ? 1 : 0, 0);
        const linear_algebra::ConciseCompression<ModularField> compression =
            linear_algebra::compress_to_concise(field, padded.slices);

        // 7 x 5 x 6 = 210 entries down to 4 x 4 x 4 = 64: compression shrinks the
        // tensor rather than merely returning something of full rank.
        check::equal("the core is smaller than the tensor",
                     static_cast<long long>(compression.slices.size() *
                                            compression.slices.front().entry_count()),
                     64);

        // The rank is unchanged, and known: the core is the fixture the padded one
        // was built from, whose rank is 7. `methods/bilinear_rank/descent_search/known_ranks.md` is where
        // the ranks this repository stands behind are collected; 7 for <2,2,2> is
        // the one nobody disputes.
        check::equal("the core is matmul_2x2x2 itself",
                     same_tensor(compression.slices, product.slices) ? 1 : 0, 1);

        // And a real rank decomposition rather than the row-by-row one: Strassen's
        // seven products, expanded onto a 7 x 5 x 6 tensor. This is
        // `rank(padded) <= 7 = rank(core)` exhibited, which is the half of "the
        // rank does not change" that compression has to provide.
        Decomposition strassen;
        strassen.factor_by_axis = {factor_from(kStrassenRowFactor),
                                   factor_from(kStrassenColumnFactor),
                                   factor_from(kStrassenSliceFactor)};
        check::equal("Strassen decomposes matmul_2x2x2 over GF(2)",
                     same_tensor(linear_algebra::tensor_from_decomposition(field, strassen),
                                 product.slices)
                         ? 1
                         : 0,
                     1);

        const Decomposition expanded =
            linear_algebra::expand_decomposition(field, compression, strassen);
        check::equal("Strassen expanded still has seven terms",
                     static_cast<long long>(expanded.term_count()), 7);
        check::equal("Strassen expanded spans the padded rows",
                     static_cast<long long>(expanded.factor_by_axis[0].columns()), 5);
        check::equal("Strassen expanded spans the padded columns",
                     static_cast<long long>(expanded.factor_by_axis[1].columns()), 6);
        check::equal("Strassen expanded spans the padded slices",
                     static_cast<long long>(expanded.factor_by_axis[2].columns()), 7);
        check::equal("Strassen expanded decomposes the padded tensor",
                     same_tensor(linear_algebra::tensor_from_decomposition(field, expanded),
                                 padded.slices)
                         ? 1
                         : 0,
                     1);
    }

    // The two degenerate shapes, because compression is a shape calculation and it
    // is where a shape calculation goes wrong. A zero tensor is deficient on every
    // axis at once, so its core has no slices and its expansions have no columns,
    // and the expansion of a no-term decomposition still has to rebuild the zeros
    // of the original shape.
    {
        const ModularField field(2);
        const std::vector<ModularMatrix> zero(2, ModularMatrix(2, 3));
        check_round_trip(field, zero, "the zero tensor");
        const linear_algebra::ConciseCompression<ModularField> empty =
            linear_algebra::compress_to_concise(field, std::vector<ModularMatrix>{});
        check::equal("the empty tensor compresses to no slices",
                     static_cast<long long>(empty.slices.size()), 0);
        check::equal("the empty tensor still has three axes",
                     static_cast<long long>(empty.expansion_by_axis.size()), 3);
    }

    // Random tensors of few terms, which are non-concise by construction: a tensor
    // of `terms` rank-one terms has every flattening rank at most `terms`, so
    // asking for fewer terms than axis positions is asking for a rank-deficient
    // flattening. Unlike the fixture above, nothing makes the kept positions the
    // leading ones, which is the case a compression built on selecting positions
    // has to get right and the fixture cannot test.
    std::mt19937 generator(11400714);
    std::size_t random_not_concise = 0;
    for (const std::int64_t characteristic : {2, 3, 5}) {
        const ModularField field(characteristic);
        std::uniform_int_distribution<std::int64_t> entries(0, characteristic - 1);
        for (int trial = 0; trial < 40; ++trial) {
            const std::size_t terms = 1 + generator() % 3;
            Decomposition source;
            for (std::size_t axis = 0; axis < 3; ++axis) {
                ModularMatrix factor(terms, 2 + generator() % 3);
                for (std::size_t entry = 0; entry < factor.entry_count(); ++entry) {
                    factor.data()[entry] = entries(generator);
                }
                source.factor_by_axis.push_back(factor);
            }
            const std::vector<ModularMatrix> slices =
                linear_algebra::tensor_from_decomposition(field, source);
            const std::string label = "random GF(" + std::to_string(characteristic) + ") trial " +
                                      std::to_string(trial);
            if (!check_round_trip(field, slices, label)) ++random_not_concise;

            // The core cannot need more than the terms it was built from, so its
            // flattening ranks bound the rank from below by no more than `terms`.
            const linear_algebra::ConciseCompression<ModularField> compression =
                linear_algebra::compress_to_concise(field, slices);
            check::equal(label + " core is within the term count",
                         linear_algebra::flattening_lower_bound(field, compression.slices) <= terms
                             ? 1
                             : 0,
                         1);
        }
    }
    // Vacuous otherwise: if the random tensors all came out concise this section
    // would be testing the identity map 120 times. 118 of the 120 are not, and the
    // check is a floor rather than that number because `uniform_int_distribution`
    // may map a generator's output differently in another standard library, which
    // would change the tensors without weakening anything.
    std::cout << "  " << random_not_concise << " of 120 random tensors were not concise\n";
    check::equal("most random tensors are not concise", random_not_concise >= 100 ? 1 : 0, 1);

    return check::report("tensor compression");
}
