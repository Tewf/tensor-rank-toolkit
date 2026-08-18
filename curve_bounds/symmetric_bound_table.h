#pragma once

#include <cstddef>

/// The published bounds on the symmetric bilinear complexity of multiplication
/// in the small algebras `F_{2^m}[y]/(y^l)`.
///
/// `µ_sym_2(m, l)` is the fewest products a *symmetric* algorithm needs for
/// multiplication in that algebra, and `µ_sym_2(m, 1)` is written `µ_sym_2(m)`:
/// multiplication in `GF(2^m)` itself. These are the numbers the
/// Chudnovsky-Chudnovsky method feeds on, because
/// [the interpolation programme](interpolation_programme.h) minimises a sum of
/// them.
///
/// The table is `[rambaud2014, Table 1]`, transcribed. Keys are
/// [references.md](../references.md). It is data, not a computation: nothing
/// here derives a bound, and where the literature knows only an upper bound
/// this says so rather than inventing a lower one.
///
/// **These are symmetric ranks, and `bilinear_rank/` computes ordinary tensor
/// rank.** The two are not the same quantity in general -- symmetric rank is at
/// least the rank -- so the agreements below are observations, not identities.
/// They happen to agree on every entry this repository can decide: `µ_sym_2(2)`,
/// `µ_sym_2(3)` and `µ_sym_2(4)` are 3, 6 and 9, and the exact search reaches
/// exactly those for GF(4), GF(8) and GF(16).
namespace curve_bounds {

/// What is known about one entry.
///
/// `lower == upper` means the value is settled. A zero `lower` means no
/// noticeable lower bound is published, which the table prints as `- U`, and it
/// must not be read as "the lower bound is zero".
struct Bound {
    bool known = false;
    std::size_t lower = 0;
    std::size_t upper = 0;

    bool settled() const { return known && lower == upper && lower != 0; }
};

/// `µ_sym_2(extension_degree, truncation)`, or an unknown entry outside the
/// published range. `truncation == 1` is multiplication in `GF(2^m)`.
Bound symmetric_bound(std::size_t extension_degree, std::size_t truncation);

/// The best upper bound the table gives, and zero when there is none. This is
/// what the interpolation programme costs a point with.
std::size_t symmetric_upper_bound(std::size_t extension_degree, std::size_t truncation);

}  // namespace curve_bounds
