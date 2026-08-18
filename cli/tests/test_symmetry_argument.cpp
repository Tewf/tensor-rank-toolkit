/// That the symmetry option is read strictly, because the parser it replaced
/// was not.
///
/// The old one skipped the word after `--symmetry` without looking at it, so a
/// misspelt group name was accepted and its dimensions read as the shape, and
/// `--symmetry 2 2 2` read a dimension as the group's name. Both ran a
/// quotiented search under a group nobody asked for.
#include <string>
#include <vector>

#include "check.h"
#include "symmetry_argument.h"

namespace {

/// Parse a command line, reporting whether it was accepted at all.
bool accepted(std::vector<std::string> words, cli::Symmetry& symmetry) {
    std::vector<char*> argv;
    for (std::string& word : words) argv.push_back(word.data());

    int position = 0;  // sits on "--symmetry", as a caller's loop would
    try {
        symmetry = cli::parse_symmetry(static_cast<int>(argv.size()), argv.data(), position);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

void check_accepts() {
    cli::Symmetry symmetry;

    check::equal("none is accepted", accepted({"--symmetry", "none"}, symmetry), 1);
    check::equal("and asks for no group",
                 symmetry.kind == cli::SymmetryKind::None, 1);

    check::equal("auto is accepted", accepted({"--symmetry", "auto"}, symmetry), 1);
    check::equal("and asks for the map's own group",
                 symmetry.kind == cli::SymmetryKind::Automatic, 1);

    check::equal("matmul with three dimensions is accepted",
                 accepted({"--symmetry", "matmul", "2", "3", "4"}, symmetry), 1);
    check::equal("and keeps all three", static_cast<long long>(symmetry.shape.size()), 3);
    check::equal("in order", static_cast<long long>(symmetry.shape[1]), 3);
}

void check_refuses() {
    cli::Symmetry symmetry;

    check::equal("a misspelt group is refused",
                 accepted({"--symmetry", "matmol", "2", "2", "2"}, symmetry), 0);
    check::equal("a missing group name is refused",
                 accepted({"--symmetry", "2", "2", "2"}, symmetry), 0);
    check::equal("a flag where a dimension belongs is refused",
                 accepted({"--symmetry", "matmul", "--target", "7", "2"}, symmetry), 0);
    check::equal("too few dimensions are refused",
                 accepted({"--symmetry", "matmul", "2", "2"}, symmetry), 0);
    check::equal("a zero dimension is refused",
                 accepted({"--symmetry", "matmul", "2", "0", "2"}, symmetry), 0);
    check::equal("nothing at all is refused", accepted({"--symmetry"}, symmetry), 0);
}

}  // namespace

int main() {
    check_accepts();
    check_refuses();
    return check::report("symmetry argument");
}
