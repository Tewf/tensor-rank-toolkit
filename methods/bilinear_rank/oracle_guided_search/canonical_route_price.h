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
/// against are [`when-canonical-pays/`](when-canonical-pays/README.md).
///
/// **There are two regimes and they are not the same question**, which is the
/// correction of 2026-08-21. `L = target - n*k` is the levels of augmentation.
///
/// At `L >= 2` the comparison is between two **trees**, and the short form is
///
///     saving ratio  rho = plain nodes / canonical nodes
///     price ratio   pi  = one canonical node / one plain node
///     entry fee     F   = presenting the group, paid once and only by canonical
///
///     it pays  <=>  rho > pi  and  the plain search costs more than F
///
/// At `L == 1` there is no tree. Both routes emit one node per `G`-orbit of the
/// pool and `rho` is exactly 1, measured at all five shapes, so a per-node ratio
/// prices nothing: what is being compared is **two roots and their leaves**, and
/// each side is written out rather than averaged.
///
///     plain root       R * sum |O_i|^2      naming one index per orbit, per element
///     canonical root   S + A|P| + Stab + (r+1) I    one pool pass, one orbit pass
///     leaves           r * (canonical scans the pool, plain walks p^target)
///
///     it pays  <=>  R * sum |O_i|^2  >  the whole right-hand side
///
/// The two sides differ in **order** and not in a constant: `least_in_orbit`
/// costs `O(sum |O_i|^2)` where `orbit_representatives` costs `O(|P|)` for the
/// same answer, so the clause is a statement about the baseline's orbit test as
/// much as about canonical augmentation. Measured at the `<3,3,3>` root: 5.06 s
/// against 0.0497 s for the identical 13 children.
///
/// **The two sides of the tree regime are not equally sound, and the file says
/// which is which.**
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
/// does: `log|G|/log n` is 1.58, 2.00, 2.41, 1.90 and 2.22 at the five shapes
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
/// [`infrastructure/run_limits/device.cpp`](../run_limits/device.cpp) keeps its crossover table in
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
    /// And the part that does: `[linton2004]`'s base changes and orbit
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

    // The four below price a **root**, and nothing above them does. A root is the
    // one node of a search that scans the whole pool, and the one-level clause is
    // the only place in this model where a node is priced as a root rather than
    // as an average over a tree.

    /// One step of `least_in_orbit`, per **squared** orbit size. It reaches an
    /// orbit breadth first and asks `std::find` over a `seen` list that grows to
    /// the whole orbit, so naming one representative costs `O(|O|^2)` and the
    /// root's whole sweep costs `O(sum |O_i|^2)`.
    /// ([`../orbit_reduction/isomorph_rejection.cpp`](../orbit_reduction/isomorph_rejection.cpp).)
    /// Measured 2.26, 1.50, 0.71, 0.49 and 0.51 ns over the five shapes: the
    /// small ones sit in cache and the large ones settle, so 0.5 is the value at
    /// the sizes where this term decides anything.
    std::size_t orbit_test_picoseconds = 500;
    /// One step of `orbit_representatives`, per pool element. It marks every
    /// element once, so it answers the same question in `O(|P|)` where the test
    /// above takes `O(sum |O_i|^2)`. Measured 108 to 190 ns an element.
    std::size_t orbit_pass_nanoseconds_per_element = 150;
    /// Forming the materialised pool, per element, which **both** routes pay
    /// before a node opens. In the model so the two sides are compared on whole
    /// runs rather than on searches: at `<2,2,2>` it is most of either run.
    /// Measured 368 to 566 ns an element.
    std::size_t pool_build_nanoseconds_per_element = 470;
    /// One leaf of the canonical route, in membership-test units, so `1000` would
    /// be one pool scan. It answers a leaf by `independent_rank_one_maps_in` over
    /// the whole pool where the plain route walks `p^target` elements through the
    /// packed GF(2) leaf, and at these shapes `p^target` is 1 024 against a pool
    /// of 261 121. Measured 1.27 to 2.0 pool scans, falling with the pool.
    std::size_t solution_leaf_picoseconds = 840;
};

/// What the group does to the pool, which no closed form here gives.
///
/// **This is the one input to the predicate that is not free**, and it is stated
/// as its own type so that a caller cannot supply it by accident. One pass of
/// `orbit_representatives` produces both, `O(|P| * generators)`: 24 us at
/// `<2,2,2>` and 50 ms at `<3,3,3>`, the latter against a 4.9 s decision it
/// prices, so it is 1% of what it decides and not free.
///
/// Zero means "not measured", and the one-level clause then refuses rather than
/// guessing. The bound it would otherwise use is orbit counting's `|O| <= |G|`,
/// and that is **125x** the measured value at `<3,3,3>`, the same mistake, in
/// the same direction, that `rho <= |G|` made before it was corrected.
struct PoolOrbits {
    /// Orbits of the pool under the stabiliser, which is exactly the number of
    /// children a root of either route emits.
    std::size_t count = 0;
    /// `sum |O_i|^2` over those orbits. A double because it is 9.9e9 at
    /// `<3,3,3>` and grows with the square of the pool.
    double summed_squares = 0;
};

/// A shape and the level being decided.
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
    /// a product shape. Both routes are handed the same six, and both take the
    /// **exact** quotient from them (`least_in_orbit` on one side and
    /// `orbit_representatives` on the other), so this counts what each pays per
    /// element and never a difference in what they reject.
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
    /// The two whole runs the one-level clause compares, setup included. Zero at
    /// two levels and above, where the comparison is per node and these do not
    /// apply.
    double plain_run_seconds = 0;
    double canonical_run_seconds = 0;
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
///
/// `orbits` is read only at one level of augmentation, where the comparison is
/// between two roots rather than between two trees; left at its default the
/// one-level clause refuses and says so.
RouteVerdict price_canonical_route(const RouteShape& shape, const CanonicalPrices& prices,
                                   const PoolOrbits& orbits = PoolOrbits());

}  // namespace bilinear_rank
