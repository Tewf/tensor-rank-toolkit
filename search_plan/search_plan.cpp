#include "search_plan.h"

#include <cstdio>
#include <limits>

#include "gf2_leaf_on_card.h"
#include "memory_budget.h"

namespace bilinear_rank {

namespace {

/// A byte count a person reads, for the one reason a pool is refused: 8.2 TiB
/// against 2.0 GiB says what 9007199254740992 against 2147483648 does not.
///
/// Binary units, because `--max-memory 2G` is two gibibytes and a line reading
/// `2.15 GB` beside a flag that said `2G` invites the reader to look for the
/// 7% somebody lost.
std::string as_bytes(double count) {
    static const char* const units[] = {"B", "KiB", "MiB", "GiB", "TiB", "PiB"};
    std::size_t unit = 0;
    while (count >= 1024.0 && unit + 1 < sizeof(units) / sizeof(units[0])) {
        count /= 1024.0;
        ++unit;
    }
    char written[32];
    std::snprintf(written, sizeof(written), count < 10.0 ? "%.2f %s" : "%.0f %s", count,
                  units[unit]);
    return written;
}

void choose_the_pool(const Machine& machine, SearchPlan& plan) {
    const std::size_t each = bytes_per_matrix(machine.rows * machine.columns);
    const bool fits =
        machine.pool_size != 0 && each <= machine.memory_budget / machine.pool_size;
    plan.pool = fits ? Pool::Materialised : Pool::Addressed;
    plan.pool_reason = std::to_string(machine.pool_size) + " maps at " + as_bytes(each) +
                       " each is " + as_bytes(static_cast<double>(machine.pool_size) * each) +
                       (fits ? ", inside the " : ", over the ") +
                       as_bytes(static_cast<double>(machine.memory_budget)) + " budget";
}

/// The leaf route, which is `p^dim` against `|P|` and nothing else.
///
/// `subspace_elements` is the search's own counter and the ceiling is the
/// search's own ceiling, so a forced walk is counted the way `rank_one_basis.cpp`
/// counts one and reaches the same verdict on the same question. Returns the
/// element count of the deepest leaf, which the device rule then weighs.
std::size_t choose_the_leaf_route(const Machine& machine, const PlanRequest& request,
                                  SearchPlan& plan) {
    if (machine.target == 0) {
        plan.leaf_route = LeafRoute::Auto;
        plan.leaf_route_reason =
            "a sweep tests leaves of many dimensions, so each takes the cheaper by size";
        return machine.pool_size;
    }

    const bool asked_to_walk = request.leaf_route == LeafRoute::Walk;
    const std::size_t ceiling =
        asked_to_walk ? std::numeric_limits<std::size_t>::max() : machine.pool_size;
    const std::size_t elements =
        subspace_elements(machine.characteristic, machine.target, ceiling);

    if (request.leaf_route == LeafRoute::Scan) {
        plan.leaf_route = LeafRoute::Scan;
        plan.leaf_route_reason = "--leaf-route scan";
        return machine.pool_size;
    }
    if (asked_to_walk) {
        // A request and not an instruction, which is what `--leaf-route` already
        // promises: a subspace whose size overflows a count cannot be walked, so
        // the pool is scanned rather than the run refused.
        plan.leaf_route = elements != 0 ? LeafRoute::Walk : LeafRoute::Scan;
        plan.leaf_route_reason = elements != 0
                                     ? "--leaf-route walk"
                                     : "--leaf-route walk, but the subspace overflows a count";
        return elements != 0 ? elements : machine.pool_size;
    }

    const bool walk = elements != 0 && elements < machine.pool_size;
    plan.leaf_route = walk ? LeafRoute::Walk : LeafRoute::Scan;
    plan.leaf_route_reason =
        (elements != 0 ? std::to_string(elements) : "more than " + std::to_string(machine.pool_size)) +
        " subspace elements at dimension " + std::to_string(machine.target) + " against " +
        std::to_string(machine.pool_size) + " pool maps";
    return walk ? elements : machine.pool_size;
}

void choose_the_device(const Machine& machine, const PlanRequest& request, std::size_t leaf_work,
                       SearchPlan& plan) {
    if (request.device == DeviceRequest::Cpu) {
        plan.device = run_limits::Device::Cpu;
        plan.device_reason = "--device cpu";
        return;
    }
    // The size before the probe, and that order is not arbitrary. Asking whether
    // a card is present is the first CUDA call a process makes, and it costs
    // **0.15 s** of driver start-up: measured on `decide-rank matmul_2x2x2
    // --target 6`, 0.186 s of process against 0.034 s from the build with no
    // toolkit, for a run whose every leaf is 64 elements and could never have
    // reached a card. A run under the floor now asks nothing and pays nothing.
    if (request.device == DeviceRequest::Auto && leaf_work < run_limits::launch_floor()) {
        plan.device = run_limits::Device::Cpu;
        plan.device_reason = std::to_string(leaf_work) +
                             " elements at the deepest leaf, under the " +
                             std::to_string(run_limits::launch_floor()) + " launch floor";
        return;
    }
    if (const char* refused = card_refusal(machine)) {
        plan.device = run_limits::Device::Cpu;
        plan.device_reason = refused;
        return;
    }
    if (request.device == DeviceRequest::Gpu) {
        plan.device = run_limits::Device::Gpu;
        plan.device_reason = "--device gpu, so the launch floor is not applied";
        return;
    }
    // The ranking and the floor together, asked of the deepest leaf this run
    // reaches. Shallower leaves ask the same question again with their own size,
    // which is why this line says what was on the table and not what every leaf
    // did.
    plan.device = run_limits::chosen_device(leaf_work);
    plan.device_reason = std::to_string(leaf_work) + " elements at the deepest leaf, at or over" +
                         " the " + std::to_string(run_limits::launch_floor()) + " launch floor";
}

std::string bracketed(const std::string& reason) {
    return reason.empty() ? "" : " (" + reason + ")";
}

}  // namespace

/// Each condition has its own sentence, because a reader who wrote `--device
/// gpu` and got the host needs to know which of the four refused rather than
/// that one of them did.
const char* card_refusal(const Machine& machine) {
    if (!machine.binary_leaf) return "the leaf is the general field path, which has no kernel";
    if (leaf_on_card() == nullptr) return "no gpu backend compiled in";
    if (!leaf_on_card()->handles(machine.rows, machine.columns)) {
        return "no kernel is compiled for this shape";
    }
    if (!run_limits::available(run_limits::Device::Gpu)) return "no card answered the probe";
    return nullptr;
}

SearchPlan plan_with_flags(const SearchPlan& replayed, const PlanRequest& request,
                           const PlanFlagsGiven& given) {
    SearchPlan plan = replayed;
    if (given.leaf_route) {
        plan.leaf_route = request.leaf_route;
        plan.leaf_route_reason = std::string("--leaf-route ") + name_of(request.leaf_route);
    }
    if (given.device) {
        plan.device = request.device == DeviceRequest::Gpu ? run_limits::Device::Gpu
                                                           : run_limits::Device::Cpu;
        plan.device_reason =
            std::string("--device ") + run_limits::name_of(plan.device) + ", over the plan file";
    }
    if (given.threads) plan.threads = request.threads;
    if (given.quotient) plan.quotient = request.quotient;
    if (given.orbit_test) plan.orbit_test = request.orbit_test;
    if (given.anchor) plan.anchor = request.anchor;
    return plan;
}

const char* name_of(Pool pool) {
    return pool == Pool::Materialised ? "materialised" : "addressed";
}

const char* name_of(Anchor anchor) { return anchor == Anchor::Map ? "map" : "heuristic"; }

const char* name_of(LeafRoute route) {
    switch (route) {
        case LeafRoute::Scan: return "scan";
        case LeafRoute::Walk: return "walk";
        case LeafRoute::Auto: return "auto";
    }
    return "?";
}

const char* name_of(OrbitTest test) {
    return test == OrbitTest::Generators ? "generators" : "full";
}

std::string name_of(const cli::Symmetry& quotient) {
    if (quotient.kind == cli::SymmetryKind::Automatic) return "auto";
    if (quotient.kind != cli::SymmetryKind::MatrixMultiplication) return "none";
    std::string written = "matmul";
    for (const std::size_t part : quotient.shape) written += " " + std::to_string(part);
    return written;
}

SearchPlan chosen_plan(const Machine& machine, const PlanRequest& request) {
    SearchPlan plan;
    plan.threads = request.threads;
    plan.quotient = request.quotient;
    plan.orbit_test = request.orbit_test;
    plan.anchor = request.anchor;

    choose_the_pool(machine, plan);
    const std::size_t leaf_work = choose_the_leaf_route(machine, request, plan);
    choose_the_device(machine, request, leaf_work, plan);
    return plan;
}

std::vector<std::pair<std::string, std::string>> plan_fields(const SearchPlan& plan) {
    return {
        {"pool", name_of(plan.pool)},
        {"leaf_route", name_of(plan.leaf_route)},
        {"device", run_limits::name_of(plan.device)},
        {"threads", std::to_string(plan.threads)},
        {"quotient", name_of(plan.quotient)},
        {"orbit_test", name_of(plan.orbit_test)},
        {"anchor", name_of(plan.anchor)},
    };
}

std::vector<std::string> plan_lines(const SearchPlan& plan) {
    // The three reasons sit beside the three fields that have a rule, in the
    // order `plan_fields` lists them. Nothing else here knows that order, which
    // is why this is a lookup by name and not by position.
    const std::vector<std::pair<std::string, const std::string*>> reasons{
        {"pool", &plan.pool_reason},
        {"leaf_route", &plan.leaf_route_reason},
        {"device", &plan.device_reason},
    };

    std::vector<std::string> lines;
    for (const auto& [name, value] : plan_fields(plan)) {
        std::string shown = name;
        for (char& letter : shown) letter = letter == '_' ? ' ' : letter;
        std::string reason;
        for (const auto& [named, text] : reasons) {
            if (named == name) reason = *text;
        }
        lines.push_back(shown + ": " + value + bracketed(reason));
    }
    return lines;
}

}  // namespace bilinear_rank
