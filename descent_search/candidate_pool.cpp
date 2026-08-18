#include "candidate_pool.h"

#include "memory_budget.h"
#include "span_basis.h"

namespace bilinear_rank {

std::vector<Matrix> rank_one_candidates(const Field& field, const std::vector<Matrix>& slices) {
    std::vector<Matrix> candidates;
    for (const Matrix& slice : slices) {
        for (Matrix& term : linear_algebra::rank_one_decomposition(field, slice)) {
            candidates.push_back(std::move(term));
        }
    }
    return candidates;
}

/// Their outer products enumerate the rank-one maps without repeats.
std::vector<std::vector<int64_t>> normalised_vectors(const Field& field, std::size_t length) {
    const auto characteristic = static_cast<std::size_t>(field.characteristic());
    std::vector<std::vector<int64_t>> vectors;
    for (std::size_t leading = 0; leading < length; ++leading) {
        std::size_t combinations = 1;
        for (std::size_t position = leading + 1; position < length; ++position) {
            combinations *= characteristic;
        }
        for (std::size_t index = 0; index < combinations; ++index) {
            std::vector<int64_t> vector(length, 0);
            vector[leading] = 1;
            std::size_t remaining = index;
            for (std::size_t position = leading + 1; position < length; ++position) {
                vector[position] = static_cast<int64_t>(remaining % characteristic);
                remaining /= characteristic;
            }
            vectors.push_back(std::move(vector));
        }
    }
    return vectors;
}

RankOnePool::RankOnePool(const Field& field, std::size_t rows, std::size_t columns)
    : field_(field),
      rows_(rows),
      columns_(columns),
      lefts_(normalised_vectors(field, rows)),
      rights_(normalised_vectors(field, columns)) {}

Matrix RankOnePool::at(std::size_t index) const {
    const std::vector<int64_t>& left = lefts_[index / rights_.size()];
    const std::vector<int64_t>& right = rights_[index % rights_.size()];

    Matrix map(rows_, columns_);
    for (std::size_t row = 0; row < rows_; ++row) {
        for (std::size_t column = 0; column < columns_; ++column) {
            field_.mul(map(row, column), left[row], right[column]);
        }
    }
    return map;
}

std::vector<Matrix> all_rank_one_maps(const Field& field, std::size_t rows, std::size_t columns) {
    const RankOnePool pool(field, rows, columns);

    // Asked before it is taken. This is the allocation that grows fastest with
    // the shape: 225 maps for 4x4, 261 121 for 9x9, and 4.3e9 for the 16x16
    // slices of 4x4 matrix multiplication, which is 8.2 TiB. The pool object
    // above costs the two vector lists and is what a caller reaches for when
    // this refuses.
    require_room("the pool of rank-one " + std::to_string(rows) + "x" + std::to_string(columns) +
                     " maps",
                 pool.size(), bytes_per_matrix(rows * columns));

    std::vector<Matrix> maps;
    maps.reserve(pool.size());
    for (std::size_t index = 0; index < pool.size(); ++index) maps.push_back(pool.at(index));
    return maps;
}

bool row_space_contains(const Field& field, const Matrix& outer, const Matrix& inner) {
    linear_algebra::SpanBasis<Field> rows(field, outer.columns());
    for (std::size_t row = 0; row < outer.rows(); ++row) rows.try_add(outer.row(row));
    for (std::size_t row = 0; row < inner.rows(); ++row) {
        if (!rows.contains(inner.row(row))) return false;
    }
    return true;
}

bool has_equivalent(const Field& field, const std::vector<Matrix>& known, const Matrix& candidate) {
    for (const Matrix& seen : known) {
        if (row_space_contains(field, seen, candidate)) return true;
    }
    return false;
}

std::vector<Matrix> one_per_row_space(const Field& field, const std::vector<Matrix>& pool) {
    std::vector<Matrix> representatives;
    for (const Matrix& candidate : pool) {
        if (!has_equivalent(field, representatives, candidate)) representatives.push_back(candidate);
    }
    return representatives;
}

}  // namespace bilinear_rank
