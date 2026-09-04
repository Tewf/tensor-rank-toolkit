#pragma once

#include <cstddef>
#include <vector>

#include "dimacs_file.h"
#include "tensor_file.h"
#include "types.h"

/// `rank(T) ≤ r` over `GF(p)`, as a formula in conjunctive normal form.
///
/// The Boolean encoding works because over GF(2) a literal *is* a field
/// element. For a larger prime that is no longer true, and the field has to be
/// built out of Booleans: every unknown becomes `p` of them, exactly one true,
/// and every product and sum becomes the field's own table written out as
/// implications.
///
/// ```
/// a[l][i] is one-hot over GF(p)             p vars, 1 + p(p−1)/2 clauses
/// q[l][i][j] = a·b                          p vars, p² clauses
/// s[l][i][j][k] = s[l−1] + q·c              p vars, 2p² clauses
/// s[r][i][j][k] = t[i][j][k]                one unit clause
/// ```
///
/// so roughly `2p²` clauses per term per tensor entry, about nine times the
/// GF(2) cost at `p = 3`, which is the only larger prime this repository uses.
///
/// **This is the hand-written one of the two GF(p) backends, and the more
/// likely of them to be quietly wrong**: the multiplication table, the addition
/// chain and the one-hot constraints are all mine, and a mistake in any of them
/// produces a confident wrong rank rather than an error.
/// [`field_theory_encoding.h`](field_theory_encoding.h) exists to disagree with
/// it. Which of the two survives is settled by measurement, in
/// [`method/`](method/).
///
/// It accepts `p = 2` as well, where it should agree with the cheaper
/// [Boolean encoding](binary_encoding.h), which is a test rather than a use.
namespace satisfiability {

struct PrimeFieldEncoding {
    formats::Cnf formula;

    std::size_t characteristic = 0;
    std::size_t products = 0;
    std::size_t rows = 0;
    std::size_t columns = 0;
    std::size_t slices = 0;

    /// `left[(term * rows + row) * characteristic + value]` is the variable
    /// saying that entry takes that field value.
    std::vector<int> left;
    std::vector<int> right;
    std::vector<int> output;

    /// The variables standing for each field value of one unknown. Named rather
    /// than indexed at the call site, because the arithmetic is the same in
    /// three places and getting it wrong reads a neighbouring unknown.
    std::vector<int> left_group(std::size_t term, std::size_t row) const;
    std::vector<int> right_group(std::size_t term, std::size_t column) const;
    std::vector<int> output_group(std::size_t term, std::size_t slice) const;
};

/// Encode "is there a decomposition of `tensor` into `products` rank-one terms",
/// over whatever prime the tensor is written over.
///
/// Throws when the characteristic is not prime or is too large to write a table
/// for, and when the encoding does not fit the budget the
/// [Boolean encoding](binary_encoding.h) also answers to.
/// `break_symmetry` adds both symmetries this encoding has: the terms are
/// ordered, and the operand vectors are normalised so their first nonzero entry
/// is 1, which is the scaling freedom GF(2) does not have. Off by default, for
/// the reason [`symmetry_breaking.h`](symmetry_breaking.h) gives.
PrimeFieldEncoding encode_prime_rank_at_most(const formats::Tensor& tensor,
                                             std::size_t products,
                                             bool break_symmetry = false);

}  // namespace satisfiability
