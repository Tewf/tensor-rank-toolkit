#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

/// Which group a search quotients its choices by, read off the command line.
///
/// Every search here can be run twice, once trying every candidate and once
/// trying one per orbit, and the two must agree wherever both finish. That is
/// the test that catches a wrong symmetry, and a wrong symmetry is the one
/// mistake nothing downstream catches: it turns a satisfiable question
/// unsatisfiable in silence, which is a false lower bound. So the choice is one
/// argument, spelled the same way by every command, parsed in one place.
///
/// It replaces three parsers that had drifted apart. The worst of them skipped
/// the word after `--symmetry` without reading it, so `--symmetry matmol 2 2 2`
/// was accepted, and so was `--symmetry 2 2 2`, which then read a dimension as
/// the group's name and the remaining two as the shape.
namespace cli {

enum class SymmetryKind {
    None,                   ///< try every candidate; the default everywhere
    Automatic,              ///< the map's own stabiliser, computed for this map
    MatrixMultiplication,   ///< the closed-form orbits of `⟨n,m,k⟩`
};

struct Symmetry {
    SymmetryKind kind = SymmetryKind::None;
    std::vector<std::size_t> shape;  ///< three entries, and only for `matmul`
};

inline const char* symmetry_usage() {
    return "  -s, --symmetry none|auto|matmul <n> <m> <k>\n"
           "        none    try every candidate, and the default, so a command\n"
           "                not asked for symmetry answers as it always did\n"
           "        auto    quotient by this map's own stabiliser; works on any\n"
           "                map, and refuses when the group is too large to build\n"
           "        matmul  quotient by the symmetries of <n,m,k>, whose orbits\n"
           "                are known in closed form and need no group at all\n";
}

/// A dimension, refused rather than guessed at.
///
/// `std::stoull` would read `--target` as zero and carry on, which is how a flag
/// becomes a shape.
inline std::size_t parse_dimension(const std::string& text) {
    if (text.empty() || text.find_first_not_of("0123456789") != std::string::npos) {
        throw std::runtime_error("'" + text + "' is not a dimension");
    }
    const std::size_t value = std::stoull(text);
    if (value == 0) throw std::runtime_error("a dimension cannot be zero");
    return value;
}

/// Read the words after `--symmetry`, leaving `position` on the last one
/// consumed so a caller's `for (++argument)` loop steps past it.
inline Symmetry parse_symmetry(int argc, char** argv, int& position) {
    if (position + 1 >= argc) {
        throw std::runtime_error("--symmetry needs none, auto, or matmul <n> <m> <k>");
    }
    const std::string kind = argv[++position];

    Symmetry symmetry;
    if (kind == "none") return symmetry;
    if (kind == "auto") {
        symmetry.kind = SymmetryKind::Automatic;
        return symmetry;
    }
    if (kind != "matmul") {
        throw std::runtime_error("'" + kind + "' is not a symmetry: expected none, auto or matmul");
    }

    symmetry.kind = SymmetryKind::MatrixMultiplication;
    for (int part = 0; part < 3; ++part) {
        if (position + 1 >= argc) {
            throw std::runtime_error("matmul needs three dimensions, <n> <m> <k>");
        }
        symmetry.shape.push_back(parse_dimension(argv[++position]));
    }
    return symmetry;
}

}  // namespace cli
