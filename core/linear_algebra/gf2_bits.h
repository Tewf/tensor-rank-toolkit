#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>

/// A matrix over GF(2) as one bit per entry, packed into 64-bit words.
///
/// Everything else in this layer carries a field element as an `int64_t`, which
/// is what makes one implementation serve GF(p) and the rationals at once. Over
/// GF(2) that is 64 bits to hold one, and the arithmetic is worse than the
/// storage: adding two vectors is a loop of Givaro `axpyin` calls where the
/// field itself only offers exclusive or.
///
/// So this is the GF(2) case and nothing else. The bit at index `i` of a
/// flattened `rows x columns` matrix is bit `i % 64` of word `i / 64`, and `i`
/// is `row * columns + column`, which is the index
/// [`SpanBasis::flatten`](span_basis.h) already uses. The two orders agreeing is
/// what lets a packed span and an `int64_t` one answer the same question.
///
/// Free functions over a raw `uint64_t*` rather than a class, because the users
/// hold their words in one flat block: a pool of packed maps is one vector, not
/// a vector of vectors, and that is most of what the representation is worth.
namespace linear_algebra {

/// Words needed to hold `bits` bits.
inline std::size_t gf2_word_count(std::size_t bits) { return (bits + 63) / 64; }

/// The low `width` bits set. `width` is at most 64; a shift by the word width is
/// undefined, which is why the wide case is written out rather than shifted.
inline std::uint64_t gf2_mask(std::size_t width) {
    return width >= 64 ? ~std::uint64_t(0) : (std::uint64_t(1) << width) - 1;
}

/// Pack `count` field elements into `words`, which must hold
/// `gf2_word_count(count)` of them. Any previous contents are overwritten, so a
/// caller can reuse one buffer for every candidate it packs.
template <class Element>
void gf2_pack(const Element* entries, std::size_t count, std::uint64_t* words) {
    for (std::size_t word = 0; word < gf2_word_count(count); ++word) words[word] = 0;
    for (std::size_t index = 0; index < count; ++index) {
        if (entries[index] != 0) words[index / 64] |= std::uint64_t(1) << (index % 64);
    }
}

/// Unpack back into `count` field elements, which is how a packed answer becomes
/// a `Matrix` again.
template <class Element>
void gf2_unpack(const std::uint64_t* words, std::size_t count, Element* entries) {
    for (std::size_t index = 0; index < count; ++index) {
        entries[index] = static_cast<Element>((words[index / 64] >> (index % 64)) & 1);
    }
}

inline bool gf2_bit(const std::uint64_t* words, std::size_t index) {
    return ((words[index / 64] >> (index % 64)) & 1) != 0;
}

inline bool gf2_is_zero(const std::uint64_t* words, std::size_t word_count) {
    for (std::size_t word = 0; word < word_count; ++word) {
        if (words[word] != 0) return false;
    }
    return true;
}

/// `target ^= source`, which over GF(2) is `target += source`.
inline void gf2_xor(std::uint64_t* target, const std::uint64_t* source, std::size_t word_count) {
    for (std::size_t word = 0; word < word_count; ++word) target[word] ^= source[word];
}

/// The index of the lowest set bit, or `width` when there is none. This is the
/// pivot column of a reduced vector.
inline std::size_t gf2_lowest_set_bit(const std::uint64_t* words, std::size_t width) {
    for (std::size_t word = 0; word < gf2_word_count(width); ++word) {
        if (words[word] == 0) continue;
        return word * 64 + static_cast<std::size_t>(std::countr_zero(words[word]));
    }
    return width;
}

/// One row of a packed `rows x columns` matrix, as the low `columns` bits of a
/// word.
///
/// A row straddles two words unless the columns divide the word width, so the
/// second word is read only when the row really reaches into it. `columns` must
/// be at most 64, which every caller here checks before packing anything: it is
/// 16 for the 4x4 slices of `<4,4,4>`, the widest shape this repository prices.
inline std::uint64_t gf2_row(const std::uint64_t* words, std::size_t columns, std::size_t row) {
    const std::size_t start = row * columns;
    const std::size_t offset = start % 64;
    std::uint64_t value = words[start / 64] >> offset;
    if (offset + columns > 64) value |= words[start / 64 + 1] << (64 - offset);
    return value & gf2_mask(columns);
}

/// Whether a packed matrix has rank exactly one.
///
/// Over GF(2) the only nonzero scalar is 1, so a nonzero matrix has rank one
/// exactly when all of its nonzero rows are the same row. That is one comparison
/// per row against `Θ(r·d·c)` field operations for the general
/// [`rank`](measures.h), and it is the test every element of a walked subspace
/// is put through.
inline bool gf2_is_rank_one(const std::uint64_t* words, std::size_t rows, std::size_t columns) {
    std::uint64_t leading = 0;
    for (std::size_t row = 0; row < rows; ++row) {
        const std::uint64_t entries = gf2_row(words, columns, row);
        if (entries == 0) continue;
        if (leading == 0) {
            leading = entries;
        } else if (entries != leading) {
            return false;
        }
    }
    return leading != 0;
}

/// The rank of a packed matrix, in machine words.
///
/// The same elimination [`rank`](measures.h) runs, over the same field, with a
/// row held as the low `columns` bits of one word instead of as `columns`
/// `int64_t`s. Clearing a row against a pivot is then one exclusive or where
/// the general path spends one Givaro `axpyin` per column, and the
/// normalisation that path does by inverting the pivot is a no-op, since over
/// GF(2) the only nonzero scalar is 1. So the two pick the same pivots and
/// count the same rank.
///
/// `reduced[column]` holds the row whose lowest set bit is `column`, which is
/// that row's pivot. A row is cleared against the row holding its own lowest
/// bit, and that exclusive or can only move the lowest bit upwards, so the loop
/// ends: at worst on a column nothing holds yet, where the row becomes a pivot,
/// and at best on zero, where the row was already spanned. That is `O(rank)`
/// word operations a row, against the `O(rank * columns)` field operations the
/// general path spends.
///
/// `columns` must be at most 64, which is [`gf2_row`](gf2_bits.h)'s limit and
/// what every caller checks before it packs anything. Only the first `columns`
/// entries of `reduced` are ever addressed, because `gf2_row` masks the rest
/// away, so only those are cleared.
inline std::size_t gf2_rank(const std::uint64_t* words, std::size_t rows, std::size_t columns) {
    if (rows == 0 || columns == 0) return 0;

    std::uint64_t reduced[64];
    for (std::size_t column = 0; column < columns; ++column) reduced[column] = 0;

    std::size_t rank = 0;
    for (std::size_t row = 0; row < rows; ++row) {
        std::uint64_t value = gf2_row(words, columns, row);
        while (value != 0) {
            const auto pivot = static_cast<std::size_t>(std::countr_zero(value));
            if (reduced[pivot] == 0) {
                reduced[pivot] = value;
                ++rank;
                break;
            }
            value ^= reduced[pivot];
        }
    }
    return rank;
}

}  // namespace linear_algebra
