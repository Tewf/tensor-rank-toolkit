/// The rules, the overrides, and the round trip, with the rules pinned to
/// tensors whose answers can be read off by hand.
///
/// **A rule that is only exercised through a search is a rule nobody can see.**
/// Every choice here used to be made inside the routine that acted on it, so the
/// only way to ask what a run would decide was to run it and watch. These checks
/// state the expected plan for a shape and a target outright, which is what makes
/// the rules explicit rather than implicit: a rule that changes now fails here,
/// by name, instead of moving a node count in a table three modules away.
///
/// **The round trip is checked field by field and not as a blob.** A plan whose
/// file drops a field replays a run that was never planned, and a file compared
/// against itself would not notice: the comparison is against the plan the rules
/// produced, through the file, on every field.
#include <sstream>
#include <string>
#include <vector>

#include "arguments.h"
#include "check.h"
#include "plan_file.h"
#include "search_plan.h"

namespace {

using bilinear_rank::Anchor;
using bilinear_rank::DeviceRequest;
using bilinear_rank::LeafRoute;
using bilinear_rank::Machine;
using bilinear_rank::OrbitTest;
using bilinear_rank::PlanRequest;
using bilinear_rank::Pool;
using bilinear_rank::SearchPlan;

/// `⟨2,2,2⟩`: 4x4 slices over GF(2), a 225-map pool, and the target this
/// repository quotes 25 399 nodes for.
Machine matmul_2x2x2(std::size_t target) {
    Machine machine;
    machine.characteristic = 2;
    machine.rows = 4;
    machine.columns = 4;
    machine.pool_size = 225;
    machine.target = target;
    machine.memory_budget = 2u * 1024 * 1024 * 1024;
    machine.binary_leaf = true;
    return machine;
}

/// `⟨4,4,4⟩`: 16x16 slices, 4 294 836 225 maps, which no budget here holds.
Machine matmul_4x4x4(std::size_t target) {
    Machine machine;
    machine.characteristic = 2;
    machine.rows = 16;
    machine.columns = 16;
    machine.pool_size = 65535u * 65535u;
    machine.target = target;
    machine.memory_budget = 2u * 1024 * 1024 * 1024;
    machine.binary_leaf = true;
    return machine;
}

std::string route_of(const SearchPlan& plan) { return bilinear_rank::name_of(plan.leaf_route); }
std::string pool_of(const SearchPlan& plan) { return bilinear_rank::name_of(plan.pool); }
std::string device_of(const SearchPlan& plan) { return run_limits::name_of(plan.device); }

void check_the_rules() {
    const PlanRequest plain;

    const SearchPlan small = bilinear_rank::chosen_plan(matmul_2x2x2(6), plain);
    check::text("a 225-map pool is held", pool_of(small), "materialised");
    check::text("and 2^6 = 64 elements is cheaper than scanning 225 of them", route_of(small),
                "walk");
    check::text("64 elements is far under any launch floor", device_of(small), "cpu");

    // Dimension 8 is 256 elements against 225 maps, which is the first target on
    // this shape where the pool is the smaller side. The rule is a comparison and
    // this is the place it turns over.
    check::text("at dimension 8 the pool is the smaller side",
                route_of(bilinear_rank::chosen_plan(matmul_2x2x2(8), plain)), "scan");

    const SearchPlan large = bilinear_rank::chosen_plan(matmul_4x4x4(47), plain);
    check::text("4.3e9 maps of 16x16 do not fit 2 GiB", pool_of(large), "addressed");
    check::text("and 2^47 elements is not cheaper than scanning them", route_of(large), "scan");
    check::text("with no backend registered the card cannot answer", device_of(large), "cpu");
    check::text("and the plan says which of the four conditions refused",
                large.device_reason, "no gpu backend compiled in");

    // The size is weighed before the card is asked about, and that order is a
    // decision: asking whether a card is present is the first CUDA call a
    // process makes and costs 0.15 s of driver start-up, which a run that could
    // never reach a card should not pay. So a small leaf is refused by the floor
    // and never by the four conditions.
    check::text("a leaf under the floor is refused by size alone", small.device_reason,
                "64 elements at the deepest leaf, under the 8192 launch floor");

    Machine odd = matmul_4x4x4(47);
    odd.characteristic = 3;
    odd.binary_leaf = false;
    check::text("and one over it, over GF(3), by the leaf it would have needed",
                bilinear_rank::chosen_plan(odd, plain).device_reason,
                "the leaf is the general field path, which has no kernel");

    Machine sweeping = matmul_2x2x2(0);
    check::text("a sweep leaves the route to each leaf",
                route_of(bilinear_rank::chosen_plan(sweeping, plain)), "auto");
}

void check_every_field_can_be_overridden() {
    PlanRequest request;
    request.leaf_route = LeafRoute::Scan;
    request.device = DeviceRequest::Cpu;
    request.threads = 12;
    request.orbit_test = OrbitTest::Generators;
    request.anchor = Anchor::Heuristic;
    request.quotient.kind = cli::SymmetryKind::MatrixMultiplication;
    request.quotient.shape = {2, 2, 2};

    const SearchPlan plan = bilinear_rank::chosen_plan(matmul_2x2x2(6), request);
    check::text("--leaf-route beats the rule", route_of(plan), "scan");
    check::text("and says so rather than showing the arithmetic", plan.leaf_route_reason,
                "--leaf-route scan");
    check::equal("--threads is carried", static_cast<long long>(plan.threads), 12);
    check::text("--orbit-test is carried", bilinear_rank::name_of(plan.orbit_test), "generators");
    check::text("--anchor is carried", bilinear_rank::name_of(plan.anchor), "heuristic");
    check::text("-s is carried whole", bilinear_rank::name_of(plan.quotient), "matmul 2 2 2");
    check::text("--device cpu is carried", plan.device_reason, "--device cpu");

    // --max-memory is the flag that controls the pool, and it is the only field
    // whose override goes through the machine rather than the request.
    Machine cramped = matmul_2x2x2(6);
    cramped.memory_budget = 1024;
    check::text("--max-memory 1K addresses a pool that would otherwise be held",
                pool_of(bilinear_rank::chosen_plan(cramped, PlanRequest())), "addressed");
}

/// `--device gpu` is a request and not an instruction, the way `--leaf-route
/// walk` already is: with no backend the host answers and the line says why,
/// rather than the run being refused. A plan taken on a machine with a card has
/// to replay on one without.
void check_asking_for_a_card_that_is_not_there() {
    PlanRequest request;
    request.device = DeviceRequest::Gpu;
    const SearchPlan plan = bilinear_rank::chosen_plan(matmul_4x4x4(47), request);
    check::text("asking for the card does not conjure one", device_of(plan), "cpu");
    check::text("and the reason is the one that refused", plan.device_reason,
                "no gpu backend compiled in");
}

void check_the_round_trip() {
    PlanRequest request;
    request.leaf_route = LeafRoute::Walk;
    request.threads = 4;
    request.orbit_test = OrbitTest::Generators;
    request.anchor = Anchor::Heuristic;
    request.quotient.kind = cli::SymmetryKind::MatrixMultiplication;
    request.quotient.shape = {2, 3, 4};
    const SearchPlan written = bilinear_rank::chosen_plan(matmul_4x4x4(20), request);

    std::ostringstream file;
    bilinear_rank::write_plan(file, written);
    std::istringstream back(file.str());
    const SearchPlan read = bilinear_rank::read_plan(back, "plan.txt");

    check::text("the pool survives the file", pool_of(read), pool_of(written));
    check::text("the leaf route survives", route_of(read), route_of(written));
    check::text("the device survives", device_of(read), device_of(written));
    check::equal("the thread count survives", static_cast<long long>(read.threads),
                 static_cast<long long>(written.threads));
    check::text("the quotient survives with its shape", bilinear_rank::name_of(read.quotient),
                bilinear_rank::name_of(written.quotient));
    check::text("the rejection rule survives", bilinear_rank::name_of(read.orbit_test),
                bilinear_rank::name_of(written.orbit_test));
    check::text("the anchor survives", bilinear_rank::name_of(read.anchor),
                bilinear_rank::name_of(written.anchor));
    check::text("and a replayed plan says where it came from", read.device_reason,
                "from plan.txt");
}

std::string refusal(const std::string& text) {
    std::istringstream stream(text);
    try {
        bilinear_rank::read_plan(stream, "plan.txt");
    } catch (const cli::ArgumentError& refused) {
        return refused.what();
    }
    return "";
}

void check_a_half_read_plan_is_refused() {
    check::text("an unknown field is refused rather than ignored", refusal("poool addressed\n"),
                "plan.txt:1: no plan field is called 'poool'");
    check::text("a value the field does not take names both", refusal("device tpu\n"),
                "plan.txt:1 device expects cpu or gpu, not 'tpu'");
    check::text("a name with no value says so", refusal("device\n"),
                "plan.txt:1: 'device' is not a 'name value' line");
    check::text("and a comment is a comment", refusal("# device tpu\n"), "");
}

}  // namespace

int main() {
    check_the_rules();
    check_every_field_can_be_overridden();
    check_asking_for_a_card_that_is_not_there();
    check_the_round_trip();
    check_a_half_read_plan_is_refused();
    return check::report("search_plan");
}
