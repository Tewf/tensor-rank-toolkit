#pragma once

#include <cstddef>

/// Whether canonical augmentation will pay on a shape, decided before the search.
///
/// `--route canonical` visits far fewer nodes than `--route exhaustive` and has so
/// far lost on the clock at every shape measured. That is a trade with a crossing
/// in it, and this file states the crossing as a number rather than an impression,
/// from quantities a caller holds **before** a node is opened: the characteristic,
/// the shape `<n,m,k>`, the pool size, the factored degree, the group order and the
/// generator count.
///
/// The derivation, its primary sources and the measured table it was corrected
/// against are [`when-canonical-pays.md`](when-canonical-pays.md). The short form:
///
///     saving ratio  rho = plain nodes / canonical nodes
///     price ratio   pi  = one canonical node / one plain node
///     entry fee     F   = presenting the group, paid once and only by canonical
///
///     it pays  <=>  rho > pi  and  the plain search costs more than F
///
/// **The two sides are not equally sound, and the file says which is which.**
///
/// The saving side has a theorem under it. `[mckay1998, Thm 3]` bounds the number
/// of times the parent test is *made* by `c` times the number of times it is
/// **passed**, `c` being the average count of `Aut(X)`-orbits on the augmentations
/// of a reducible `X`. So the price is proportional to the tree that is kept and
/// never to the tree that is avoided, which is exactly what makes a per-node
/// comparison the right shape for this question. Orbit counting caps the other end:
/// the `G`-orbits on the `j`-subsets of the pool number at least `C(|P|,j)/|G|`, so
/// `rho <= |G|`, with equality approached only where the action is free.
///
/// The price side has two operations and one of them is bounded.
///
/// **The canonical image is bounded and, at these shapes, polynomially.**
/// `[linton2004]` §3.4 splits it four ways: the stabiliser chain, which "need not
/// be accounted to the smallest image algorithm" because it is built once;
/// `k` base changes at `O(n (log n)^2 (log|G|)^2)`; the orbits under `G_{i-1}` at
/// `O(k n (log n)^c)` in total; and `O(k (log n)^c)` per candidate. The candidate
/// count is the only unbounded part, and its three bounds are `O(k!)`, `O(n^k)` or
/// `O(2^n)`, and `k|G|`. **Here `k|G|` is by far the smallest**, and Linton's own
/// third conclusion applies verbatim: "if `|G|` grows polynomially in `n` then the
/// running time is polynomial in `n` for any `k`". On the factored presentation it
/// does — `log|G|/log n` is 1.58, 2.00, 2.41, 1.90 and 2.22 at the five shapes
/// measured, so `|G| ~ n^2` throughout. He adds that "experiment suggests that the
/// actual number of candidates is much smaller", which is why the constants below
/// are measured and the bound is quoted rather than used.
///
/// **The setwise stabiliser is not bounded.** It is GI-hard (`[luks1993]`, Prop.
/// 4.2, with STAB, INTER and CENT polynomial-time equivalent in Prop. 4.3), not
/// known to be in P, quasipolynomial through Babai's string isomorphism result, and
/// implemented here by a backtrack search for which no subexponential worst case is
/// proved. Every number attached to it below is measured on this machine and is not
/// a bound. It is the one term in this model that a different shape may falsify
/// outright rather than merely shift.
namespace bilinear_rank {

/// What the operations cost here, measured by `price-canonical-route`.
///
/// In code rather than in `tunables.conf` for the reason
/// [`run_limits/device.cpp`](../run_limits/device.cpp) keeps its crossover table in
/// code and only the floor that *decides* in the file: a measured table is not a
/// knob, and the command that produced it ships beside it. Picoseconds where a
/// nanosecond would round to zero.
struct CanonicalPrices {
    /// One `ReducedBasis::contains` against one pool element, per span dimension
    /// per matrix entry. Measured 0.60 ns +- 7% across a shape range of 11x, which
    /// is the one constant here that behaves like a constant.
    std::size_t membership_picoseconds = 600;
    /// One plain node, in the same units. Larger than a membership test and far
    /// smaller than a whole pool scan, because `opens_a_branch` rejects most of the
    /// pool before the containment test and the `from` index leaves only a suffix.
    /// Measured 11 to 31 ns; the middle of that is as sharp as this gets.
    std::size_t plain_node_picoseconds = 20000;
    /// One canonical image: the part that does not scale with the degree.
    std::size_t image_floor_nanoseconds = 10800;
    /// And the part that does — `[linton2004]`'s base changes and orbit
    /// computations, both linear in the degree up to logarithms.
    std::size_t image_nanoseconds_per_point = 74;
    /// One setwise stabiliser, per axis point. **PROVISIONAL and known weak**: the
    /// measured values run from 1.0 to 9.0 ns a point over the five shapes with no
    /// law through them, which is what an operation with no proven bound looks like
    /// from outside.
    std::size_t stabiliser_nanoseconds_per_point = 1000;
    /// Presenting the group on the axes, per axis point, once per search.
    /// `[linton2004]` §2 prices the chain at `O(n log n (log|G|)^4)` through
    /// `[seress2003, Thm 4.5.5]`; measured 2.7 to 4.6 us a point here.
    std::size_t presentation_nanoseconds_per_point = 3000;
    /// Candidate children per canonical node, which is `[mckay1998, Thm 3]`'s `c`.
    /// **Fitted, and 2 rather than the 5 to 12 the sweeps show**, because it stands
    /// in this model for more than the branching alone: it multiplies the canonical
    /// images a node asks for, `candidate_parents` keeps only the *reachable*
    /// hyperplanes where `parents` below counts them all, and the parent test stops
    /// at the first candidate that beats the parent rather than naming every one.
    std::size_t branching = 2;
};

/// A shape and the level being decided, which is all the predicate is given.
struct RouteShape {
    std::size_t characteristic = 2;
    std::size_t rows = 2;     ///< `n` of `<n,m,k>`
    std::size_t inner = 2;    ///< `m`
    std::size_t columns = 2;  ///< `k`
    /// The subspace dimension the question asks about, so `target - n*k` levels of
    /// augmentation above the slices. `n*k` is the slice count, and the span
    /// dimension of a concise product tensor.
    std::size_t target = 6;
    /// Generators the group is handed as: two per general linear factor, so six for
    /// a product shape. It is also about what the baseline achieves on its own,
    /// since `--route exhaustive` strikes out a child that a **single** generator
    /// sends earlier.
    std::size_t generators = 6;
};

/// Both sides of the break-even, and the verdict.
struct RouteVerdict {
    std::size_t pool_size = 0;
    std::size_t degree = 0;  ///< `a + b`, the factored presentation's domain
    double group_order = 0;
    std::size_t levels = 0;
    double plain_node_seconds = 0;
    double pool_scan_seconds = 0;
    double image_seconds = 0;
    double stabiliser_seconds = 0;
    double canonical_node_seconds = 0;
    double presentation_seconds = 0;
    double price_ratio = 0;   ///< `pi`
    double saving_ratio = 0;  ///< `rho`
    /// `pi / rho`: predicted canonical seconds over plain seconds. Below one is a
    /// win, and the number says by how much either way, which a boolean does not.
    double predicted_cost = 0;
    bool pays = false;
    /// Why not, when not. Empty when it pays.
    const char* refusal = "";
};

/// `|GL_order(F_p)|`, in closed form, since the groups that matter hold no list:
/// `|GL_3(F_2)|^3` is 4 741 632 and `<4,4,4>`'s is 8.2e12.
double general_linear_order(std::size_t characteristic, std::size_t order);

/// The order of the stabiliser of a product tensor's slice space,
/// `|GL_n||GL_m||GL_k|/(p-1)^2` by `[covanov2019, Thm. 17]`. Exact over GF(2),
/// where the scalar quotient is trivial, and that is every shape measured here.
double product_group_order(const RouteShape& shape);

/// Whether `--route canonical` is worth taking on this shape, and the arithmetic
/// behind the answer.
RouteVerdict price_canonical_route(const RouteShape& shape, const CanonicalPrices& prices);

}  // namespace bilinear_rank
