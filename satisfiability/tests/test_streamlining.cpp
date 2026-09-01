// The shaping devices against the 2x2x2 fixture, whose quota the formula
// derives instead of hardcoding: 8 odd entries over 7 products is one double
// and six singles, the small case of Heule's 19 and 4.
#include <cstdlib>
#include <set>
#include <string>
#include <vector>

#include "binary_encoding.h"
#include "check.h"
#include "streamlining.h"
#include "tensor_file.h"

int main(int argc, char** argv) {
    const std::string fixtures = argc > 1 ? argv[1] : "fixtures";
    const auto tensor = linear_algebra::read_tensor_file(fixtures + "/matmul_2x2x2.tensor");

    // The type test doubles as the shape check: a wrong inner dimension throws.
    {
        auto encoding = satisfiability::encode_binary_rank_at_most(tensor, 7);
        bool refused = false;
        try {
            satisfiability::streamline_matmul(encoding, {0, 4, 0}, {});
        } catch (const std::invalid_argument&) {
            refused = true;
        }
        check::equal("a wrong inner dimension is refused", refused ? 1 : 0, 1);
    }

    auto encoding = satisfiability::encode_binary_rank_at_most(tensor, 7);
    const std::size_t before = encoding.formula.clauses.size();
    satisfiability::Streamliners devices;
    devices.pair_type3 = true;
    devices.zero_fraction = 0.5;
    devices.seed = 3;
    satisfiability::streamline_matmul(encoding, {0, 2, 0}, devices);

    // The pairing adds one unit per (odd entry, product): 8 entries times 7.
    // The zeroing adds about half of the light terms' pool, which excludes the
    // cross terms the pairing forces, so its exact count depends on the draw.
    // Every added clause is a unit, and no literal may carry both signs: a
    // contradictory pair is the cross-term channel the closure exists to close.
    const std::size_t added = encoding.formula.clauses.size() - before;
    check::equal("the pairing's units and a nonempty zeroing are there",
                 added > 8 * 7 + 100 ? 1 : 0, 1);
    std::size_t units = 0, positives = 0;
    std::set<int> seen;
    std::size_t contradictions = 0;
    for (std::size_t index = before; index < encoding.formula.clauses.size(); ++index) {
        const auto& clause = encoding.formula.clauses[index];
        units += clause.size() == 1;
        positives += clause.size() == 1 && clause.front() > 0;
        if (clause.size() == 1) {
            contradictions += seen.count(-clause.front());
            seen.insert(clause.front());
        }
    }
    check::equal("every added clause is a unit", static_cast<long long>(units),
                 static_cast<long long>(added));
    check::equal("no unit contradicts another", static_cast<long long>(contradictions), 0);
    // One positive unit per odd entry: each type-3 term lives in exactly one product.
    check::equal("each odd entry is assigned one product", static_cast<long long>(positives), 8);

    // The quota: reading the positive units back, one product owns two odd
    // entries and the other six own one each.
    std::vector<int> owned(7, 0);
    for (std::size_t index = 0; index < encoding.formula.parities.size(); ++index) {
        const auto& parity = encoding.formula.parities[index];
        if (!parity.value) continue;
        for (std::size_t term = 0; term < parity.literals.size(); ++term) {
            for (std::size_t clause = before; clause < encoding.formula.clauses.size(); ++clause) {
                if (encoding.formula.clauses[clause].size() == 1 &&
                    encoding.formula.clauses[clause].front() == parity.literals[term]) {
                    owned[term] += 1;
                }
            }
        }
    }
    int doubles = 0, singles = 0;
    for (int count : owned) {
        doubles += count == 2;
        singles += count == 1;
    }
    check::equal("one product owns two odd entries", static_cast<long long>(doubles), 1);
    check::equal("six products own one each", static_cast<long long>(singles), 6);

    return check::report("streamlining");
}
