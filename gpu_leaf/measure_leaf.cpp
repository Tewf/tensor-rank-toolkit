#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "candidate_pool.h"
#include "check_agreement.h"
#include "check_span_agreement.h"
#include "exhaustive_search.h"
#include "gf2_leaf.h"
#include "gpu_leaf.h"
#include "host_reference.h"
#include "leaf_question.h"
#include "question_packing.h"

/// The harness: the same leaf asked of one core and of the card, checked for
/// agreement, and priced.
///
/// `check` asks the questions whose answers can be compared, `measure` asks the
/// ones with enough weight to time, and neither is evidence about the other:
/// a millisecond row says nothing about speed and a four-billion-element row
/// cannot be checked against a sequential scan that would take an hour. What
/// makes the second trustworthy is that the first passed on the same kernel.
///
/// Every timing here follows [`../MEASURING.md`](../MEASURING.md): one core
/// where it says one core, fastest of three, on a quiet machine, under
/// `flock /tmp/bilinear-measure.lock`.
namespace {

using bilinear_rank::Addressed;
using bilinear_rank::Field;
using bilinear_rank::Gf2Leaf;
using bilinear_rank::Matrix;
using bilinear_rank::RankOnePool;
using bilinear_rank::ReducedBasis;
using bilinear_rank::SearchBudget;

constexpr std::size_t kSurvivorCapacity = 1u << 22;

/// Where a timed host run puts its answer, so that no compiler is entitled to
/// notice nobody reads it.
///
/// The two host reference routes are inline in a header and their result is a
/// local vector, which is exactly the shape a compiler may delete outright. It
/// did not, and the seconds in the tables are far too long for a deleted loop,
/// but a row that depends on an optimiser declining an opportunity is not a
/// measurement of anything.
volatile std::size_t survivors_found = 0;

/// One question, held in both representations, so the two sides are asked the
/// same thing and not merely the same shape.
class Question {
   public:
    Question(std::size_t rows, std::size_t columns, std::size_t dimension, bool dense)
        : field_(2),
          pool_(field_, rows, columns),
          span_(dense ? gpu_leaf::dense_span(field_, pool_, rows, columns, dimension)
                      : gpu_leaf::sparse_span(field_, pool_, rows * columns, dimension)),
          packed_(gpu_leaf::packed_question(field_, rows, columns, span_)),
          candidates_{pool_},
          leaf_(field_, candidates_, rows, columns) {}

    const RankOnePool& pool() const { return pool_; }
    const ReducedBasis& span() const { return span_; }
    const gpu_leaf::LeafQuestion& packed() const { return packed_; }
    const Gf2Leaf<Addressed>& leaf() const { return leaf_; }

   private:
    Field field_;
    RankOnePool pool_;
    ReducedBasis span_;
    gpu_leaf::LeafQuestion packed_;
    Addressed candidates_;
    Gf2Leaf<Addressed> leaf_;
};

double fastest_of_three(const std::function<void()>& body) {
    double best = 0.0;
    for (int run = 0; run < 3; ++run) {
        const auto started = std::chrono::steady_clock::now();
        body();
        const double seconds =
            std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
        if (run == 0 || seconds < best) best = seconds;
    }
    return best;
}

void report(const char* what, double elements, double seconds) {
    std::printf("| %-46s | %14.0f | %9.3f | %10.1f | %9.3e |\n", what, elements, seconds,
                seconds * 1e9 / elements, elements / seconds);
    std::fflush(stdout);
}

void report_header(const char* title) {
    std::printf("\n%s\n\n", title);
    std::printf("| %-46s | %14s | %9s | %10s | %9s |\n", "what", "elements", "seconds",
                "ns/element", "elements/s");
    std::printf("|%s|%s|%s|%s|%s|\n", std::string(48, '-').c_str(), std::string(16, '-').c_str(),
                std::string(11, '-').c_str(), std::string(12, '-').c_str(),
                std::string(11, '-').c_str());
}

// --- agreement --------------------------------------------------------------

bool same_indices(const std::vector<std::uint64_t>& left,
                  const std::vector<std::uint64_t>& right) {
    return left == right;
}

int failures = 0;

void announce(const char* name, bool passed, std::size_t survivors, std::size_t kept) {
    std::printf("  %-46s %s  %8zu survivors, %3zu kept\n", name, passed ? "agree" : "DIFFER",
                survivors, kept);
    std::fflush(stdout);
    if (!passed) ++failures;
}

void check_scan(const Question& question, std::size_t left_rows, std::size_t needed,
                const char* name) {
    const gpu_leaf::LeafQuestion& packed = question.packed();
    const std::vector<std::uint64_t> host = gpu_leaf::scan_pool_on_host(packed, 0, left_rows);
    const gpu_leaf::GpuSurvivors card =
        gpu_leaf::scan_pool_on_gpu(packed, 0, left_rows, kSurvivorCapacity);

    SearchBudget budget(1'000'000'000, left_rows * packed.right_count);
    const std::vector<Matrix> shipped =
        question.leaf().by_scanning_the_pool(question.span(), needed, &budget);
    const std::vector<Matrix> rebuilt =
        gpu_leaf::maps_kept_from_scan(question.pool(), packed, card.indices, needed);

    const bool passed = !card.overflowed && same_indices(host, card.indices) &&
                        gpu_leaf::same_maps(shipped, rebuilt);
    announce(name, passed, card.indices.size(), rebuilt.size());
}

void check_walk(const Question& question, std::uint64_t elements, std::size_t needed,
                const char* name) {
    const gpu_leaf::LeafQuestion& packed = question.packed();
    const std::vector<std::uint64_t> host = gpu_leaf::walk_subspace_on_host(packed, 1, elements);
    const gpu_leaf::GpuSurvivors card =
        gpu_leaf::walk_subspace_on_gpu(packed, 1, elements, kSurvivorCapacity);

    SearchBudget budget(1'000'000'000, elements);
    const std::vector<Matrix> shipped =
        question.leaf().by_walking_the_subspace(question.span(), needed, elements, &budget);
    const std::vector<Matrix> rebuilt = gpu_leaf::maps_kept_from_walk(packed, card.indices, needed);

    const bool passed = !card.overflowed && same_indices(host, card.indices) &&
                        gpu_leaf::same_maps(shipped, rebuilt);
    announce(name, passed, card.indices.size(), rebuilt.size());
}

void announce_ranks(const char* name, bool passed, std::size_t elements, std::size_t differ) {
    std::printf("  %-46s %s  %8zu elements, %3zu differ\n", name, passed ? "agree" : "DIFFER",
                elements, differ);
    std::fflush(stdout);
    if (!passed) ++failures;
}

/// Every rank of a span, the card against `span_element_ranks`, slot for slot.
///
/// **The host column here cannot become the card**, which is the same property
/// that makes the leaf rows trustworthy and holds for the same reason:
/// `measure-leaf` does not link `gpu_leaf_registration`, so
/// `span_ranks_on_card()` is null in this program and `span_element_ranks`
/// answers on the host whatever hardware is present.
///
/// `scattered_slices` rather than anything structured, because a span whose every
/// element has the same rank would compare two vectors of one number and pass
/// whatever the kernel did.
void check_span_ranks(std::size_t rows, std::size_t columns, std::size_t slices,
                      const char* name) {
    const std::vector<Matrix> made = gpu_leaf::scattered_slices(rows, columns, slices, 0x5EEDull);
    const Field field(2);
    const std::vector<std::size_t> host = bilinear_rank::span_element_ranks(field, made);

    const gpu_leaf::SpanQuestion question = gpu_leaf::packed_span(made);
    const gpu_leaf::GpuRanks card = gpu_leaf::rank_span_on_gpu(question, 0, question.elements());

    const std::size_t differ = gpu_leaf::ranks_that_differ(host, card.ranks);
    announce_ranks(name, differ == 0, host.size(), differ);
}

void check_everything() {
    std::printf("\nAgreement, survivor by survivor and map by map\n\n");
    check_scan(Question(4, 4, 6, false), 15, 6, "scan 4x4 dim 6, whole pool");
    check_scan(Question(4, 4, 8, true), 15, 8, "scan 4x4 dim 8 dense, whole pool");
    check_scan(Question(5, 5, 10, false), 31, 10, "scan 5x5 dim 10, whole pool");
    check_scan(Question(9, 9, 12, false), 511, 12, "scan 9x9 dim 12, whole pool");
    check_scan(Question(9, 9, 14, true), 511, 14, "scan 9x9 dim 14 dense, whole pool");
    check_scan(Question(16, 16, 47, true), 4, 47, "scan 16x16 dim 47 dense, 4 rows of the grid");
    check_scan(Question(16, 16, 47, false), 64, 47, "scan 16x16 dim 47, 64 rows of the grid");
    check_walk(Question(4, 4, 6, false), 1ull << 6, 6, "walk 4x4 dim 6");
    check_walk(Question(5, 5, 12, false), 1ull << 12, 12, "walk 5x5 dim 12");
    check_walk(Question(9, 9, 9, true), 1ull << 9, 9, "walk 9x9 dim 9 dense");
    check_walk(Question(9, 9, 17, false), 1ull << 17, 17, "walk 9x9 dim 17");
    check_walk(Question(16, 16, 16, true), 1ull << 16, 47, "walk 16x16 dim 16 dense");
    check_walk(Question(16, 16, 20, false), 1ull << 20, 47, "walk 16x16 dim 20");

    // The span seam, on the four shapes a kernel is compiled for. The dimensions
    // are the ones `lower-the-bound` actually reaches, 11 to 13, which is also
    // why every one of them is below the 8 192 launch floor: what is checked
    // here is that the two agree, not that the card would be asked.
    std::printf("\nAgreement, rank by rank, over a span's elements\n\n");
    check_span_ranks(4, 4, 11, "span ranks 4x4, dimension 11");
    check_span_ranks(5, 5, 12, "span ranks 5x5, dimension 12");
    check_span_ranks(9, 9, 13, "span ranks 9x9, dimension 13");
    check_span_ranks(16, 16, 12, "span ranks 16x16, dimension 12");
}

// --- weight -----------------------------------------------------------------

/// The shipped leaf on however many threads, each asked the same prefix.
///
/// A `Gf2Leaf` scan starts at index 0 and there is no way to start it elsewhere
/// without changing it, so twelve threads cannot be given twelve slices of one
/// leaf. They are given the same slice instead: every element costs the same, so
/// the aggregate rate is what twelve cores do on this work, and it is not a
/// partition of one leaf.
double shipped_scan(const Question& question, std::size_t left_rows, std::size_t threads) {
    const std::size_t elements = left_rows * question.packed().right_count;
    return fastest_of_three([&] {
        std::vector<std::thread> workers;
        for (std::size_t worker = 0; worker < threads; ++worker) {
            workers.emplace_back([&] {
                SearchBudget budget(1'000'000'000, elements);
                question.leaf().by_scanning_the_pool(question.span(), 47, &budget);
            });
        }
        for (std::thread& worker : workers) worker.join();
    });
}

double shipped_walk(const Question& question, std::uint64_t elements, std::size_t threads) {
    return fastest_of_three([&] {
        std::vector<std::thread> workers;
        for (std::size_t worker = 0; worker < threads; ++worker) {
            workers.emplace_back([&] {
                SearchBudget budget(1'000'000'000, elements);
                question.leaf().by_walking_the_subspace(question.span(), 47, elements, &budget);
            });
        }
        for (std::thread& worker : workers) worker.join();
    });
}

/// The kernel's arithmetic on the host, over the same range on every thread.
double packed_scan(const gpu_leaf::LeafQuestion& packed, std::size_t left_rows,
                   std::size_t threads) {
    return fastest_of_three([&] {
        std::vector<std::thread> workers;
        for (std::size_t worker = 0; worker < threads; ++worker) {
            workers.emplace_back(
                [&] { survivors_found = gpu_leaf::scan_pool_on_host(packed, 0, left_rows).size(); });
        }
        for (std::thread& worker : workers) worker.join();
    });
}

double packed_walk(const gpu_leaf::LeafQuestion& packed, std::uint64_t elements,
                   std::size_t threads) {
    return fastest_of_three([&] {
        std::vector<std::thread> workers;
        for (std::size_t worker = 0; worker < threads; ++worker) {
            workers.emplace_back([&] {
                survivors_found = gpu_leaf::walk_subspace_on_host(packed, 1, elements).size();
            });
        }
        for (std::thread& worker : workers) worker.join();
    });
}

double fastest_gpu_scan(const gpu_leaf::LeafQuestion& packed, std::size_t left_rows) {
    double best = 0.0;
    for (int run = 0; run < 4; ++run) {  // the first launch pays for the context
        const gpu_leaf::GpuSurvivors card =
            gpu_leaf::scan_pool_on_gpu(packed, 0, left_rows, kSurvivorCapacity);
        if (card.overflowed) throw std::runtime_error("survivor buffer overflowed");
        if (run == 1 || card.wall_seconds < best) best = card.wall_seconds;
    }
    return best;
}

double fastest_gpu_walk(const gpu_leaf::LeafQuestion& packed, std::uint64_t elements) {
    double best = 0.0;
    for (int run = 0; run < 4; ++run) {
        const gpu_leaf::GpuSurvivors card =
            gpu_leaf::walk_subspace_on_gpu(packed, 1, elements, kSurvivorCapacity);
        if (card.overflowed) throw std::runtime_error("survivor buffer overflowed");
        if (run == 1 || card.wall_seconds < best) best = card.wall_seconds;
    }
    return best;
}

void measure_the_scan(std::size_t shipped_rows, std::size_t packed_rows) {
    const Question question(16, 16, 47, false);
    const gpu_leaf::LeafQuestion& packed = question.packed();
    const double per_row = static_cast<double>(packed.right_count);
    // Refused rather than clamped. A row count past the end of the grid would
    // read off both the host's mask table and the card's, and a run that quietly
    // scanned fewer elements than it divided by would publish a wrong rate.
    if (shipped_rows > packed.left_count || packed_rows > packed.left_count) {
        throw std::runtime_error("asked for more rows of the grid than there are");
    }

    report_header("The pool scan at 16x16 over GF(2), dimension 47, the leaf route <4,4,4> takes");
    report("shipped leaf, addressed pool, 1 core", shipped_rows * per_row,
           shipped_scan(question, shipped_rows, 1));
    report("shipped leaf, addressed pool, 12 threads", 12 * shipped_rows * per_row,
           shipped_scan(question, shipped_rows, 12));
    report("packed generation on the host, 1 core", packed_rows * per_row,
           packed_scan(packed, packed_rows, 1));
    report("packed generation on the host, 12 threads", 12 * packed_rows * per_row,
           packed_scan(packed, packed_rows, 12));
    // The two card rows are the control on the host rows: a host row covers a
    // prefix of the grid and the last one covers all of it, so if the prefix
    // were unrepresentative these two would not agree.
    report("one RTX 4060, the rows the host was given", packed_rows * per_row,
           fastest_gpu_scan(packed, packed_rows));
    report("one RTX 4060, the whole pool", static_cast<double>(packed.pool_size()),
           fastest_gpu_scan(packed, packed.left_count));
}

/// Every row here walks the **whole** subspace and not a prefix of one.
///
/// Both host routes skip a basis row whose digit is zero and the kernel cannot,
/// so a prefix would have compared an average popcount against a full one and
/// credited the host with work it did not do. A whole walk of dimension 27 is
/// ten seconds of one core, which is a measurement, so that is the one asked.
void measure_the_walk() {
    constexpr std::uint64_t kWholeSubspace = 1ull << 27;
    const Question question(16, 16, 27, false);
    const gpu_leaf::LeafQuestion& packed = question.packed();

    report_header("The subspace walk at 16x16 over GF(2), dimension 27, whole subspace");
    report("shipped leaf, 1 core", static_cast<double>(kWholeSubspace),
           shipped_walk(question, kWholeSubspace, 1));
    report("shipped leaf, 12 threads", static_cast<double>(12 * kWholeSubspace),
           shipped_walk(question, kWholeSubspace, 12));
    report("packed generation on the host, 1 core", static_cast<double>(kWholeSubspace),
           packed_walk(packed, kWholeSubspace, 1));
    report("packed generation on the host, 12 threads", static_cast<double>(12 * kWholeSubspace),
           packed_walk(packed, kWholeSubspace, 12));
    report("one RTX 4060", static_cast<double>(kWholeSubspace),
           fastest_gpu_walk(packed, kWholeSubspace));
}

/// Dimension 31 is the widest subspace `⟨4,4,4⟩` ever walks: past it `2^dim`
/// exceeds the 4 294 836 225 maps of the pool and
/// [`rank_one_basis.cpp`](../exhaustive_search/rank_one_basis.cpp) scans
/// instead.
///
/// The card alone, because one core walking it is over three minutes and
/// nothing is learned by spending nine on three runs of it. The row is what the
/// card does on the largest walk the search can pose, and it is not a ratio.
void measure_the_widest_walk() {
    constexpr std::uint64_t kWholeSubspace = 1ull << 31;
    const Question question(16, 16, 31, false);
    report_header("The widest walk <4,4,4> can pose: dimension 31, on the card alone");
    report("one RTX 4060", static_cast<double>(kWholeSubspace),
           fastest_gpu_walk(question.packed(), kWholeSubspace));
}

// --- the launch floor -------------------------------------------------------

/// Where one launch starts beating the host, which is the number
/// [`../run_limits/device.h`](../run_limits/device.h) keeps a bulk question off
/// the card below.
///
/// **The card's column is `wall_seconds`, never `kernel_seconds`.** A floor
/// exists because a launch costs more than small work, so the launch and the
/// copy back are precisely the two things that may not be left out of it.
///
/// **A walk row is a whole subspace, not a prefix of a wider one.** The kernel
/// rebuilds an element from every basis row, so its per-element cost is the
/// dimension; a leaf walks all `2^dim` elements of the span it was handed and
/// never a prefix, so dimension and element count move together here exactly as
/// they do in a search.
///
/// **A scan row is whole rows of the outer-product grid**, because that is the
/// smallest thing either side does: `by_carrying_a_residual` Gray-walks a whole
/// row before it consults the budget, and the kernel launches whole rows too.
void floor_header(const char* title) {
    std::printf("\n%s\n\n", title);
    std::printf("| %-24s | %12s | %11s | %11s | %8s | %s |\n", "what", "elements", "host s",
                "card wall s", "speed-up", "wins");
    std::printf("|%s|%s|%s|%s|%s|%s|\n", std::string(26, '-').c_str(),
                std::string(14, '-').c_str(), std::string(13, '-').c_str(),
                std::string(13, '-').c_str(), std::string(10, '-').c_str(),
                std::string(6, '-').c_str());
}

void floor_row(const std::string& what, double elements, double host, double card) {
    std::printf("| %-24s | %12.0f | %11.6f | %11.6f | %8.2f | %-4s |\n", what.c_str(), elements,
                host, card, host / card, host > card ? "card" : "host");
    std::fflush(stdout);
}

/// The shipped leaf walking a whole subspace, bounded so an abandoned leaf and a
/// finished one cost the same. `needed` is past the dimension, so the greedy
/// never stops the walk early and every row prices the same number of elements.
double shipped_whole_walk(const Question& question, std::uint64_t elements) {
    return fastest_of_three([&] {
        SearchBudget budget(1'000'000'000, elements);
        survivors_found =
            question.leaf().by_walking_the_subspace(question.span(), 47, elements, &budget).size();
    });
}

/// The shipped leaf scanning a prefix of the pool, stopped by the leaf budget at
/// the same element the launch stops at.
double shipped_bounded_scan(const Question& question, std::size_t elements) {
    return fastest_of_three([&] {
        SearchBudget budget(1'000'000'000, elements);
        survivors_found = question.leaf().by_scanning_the_pool(question.span(), 47, &budget).size();
    });
}

/// The smallest whole subspace the card wins, or zero if it wins none of them.
std::uint64_t walk_crossover(std::size_t rows, std::size_t columns, std::size_t widest,
                             const char* title) {
    floor_header(title);
    std::uint64_t crossed = 0;
    for (std::size_t dimension = 8; dimension <= widest; ++dimension) {
        const Question question(rows, columns, dimension, false);
        const std::uint64_t elements = std::uint64_t(1) << dimension;
        const double host = shipped_whole_walk(question, elements);
        const double card = fastest_gpu_walk(question.packed(), elements);
        floor_row("dimension " + std::to_string(dimension), static_cast<double>(elements), host,
                  card);
        if (crossed == 0 && card < host) crossed = elements;
    }
    return crossed;
}

/// The same, over whole rows of the outer-product grid. `ceiling` stops the
/// sweep once a host row costs more than the crossover is worth: the ratio only
/// grows with the element count, so nothing past the first win is evidence.
std::uint64_t scan_crossover(std::size_t rows, std::size_t columns, std::size_t dimension,
                             std::size_t ceiling, const char* title) {
    const Question question(rows, columns, dimension, false);
    const gpu_leaf::LeafQuestion& packed = question.packed();
    floor_header(title);
    std::uint64_t crossed = 0;
    for (std::size_t grid_rows = 1; grid_rows <= packed.left_count; grid_rows *= 2) {
        const std::size_t elements = grid_rows * packed.right_count;
        if (elements > ceiling) break;
        const double host = shipped_bounded_scan(question, elements);
        const double card = fastest_gpu_scan(packed, grid_rows);
        floor_row(std::to_string(grid_rows) + (grid_rows == 1 ? " row" : " rows"),
                  static_cast<double>(elements), host, card);
        if (crossed == 0 && card < host) crossed = elements;
    }
    return crossed;
}

/// Both routes on each of the four shapes a kernel is compiled for, because a
/// floor that holds on the widest one and not on the narrowest is not a floor.
/// The narrow shapes are the hard case: their host element is a single word, so
/// the host is cheapest exactly where a launch is not.
void measure_the_floor() {
    const std::uint64_t walk_16 = walk_crossover(16, 16, 22, "Subspace walk, 16x16");
    const std::uint64_t walk_9 = walk_crossover(9, 9, 18, "Subspace walk, 9x9");
    const std::uint64_t walk_5 = walk_crossover(5, 5, 20, "Subspace walk, 5x5");
    const std::uint64_t walk_4 = walk_crossover(4, 4, 16, "Subspace walk, 4x4");
    const std::uint64_t scan_16 =
        scan_crossover(16, 16, 47, 1u << 23, "Pool scan, 16x16, dimension 47");
    const std::uint64_t scan_9 = scan_crossover(9, 9, 23, 1u << 23, "Pool scan, 9x9, dimension 23");
    const std::uint64_t scan_5 = scan_crossover(5, 5, 10, 1u << 23, "Pool scan, 5x5, dimension 10");
    const std::uint64_t scan_4 = scan_crossover(4, 4, 8, 1u << 23, "Pool scan, 4x4, dimension 8");

    const auto shown = [](std::uint64_t count) { return static_cast<unsigned long long>(count); };
    std::printf("\nFirst element count the card wins; 0 means it never did\n\n");
    std::printf("  walk 16x16 %12llu    scan 16x16 %12llu\n", shown(walk_16), shown(scan_16));
    std::printf("  walk  9x9  %12llu    scan  9x9  %12llu\n", shown(walk_9), shown(scan_9));
    std::printf("  walk  5x5  %12llu    scan  5x5  %12llu\n", shown(walk_5), shown(scan_5));
    std::printf("  walk  4x4  %12llu    scan  4x4  %12llu\n", shown(walk_4), shown(scan_4));
    const std::uint64_t largest =
        std::max(std::max(std::max(walk_16, walk_9), std::max(walk_5, walk_4)),
                 std::max(std::max(scan_16, scan_9), std::max(scan_5, scan_4)));
    std::printf("\n  largest crossover %llu, so no route loses at or above it\n", shown(largest));
}

}  // namespace

int main(int argc, char** argv) try {
    const std::string what = argc > 1 ? argv[1] : "check";
    std::printf("device: %s\n", gpu_leaf::device_description().c_str());

    if (what == "check" || what == "both") check_everything();
    if (what == "measure" || what == "both") {
        const std::size_t shipped_rows = argc > 2 ? std::strtoull(argv[2], nullptr, 10) : 200;
        const std::size_t packed_rows = argc > 3 ? std::strtoull(argv[3], nullptr, 10) : 2000;
        measure_the_scan(shipped_rows, packed_rows);
        measure_the_walk();
        measure_the_widest_walk();
    }
    if (what == "floor") measure_the_floor();
    if (failures != 0) std::printf("\n%d case(s) disagreed\n", failures);
    return failures == 0 ? 0 : 1;
} catch (const std::exception& failure) {
    // Everything that can go wrong here is an exception, and `cuda_guard.cuh`
    // exists so that one of them names the call that failed. Letting it reach
    // `std::terminate` would throw that name away and abort on a signal instead
    // of an exit code.
    std::fprintf(stderr, "measure-leaf: %s\n", failure.what());
    return 2;
}
