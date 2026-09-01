/// The two ways a pool element becomes bits agree, on every element.
///
/// Where the bit table fits, `Gf2Leaf` packs the whole pool once and reads rows
/// out of it. Where it does not, the element is formed on demand, and this is
/// the route `<4,4,4>` takes because its table would be 137 GiB. That route used
/// to build a `Matrix` through Givaro and pack it back one field element at a
/// time; it now writes the outer product straight into words, one shift and one
/// or per set bit of the left vector.
///
/// **A silent disagreement here is a wrong answer nothing downstream catches**,
/// because both routes feed the same membership test and a corrupted element
/// simply fails to be in the span. So the two are run against each other on the
/// same question, on every fixture small enough to hold both, and the maps they
/// return are compared as maps rather than counted.
///
/// The table is forced off by shrinking the memory budget, which is the same
/// switch the real run makes when the shape is too big, so this exercises the
/// path rather than a copy of it.
#include <string>
#include <vector>

#include "candidate_pool.h"
#include "check.h"
#include "gf2_leaf.h"
#include "memory_budget.h"
#include "span_basis.h"
#include "tensor_file.h"

namespace {

using bilinear_rank::Field;
using bilinear_rank::Matrix;

bool same(const std::vector<Matrix>& left, const std::vector<Matrix>& right) {
    if (left.size() != right.size()) return false;
    for (std::size_t index = 0; index < left.size(); ++index) {
        if (left[index].rows() != right[index].rows()) return false;
        if (left[index].columns() != right[index].columns()) return false;
        for (std::size_t entry = 0; entry < left[index].entry_count(); ++entry) {
            if (left[index].data()[entry] != right[index].data()[entry]) return false;
        }
    }
    return true;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc <= 1) {
        std::cout << "usage: test_packed_generation <fixtures>\n";
        return 1;
    }
    const std::string directory = argv[1];

    for (const char* name : {"f2_2x2", "f2_2x3", "f2_5x5", "gf16_multiplication",
                             "matmul_2x2x2", "matmul_2x2x3", "cyclic_f2_5"}) {
        const auto tensor = formats::read_tensor_file(directory + "/" + name + ".tensor");
        const Field field(tensor.characteristic);
        if (tensor.characteristic != 2) continue;
        const std::size_t rows = tensor.slices.front().rows();
        const std::size_t columns = tensor.slices.front().columns();
        const bilinear_rank::RankOnePool addressed(field, rows, columns);
        const bilinear_rank::Addressed pool{addressed};

        linear_algebra::SpanBasis<Field> span(field, rows * columns);
        for (const Matrix& slice : tensor.slices) span.try_add(slice);
        const std::size_t wanted = span.dimension();

        // With room, the whole pool is packed once and read from the table.
        run_limits::set_memory_budget(std::size_t(2) << 30);
        const bilinear_rank::Gf2Leaf<bilinear_rank::Addressed> tabled(field, pool, rows, columns);
        const std::vector<Matrix> from_table = tabled.by_scanning_the_pool(span, wanted);

        // With none, every element is formed on demand, which is the route the
        // shapes that matter take.
        run_limits::set_memory_budget(1);
        const bilinear_rank::Gf2Leaf<bilinear_rank::Addressed> onthefly(field, pool, rows, columns);
        const std::vector<Matrix> from_masks = onthefly.by_scanning_the_pool(span, wanted);
        run_limits::set_memory_budget(std::size_t(2) << 30);

        check::equal(std::string(name) + ": both routes agree on span(T)",
                     static_cast<long long>(same(from_table, from_masks)), 1);

        // span(T) is the wrong place to exercise this on a product shape: a
        // slice of <n,m,k> has rank m, and nothing in the span of those is rank
        // one at all for m > 1, so the scan finds nothing and compares nothing.
        // A span built out of pool elements has a rank-one basis by
        // construction, so both routes have to form and return every one of
        // them, and the maps are compared as maps.
        linear_algebra::SpanBasis<Field> from_pool(field, rows * columns);
        std::vector<Matrix> chosen;
        for (std::size_t index = 0; index < pool.size() && chosen.size() < 4; ++index) {
            const Matrix map = pool[index];
            if (from_pool.try_add(map)) chosen.push_back(map);
        }
        const std::vector<Matrix> table_on_pool =
            tabled.by_scanning_the_pool(from_pool, chosen.size());
        const std::vector<Matrix> masks_on_pool =
            onthefly.by_scanning_the_pool(from_pool, chosen.size());
        check::equal(std::string(name) + ": found a rank-one basis of a rank-one span",
                     static_cast<long long>(table_on_pool.size()),
                     static_cast<long long>(chosen.size()));
        check::equal(std::string(name) + ": both routes agree on it",
                     static_cast<long long>(same(table_on_pool, masks_on_pool)), 1);
    }

    return check::report("packed generation");
}
