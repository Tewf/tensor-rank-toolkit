#pragma once

#include <cstdint>
#include <string>

#include "binary_encoding.h"

/// Heule's instance-shaping devices for a matrix-multiplication question, as
/// unit clauses on the encoding's triple variables (`[heule2021]` section 3,
/// dissected in [shaped-encodings/review.md](shaped-encodings/review.md)).
///
/// **Every device here is a streamliner**: it keeps a slice of the solution
/// space and discards the rest, so a shaped formula's no - or a run that
/// exhausts it - proves NOTHING about the unshaped question. A found model
/// still certifies, which is the only claim this class of instance can make.
namespace satisfiability {

/// The `<n, m, p>` a matmul tensor was built from. Only `inner` need be given:
/// `n` and `p` follow from the encoding's mode sizes and are validated there.
struct MatmulShape {
    std::size_t rows = 0;     // n
    std::size_t inner = 0;    // m
    std::size_t columns = 0;  // p
};

struct Streamliners {
    /// Distribute the type-3 terms (the odd Brent equations' summands) over the
    /// products under the quota of `[heule2019]`: with T3 odd entries and r
    /// products, T3 - r products carry two and the rest one, which is the
    /// hardcoded 19x1 + 4x2 of their 3x3x23 instances.
    bool pair_type3 = false;

    /// Force this share of the type-0/1/2 (entry, product) terms to zero,
    /// "motivated by the observation that in most of the known solutions,
    /// almost all these terms are zero"; their choice was one half.
    double zero_fraction = 0.0;

    /// Neighborhood fixing, their strongest device: instantiate this share of
    /// the alpha/beta/gamma variables from a known scheme (their choice ~50 %).
    /// `fixing_scheme` is a file of the products' coefficient matrices as
    /// nested braces of integers (the FastMatrixMultiplication .m layout);
    /// coefficients are read modulo 2, and the scheme is verified against
    /// every Brent parity before a single variable is fixed.
    std::string fixing_scheme;
    double fixing_fraction = 0.0;

    std::uint64_t seed = 1;
};

/// Applies the requested devices to a GF(2) matmul encoding in place.
///
/// Throws when `inner` does not divide the encoding's modes, when the entry
/// types contradict the tensor (the shape is then not this tensor's), or when
/// the pairing quota is infeasible (needs r <= T3 <= 2r).
void streamline_matmul(BinaryEncoding& encoding, const MatmulShape& shape,
                       const Streamliners& devices);

}  // namespace satisfiability
