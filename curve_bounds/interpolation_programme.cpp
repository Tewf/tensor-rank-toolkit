#include "interpolation_programme.h"

#include <algorithm>
#include <limits>
#include <map>
#include <string>

#include "memory_budget.h"
#include "symmetric_bound_table.h"

namespace curve_bounds {

namespace {

constexpr std::size_t kUnreachable = std::numeric_limits<std::size_t>::max();

/// The multiplicities a point of this degree can be costed at, with their
/// prices. A multiplicity the table does not publish is simply not offered,
/// which is why an unknown entry can never leak into a bound.
std::vector<std::pair<std::size_t, std::size_t>> priced_multiplicities(std::size_t degree,
                                                                      std::size_t degree_budget) {
    std::vector<std::pair<std::size_t, std::size_t>> priced;
    for (std::size_t multiplicity = 1; multiplicity * degree <= degree_budget; ++multiplicity) {
        const std::size_t price = symmetric_upper_bound(degree, multiplicity);
        if (price == 0) continue;
        priced.emplace_back(multiplicity, price);
    }
    return priced;
}

/// For one degree: the cheapest way to spend exactly `g` degree units on at
/// most `available` distinct points of that degree.
///
/// `spent[g]` is the cost and `taken[g]` the multiset of multiplicities behind
/// it. Points of equal degree are interchangeable, so a multiset is the whole
/// of the choice.
struct SupplyPlan {
    std::vector<std::size_t> spent;
    std::vector<std::vector<std::size_t>> taken;
};

/// Where one improving transition came from, instead of the multiset it built.
///
/// Carrying the multiset meant copying an O(available) vector per improving
/// transition, which makes witness tracking O(A^2 * B) for a table that is
/// O(A * B). One parent pointer per cell and one walk back per reachable degree
/// puts it back at O(A * B).
struct Step {
    std::size_t previous_used = 0;
    std::size_t multiplicity = 0;
};

SupplyPlan plan_for(const PointSupply& supply, std::size_t degree_budget) {
    const auto priced = priced_multiplicities(supply.degree, degree_budget);

    // No more points of this degree can be used than the budget can pay for at
    // one multiplicity each, whatever the supply claims to hold. Sizing the table
    // by `available` instead cost about 480 MB of empty vector headers for
    // `available = 100 000` and a budget of 200, for a frontier of 201 by 201.
    const std::size_t usable = std::min(supply.available, degree_budget / supply.degree);

    // **And `degree_budget` is `--degree`, which takes any whole number.** The
    // clamp above bounds one side of the frontier and nothing bounded the other,
    // so `--degree 100000` on a supply of degree 1 asked for a 100 001 by 100 001
    // frontier in two tables, about 240 GB, and the process was simply killed.
    // Priced against the one budget that decides these, so it is refused with the
    // number instead — `--max-memory` moves it where the machine has the room.
    // Asked a row at a time and then a frontier of rows, rather than as one
    // product: `(usable + 1) * (degree_budget + 1)` is exactly the multiplication
    // that overflows on the input this is here to refuse. One row inside the
    // budget makes the second call's `bytes_each` safe by construction.
    const std::string frontier = "the interpolation frontier at degree " +
                                 std::to_string(degree_budget);
    const std::size_t cell = sizeof(std::size_t) + sizeof(Step);
    bilinear_rank::require_room(frontier, degree_budget + 1, cell);
    bilinear_rank::require_room(frontier, usable + 1, (degree_budget + 1) * cell);

    // by_points[p][g]: cost of using exactly p points consuming exactly g.
    std::vector<std::vector<std::size_t>> by_points(usable + 1,
                                                    std::vector<std::size_t>(degree_budget + 1,
                                                                             kUnreachable));
    std::vector<std::vector<Step>> came_from(usable + 1, std::vector<Step>(degree_budget + 1));
    by_points[0][0] = 0;

    for (std::size_t points = 0; points < usable; ++points) {
        for (std::size_t used = 0; used <= degree_budget; ++used) {
            if (by_points[points][used] == kUnreachable) continue;
            for (const auto& [multiplicity, price] : priced) {
                const std::size_t next = used + multiplicity * supply.degree;
                if (next > degree_budget) continue;
                const std::size_t cost = by_points[points][used] + price;
                if (cost >= by_points[points + 1][next]) continue;

                by_points[points + 1][next] = cost;
                came_from[points + 1][next] = Step{used, multiplicity};
            }
        }
    }

    SupplyPlan plan;
    plan.spent.assign(degree_budget + 1, kUnreachable);
    plan.taken.assign(degree_budget + 1, {});
    std::vector<std::size_t> fewest(degree_budget + 1, 0);
    for (std::size_t points = 0; points <= usable; ++points) {
        for (std::size_t used = 0; used <= degree_budget; ++used) {
            if (by_points[points][used] >= plan.spent[used]) continue;
            plan.spent[used] = by_points[points][used];
            fewest[used] = points;
        }
    }

    for (std::size_t used = 0; used <= degree_budget; ++used) {
        if (plan.spent[used] == kUnreachable) continue;
        std::size_t points = fewest[used];
        std::size_t here = used;
        while (points > 0) {
            const Step step = came_from[points][here];
            plan.taken[used].push_back(step.multiplicity);
            here = step.previous_used;
            --points;
        }
    }
    return plan;
}

}  // namespace

BoundResult minimise_interpolation_bound(const std::vector<PointSupply>& supply,
                                       std::size_t divisor_degree) {
    const std::size_t degree_budget = divisor_degree;
    std::vector<SupplyPlan> plans;
    plans.reserve(supply.size());
    for (const PointSupply& one : supply) {
        if (one.degree == 0 || one.available == 0) {
            plans.push_back(SupplyPlan{std::vector<std::size_t>(degree_budget + 1, kUnreachable),
                                       std::vector<std::vector<std::size_t>>(degree_budget + 1)});
            plans.back().spent[0] = 0;
            continue;
        }
        plans.push_back(plan_for(one, degree_budget));
    }

    // Combine the degrees: best[s][g] is the cheapest use of the first s
    // supplies consuming exactly g.
    std::vector<std::vector<std::size_t>> best(supply.size() + 1,
                                               std::vector<std::size_t>(degree_budget + 1,
                                                                        kUnreachable));
    std::vector<std::vector<std::size_t>> consumed(supply.size() + 1,
                                                   std::vector<std::size_t>(degree_budget + 1, 0));
    best[0][0] = 0;

    for (std::size_t index = 0; index < supply.size(); ++index) {
        for (std::size_t used = 0; used <= degree_budget; ++used) {
            if (best[index][used] == kUnreachable) continue;
            for (std::size_t here = 0; used + here <= degree_budget; ++here) {
                if (plans[index].spent[here] == kUnreachable) continue;
                const std::size_t cost = best[index][used] + plans[index].spent[here];
                if (cost >= best[index + 1][used + here]) continue;
                best[index + 1][used + here] = cost;
                consumed[index + 1][used + here] = here;
            }
        }
    }

    BoundResult programme;
    if (divisor_degree == 0 || best[supply.size()][divisor_degree] == kUnreachable) return programme;
    programme.solved = true;
    programme.bound = best[supply.size()][divisor_degree];
    programme.degree_used = divisor_degree;

    // Walk the choices back, collecting equal (degree, multiplicity) pairs.
    std::map<std::pair<std::size_t, std::size_t>, std::size_t> counts;
    std::size_t remaining = programme.degree_used;
    for (std::size_t index = supply.size(); index > 0; --index) {
        const std::size_t here = consumed[index][remaining];
        for (std::size_t multiplicity : plans[index - 1].taken[here]) {
            ++counts[{supply[index - 1].degree, multiplicity}];
        }
        remaining -= here;
    }
    for (const auto& [key, count] : counts) {
        programme.chosen.push_back(Selection{key.first, key.second, count});
    }
    return programme;
}

}  // namespace curve_bounds
