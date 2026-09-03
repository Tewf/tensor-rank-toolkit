#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "device.h"
#include "isomorph_rejection.h"
#include "rank_one_basis.h"
#include "symmetry_argument.h"

/// One place where a run decides how it will be carried out, and says why.
///
/// Every choice here already existed and each was made somewhere different: the
/// pool in `decide_rank_main.cpp`, the leaf route inside `rank_one_basis.cpp`,
/// the device in [`../run_limits/device.h`](../run_limits/device.h) with nothing
/// calling it at all, and the remaining four in the argument loop. Scattered,
/// they could not be reported together, could not be overridden together, and
/// could not be written down and replayed somewhere else, which is exactly what
/// a cloud run reproducing a laptop's decisions needs.
///
/// **No rule moved and none of them is restated here.** `chosen_plan` asks the
/// module that owns each one: `subspace_elements`
/// ([`../exhaustive_search/rank_one_basis.h`](../exhaustive_search/rank_one_basis.h))
/// for the leaf route, `bytes_per_matrix` against `memory_budget`
/// ([`../run_limits/memory_budget.h`](../run_limits/memory_budget.h)) for the
/// pool, `chosen_device` for the device. What is new is that one struct holds
/// all seven answers, one function explains them in the sentences a run prints,
/// and [`plan_file.h`](plan_file.h) round-trips them through a file.
///
/// **Precedence, strongest first: an explicit flag, then a plan file, then the
/// rule.** That is the precedence [`../cli/tunables.h`](../cli/tunables.h)
/// already sets out for the numbers, and a second one would be a second thing to
/// remember.
namespace bilinear_rank {

/// Whether the pool of rank-one maps is held or formed on demand.
enum class Pool { Materialised, Addressed };

/// What the search starts from.
enum class Anchor { Map, Heuristic };

/// What `--device` asked for. `Auto` leaves the answer to `chosen_device`, which
/// is the ranking and the launch floor together.
enum class DeviceRequest { Auto, Cpu, Gpu };

/// The seven choices one run makes, and why the three that have a rule came out
/// the way they did.
///
/// The four without a rule are flags and nothing else, so they carry no reason:
/// a line saying `threads: 1` has already said everything there is to say.
struct SearchPlan {
    Pool pool = Pool::Materialised;
    LeafRoute leaf_route = LeafRoute::Auto;
    run_limits::Device device = run_limits::Device::Cpu;
    std::size_t threads = 1;
    cli::Symmetry quotient;
    OrbitTest orbit_test = OrbitTest::Full;
    Anchor anchor = Anchor::Map;

    /// Empty on a plan read back from a file, which carries the decisions and
    /// not the arithmetic that reached them.
    std::string pool_reason;
    std::string leaf_route_reason;
    std::string device_reason;
};

/// What the tensor and this machine say. Everything the rules read is in here,
/// so a rule is testable without a tensor file, a pool, or a card.
struct Machine {
    std::size_t characteristic = 2;
    std::size_t rows = 0;
    std::size_t columns = 0;
    std::size_t pool_size = 0;

    /// The dimension of the deepest leaf, which is `--target`. Zero for a sweep,
    /// whose leaves are of many dimensions and whose route therefore stays
    /// `Auto` and is settled one leaf at a time.
    std::size_t target = 0;

    /// Bytes one bulk allocation may take, from `--max-memory`.
    std::size_t memory_budget = 0;

    /// Whether the GF(2) leaf answers here, from `gf2_leaf_applies`. The card
    /// has kernels for that leaf and for no other, so this decides the device
    /// before any size does.
    bool binary_leaf = false;
};

/// What the command line asked for, each field defaulting to what the rule or
/// the compiled default would have given.
struct PlanRequest {
    DeviceRequest device = DeviceRequest::Auto;
    LeafRoute leaf_route = LeafRoute::Auto;
    std::size_t threads = 1;
    cli::Symmetry quotient;
    OrbitTest orbit_test = OrbitTest::Full;
    Anchor anchor = Anchor::Map;
};

/// Which of the request's fields a flag actually set, so that a plan read from a
/// file can supply the rest. The precedence is `tunables.h`'s and not a second
/// one: an explicit flag, then the file, then the rule.
struct PlanFlagsGiven {
    bool device = false;
    bool leaf_route = false;
    bool threads = false;
    bool quotient = false;
    bool orbit_test = false;
    bool anchor = false;
};

SearchPlan chosen_plan(const Machine& machine, const PlanRequest& request);

/// A plan read from a file, with the flags this command line gave laid over it.
SearchPlan plan_with_flags(const SearchPlan& replayed, const PlanRequest& request,
                           const PlanFlagsGiven& given);

/// Why the card cannot answer a leaf of this run, or null where it can.
///
/// Asked again where a plan arrives from a file, because a plan naming the card
/// is a fact about the machine it was taken on. **A missing card must never
/// produce a wrong answer**, so a replayed `gpu` on a machine with none is
/// declined with its reason rather than attempted.
const char* card_refusal(const Machine& machine);

/// The seven decisions as `name`, `value`, under the names
/// [`plan_file.h`](plan_file.h) spells them with.
///
/// The printed lines and the written file are both made from this one table, so
/// a field can be added to either only by adding it to both, and the two cannot
/// come to spell a route differently.
std::vector<std::pair<std::string, std::string>> plan_fields(const SearchPlan& plan);

/// The plan as the lines a command prints, in `cli::result()`'s style: one field
/// a line, its value, and its reason in brackets where it has one. The caller
/// indents them, because how far in they sit is the command's business.
std::vector<std::string> plan_lines(const SearchPlan& plan);

/// The words the file format and the printed lines both spell a value with, so
/// the two cannot drift into disagreeing about what `walk` is called.
const char* name_of(Pool pool);
const char* name_of(Anchor anchor);
const char* name_of(LeafRoute route);
const char* name_of(OrbitTest test);
std::string name_of(const cli::Symmetry& quotient);

}  // namespace bilinear_rank
