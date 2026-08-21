#include "minimum_weight_basis.h"

#include <algorithm>

#include "memory_budget.h"
#include "measures.h"
#include "reflected_gray_walk.h"
#include "span_basis.h"
#include "span_enumeration.h"

namespace bilinear_rank {

std::vector<Matrix> minimum_weight_basis_with(const Field& field, const std::vector<Matrix>& slices,
                               const Matrix& candidate, const std::vector<std::size_t>& known) {
    std::vector<Matrix> enlarged = slices;
    enlarged.push_back(candidate);
    return minimum_weight_basis(field, enlarged, known);
}

namespace {

/// The span walked one slice at a time, with the index carried alongside.
///
/// Both loops in this file used to rebuild element `index` from
/// `coefficient_vector(index)`, which is `O(k · width)` multiply-accumulates for
/// a thing that is `O(width)` field additions away from the element before it —
/// under an order where consecutive strings differ in one digit by one, which is
/// what [`ReflectedGrayWalk`](reflected_gray_walk.h) is. This is that walk with
/// the base-`p` value of the string maintained beside it, `± p^digit` a step, so
/// a caller still knows which slot it is standing on.
///
/// **The order is invisible to both callers**, and that is the whole reason this
/// is safe: one writes `ranks[index]`, the other sorts by `(rank, index)`. See
/// [`minimum_weight_basis.h`](minimum_weight_basis.h) on where it would not be.
class SpanElements {
   public:
    SpanElements(const Field& field, const std::vector<Matrix>& slices)
        : field_(field),
          slices_(slices),
          radix_(static_cast<std::size_t>(field.characteristic())),
          place_(slices.size()),
          walk_(slices.size(), radix_) {
        // The all-zero combination, which is the string the walk starts on.
        linear_combination_into(field_, slices_, std::vector<int64_t>(slices_.size(), 0), element_);
        width_ = element_.entry_count();
        for (std::size_t position = 0, weight = 1; position < slices_.size(); ++position) {
            place_[position] = weight;
            weight *= radix_;
        }
    }

    /// Step to the next element, or say the span is used up. The index of the
    /// element now in `at()` is `index()`.
    bool advance() {
        ReflectedGrayWalk::Step step;
        if (!walk_.advance(step)) return false;

        // One digit moved by one, so one slice goes in or comes out. No
        // multiplication, and no rebuild.
        const Element* slice = slices_[step.digit].data();
        Element* entries = element_.data();
        if (step.upward) {
            index_ += place_[step.digit];
            for (std::size_t entry = 0; entry < width_; ++entry) {
                field_.addin(entries[entry], slice[entry]);
            }
        } else {
            index_ -= place_[step.digit];
            for (std::size_t entry = 0; entry < width_; ++entry) {
                field_.subin(entries[entry], slice[entry]);
            }
        }
        return true;
    }

    std::size_t index() const { return index_; }
    const Matrix& at() const { return element_; }

   private:
    const Field& field_;
    const std::vector<Matrix>& slices_;
    std::size_t radix_;
    std::size_t width_ = 0;
    /// `p^position`, so the index is carried rather than recovered.
    std::vector<std::size_t> place_;
    ReflectedGrayWalk walk_;
    Matrix element_;
    std::size_t index_ = 0;
};

}  // namespace

std::vector<std::size_t> span_element_ranks(const Field& field,
                                            const std::vector<Matrix>& slices) {
    const std::size_t combinations = span_size(field, slices.size());
    require_room("the ranks of a span of " + std::to_string(slices.size()) + " slices",
                 combinations, sizeof(std::size_t));

    std::vector<std::size_t> ranks(combinations);
    SpanElements walk(field, slices);
    ranks[0] = linear_algebra::rank(field, walk.at());
    while (walk.advance()) {
        ranks[walk.index()] = linear_algebra::rank(field, walk.at());
    }
    return ranks;
}

std::vector<Matrix> minimum_weight_basis(const Field& field, const std::vector<Matrix>& slices,
                                   const std::vector<std::size_t>& ranks_without_last) {
    const std::size_t width = linear_algebra::flattened_width<Field>(slices);
    const std::size_t dimension = linear_algebra::span_of(field, slices).dimension();
    const std::size_t combinations = span_size(field, slices.size());

    // Every element of the span, cheapest first. Index 0 is the zero
    // combination and is skipped: it can never enter a basis.
    //
    // Only the rank and the index are held. The element itself is rebuilt from
    // its index by `linear_combination` when the greedy actually reaches it, which is at
    // most `dimension` times. Holding the matrices instead costs
    // `p^slices * (56 + 8*n*m)` bytes: 134 MB for the sixteen slices of 4x4
    // matrix multiplication, against 1 MB this way.
    struct Candidate {
        std::size_t rank;
        std::size_t index;
    };
    require_room("the span of " + std::to_string(slices.size()) + " slices",
                 combinations - 1, sizeof(Candidate));

    std::vector<Candidate> candidates;
    candidates.reserve(combinations - 1);
    // The same Gray walk `span_element_ranks` takes, for the same saving: the
    // element is one slice away from the one before it rather than `k`
    // multiply-accumulates away from its own index. The walk starts on the
    // all-zero string, which is index 0 and is skipped here as it was before,
    // and visits every other index exactly once — so this pushes the same
    // `combinations - 1` candidates.
    //
    // **The elements a `ranks_without_last` covers are still not ranked**, which
    // is the whole of that optimisation; they are stepped over rather than
    // skipped, because the walk is what carries the element to the ones that
    // are. One pass over the entries against the `k` the rebuild cost.
    SpanElements walk(field, slices);
    while (walk.advance()) {
        const std::size_t index = walk.index();
        if (index < ranks_without_last.size()) {
            candidates.push_back({ranks_without_last[index], index});
            continue;
        }
        candidates.push_back({linear_algebra::rank(field, walk.at()), index});
    }

    // Sort by rank, ties broken by enumeration order, to ensure reproducible
    // results. **That is a total order on distinct indices**, so the sequence
    // the walk pushed them in cannot reach this answer: the sorted vector is the
    // one the index-order loop produced, entry for entry.
    std::sort(candidates.begin(), candidates.end(),
              [](const Candidate& left, const Candidate& right) {
                  if (left.rank != right.rank) return left.rank < right.rank;
                  return left.index < right.index;
              });

    std::vector<Matrix> basis;
    ReducedBasis span(field, width);
    for (const Candidate& candidate : candidates) {
        if (basis.size() == dimension) break;
        Matrix element = linear_combination(
            field, slices, coefficient_vector(candidate.index, slices.size(), field.characteristic()));
        if (span.try_add(element)) {
            basis.push_back(std::move(element));
        }
    }
    return basis;
}

}  // namespace bilinear_rank
