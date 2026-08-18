#include "gf2_leaf.h"

#include "candidate_pool.h"
#include "gf2_bits.h"
#include "memory_budget.h"
#include "span_basis.h"

namespace bilinear_rank {

bool gf2_leaf_applies(const Field& field, std::size_t columns) {
    return static_cast<std::size_t>(field.characteristic()) == 2 && columns <= 64;
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
template <typename Candidates>
Gf2Leaf<Candidates>::Gf2Leaf(const Candidates& pool, std::size_t rows, std::size_t columns)
    : pool_(pool),
      rows_(rows),
      columns_(columns),
      width_(rows * columns),
      words_per_map_(linear_algebra::gf2_word_count(rows * columns)) {
    const std::size_t bytes_each = sizeof(std::uint64_t) * words_per_map_;
    if (pool.size() == 0 || bytes_each > memory_budget() / pool.size()) return;

    table_.resize(pool.size() * words_per_map_);
    for (std::size_t index = 0; index < pool.size(); ++index) {
        linear_algebra::gf2_pack(pool[index].data(), width_, &table_[index * words_per_map_]);
    }
}

template <typename Candidates>
const std::uint64_t* Gf2Leaf<Candidates>::bits_of(std::size_t index,
                                                  std::vector<std::uint64_t>& buffer) const {
    if (!table_.empty()) return &table_[index * words_per_map_];
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

/// The subspace element at `index` is the exclusive or of the basis rows whose
/// bit is set in `index`, which is the binary digits the general path multiplies
/// by. Same order, same elements, same answer.
///
/// `elements` is below the pool size wherever this route is chosen, so the
/// dimension is well below the word width and the shift is always defined.
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
