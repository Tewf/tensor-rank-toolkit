/// That a survivor buffer too small to hold the answer never produces a wrong
/// one.
///
/// `GpuSurvivors::overflowed` means the threads past the capacity never wrote,
/// and which of them those were is a race between blocks, so the indices are not
/// a short answer. They are not an answer. Two behaviours follow and both are
/// asserted here against the host's own survivor list, element for element:
///
/// **A range the chunking can rescue comes back whole.** Halving the bite halves
/// the survivors expected of it, and the bites are ascending and disjoint, so
/// the list is still in index order, which is what keeps the host's greedy
/// giving the same rank-one basis.
///
/// **A range it cannot rescue is declined.** Past six halvings the leaf is handed
/// back to the host, which is always correct; what may never happen is a
/// truncated list returned as though it were complete.
///
/// The questions are the dense ones from `question_packing.h`, which exist for
/// exactly this: a span containing every map whose only nonzero row is the
/// first has `2^columns - 1` survivors instead of a handful, so the survivor
/// path is exercised rather than two empty lists being compared.
#include <cstdint>
#include <string>
#include <vector>

#include "bilinear_rank_aliases.h"
#include "candidate_pool.h"
#include "check.h"
#include "host_reference.h"
#include "leaf_backend.h"
#include "question_packing.h"

namespace {

using bilinear_rank::Field;
using bilinear_rank::RankOnePool;

/// The leaf in the shape the seam passes, over arrays the caller keeps alive.
/// `PackedLeaf` carries a vector as a `uint64_t` where `LeafQuestion` carries it
/// as a `uint32_t`, so the two mask tables are widened here rather than shared.
struct Borrowed {
    std::vector<std::uint64_t> lefts;
    std::vector<std::uint64_t> rights;

    bilinear_rank::PackedLeaf of(const gpu_leaf::LeafQuestion& question) {
        lefts.assign(question.left_masks.begin(), question.left_masks.end());
        rights.assign(question.right_masks.begin(), question.right_masks.end());
        bilinear_rank::PackedLeaf leaf;
        leaf.rows = question.rows;
        leaf.columns = question.columns;
        leaf.words = question.words;
        leaf.left_count = question.left_count;
        leaf.right_count = question.right_count;
        leaf.dimension = question.dimension();
        leaf.left_masks = lefts.data();
        leaf.right_masks = rights.data();
        leaf.span_rows = question.span_rows.data();
        leaf.pivots = question.pivots.data();
        return leaf;
    }
};

gpu_leaf::LeafQuestion dense_question(std::size_t rows, std::size_t columns,
                                      std::size_t dimension) {
    static Field field(2);
    const RankOnePool pool(field, rows, columns);
    return gpu_leaf::packed_question(
        field, rows, columns, gpu_leaf::dense_span(field, pool, rows, columns, dimension));
}

void check_a_scan(std::size_t rows, std::size_t columns, std::size_t dimension,
                  std::size_t capacity, bool expect_an_answer, const std::string& what) {
    const gpu_leaf::LeafQuestion question = dense_question(rows, columns, dimension);
    Borrowed borrowed;
    const bilinear_rank::PackedLeaf leaf = borrowed.of(question);

    gpu_leaf::set_survivor_capacity(capacity);
    std::vector<std::uint64_t> survivors;
    const bool answered = gpu_leaf::card_backend().scan(leaf, question.left_count, survivors);
    gpu_leaf::set_survivor_capacity(1u << 16);

    check::equal(what + ": answered", static_cast<long long>(answered),
                 static_cast<long long>(expect_an_answer));
    if (!answered) {
        check::equal(what + ": and left the list alone",
                     static_cast<long long>(survivors.size()), 0);
        return;
    }
    check::equal(what + ": the host's survivors, element for element",
                 static_cast<long long>(
                     survivors == gpu_leaf::scan_pool_on_host(question, 0, question.left_count)),
                 1);
}

void check_a_walk(std::size_t rows, std::size_t columns, std::size_t dimension,
                  std::size_t capacity, bool expect_an_answer, const std::string& what) {
    const gpu_leaf::LeafQuestion question = dense_question(rows, columns, dimension);
    Borrowed borrowed;
    const bilinear_rank::PackedLeaf leaf = borrowed.of(question);
    const std::uint64_t elements = std::uint64_t(1) << dimension;

    gpu_leaf::set_survivor_capacity(capacity);
    std::vector<std::uint64_t> survivors;
    const bool answered = gpu_leaf::card_backend().walk(leaf, elements, survivors);
    gpu_leaf::set_survivor_capacity(1u << 16);

    check::equal(what + ": answered", static_cast<long long>(answered),
                 static_cast<long long>(expect_an_answer));
    if (!answered) {
        check::equal(what + ": and left the list alone",
                     static_cast<long long>(survivors.size()), 0);
        return;
    }
    check::equal(what + ": the host's survivors, element for element",
                 static_cast<long long>(
                     survivors == gpu_leaf::walk_subspace_on_host(question, 1, elements)),
                 1);
}

}  // namespace

int main() {
    // The control: a capacity nothing overflows, so the chunking never runs and
    // the two sides agree for the ordinary reason.
    check_a_scan(9, 9, 14, 1u << 16, true, "scan 9x9 dim 14 dense, room to spare");
    check_a_walk(9, 9, 9, 1u << 16, true, "walk 9x9 dim 9 dense, room to spare");

    // 523 survivors into 512 slots and 511 into 256: a halving each, which is
    // what the recovery is for.
    check_a_scan(9, 9, 14, 512, true, "scan 9x9 dim 14 dense, 512 slots");
    check_a_walk(9, 9, 9, 256, true, "walk 9x9 dim 9 dense, 256 slots");

    // **A scan is cut into rows of the grid, and this span puts 511 of its 523
    // survivors in one row**: every map whose only nonzero row is the first is
    // one left vector against every right. So no split separates them, seven
    // bites of the range all contain that row, and the leaf goes back to the
    // host. That is the honest limit of chunking a grid by rows, and it is here
    // so that a change which quietly started truncating instead would fail.
    check_a_scan(9, 9, 14, 256, false, "scan 9x9 dim 14 dense, 256 slots");

    // Four slots for 65 535 survivors is fourteen halvings against the six
    // allowed, so the leaf goes back to the host rather than coming back short.
    check_a_scan(16, 16, 47, 4, false, "scan 16x16 dim 47 dense, 4 slots");
    check_a_walk(16, 16, 16, 4, false, "walk 16x16 dim 16 dense, 4 slots");

    return check::report("survivor overflow");
}
