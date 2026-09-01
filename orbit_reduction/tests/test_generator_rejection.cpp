/// That the cheap rejection rule answers what the exact one answers, and that
/// nothing it strikes out was the last member of its orbit.
///
/// `--orbit-test generators` replaces an orbit walk with `|residual|` lookups
/// and therefore leaves duplicate branches standing. Duplicates cost nodes and
/// nothing else; what would not be harmless is a rule that removed **every**
/// representative of some orbit, because that turns a refutation into a lower
/// bound nobody downstream can see is false — the same failure mode
/// `test_symmetry_agreement.cpp` exists for, reached by a different route.
///
/// So two things are asserted here. First, that both rules reach the same
/// verdict on every question asked, satisfiable and refuted, which is the check
/// that matters. Second, the property the derivation in
/// [`../isomorph_rejection.h`](../isomorph_rejection.h) turns on: with the
/// orbits brute-forced from all 216 symmetries of `⟨2,2,2⟩`, at least one member
/// of each survives the generator-only rule. A sabotaged rule that strikes out
/// the orbit minimum is run through the same check, and it must fail it, or the
/// check is asserting nothing.
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "automorphism.h"
#include "candidate_pool.h"
#include "check.h"
#include "exhaustive_search.h"
#include "group_construction.h"
#include "isomorph_rejection.h"
#include "orbit_search.h"
#include "parallel.h"
#include "pool_orbits.h"
#include "tensor_file.h"

namespace {

using bilinear_rank::Automorphism;
using bilinear_rank::Field;
using bilinear_rank::Matrix;
using bilinear_rank::OrbitTest;
using bilinear_rank::PoolAction;

/// What a search concluded, keeping "refuted" and "ran out of budget" apart.
/// Collapsing those two is how a budget becomes a proof.
enum class Verdict { Found, Refuted, Undecided };

const char* name_of(Verdict verdict) {
    switch (verdict) {
        case Verdict::Found: return "found";
        case Verdict::Refuted: return "refuted";
        case Verdict::Undecided: return "undecided";
    }
    return "?";
}

struct Run {
    Verdict verdict = Verdict::Undecided;
    long long nodes = 0;
};

/// One quotiented search under one rejection rule. The rule is process-wide, so
/// it is put back to the default the moment the search returns: a test that
/// leaves it moved would silently change every later question in this file.
Run quotiented(const Field& field, const std::vector<Matrix>& slices,
               const std::vector<Matrix>& pool, const std::vector<Automorphism>& group,
               std::size_t target, OrbitTest test) {
    run_limits::set_worker_count(1);
    bilinear_rank::set_orbit_test(test);

    bilinear_rank::SearchBudget budget{4'000'000};
    std::vector<Matrix> products;
    const bool found = bilinear_rank::expand_subspace_up_to_symmetry(field, slices, pool, group,
                                                                     target, budget, products);
    bilinear_rank::set_orbit_test(OrbitTest::Full);

    Run run;
    run.nodes = static_cast<long long>(budget.nodes_visited.load());
    run.verdict = found ? Verdict::Found
                        : (budget.tree_fully_walked ? Verdict::Refuted : Verdict::Undecided);
    return run;
}

/// Whether a question names a matrix multiplication shape, in which case the
/// group comes from the closed form; otherwise every automorphism pair is built,
/// which is only affordable on the small maps.
struct Question {
    const char* fixture;
    std::size_t target;
    Verdict expected;
    std::size_t matmul[3];  // all zero when the ambient group is built by brute force
};

constexpr Question kQuestions[] = {
    {"f2_2x2", 3, Verdict::Found, {0, 0, 0}},
    {"f2_2x2", 2, Verdict::Refuted, {0, 0, 0}},
    {"f2_2x3", 5, Verdict::Found, {0, 0, 0}},
    {"f2_2x3", 4, Verdict::Refuted, {0, 0, 0}},
    {"matmul_2x2x2", 7, Verdict::Found, {2, 2, 2}},
    {"matmul_2x2x2", 6, Verdict::Refuted, {2, 2, 2}},
};

std::vector<Automorphism> group_for(const Question& question, const Field& field,
                                    const std::vector<Matrix>& slices) {
    if (question.matmul[0] != 0) {
        bilinear_rank::require_matmul_shape(slices, question.matmul[0], question.matmul[1],
                                            question.matmul[2]);
        return bilinear_rank::stabiliser_of(
            field, slices,
            bilinear_rank::matrix_multiplication_symmetry_generators(
                field, question.matmul[0], question.matmul[1], question.matmul[2]));
    }
    return bilinear_rank::stabiliser_of(
        field, slices,
        bilinear_rank::all_automorphisms(field, slices.front().rows(), slices.front().columns()));
}

/// The orbits of `0 .. count-1` under a group given element by element, each as
/// the list of its members in increasing order.
///
/// Brute force on purpose: `pool_orbit_representatives` gets the same answer by
/// orbit-stabiliser and Schreier's lemma, and using it here would check that
/// routine against itself. The whole 216-element group applied to all 225 pool
/// indices is 48 600 lookups, which is nothing, and it is the one construction
/// in this file that assumes nothing at all.
std::vector<std::vector<std::uint32_t>> orbits_of_the_pool(const PoolAction& whole,
                                                           std::size_t elements,
                                                           std::size_t count) {
    std::vector<std::uint32_t> owner(count, static_cast<std::uint32_t>(-1));
    std::vector<std::vector<std::uint32_t>> orbits;
    for (std::uint32_t start = 0; start < count; ++start) {
        if (owner[start] != static_cast<std::uint32_t>(-1)) continue;
        owner[start] = static_cast<std::uint32_t>(orbits.size());
        std::vector<std::uint32_t> members{start};
        for (std::size_t element = 0; element < elements; ++element) {
            const std::uint32_t image = whole.image(element, start);
            if (owner[image] != static_cast<std::uint32_t>(-1)) continue;
            owner[image] = static_cast<std::uint32_t>(orbits.size());
            members.push_back(image);
        }
        std::sort(members.begin(), members.end());
        orbits.push_back(std::move(members));
    }
    return orbits;
}

/// The generator-only rule with the one member it is supposed to protect removed.
///
/// This is not an arbitrary break. Step 2 of the derivation says the least member
/// of `orbit ∩ [from, |pool|)` cannot be sent below itself by any single element,
/// and is therefore the member that always survives; striking exactly that one
/// out is the smallest edit that contradicts the step, and if the coverage check
/// cannot see it the check is worthless.
bool sabotaged(const PoolAction& action, const std::vector<std::uint32_t>& residual,
               std::uint32_t point, std::uint32_t from, std::uint32_t orbit_minimum) {
    if (point == orbit_minimum) return false;
    return bilinear_rank::least_under_generators(action, residual, point, from);
}

/// How many orbits keep nobody, over every live suffix the search can reach.
///
/// `from = 0` is the case the coverage claim is usually stated at and is the
/// weakest of them: an orbit is largest there, so it has the most chances to keep
/// somebody. The recursion advances `from` at every level, and by the tail of the
/// pool an orbit meets the live suffix in one element, which is where a rule that
/// loses the minimum loses the orbit outright. So both are counted.
struct Coverage {
    long long emptied_at_zero = 0;
    long long emptied_anywhere = 0;
};

template <typename Rule>
Coverage coverage_under(const std::vector<std::vector<std::uint32_t>>& orbits, std::size_t count,
                        Rule rule) {
    Coverage coverage;
    for (std::uint32_t from = 0; from < count; ++from) {
        for (const std::vector<std::uint32_t>& orbit : orbits) {
            const auto live = std::lower_bound(orbit.begin(), orbit.end(), from);
            if (live == orbit.end()) continue;  // the orbit is behind the frontier

            bool kept = false;
            for (auto member = live; member != orbit.end() && !kept; ++member) {
                if (rule(*member, from, *live)) kept = true;
            }
            if (kept) continue;
            ++coverage.emptied_anywhere;
            if (from == 0) ++coverage.emptied_at_zero;
        }
    }
    return coverage;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc <= 1) {
        std::cout << "usage: test_generator_rejection <fixtures>\n";
        return 1;
    }
    const std::string directory = argv[1];

    // The verdicts, which is the check that matters. The node counts are printed
    // beside them because the ratio between the two is the whole point of the
    // cheap rule; they are not asserted on, since they are a property of the pool
    // order as much as of the rule and
    // `../what-the-quotient-costs.md` says so.
    for (const Question& question : kQuestions) {
        const formats::Tensor tensor =
            formats::read_tensor_file(directory + "/" + question.fixture + ".tensor");
        const Field field(tensor.characteristic);
        const std::vector<Matrix> pool =
            bilinear_rank::all_rank_one_maps(field, tensor.rows(), tensor.columns());
        const std::vector<Automorphism> group = group_for(question, field, tensor.slices);

        const std::string label =
            std::string(question.fixture) + " at k = " + std::to_string(question.target);
        const Run exact = quotiented(field, tensor.slices, pool, group, question.target,
                                     OrbitTest::Full);
        const Run cheap = quotiented(field, tensor.slices, pool, group, question.target,
                                     OrbitTest::Generators);

        check::equal(label + ", exact rule", static_cast<long long>(exact.verdict),
                     static_cast<long long>(question.expected));
        check::equal(label + ", generator rule", static_cast<long long>(cheap.verdict),
                     static_cast<long long>(question.expected));
        std::cout << "        " << label << ": " << exact.nodes << " nodes exactly, "
                  << cheap.nodes << " by generators (" << name_of(exact.verdict) << ")\n";
    }

    // Orbit coverage on `⟨2,2,2⟩`, whose group is the largest here and whose
    // generators are the six the search is actually handed.
    {
        const formats::Tensor tensor =
            formats::read_tensor_file(directory + "/matmul_2x2x2.tensor");
        const Field field(tensor.characteristic);
        const std::size_t rows = tensor.rows();
        const std::size_t columns = tensor.columns();

        const std::vector<Automorphism> whole_group = bilinear_rank::stabiliser_of(
            field, tensor.slices,
            bilinear_rank::matrix_multiplication_symmetries(field, 2, 2, 2));
        const std::vector<Automorphism> generators = bilinear_rank::stabiliser_of(
            field, tensor.slices,
            bilinear_rank::matrix_multiplication_symmetry_generators(field, 2, 2, 2));
        check::equal("the whole group of <2,2,2>",
                     static_cast<long long>(whole_group.size()), 216);
        check::equal("the generators the search is handed",
                     static_cast<long long>(generators.size()), 6);

        const std::size_t count =
            bilinear_rank::all_rank_one_maps(field, rows, columns).size();
        check::equal("the pool it acts on", static_cast<long long>(count), 225);

        const PoolAction whole(field, whole_group, rows, columns);
        const PoolAction by_generators(field, generators, rows, columns);
        const std::vector<std::vector<std::uint32_t>> orbits =
            orbits_of_the_pool(whole, whole_group.size(), count);
        check::equal("orbits of the pool, brute-forced from all 216",
                     static_cast<long long>(orbits.size()), 5);

        std::vector<std::uint32_t> residual(generators.size());
        for (std::uint32_t index = 0; index < generators.size(); ++index) residual[index] = index;

        const Coverage kept = coverage_under(
            orbits, count, [&](std::uint32_t member, std::uint32_t from, std::uint32_t) {
                return bilinear_rank::least_under_generators(by_generators, residual, member,
                                                             from);
            });
        check::equal("orbits emptied by the generator rule at from = 0", kept.emptied_at_zero, 0);
        check::equal("orbits emptied by the generator rule at any from", kept.emptied_anywhere, 0);

        // And the same check against a rule that is wrong in the one way the
        // derivation forbids. A count of zero here would mean the two checks
        // above pass whatever the rule does, which is the only way they could be
        // reassuring and worthless at once.
        const Coverage broken = coverage_under(
            orbits, count,
            [&](std::uint32_t member, std::uint32_t from, std::uint32_t minimum) {
                return sabotaged(by_generators, residual, member, from, minimum);
            });
        std::cout << "        sabotage: " << broken.emptied_at_zero
                  << " orbits emptied at from = 0, " << broken.emptied_anywhere
                  << " over all " << count << " suffixes\n";
        if (broken.emptied_anywhere <= 0) {
            std::cout << "  FAIL  the coverage check cannot see a rule that drops the orbit "
                         "minimum, so it is asserting nothing\n";
            ++check::failure_count;
        } else {
            std::cout << "  ok    the coverage check sees the sabotage\n";
        }

        // **And this is why the sweep over `from` is in the check at all.**
        // `⟨2,2,2⟩` has five orbits over 225 elements, so at `from = 0` each is
        // large enough that dropping its minimum still leaves somebody standing,
        // and a coverage check written at `from = 0` alone would pass a rule that
        // loses whole orbits deeper in the tree. Asserted rather than remarked,
        // so that a future orbit structure which does break at zero is a visible
        // change here and not a silent strengthening.
        check::equal("from = 0 alone cannot see the sabotage, which is why it is not alone",
                     broken.emptied_at_zero, 0);
    }

    return check::report("generator-only isomorph rejection");
}
