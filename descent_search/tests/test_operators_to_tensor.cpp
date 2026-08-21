/// That ⟨L,R,P⟩ means the same thing on both sides of the exchange.
///
/// `algorithm_recovery.h` says output `i` is `sum_j decode[i][j]` times product
/// `j`, and PLinOpt says the same about its `stem_{L,R,P}.sms` triple. Two
/// descriptions agreeing in prose is not agreement: the two conventions differ
/// by a transpose or a vectorisation order in most of the ways they could
/// differ, and every one of those would still produce a plausible tensor.
///
/// So the check is arithmetic, over his bytes and ours at once. Rebuild the map
/// from operators he published and compare it, entry for entry, with a fixture
/// this repository wrote from the definition of the map. Nothing is shared
/// between the two sides: `2x2x2_7_Strassen_{L,R,P}.sms` came out of his `data/`
/// and `matmul_2x2x2.tensor` out of `make-tensor --matmul 2 2 2 2`.
///
/// It is also the only check anywhere that the row-major vectorisation
/// `[a11 a12 a21 a22]` in `src/MMchecker.cpp` is the one `make-tensor` uses.
#include <string>
#include <vector>

#include "algorithm_recovery.h"
#include "check.h"
#include "map_construction.h"
#include "sms_file.h"
#include "tensor_file.h"

namespace {

using bilinear_rank::Algorithm;
using bilinear_rank::Field;
using bilinear_rank::Matrix;

/// The map the triple `<fixtures>/plinopt/<stem>_{L,R,P}.sms` computes over
/// GF(p), which is what `operators-to-tensor` writes out.
std::vector<Matrix> map_of(const std::string& fixtures, const std::string& stem,
                           const Field& field) {
    const std::string at = fixtures + "/plinopt/" + stem;
    const Algorithm algorithm{linear_algebra::read_sms_file(at + "_L.sms", field),
                              linear_algebra::read_sms_file(at + "_R.sms", field),
                              linear_algebra::read_sms_file(at + "_P.sms", field)};
    return bilinear_rank::map_computed_by(field, algorithm);
}

/// Entries that differ between two maps, and `-1` when even their shapes do.
long long differences(const std::vector<Matrix>& left, const std::vector<Matrix>& right) {
    if (left.size() != right.size()) return -1;
    long long differing = 0;
    for (std::size_t slice = 0; slice < left.size(); ++slice) {
        if (left[slice].rows() != right[slice].rows() ||
            left[slice].columns() != right[slice].columns()) {
            return -1;
        }
        for (std::size_t entry = 0; entry < left[slice].entry_count(); ++entry) {
            if (left[slice].data()[entry] != right[slice].data()[entry]) ++differing;
        }
    }
    return differing;
}

}  // namespace

int main(int argc, char** argv) {
    const std::string fixtures = argc > 1 ? argv[1] : "fixtures";
    const Field gf2(2);

    // Strassen's 7 products, published in his data/, rebuild the <2,2,2> matrix
    // multiplication tensor this repository generates from its definition.
    const std::vector<Matrix> strassen = map_of(fixtures, "2x2x2_7_Strassen", gf2);
    const linear_algebra::Tensor matmul =
        linear_algebra::read_tensor_file(fixtures + "/matmul_2x2x2.tensor");
    check::equal("Strassen's operators rebuild matmul_2x2x2, slice count",
                 static_cast<long long>(strassen.size()),
                 static_cast<long long>(matmul.slices.size()));
    check::equal("Strassen's operators rebuild matmul_2x2x2, entry for entry",
                 differences(strassen, matmul.slices), 0);

    // And Karatsuba's 3, a polynomial multiplication rather than a matrix one,
    // typed `M` rather than `R`, rebuild f2_2x2.
    const std::vector<Matrix> karatsuba = map_of(fixtures, "1o1o2_3_Karatsuba", gf2);
    const linear_algebra::Tensor polynomial =
        linear_algebra::read_tensor_file(fixtures + "/f2_2x2.tensor");
    check::equal("Karatsuba's operators rebuild f2_2x2, entry for entry",
                 differences(karatsuba, polynomial.slices), 0);

    // The two triples are different algorithms for different maps, so a
    // construction that ignored its arguments would pass the two checks above
    // and fail this one.
    check::equal("and the two maps are not the same map",
                 differences(strassen, karatsuba), -1);

    // A shape where the two operands differ, which neither triple above has:
    // ⟨2,2,2⟩ and ⟨1,1,2⟩ both give L and R the same number of columns, so
    // building each slice transposed rebuilds them both correctly and is caught
    // by nothing. `3x4x7_63_rational` is L 63x12 against R 63x28. Compared with
    // the generator rather than with a fixture because a fixture for it would
    // exist only for this line; it is the same construction `make-tensor` runs.
    const Field gf3(3);
    const std::vector<Matrix> asymmetric = map_of(fixtures, "3x4x7_63_rational", gf3);
    check::equal("a <3,4,7> algorithm rebuilds the <3,4,7> map, entry for entry",
                 differences(asymmetric, bilinear_rank::matrix_multiplication_tensor(3, 4, 7)),
                 0);

    return check::report("operators to tensor");
}
