#include "gf2_leaf.h"

#include <algorithm>
#include <bit>

#include "candidate_pool.h"
#include "gf2_bits.h"
#include "memory_budget.h"
#include "span_basis.h"

namespace bilinear_rank {

namespace {
bool offered = true;
}  // namespace

void set_gf2_leaf_offered(bool wanted) { offered = wanted; }
bool gf2_leaf_offered() { return offered; }

bool gf2_leaf_applies(const Field& field, std::size_t columns) {
    return offered && static_cast<std::size_t>(field.characteristic()) == 2 && columns <= 64;
}

/// Packing the pool once is most of what the representation is worth: a
/// membership test then costs one XOR per basis row and no conversion at all.
///
/// It is priced against the same budget the materialised pool is priced against,
/// and skipped rather than refused when it does not fit, because an addressed
/// pool exists exactly for the shapes where nothing pool-sized may be held. At
/// `<4,4,4>` the table would be 4.3e9 maps of four words, which is 137 GiB, so
/// there the maps are packed one at a time into the caller's buffer and the win
/// is the reduction alone.
namespace {

/// A vector of GF(2) entries as the low bits of one word. Widths above 64 do not
/// reach here: `gf2_leaf_applies` refuses them.
std::uint64_t mask_of(const std::vector<int64_t>& entries) {
    std::uint64_t bits = 0;
    for (std::size_t index = 0; index < entries.size(); ++index) {
        if (entries[index] != 0) bits |= std::uint64_t(1) << index;
    }
    return bits;
}

/// The longest vector whose whole list is worth holding.
///
/// Over GF(2) a list of normalised vectors is every nonzero one, so it is
/// `2^length - 1` long: exponential in a shape this file does not choose, and
/// `gf2_leaf_applies` allows 64 columns, whose list would be 1.8e19 vectors.
/// Twenty is 1 048 575 of them, sixteen times the widest list any shape this
/// repository prices asks for — 65 535 for the 16x16 slices of `<4,4,4>` — and a
/// shape past it scans the pool directly rather than allocating what no machine
/// has. Nothing here reaches it; it is a ceiling on an exponential and not a
/// tuning knob, which is why it is not in `tunables.conf`.
constexpr std::size_t longest_vector_listed = 20;

/// No right-hand vector's index, for an entry of the inverse nothing claimed.
constexpr std::uint32_t unclaimed = ~std::uint32_t(0);

}  // namespace

template <typename Candidates>
Gf2Leaf<Candidates>::Gf2Leaf(const Field& field, const Candidates& pool, std::size_t rows,
                             std::size_t columns)
    : pool_(pool),
      rows_(rows),
      columns_(columns),
      width_(rows * columns),
      words_per_map_(linear_algebra::gf2_word_count(rows * columns)) {
    if (pool.size() == 0) return;
    note_the_grid(field);

    const std::size_t bytes_each = sizeof(std::uint64_t) * words_per_map_;
    // No table, so every element is formed on demand, which `bits_of` does from
    // the two lists above where there are any and out of the pool where not.
    if (bytes_each > memory_budget() / pool.size()) return;

    table_.resize(pool.size() * words_per_map_);
    for (std::size_t index = 0; index < pool.size(); ++index) {
        linear_algebra::gf2_pack(pool[index].data(), width_, &table_[index * words_per_map_]);
    }
}

/// The pool as the grid of outer products it is, when it is one.
///
/// Element `left * right_count + right` is `lefts[left]` against
/// `rights[right]`, which is the order `RankOnePool::at` indexes and
/// `all_rank_one_maps` builds in. A caller may hand over any vector of maps, so
/// the grid is checked rather than assumed, and the two lists are dropped when
/// the check fails.
///
/// **Asked whether or not the table fits.** The lists used to be built only
/// where the table did not, since forming an element in bits was all they were
/// for; the scan below wants them everywhere, and they are cheap against the
/// grid they describe — 511 masks a side at 9x9, against the 261 121-map table
/// that route keeps.
template <typename Candidates>
void Gf2Leaf<Candidates>::note_the_grid(const Field& field) {
    if (rows_ > longest_vector_listed || columns_ > longest_vector_listed) return;

    const std::vector<std::vector<int64_t>> lefts = normalised_vectors(field, rows_);
    const std::vector<std::vector<int64_t>> rights = normalised_vectors(field, columns_);
    if (lefts.size() * rights.size() != pool_.size()) return;

    left_masks_.reserve(lefts.size());
    right_masks_.reserve(rights.size());
    for (const std::vector<int64_t>& left : lefts) left_masks_.push_back(mask_of(left));
    for (const std::vector<int64_t>& right : rights) right_masks_.push_back(mask_of(right));
    right_count_ = rights.size();

    invert_the_right_masks();
}

/// Which right-hand vector carries each pattern, so an element found by its bits
/// can be named by its pool index.
///
/// The scan below walks patterns and owes the caller a map at a pool index, and
/// **`normalised_vectors` does not enumerate patterns in increasing order**: it
/// goes by leading nonzero position, so `right_masks_[j]` is `1, 3, 5, ...` and
/// not `j + 1`. Over GF(2) a normalised vector is any nonzero one, so the
/// patterns are exactly `1 .. 2^columns - 1` and one entry per pattern inverts
/// the whole list: 256 KB at the sixteen columns of `<4,4,4>`, against the
/// 16 MB of masks it indexes.
///
/// **That bijection is checked and not assumed.** It is a fact about a function
/// in another module at one characteristic, and were it ever to stop holding,
/// this would name a survivor by the wrong index — a wrong answer, and not a
/// crash. A list that is not a bijection leaves the inverse empty, and a scan
/// with no inverse is the direct one.
template <typename Candidates>
void Gf2Leaf<Candidates>::invert_the_right_masks() {
    const std::size_t patterns = std::size_t(1) << columns_;
    if (right_count_ != patterns - 1) return;

    right_index_of_mask_.assign(patterns, unclaimed);
    for (std::size_t right = 0; right < right_count_; ++right) {
        const std::uint64_t mask = right_masks_[right];
        if (mask == 0 || mask >= patterns || right_index_of_mask_[mask] != unclaimed) {
            right_index_of_mask_.clear();
            return;
        }
        right_index_of_mask_[mask] = static_cast<std::uint32_t>(right);
    }
}

template <typename Candidates>
const std::uint64_t* Gf2Leaf<Candidates>::bits_of(std::size_t index,
                                                  std::vector<std::uint64_t>& buffer) const {
    if (!table_.empty()) return &table_[index * words_per_map_];
    if (!left_masks_.empty()) {
        const std::uint64_t left = left_masks_[index / right_count_];
        const std::uint64_t right = right_masks_[index % right_count_];
        for (std::size_t word = 0; word < words_per_map_; ++word) buffer[word] = 0;
        for (std::size_t row = 0; row < rows_; ++row) {
            if (((left >> row) & 1) == 0) continue;
            const std::size_t start = row * columns_;
            const std::size_t offset = start % 64;
            buffer[start / 64] |= right << offset;
            // Only when the row straddles a word, which also makes `offset`
            // nonzero, since `columns_ <= 64` is the precondition above.
            if (offset + columns_ > 64) buffer[start / 64 + 1] |= right >> (64 - offset);
        }
        return buffer.data();
    }
    linear_algebra::gf2_pack(pool_[index].data(), width_, buffer.data());
    return buffer.data();
}

/// The span again, in bits.
///
/// Its rows arrive already in reduced row echelon form, so `try_add` finds the
/// same pivots and changes no row on the way through: this is a repacking and
/// not a second elimination. Costing one per leaf against a whole pool scan, it
/// does not show up in a measurement.
template <typename Candidates>
linear_algebra::Gf2SpanBasis Gf2Leaf<Candidates>::packed(const ReducedBasis& span) const {
    linear_algebra::Gf2SpanBasis reachable(width_);
    std::vector<std::uint64_t> row(words_per_map_), scratch(words_per_map_);
    for (const std::vector<Element>& entries : span.rows()) {
        linear_algebra::gf2_pack(entries.data(), width_, row.data());
        reachable.try_add(row.data(), scratch);
    }
    return reachable;
}

template <typename Candidates>
Matrix Gf2Leaf<Candidates>::unpacked(const std::uint64_t* words) const {
    Matrix map(rows_, columns_);
    linear_algebra::gf2_unpack(words, width_, map.data());
    return map;
}

template <typename Candidates>
std::vector<Matrix> Gf2Leaf<Candidates>::by_scanning_the_pool(const ReducedBasis& span,
                                                              std::size_t needed,
                                                              SearchBudget* budget) const {
    if (!carries_a_residual()) return by_scanning_the_pool_directly(span, needed, budget);
    return by_carrying_a_residual(span, needed, budget);
}

template <typename Candidates>
std::vector<Matrix> Gf2Leaf<Candidates>::by_scanning_the_pool_directly(const ReducedBasis& span,
                                                                       std::size_t needed,
                                                                       SearchBudget* budget) const {
    const linear_algebra::Gf2SpanBasis reachable = packed(span);
    linear_algebra::Gf2SpanBasis independent(width_);
    std::vector<std::uint64_t> scratch(words_per_map_), buffer(words_per_map_);

    std::vector<Matrix> found;
    for (std::size_t index = 0; index < pool_.size(); ++index) {
        // Once what is left cannot reach the target, the answer is already no.
        if (found.size() + (pool_.size() - index) < needed) break;
        // Whereas this break withdraws the answer rather than giving one.
        if (budget != nullptr && !budget->may_examine(index)) break;
        const std::uint64_t* map = bits_of(index, buffer);
        if (!reachable.contains(map, scratch)) continue;
        if (independent.try_add(map, scratch)) {
            // The matrix, not the bits: the caller is owed the products.
            found.push_back(pool_[index]);
            if (found.size() == needed) break;
        }
    }
    return found;
}

/// Where each column of a right-hand vector lands in the span, for one left
/// vector `left`.
///
/// The bits of `left (x) e_column` are one per set bit of `left`, at
/// `row * columns + column`, and reducing each of those against the span is the
/// whole per-left cost of the scan below: `columns` reductions where a direct
/// scan pays one per element.
template <typename Candidates>
void Gf2Leaf<Candidates>::residuals_of_one_left(const linear_algebra::Gf2SpanBasis& reachable,
                                                std::uint64_t left,
                                                std::vector<std::uint64_t>& per_column) const {
    for (std::size_t column = 0; column < columns_; ++column) {
        std::uint64_t* residual = &per_column[column * words_per_map_];
        for (std::size_t word = 0; word < words_per_map_; ++word) residual[word] = 0;
        for (std::size_t row = 0; row < rows_; ++row) {
            if (((left >> row) & 1) == 0) continue;
            const std::size_t bit = row * columns_ + column;
            residual[bit / 64] |= std::uint64_t(1) << (bit % 64);
        }
        reachable.reduce(residual);
    }
}

/// The same scan with the reduction hoisted out of it: `O(1)` an element where
/// the direct scan is `O(dim)`, and no element ever formed.
///
/// **Reduction modulo a subspace is linear.** The rows arrive in reduced row
/// echelon form, where a pivot appears in its own row and nowhere else, so
/// `reduce` tests the candidate's own bit at each pivot and never a bit some
/// earlier row changed: it is `x` plus a fixed subset of the rows chosen by `x`,
/// which is a linear map. Hence `reduce(x ^ y) == reduce(x) ^ reduce(y)`.
///
/// **And the pool is a grid.** Element `left * right_count + right` is
/// `left_masks_[left] (x) right_masks_[right]`, the outer product is linear in
/// its right operand, and so for a fixed left vector the reduction of the whole
/// row of the grid is generated by `columns` reductions. Walking the patterns in
/// reflected Gray code order changes one column per step, so each element costs
/// one exclusive or onto a running residual and a test for zero — and a residual
/// of zero is exactly `reachable.contains` returning true.
///
/// **Four things make it the same answer and not merely an equivalent one**,
/// which is what `tests/test_residual_scan.cpp` asserts map by map:
///
///  - `try_add` is order-dependent, so the greedy must see the survivors in pool
///    index order. Gray order is not that order, so each left vector is done in
///    two phases: walk, then sort what survived, then offer them. Pool index is
///    `left * right_count + right`, so ascending lefts with ascending rights
///    inside is index order exactly.
///  - A survivor is owed as a map at an index, and `right_index_of_mask_` is
///    what turns a pattern back into one.
///  - The budget is spent per element examined, so the second phase walks every
///    index of the left vector's row and not only the survivors, and asks
///    `may_examine` once apiece with the same running count the direct scan
///    passes. A leaf abandons at the same element either way.
///  - The early exit is the same expression at the same index. It is also
///    asked once before a Gray walk, where it can only give the same answer it
///    would give at that walk's first index, so a walk whose whole row is
///    already out of reach is not taken.
template <typename Candidates>
std::vector<Matrix> Gf2Leaf<Candidates>::by_carrying_a_residual(const ReducedBasis& span,
                                                                std::size_t needed,
                                                                SearchBudget* budget) const {
    const linear_algebra::Gf2SpanBasis reachable = packed(span);
    linear_algebra::Gf2SpanBasis independent(width_);
    std::vector<std::uint64_t> scratch(words_per_map_), buffer(words_per_map_);

    const std::size_t patterns = std::size_t(1) << columns_;
    std::vector<std::uint64_t> per_column(columns_ * words_per_map_);
    std::vector<std::uint64_t> residual(words_per_map_);
    std::vector<std::size_t> survivors;

    std::vector<Matrix> found;
    for (std::size_t left = 0; left < left_masks_.size(); ++left) {
        const std::size_t base = left * right_count_;
        if (found.size() + (pool_.size() - base) < needed) break;
        residuals_of_one_left(reachable, left_masks_[left], per_column);

        // The empty pattern reduces to zero and is no vector's, so the walk
        // starts there and counts from the step after it.
        survivors.clear();
        for (std::uint64_t& word : residual) word = 0;
        std::size_t previous = 0;
        for (std::size_t step = 1; step < patterns; ++step) {
            const std::size_t pattern = step ^ (step >> 1);
            const auto column = static_cast<std::size_t>(std::countr_zero(pattern ^ previous));
            previous = pattern;
            linear_algebra::gf2_xor(residual.data(), &per_column[column * words_per_map_],
                                    words_per_map_);
            if (linear_algebra::gf2_is_zero(residual.data(), words_per_map_)) {
                survivors.push_back(right_index_of_mask_[pattern]);
            }
        }
        std::sort(survivors.begin(), survivors.end());

        std::size_t next = 0;
        for (std::size_t right = 0; right < right_count_; ++right) {
            const std::size_t index = base + right;
            if (found.size() + (pool_.size() - index) < needed) return found;
            if (budget != nullptr && !budget->may_examine(index)) return found;
            if (next == survivors.size() || survivors[next] != right) continue;
            ++next;
            if (independent.try_add(bits_of(index, buffer), scratch)) {
                // The matrix, not the bits: the caller is owed the products.
                found.push_back(pool_[index]);
                if (found.size() == needed) return found;
            }
        }
    }
    return found;
}

namespace {

/// How many of the lowest basis rows one batch of the walk varies.
///
/// A batch is `2 ^ digits_per_batch` consecutive elements: everything the Gray
/// order below is confined to, and everything the survivor list has to hold. It
/// is a bound on a buffer and not a tuning knob, which is why it is not in
/// `tunables.conf` — each step of it doubles a 1 KB list and halves a per-batch
/// cost that is already a copy of four words, at the widest shape this
/// repository prices, against 256 elements.
constexpr std::size_t digits_per_batch = 8;

}  // namespace

/// The same walk with the rebuild hoisted out of it: one exclusive or an
/// element, where rebuilding from the digits of an index is `O(dim)` of them
/// plus a buffer to clear.
///
/// **Consecutive strings of a reflected Gray code differ in one digit.** Over
/// GF(2) that code is `G(i) = i ^ (i >> 1)`, and the digit that changes between
/// `G(i - 1)` and `G(i)` is the lowest set bit of `i`, so a running combination
/// of basis rows is carried from one element to the next by a single
/// `gf2_xor`. That is `dim / 2` exclusive ors and one buffer clear saved
/// apiece, so what it is worth grows with the dimension.
///
/// **The radix-`p` machinery is deliberately not reused.**
/// [`reflected_gray_walk.h`](reflected_gray_walk.h) carries the same order for a
/// general characteristic, where a successor has to know which way each digit is
/// travelling and needs Knuth's focus pointers to stay `O(1)`. At radix two a
/// digit only ever flips, the reflection is `i ^ (i >> 1)` in closed form, and
/// the digit to move is `countr_zero(i)` — one instruction, no state, nothing to
/// keep in step with the walk. Instantiating the class here would buy a shared
/// name and pay two vectors and an indirection per element for it.
///
/// **The order is the whole risk, and the batching is what answers it.**
/// `Gf2SpanBasis::try_add` accepts candidates in the order it is offered them,
/// so the rank-one basis handed back is a fact about that order, and Gray order
/// is not index order. A leaf here can be 134 million elements, so the survivors
/// cannot simply be collected and sorted the way
/// `by_carrying_a_residual` sorts one left vector's row.
///
/// So the walk is cut into batches of `2 ^ digits_per_batch` elements and each
/// batch is drained in index order:
///
///  - **A batch is a contiguous, aligned block of indices, and the batches run
///    in increasing order.** That is why the Gray order is over the *low*
///    `digits_per_batch` digits only, with the digits above them counted
///    upward in plain binary. Cutting one global Gray walk into runs of
///    `2 ^ k` steps would not do it: run `m` of such a walk covers the index
///    block `G(m)`, so the blocks themselves arrive in Gray order — for
///    `k = 2` over four digits, blocks 0, 1, 3, 2 — and the greedy would see
///    block 3 before block 2.
///  - **So a batch boundary cannot change which element the greedy sees
///    first.** The drained indices are `1, 2, ... elements - 1` with no gap and
///    no repeat, exactly the sequence the rebuild offers, and the Gray order
///    never reaches `try_add` at all: it decides only *whether* an element is
///    rank one, which is a property of the element and not of when it was
///    visited.
///  - **The budget is spent per element examined**, so the drain asks
///    `may_examine` once at every index of the batch and not only at the
///    survivors, with the same running count the rebuild passes. A leaf
///    abandons at the same element either way. It is skipped only where there
///    is no budget to spend and no survivor to offer, where it has nothing to
///    do.
///  - **A survivor is formed again from the batch's first element and the
///    digits its offset names.** The same rows and the same exclusive or, so
///    the same words the walk tested, and at most `digits_per_batch` of them
///    for something that happens once per survivor rather than once per
///    element.
///
/// The batch's own first element is carried across boundaries too: stepping
/// `first` on by one batch is a binary increment of the digits above
/// `digits_per_batch`, which flips the trailing ones and the zero above them —
/// two rows on average, once per batch.
///
/// `elements` is below the pool size wherever this route is chosen, so the
/// dimension is well below the word width and the shift is always defined. It
/// need not be `2 ^ dim`: a final batch reaching past it is walked whole and
/// drained only as far as `elements`, which costs a few rank tests nobody
/// examines and keeps the batch a power of two, which is what makes the Gray
/// order cover it exactly once.
template <typename Candidates>
std::vector<Matrix> Gf2Leaf<Candidates>::by_walking_the_subspace(const ReducedBasis& span,
                                                                 std::size_t needed,
                                                                 std::size_t elements,
                                                                 SearchBudget* budget) const {
    const std::vector<std::vector<Element>>& basis = span.rows();
    std::vector<std::uint64_t> rows(basis.size() * words_per_map_);
    for (std::size_t index = 0; index < basis.size(); ++index) {
        linear_algebra::gf2_pack(basis[index].data(), width_, &rows[index * words_per_map_]);
    }

    linear_algebra::Gf2SpanBasis independent(width_);
    std::vector<std::uint64_t> combination(words_per_map_), scratch(words_per_map_);
    // The batch's first element, and the buffer a survivor is formed back into.
    std::vector<std::uint64_t> batch_start(words_per_map_), element(words_per_map_);

    // A basis shorter than the batch is one batch: the digits the Gray order
    // varies are exactly the ones there are, and no boundary is ever crossed.
    const std::size_t digits = std::min(basis.size(), digits_per_batch);
    const std::size_t batch = std::size_t(1) << digits;
    std::vector<std::uint32_t> survivors;
    survivors.reserve(batch);

    std::vector<Matrix> found;
    for (std::size_t first = 0; first < elements; first += batch) {
        if (first != 0) {
            const std::size_t batches_done = first >> digits;
            const auto flipped = static_cast<std::size_t>(std::countr_zero(batches_done));
            for (std::size_t step = 0; step <= flipped; ++step) {
                linear_algebra::gf2_xor(batch_start.data(),
                                        &rows[(digits + step) * words_per_map_], words_per_map_);
            }
        }

        survivors.clear();
        std::copy(batch_start.begin(), batch_start.end(), combination.begin());
        // Offset zero is the batch's first element, which no step of the walk
        // below lands on. In the first batch it is the zero map, which the
        // rebuild never examined either.
        if (first != 0 && linear_algebra::gf2_is_rank_one(combination.data(), rows_, columns_)) {
            survivors.push_back(0);
        }
        for (std::size_t step = 1; step < batch; ++step) {
            const auto digit = static_cast<std::size_t>(std::countr_zero(step));
            linear_algebra::gf2_xor(combination.data(), &rows[digit * words_per_map_],
                                    words_per_map_);
            if (linear_algebra::gf2_is_rank_one(combination.data(), rows_, columns_)) {
                survivors.push_back(static_cast<std::uint32_t>(step ^ (step >> 1)));
            }
        }
        // Gray order into index order, over a list of at most `batch` entries.
        std::sort(survivors.begin(), survivors.end());
        if (survivors.empty() && budget == nullptr) continue;

        std::size_t next = 0;
        for (std::size_t offset = (first == 0 ? 1 : 0); offset < batch; ++offset) {
            const std::size_t index = first + offset;
            if (index >= elements) return found;
            if (budget != nullptr && !budget->may_examine(index)) return found;
            if (next == survivors.size() || survivors[next] != offset) continue;
            ++next;

            std::copy(batch_start.begin(), batch_start.end(), element.begin());
            for (std::size_t digit = 0; digit < digits; ++digit) {
                if (((offset >> digit) & 1) == 0) continue;
                linear_algebra::gf2_xor(element.data(), &rows[digit * words_per_map_],
                                        words_per_map_);
            }
            if (independent.try_add(element.data(), scratch)) {
                found.push_back(unpacked(element.data()));
                if (found.size() == needed) return found;
            }
        }
    }
    return found;
}

/// The subspace element at `index` is the exclusive or of the basis rows whose
/// bit is set in `index`, which is the binary digits the general path multiplies
/// by. Same order, same elements, same answer.
///
/// `elements` is below the pool size wherever this route is chosen, so the
/// dimension is well below the word width and the shift is always defined.
template <typename Candidates>
std::vector<Matrix> Gf2Leaf<Candidates>::by_rebuilding_each_element(const ReducedBasis& span,
                                                                    std::size_t needed,
                                                                    std::size_t elements,
                                                                    SearchBudget* budget) const {
    const std::vector<std::vector<Element>>& basis = span.rows();
    std::vector<std::uint64_t> rows(basis.size() * words_per_map_);
    for (std::size_t index = 0; index < basis.size(); ++index) {
        linear_algebra::gf2_pack(basis[index].data(), width_, &rows[index * words_per_map_]);
    }

    linear_algebra::Gf2SpanBasis independent(width_);
    std::vector<std::uint64_t> combination(words_per_map_), scratch(words_per_map_);

    std::vector<Matrix> found;
    for (std::size_t index = 1; index < elements; ++index) {
        if (budget != nullptr && !budget->may_examine(index)) break;
        for (std::uint64_t& word : combination) word = 0;
        for (std::size_t digit = 0; digit < basis.size(); ++digit) {
            if (((index >> digit) & 1) == 0) continue;
            linear_algebra::gf2_xor(combination.data(), &rows[digit * words_per_map_],
                                    words_per_map_);
        }
        if (!linear_algebra::gf2_is_rank_one(combination.data(), rows_, columns_)) continue;

        if (independent.try_add(combination.data(), scratch)) {
            found.push_back(unpacked(combination.data()));
            if (found.size() == needed) break;
        }
    }
    return found;
}

template class Gf2Leaf<std::vector<Matrix>>;
template class Gf2Leaf<Addressed>;

}  // namespace bilinear_rank
