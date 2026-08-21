#include "plan_file.h"

#include <fstream>
#include <sstream>
#include <vector>

#include "arguments.h"

namespace bilinear_rank {

namespace {

/// One value refused with the field it belongs to and the words it could have
/// been, which is `arguments.h`'s standard: a refusal that names neither is the
/// fault this repository already removed from its command lines once.
[[noreturn]] void refuse(const std::string& where, const std::string& field,
                         const std::string& word, const std::string& expected) {
    throw cli::ArgumentError(where + " " + field + " expects " + expected + ", not '" + word + "'");
}

cli::Symmetry quotient_from(const std::vector<std::string>& words, const std::string& where) {
    cli::Symmetry quotient;
    if (words[1] == "none") return quotient;
    if (words[1] == "auto") {
        quotient.kind = cli::SymmetryKind::Automatic;
        return quotient;
    }
    if (words[1] != "matmul" || words.size() != 5) {
        refuse(where, "quotient", words[1], "none, auto, or matmul with three dimensions");
    }
    quotient.kind = cli::SymmetryKind::MatrixMultiplication;
    for (std::size_t part = 2; part < words.size(); ++part) {
        quotient.shape.push_back(cli::parse_count(where + " quotient", words[part]));
    }
    return quotient;
}

void set_field(SearchPlan& plan, const std::vector<std::string>& words, const std::string& where) {
    const std::string& field = words[0];
    const std::string& word = words[1];
    if (field == "pool") {
        if (word == "materialised") plan.pool = Pool::Materialised;
        else if (word == "addressed") plan.pool = Pool::Addressed;
        else refuse(where, field, word, "materialised or addressed");
    } else if (field == "leaf_route") {
        if (word == "scan") plan.leaf_route = LeafRoute::Scan;
        else if (word == "walk") plan.leaf_route = LeafRoute::Walk;
        else if (word == "auto") plan.leaf_route = LeafRoute::Auto;
        else refuse(where, field, word, "scan, walk or auto");
    } else if (field == "device") {
        if (word == "cpu") plan.device = run_limits::Device::Cpu;
        else if (word == "gpu") plan.device = run_limits::Device::Gpu;
        else refuse(where, field, word, "cpu or gpu");
    } else if (field == "threads") {
        plan.threads = cli::parse_count(where + " threads", word);
    } else if (field == "quotient") {
        plan.quotient = quotient_from(words, where);
    } else if (field == "orbit_test") {
        if (word == "full") plan.orbit_test = OrbitTest::Full;
        else if (word == "generators") plan.orbit_test = OrbitTest::Generators;
        else refuse(where, field, word, "full or generators");
    } else if (field == "anchor") {
        if (word == "map") plan.anchor = Anchor::Map;
        else if (word == "heuristic") plan.anchor = Anchor::Heuristic;
        else refuse(where, field, word, "map or heuristic");
    } else {
        throw cli::ArgumentError(where + ": no plan field is called '" + field + "'");
    }
}

}  // namespace

void write_plan(std::ostream& out, const SearchPlan& plan) {
    out << "# A search plan, from decide-rank --plan-out. --plan-in replays it.\n"
           "# `name value`, one per line; # starts a comment. Every field is written,\n"
           "# so a plan says what it chose and never what it left to a rule.\n";
    for (const auto& [name, value] : plan_fields(plan)) out << name << " " << value << "\n";
}

void write_plan_file(const std::string& path, const SearchPlan& plan) {
    std::ofstream file(path);
    if (!file) throw cli::ArgumentError("--plan-out cannot write '" + path + "'");
    write_plan(file, plan);
}

SearchPlan read_plan(std::istream& text, const std::string& path) {
    SearchPlan plan;
    std::string line;
    for (std::size_t number = 1; std::getline(text, line); ++number) {
        const std::string content = line.substr(0, line.find('#'));
        std::istringstream words(content);
        std::vector<std::string> read;
        for (std::string word; words >> word;) read.push_back(word);
        if (read.empty()) continue;

        const std::string where = path + ":" + std::to_string(number);
        if (read.size() < 2) {
            throw cli::ArgumentError(where + ": '" + read[0] + "' is not a 'name value' line");
        }
        set_field(plan, read, where);
    }
    plan.pool_reason = "from " + path;
    plan.leaf_route_reason = plan.pool_reason;
    plan.device_reason = plan.pool_reason;
    return plan;
}

SearchPlan read_plan_file(const std::string& path) {
    std::ifstream file(path);
    if (!file) throw cli::ArgumentError("--plan-in names '" + path + "', which cannot be read");
    return read_plan(file, path);
}

}  // namespace bilinear_rank
