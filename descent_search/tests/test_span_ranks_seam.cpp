/// The seam a card is offered a span through, exercised on a machine with no card.
///
/// `ranks_from_a_card` in
/// [`../minimum_weight_basis.cpp`](../minimum_weight_basis.cpp) has five gates and
/// every one of them decides whether a published count moves. Until this file
/// none of them could be exercised without `nvcc`: the only backend that exists
/// is the CUDA one, so a build without the toolkit took the host path and proved
/// nothing about the other. A fake backend is enough, because what is under test
/// is the dispatch and the packing rather than any kernel.
///
/// **The fake answers out of the packed span it was handed, and that is the
/// point.** It rebuilds element `i` as the exclusive or of the slices whose bit
/// is set in `i`, which is the contract
/// [`../span_ranks_on_card.h`](../span_ranks_on_card.h) states, and ranks it with
/// the same host routine. So agreement with the unregistered answer checks the
/// packing, the slice order and the index convention together: a backend handed a
/// reordered or mis-packed span fails here rather than in a rank nobody can see.
///
/// **The floor is moved rather than reached.** A span crossing the shipped 8 192
/// needs thirteen slices, which is 8 192 host ranks a check; `set_launch_floor`
/// is the same knob `tunables.conf` turns, so the boundary is asserted on both
/// sides at a size a test can afford.
#include <algorithm>
#include <cstdint>
#include <vector>

#include "check.h"
#include "device.h"
#include "gf2_bits.h"
#include "measures.h"
#include "minimum_weight_basis.h"
#include "span_ranks_on_card.h"

namespace {

using bilinear_rank::Element;
using bilinear_rank::Field;
using bilinear_rank::Matrix;
using bilinear_rank::PackedSpan;

/// What the fake was asked, so that a test can assert it was not asked at all.
struct Asked {
    std::size_t ranks_calls = 0;
    std::size_t rows = 0;
    std::size_t columns = 0;
    std::size_t slices = 0;
    std::uint64_t elements = 0;
};

Asked asked;
bool shape_is_handled = true;
bool backend_answers = true;

void forget_what_was_asked() { asked = Asked{}; }

bool fake_handles(std::size_t rows, std::size_t columns) {
    (void)rows;
    (void)columns;
    return shape_is_handled;
}

/// Every rank, read out of the packed span the seam handed over and out of
/// nothing else.
bool fake_ranks(const PackedSpan& span, std::uint64_t elements, std::vector<std::size_t>& ranks) {
    ++asked.ranks_calls;
    asked.rows = span.rows;
    asked.columns = span.columns;
    asked.slices = span.slices;
    asked.elements = elements;
    if (!backend_answers) return false;

    const Field field(2);
    const std::size_t width = span.rows * span.columns;
    std::vector<std::uint64_t> element(span.words);
    Matrix rebuilt(span.rows, span.columns);
    for (std::uint64_t index = 0; index < elements; ++index) {
        std::fill(element.begin(), element.end(), std::uint64_t(0));
        for (std::size_t slice = 0; slice < span.slices; ++slice) {
            if (((index >> slice) & 1ull) == 0ull) continue;
            linear_algebra::gf2_xor(element.data(), span.slice_rows + slice * span.words,
                                    span.words);
        }
        linear_algebra::gf2_unpack(element.data(), width, rebuilt.data());
        ranks[static_cast<std::size_t>(index)] = linear_algebra::rank(field, rebuilt);
    }
    return true;
}

const bilinear_rank::SpanRanksOnCard fake_card{&fake_handles, &fake_ranks};

bool a_card_is_present() { return true; }

/// A span whose elements take several different ranks, so that a backend
/// returning a constant would be caught. Entry `(row, column)` of slice `s` is
/// set when `(row + column + s)` is divisible by three, which is nothing clever
/// and merely not uniform.
std::vector<Matrix> some_slices(std::size_t count, std::size_t rows, std::size_t columns) {
    std::vector<Matrix> slices;
    for (std::size_t slice = 0; slice < count; ++slice) {
        Matrix matrix(rows, columns);
        for (std::size_t row = 0; row < rows; ++row) {
            for (std::size_t column = 0; column < columns; ++column) {
                matrix(row, column) = static_cast<Element>((row + column + slice) % 3 == 0 ? 1 : 0);
            }
        }
        slices.push_back(std::move(matrix));
    }
    return slices;
}

bool same_ranks(const std::vector<std::size_t>& left, const std::vector<std::size_t>& right) {
    return left == right;
}

}  // namespace

int main() {
    const Field gf2(2);
    const Field gf3(3);
    const std::vector<Matrix> slices = some_slices(6, 4, 5);
    const std::uint64_t elements = 64;

    // The answer with nothing registered, which is what every decline must
    // reproduce and what an accepted answer must agree with.
    bilinear_rank::register_span_ranks_on_card(nullptr);
    const std::vector<std::size_t> host = bilinear_rank::span_element_ranks(gf2, slices);
    check::equal("the host ranks the whole span", static_cast<long long>(host.size()),
                 static_cast<long long>(elements));

    run_limits::register_gpu_backend(&a_card_is_present);
    check::equal("a registered probe makes the card available",
                 run_limits::available(run_limits::Device::Gpu) ? 1 : 0, 1);

    // Gate 1: nothing registered. The seam is null for the whole run, which is
    // every build without `nvcc`, and the host answers without being asked to.
    run_limits::set_launch_floor(1);
    forget_what_was_asked();
    const std::vector<std::size_t> unregistered = bilinear_rank::span_element_ranks(gf2, slices);
    check::equal("no backend registered, so nothing was asked",
                 static_cast<long long>(asked.ranks_calls), 0);
    check::equal("no backend registered, so the host answered",
                 same_ranks(unregistered, host) ? 1 : 0, 1);

    bilinear_rank::register_span_ranks_on_card(&fake_card);

    // Gate 2: every gate open. The backend is asked, and what it returns is what
    // the caller gets -- so a card that fired is visible here rather than
    // indistinguishable from the host.
    shape_is_handled = true;
    backend_answers = true;
    forget_what_was_asked();
    const std::vector<std::size_t> carded = bilinear_rank::span_element_ranks(gf2, slices);
    check::equal("every gate open, so the card was asked once",
                 static_cast<long long>(asked.ranks_calls), 1);
    check::equal("the card was handed the shape it was offered",
                 static_cast<long long>(asked.rows * 100 + asked.columns), 405);
    check::equal("the card was handed every slice", static_cast<long long>(asked.slices),
                 static_cast<long long>(slices.size()));
    check::equal("the card was asked for the whole span",
                 static_cast<long long>(asked.elements), static_cast<long long>(elements));
    // The packing, the slice order and the index convention, all three at once:
    // the fake read the span out of `PackedSpan` alone.
    check::equal("the card's ranks are the host's, slot for slot",
                 same_ranks(carded, host) ? 1 : 0, 1);

    // Gate 3: a shape no kernel was compiled for. This is the gate <3,4,5> stops
    // at -- its operands are 12x20 and `span_ranks_handle` carries four square
    // shapes -- so it is the one that decides whether a CUDA install changes
    // anything for that tensor.
    shape_is_handled = false;
    forget_what_was_asked();
    const std::vector<std::size_t> unhandled = bilinear_rank::span_element_ranks(gf2, slices);
    check::equal("an unhandled shape is never asked for ranks",
                 static_cast<long long>(asked.ranks_calls), 0);
    check::equal("an unhandled shape falls back to the host",
                 same_ranks(unhandled, host) ? 1 : 0, 1);
    shape_is_handled = true;

    // Gate 4: a field that is not GF(2). The bit-packed arithmetic a kernel does
    // is not this arithmetic, so the backend must not see it at all.
    const std::vector<Matrix> over_gf3 = some_slices(3, 4, 5);
    forget_what_was_asked();
    const std::vector<std::size_t> ternary = bilinear_rank::span_element_ranks(gf3, over_gf3);
    check::equal("a field that is not GF(2) is never offered to the card",
                 static_cast<long long>(asked.ranks_calls), 0);
    check::equal("a ternary span is ranked over 3^k elements",
                 static_cast<long long>(ternary.size()), 27);

    // Gate 5: the launch floor, which is asked at the seam and not inside the
    // backend. One below and one at, because `chosen_device` compares with `<`
    // and a span exactly at the floor is the card's.
    forget_what_was_asked();
    run_limits::set_launch_floor(elements + 1);
    const std::vector<std::size_t> under = bilinear_rank::span_element_ranks(gf2, slices);
    check::equal("a span under the launch floor stays on the host",
                 static_cast<long long>(asked.ranks_calls), 0);
    check::equal("a span under the floor is the host's answer",
                 same_ranks(under, host) ? 1 : 0, 1);

    forget_what_was_asked();
    run_limits::set_launch_floor(elements);
    const std::vector<std::size_t> at_the_floor = bilinear_rank::span_element_ranks(gf2, slices);
    check::equal("a span exactly at the launch floor goes to the card",
                 static_cast<long long>(asked.ranks_calls), 1);
    check::equal("a span at the floor still answers what the host would",
                 same_ranks(at_the_floor, host) ? 1 : 0, 1);

    // Gate 6: the backend itself declining. A CUDA call that failed becomes the
    // host answering rather than the run stopping, and the vector it left
    // untouched must not reach a caller half filled.
    run_limits::set_launch_floor(1);
    backend_answers = false;
    forget_what_was_asked();
    const std::vector<std::size_t> declined = bilinear_rank::span_element_ranks(gf2, slices);
    check::equal("a backend that declined was still asked",
                 static_cast<long long>(asked.ranks_calls), 1);
    check::equal("a backend that declined leaves the host's answer",
                 same_ranks(declined, host) ? 1 : 0, 1);
    backend_answers = true;

    // Left as it was found, since these are process-wide and a later check in
    // the same binary would read them.
    bilinear_rank::register_span_ranks_on_card(nullptr);
    run_limits::register_gpu_backend(nullptr);
    run_limits::set_launch_floor(8192);
    return check::report("span_ranks_seam");
}
