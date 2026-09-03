/// `price-canonical-route`: what the canonical route's operations cost here.
///
/// [`canonical_route_price.h`](canonical_route_price.h) decides whether
/// `--route canonical` is worth taking from four nanosecond prices. This
/// measures them, so the numbers and the thing that produced them ship together,
/// as `measure-leaf floor` does for `device_launch_floor`.
///
/// **An instrument and not a tool, which is why it is here and not in
/// `commands/`.** What it prints is nanoseconds, and
/// [`../MEASURING.md`](../MEASURING.md)'s line is that counts reproduce anywhere
/// and timings do not: a reader arriving with a bilinear map has no question
/// this answers. `measure-leaf` sits exactly here relative to `gpu_leaf/`, and
/// [`../OPTIONS/one-question-per-command.md`](../OPTIONS/one-question-per-command.md)
/// is where the twelve that *are* tools are each given their one question.
///
/// It prices one pool scan, one canonical image, one setwise stabiliser and one
/// presentation, prints them in the units `tunables.conf` spells, and then prints
/// the predicate's verdict for the shape so a reading can be checked against a
/// sweep without arithmetic in between.
#include <cstdint>
#include <string>
#include <vector>

#include "arguments.h"
#include "bilinear_rank_aliases.h"
#include "candidate_pool.h"
#include "canonical_route_price.h"
#include "exit_code.h"
#include "group_construction.h"
#include "pool_orbits.h"
#include "pool_set_canon.h"
#include "report.h"
#include "span_basis.h"
#include "symmetry_argument.h"
#include "tensor_file.h"
#include "timing.h"

namespace {

void usage() {
    cli::note() << "usage: price-canonical-route <tensor-file> -s matmul <n> <m> <k>\n"
                   "                             [--target k] [--calls N] [--help]\n"
                   "\n"
                   "  --target k   the subspace dimension to price a node at, which decides\n"
                   "               how many levels of augmentation the shape is asked for.\n"
                   "               The tensor's own span dimension by default, plus one\n"
                   "  --calls N    calls to time each group operation over, 32 by default.\n"
                   "               The minimum is reported, which is the estimate of the\n"
                   "               work rather than of what else the machine was doing\n"
                   "  --help       print this and stop, as exit 2\n"
                   "\n"
                   "  exit: 0 priced  2 usage  5 error";
}

/// The orbits of the pool under the group, and the sum of their squared sizes.
///
/// The one input to `price_canonical_route` that no closed form gives, and the
/// one the one-level clause turns on: `least_in_orbit` names a representative by
/// a breadth-first walk whose `seen` list is scanned linearly, so the plain root
/// costs `sum |O_i|^2` where `orbit_representatives` costs `|P|`.
///
/// It is found here the way the plain search's root finds it: from the same
/// `PoolAction`, over the same pool, so the count this reports is the number of
/// children either route's root emits, and `count + 1` is the node count of both.
bilinear_rank::PoolOrbits pool_orbits(const bilinear_rank::PoolAction& action,
                                      std::size_t generators, std::size_t pool_size) {
    std::vector<std::uint32_t> home(pool_size, 0xffffffffu);
    bilinear_rank::PoolOrbits orbits;
    for (std::uint32_t index = 0; index < pool_size; ++index) {
        if (home[index] != 0xffffffffu) continue;
        const std::uint32_t mark = static_cast<std::uint32_t>(orbits.count++);
        std::vector<std::uint32_t> frontier{index};
        home[index] = mark;
        double reached = 0;
        while (!frontier.empty()) {
            const std::uint32_t taken = frontier.back();
            frontier.pop_back();
            ++reached;
            for (std::size_t element = 0; element < generators; ++element) {
                const std::uint32_t image = action.image(element, taken);
                if (home[image] != 0xffffffffu) continue;
                home[image] = mark;
                frontier.push_back(image);
            }
        }
        orbits.summed_squares += reached * reached;
    }
    return orbits;
}

/// A subspace of `target` dimensions above the slices, and its pool content.
///
/// This is the object a node holds, built the way the search builds one: pool
/// elements adjoined in index order until the dimension is reached. Its content is
/// what the parent test canonises, so pricing a canonical image on anything else
/// would price a different call.
std::vector<std::size_t> node_content(const bilinear_rank::Field& field,
                                      const std::vector<bilinear_rank::Matrix>& slices,
                                      const std::vector<bilinear_rank::Matrix>& pool,
                                      std::size_t target) {
    std::vector<bilinear_rank::Matrix> current = slices;
    bilinear_rank::ReducedBasis reached = linear_algebra::span_of(field, slices);
    std::vector<bilinear_rank::Element> scratch;
    for (std::size_t index = 0; index < pool.size() && reached.dimension() < target; ++index) {
        if (reached.contains(pool[index], scratch)) continue;
        reached.try_add(pool[index]);
        current.push_back(pool[index]);
    }
    return bilinear_rank::pool_inside(field, pool, current);
}

/// Seconds for the fastest of `calls` runs of `work`, which is the protocol
/// `MEASURING.md` states for every timing in this repository.
template <class Work>
double fastest(std::size_t calls, Work work) {
    double best = 0;
    for (std::size_t call = 0; call < calls; ++call) {
        const cli::Clock::time_point started = cli::Clock::now();
        work();
        const double taken = cli::elapsed_seconds(started);
        if (call == 0 || taken < best) best = taken;
    }
    return best;
}

int run(int argc, char** argv) {
    cli::Symmetry symmetry;
    long long target = -1;
    std::size_t calls = 32;

    cli::Arguments arguments(argc, argv);
    while (arguments.next_flag()) {
        if (arguments.is("--help", "-h")) {
            usage();
            return cli::exit_status(cli::ExitCode::Usage);
        } else if (arguments.is("--target")) {
            target = arguments.whole_number();
        } else if (arguments.is("--calls")) {
            calls = arguments.count();
        } else if (arguments.is("--symmetry", "-s")) {
            symmetry = arguments.parsed_by(cli::parse_symmetry);
        } else {
            arguments.refuse();
        }
    }
    if (arguments.no_file_named() || symmetry.kind != cli::SymmetryKind::MatrixMultiplication) {
        usage();
        return cli::exit_status(cli::ExitCode::Usage);
    }

    const formats::Tensor tensor = formats::read_tensor_file(arguments.filename());
    const bilinear_rank::Field field(tensor.characteristic);
    const std::vector<bilinear_rank::Matrix> pool =
        bilinear_rank::all_rank_one_maps(field, tensor.rows(), tensor.columns());
    const std::vector<bilinear_rank::Automorphism> generators =
        bilinear_rank::matrix_multiplication_symmetry_generators(field, symmetry.shape[0],
                                                                 symmetry.shape[1],
                                                                 symmetry.shape[2]);

    const std::size_t span = linear_algebra::span_of(field, tensor.slices).dimension();
    const std::size_t level = target < 0 ? span + 1 : static_cast<std::size_t>(target);
    const std::vector<std::size_t> content = node_content(field, tensor.slices, pool, level);

    const double presentation = fastest(3, [&] {
        bilinear_rank::PoolSetCanon built(field, generators, tensor.rows(), tensor.columns());
        (void)built.size();
    });
    const bilinear_rank::PoolSetCanon canon(field, generators, tensor.rows(), tensor.columns());

    const bilinear_rank::ReducedBasis root = linear_algebra::span_of(field, tensor.slices);
    const double scan = fastest(calls, [&] {
        std::vector<bilinear_rank::Element> scratch;
        std::size_t inside = 0;
        for (const bilinear_rank::Matrix& candidate : pool) {
            if (root.contains(candidate, scratch)) ++inside;
        }
        (void)inside;
    });
    const double image = fastest(calls, [&] { (void)canon.canonical(content); });
    const double stabiliser = fastest(calls, [&] { (void)canon.stabiliser_generators(content); });

    bilinear_rank::RouteShape shape;
    shape.characteristic = static_cast<std::size_t>(tensor.characteristic);
    shape.rows = symmetry.shape[0];
    shape.inner = symmetry.shape[1];
    shape.columns = symmetry.shape[2];
    shape.target = level;
    shape.generators = generators.size();

    // Timed on `orbit_representatives`, which is the call the canonical route's
    // root actually makes, and not on `pool_orbits` below, which is this file's
    // own pass and is cheaper because it wants nothing back but the sizes.
    const bilinear_rank::PoolAction action(field, generators, tensor.rows(), tensor.columns());
    std::vector<std::uint32_t> every(pool.size());
    for (std::uint32_t index = 0; index < pool.size(); ++index) every[index] = index;
    const double orbit_pass =
        fastest(calls, [&] { (void)bilinear_rank::orbit_representatives(action, every); });
    const bilinear_rank::PoolOrbits orbits = pool_orbits(action, generators.size(), pool.size());

    const bilinear_rank::RouteVerdict predicted =
        bilinear_rank::price_canonical_route(shape, bilinear_rank::CanonicalPrices(), orbits);

    cli::result() << "shape: <" << shape.rows << "," << shape.inner << "," << shape.columns
                  << ">, pool " << pool.size() << ", degree " << predicted.degree << ", group "
                  << predicted.group_order << ", " << generators.size() << " generators\n";
    cli::result() << "node: dimension " << level << ", " << content.size()
                  << " pool elements inside, " << predicted.levels << " levels above the span\n";
    cli::result() << "measured, fastest of " << calls << ":\n";
    cli::result() << "  pool scan: " << scan << " s, " << scan / pool.size() * 1e9
                  << " ns an element  (pool_scan_nanoseconds)\n";
    cli::result() << "  canonical image: " << image << " s, "
                  << image / predicted.degree * 1e9 << " ns an axis point\n";
    cli::result() << "  setwise stabiliser: " << stabiliser << " s, " << stabiliser / image
                  << "x one canonical image\n";
    cli::result() << "  presentation: " << presentation << " s, "
                  << presentation / predicted.degree * 1e9
                  << " ns an axis point  (presentation_nanoseconds)\n";
    cli::result() << "  orbit pass: " << orbit_pass << " s, " << orbit_pass / pool.size() * 1e9
                  << " ns a pool element  (orbit_pass_nanoseconds)\n";
    cli::result() << "orbits of the pool: " << orbits.count << ", sum of squared sizes "
                  << orbits.summed_squares << ", so a root of either route has "
                  << orbits.count << " children\n";
    cli::result() << "predicted, from the shape and those orbits:\n";
    cli::result() << "  plain node " << predicted.plain_node_seconds << " s, pool scan "
                  << predicted.pool_scan_seconds << " s, canonical image "
                  << predicted.image_seconds << " s, setwise stabiliser "
                  << predicted.stabiliser_seconds << " s\n";
    cli::result() << "  price ratio " << predicted.price_ratio << ", saving ratio "
                  << predicted.saving_ratio << ", canonical seconds over plain "
                  << predicted.predicted_cost << "\n";
    cli::result() << "verdict: "
                  << (predicted.pays ? "canonical augmentation pays here" : predicted.refusal)
                  << "\n";
    return cli::exit_status(cli::ExitCode::Yes);
}

}  // namespace

int main(int argc, char** argv) {
    try {
        return run(argc, argv);
    } catch (const cli::ArgumentError& error) {
        cli::note() << "price-canonical-route: " << error.what();
        return cli::exit_status(cli::ExitCode::Usage);
    } catch (const std::exception& error) {
        cli::note() << "price-canonical-route: " << error.what();
        return cli::exit_status(cli::ExitCode::Error);
    }
}
