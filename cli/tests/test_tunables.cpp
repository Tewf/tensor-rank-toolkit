/// That the shipped file says exactly what the code compiles in, and that a
/// typo in it stops the run.
///
/// **What this file cannot see, and `check_tunables_bound_a_run.sh` beside it
/// does.** Every check here reads `tunables.h` on both sides of the comparison,
/// so all of it passed throughout the period when nothing outside `cli/`
/// included that header: each command was still bounded by its own literal and
/// the file bounded nothing at all. The sibling script spends a process on the
/// question this one cannot ask, which is whether a number in a file reaches the
/// search.
///
/// `tunables.h` promises that "every default here is the number that was
/// compiled in before, so a machine with no file behaves exactly as it did", and
/// that `tunables.conf` "writes out every default, so the format needs no
/// documentation apart from itself". Both are claims about two files agreeing,
/// and nothing checked them: the file could drift from the defaults and every
/// published number would quietly depend on which directory a command ran in.
/// That is what the last check here is for.
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "check.h"
#include "tunables.h"

namespace {

/// The refusal a bad line produces, or "" if it was accepted. The message is the
/// interface here exactly as it is for the argument parser, so it is compared
/// whole rather than merely asserted to be non-empty.
std::string refusal(const std::string& text) {
    std::istringstream stream(text);
    cli::Tunables values;
    try {
        cli::read_tunables_from(stream, "test.conf", values);
    } catch (const cli::ArgumentError& refused) {
        return refused.what();
    }
    return "";
}

void check_the_defaults_are_the_documented_numbers() {
    const cli::Tunables values;
    check::equal("search node limit", static_cast<long long>(values.search_node_limit), 5'000'000);
    check::equal("ilp node limit", static_cast<long long>(values.ilp_node_limit), 200'000);
    check::equal("plateau state budget", static_cast<long long>(values.plateau_state_budget),
                 200'000);
    check::equal("sat memory follows the machine", static_cast<long long>(values.sat_memory_megabytes),
                 static_cast<long long>(run_limits::suggested_memory_budget() >> 20));
    check::equal("sat timeout", static_cast<long long>(values.sat_timeout_seconds), 300);
    check::equal("ilp time limit", static_cast<long long>(values.ilp_time_limit_seconds), 300);
    check::text("kissat is asked first", values.sat_solver_order.front(), "kissat");
    check::text("the built-in trails", values.ilp_backend_order.back(), "built-in");
}

void check_a_file_is_read_the_way_the_output_is_written() {
    cli::Tunables values;
    std::istringstream stream(
        "# a whole-line comment\n"
        "\n"
        "search_node_limit = 17   # and a trailing one\n"
        "  sat_solver_order = cadical kissat  \n");
    cli::read_tunables_from(stream, "test.conf", values);

    check::equal("a value is read", static_cast<long long>(values.search_node_limit), 17);
    check::equal("a list is read", static_cast<long long>(values.sat_solver_order.size()), 2);
    check::text("in order", values.sat_solver_order.front(), "cadical");
    check::equal("and what the file omits keeps its default",
                 static_cast<long long>(values.sat_timeout_seconds), 300);
}

void check_a_typo_stops_the_run() {
    check::text("an unknown name is refused rather than ignored",
                refusal("search_node_limit_typo = 5\n"),
                "test.conf:1: no tunable is called 'search_node_limit_typo'");
    check::text("a line that is not name = value says so", refusal("search_node_limit 5\n"),
                "test.conf:1: 'search_node_limit 5' is not a 'name = value' line");
    check::text("a count that is not one names the tunable", refusal("sat_timeout_seconds = soon\n"),
                "test.conf:1 sat_timeout_seconds expects a count, not 'soon'");
    check::text("an empty list is refused", refusal("sat_solver_order =\n"),
                "test.conf:1 sat_solver_order expects at least one name");
    check::text("a good file is accepted", refusal("ilp_node_limit = 5\n"), "");
}

/// Which tunables the file actually sets, by name, comments stripped the way
/// `read_tunables_from` strips them.
///
/// Needed because the comparison below cannot see an omission. It seeds
/// `from_file` with the compiled defaults and reads the file over the top, so a
/// field the file never mentions keeps the compiled value and is then compared
/// against the compiled value: the line prints `ok` and covers nothing. Deleting
/// three fields from `tunables.conf` left this file passing, which is the same
/// shape as the fault its own header comment describes, one level up.
std::set<std::string> names_set_by(const std::string& path) {
    std::set<std::string> named;
    std::ifstream file(path);
    std::string line;
    while (std::getline(file, line)) {
        const std::string content = cli::trimmed(line.substr(0, line.find('#')));
        const std::size_t split = content.find('=');
        if (content.empty() || split == std::string::npos) continue;
        named.insert(cli::trimmed(content.substr(0, split)));
    }
    return named;
}

/// The one that earns this file's place: the shipped `tunables.conf` must set
/// every tunable to the value already compiled in, or deleting the file changes
/// behaviour and every table in the README depends on a working directory.
///
/// "Set" is two claims, and both are checked: that the file names the tunable at
/// all, and that the value it names is the compiled one. Only the second was
/// checked before, and the first is what `tunables.h` promises when it says the
/// file "writes out every default, so the format needs no documentation apart
/// from itself".
void check_the_shipped_file_matches_the_defaults(const std::string& root) {
    const std::string path = root + "/tunables.conf";
    std::ifstream file(path);
    if (!file) {
        check::text("tunables.conf is where the tests were told to look", path, "a readable file");
        return;
    }

    const std::set<std::string> named = names_set_by(path);
    cli::Tunables from_file;
    cli::read_tunables_from(file, path, from_file);
    const cli::Tunables compiled;

    for (const auto& counted : cli::counted_tunables()) {
        check::equal("tunables.conf writes out " + counted.first,
                     static_cast<long long>(named.count(counted.first)), 1);
    }
    for (const auto& listed : cli::listed_tunables()) {
        check::equal("tunables.conf writes out " + listed.first,
                     static_cast<long long>(named.count(listed.first)), 1);
    }

    for (const auto& [name, field] : cli::counted_tunables()) {
        check::equal("tunables.conf " + name + " is the compiled default",
                     static_cast<long long>(from_file.*field),
                     static_cast<long long>(compiled.*field));
    }
    for (const auto& [name, field] : cli::listed_tunables()) {
        const std::vector<std::string>& read = from_file.*field;
        const std::vector<std::string>& built = compiled.*field;
        std::string joined;
        for (const std::string& word : read) joined += (joined.empty() ? "" : " ") + word;
        std::string expected;
        for (const std::string& word : built) expected += (expected.empty() ? "" : " ") + word;
        check::text("tunables.conf " + name + " is the compiled default", joined, expected);
    }
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        check::text("this test needs the repository root as its one argument", "nothing", "a path");
        return check::report("tunables");
    }

    check_the_defaults_are_the_documented_numbers();
    check_a_file_is_read_the_way_the_output_is_written();
    check_a_typo_stops_the_run();
    check_the_shipped_file_matches_the_defaults(argv[1]);
    return check::report("tunables");
}
