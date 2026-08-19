#pragma once

#include <cstdint>
#include <vector>

#include "bilinear_rank_aliases.h"
#include "candidate_pool.h"
#include "gf2_bits.h"
#include "leaf_question.h"
#include "span_basis.h"

/// Building a [`LeafQuestion`](leaf_question.h) out of the repository's own
/// types: the pool's two vector lists, and a span to test membership in.
///
/// Nothing here is a second implementation of anything. The vector lists are
/// [`normalised_vectors`](../descent_search/candidate_pool.h)'s, so the index a
/// kernel derives its candidate from is the index `RankOnePool::at` would have
/// used, and the packing is [`gf2_pack`](../linear_algebra/gf2_bits.h)'s.
namespace gpu_leaf {

/// A vector of GF(2) entries as the low bits of one word.
inline std::uint32_t mask_of(const std::vector<std::int64_t>& entries) {
    std::uint32_t mask = 0;
    for (std::size_t index = 0; index < entries.size(); ++index) {
        if (entries[index] != 0) mask |= std::uint32_t(1) << index;
    }
    return mask;
}

/// The question, from the same two lists the pool is built from and the same
/// span the leaf was handed.
inline LeafQuestion packed_question(const bilinear_rank::Field& field, std::size_t rows,
                                    std::size_t columns, const bilinear_rank::ReducedBasis& span) {
    LeafQuestion question;
    question.rows = rows;
    question.columns = columns;
    question.width = rows * columns;
    question.words = linear_algebra::gf2_word_count(question.width);

    for (const std::vector<std::int64_t>& left : bilinear_rank::normalised_vectors(field, rows)) {
        question.left_masks.push_back(mask_of(left));
    }
    for (const std::vector<std::int64_t>& right :
         bilinear_rank::normalised_vectors(field, columns)) {
        question.right_masks.push_back(mask_of(right));
    }
    question.left_count = question.left_masks.size();
    question.right_count = question.right_masks.size();

    question.span_rows.resize(span.rows().size() * question.words);
    for (std::size_t row = 0; row < span.rows().size(); ++row) {
        std::uint64_t* target = &question.span_rows[row * question.words];
        linear_algebra::gf2_pack(span.rows()[row].data(), question.width, target);
        question.pivots.push_back(static_cast<std::uint32_t>(
            linear_algebra::gf2_lowest_set_bit(target, question.width)));
    }
    return question;
}

/// A pool index from a seed, so a question is the same one on every run.
inline std::size_t scattered_index(std::uint64_t& seed, std::size_t bound) {
    seed += 0x9E3779B97F4A7C15ull;
    std::uint64_t mixed = (seed ^ (seed >> 30)) * 0xBF58476D1CE4E5B9ull;
    mixed = (mixed ^ (mixed >> 27)) * 0x94D049BB133111EBull;
    return static_cast<std::size_t>((mixed ^ (mixed >> 31)) % bound);
}

/// Pool elements at scattered indices, added until the span has the dimension
/// asked for.
inline void widen_with_pool_elements(bilinear_rank::ReducedBasis& span,
                                     const bilinear_rank::RankOnePool& pool, std::size_t dimension,
                                     std::uint64_t seed) {
    while (span.dimension() < dimension) span.try_add(pool.at(scattered_index(seed, pool.size())));
}

/// A span of the requested dimension whose members are almost never in the
/// pool: the shape a real leaf has, where a scan tests four billion maps and
/// keeps a handful.
inline bilinear_rank::ReducedBasis sparse_span(const bilinear_rank::Field& field,
                                               const bilinear_rank::RankOnePool& pool,
                                               std::size_t width, std::size_t dimension) {
    bilinear_rank::ReducedBasis span(field, width);
    widen_with_pool_elements(span, pool, dimension, 0x1234567890ABCDEFull);
    return span;
}

/// A span containing every map whose only nonzero row is the first, so a scan
/// of it has `2^columns - 1` survivors instead of a handful.
///
/// It exists to check the survivor path rather than to time anything. A leaf
/// that keeps almost nothing exercises one branch of the kernel, and a
/// comparison that only ever compares two empty lists compares nothing.
inline bilinear_rank::ReducedBasis dense_span(const bilinear_rank::Field& field,
                                              const bilinear_rank::RankOnePool& pool,
                                              std::size_t rows, std::size_t columns,
                                              std::size_t dimension) {
    bilinear_rank::ReducedBasis span(field, rows * columns);
    for (std::size_t column = 0; column < columns; ++column) {
        bilinear_rank::Matrix map(rows, columns);
        map(0, column) = 1;
        span.try_add(map);
    }
    widen_with_pool_elements(span, pool, dimension, 0xC0FFEE1234567ull);
    return span;
}

}  // namespace gpu_leaf
