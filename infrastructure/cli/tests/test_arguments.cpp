/// That a command line is read strictly, and that a refusal names the flag.
///
/// Both faults asserted against here were in about forty copies of the same
/// loop. `--target` with nothing after it was reported as an unrecognised
/// option, and `--target abc` was reported as `stoull`, which is the name of the
/// function that threw and not of the argument that was wrong. The messages are
/// the interface, so the messages are what these checks compare.
#include <string>
#include <vector>

#include "arguments.h"
#include "check.h"
#include "symmetry_argument.h"

namespace {

/// The loop the commands now write, with one branch of every kind: a bare flag,
/// a string, a signed number, two counts, a size, and a parser that reads several
/// words of its own. What the commands do with it is asserted against the built
/// binaries in `check_argument_grammar.sh`; this file asserts the walk.
struct Parsed {
    long long target = -1;
    std::size_t threads = 1;
    std::size_t steps = 3;
    std::size_t memory = 0;
    bool as_json = false;
    std::string anchor;
    std::string file;
    bool no_file = false;
    bool wants_help = false;
    cli::Symmetry symmetry;
};

/// Empty when the line was accepted, otherwise the refusal, exactly as a caller
/// would print it after its own `command: ` prefix.
std::string refusal(std::vector<std::string> words, Parsed& parsed) {
    std::string program = "decide-rank";
    std::vector<char*> argv{program.data()};
    for (std::string& word : words) argv.push_back(word.data());

    cli::Arguments arguments(static_cast<int>(argv.size()), argv.data());
    try {
        while (arguments.next_flag()) {
            if (arguments.is("--help", "-h")) {
                parsed.wants_help = true;
            } else if (arguments.is("--json")) {
                parsed.as_json = true;
            } else if (arguments.is("--anchor")) {
                parsed.anchor = arguments.text();
            } else if (arguments.is("--target")) {
                parsed.target = arguments.whole_number();
            } else if (arguments.is("--threads")) {
                parsed.threads = arguments.count();
            } else if (arguments.is("--steps")) {
                parsed.steps = arguments.count(1, 3);
            } else if (arguments.is("--max-memory")) {
                parsed.memory = arguments.memory_size();
            } else if (arguments.is("--symmetry", "-s")) {
                parsed.symmetry = arguments.parsed_by(cli::parse_symmetry);
            } else {
                arguments.refuse();
            }
        }
        parsed.file = arguments.filename();
        parsed.no_file = arguments.no_file_named();
        return "";
    } catch (const std::exception& problem) {
        return problem.what();
    }
}

void check_reads_a_line() {
    Parsed parsed;
    check::text("a whole line is accepted",
                refusal({"tensor.sms", "--target", "7", "--threads", "6", "--json"}, parsed), "");
    check::equal("the target is read", parsed.target, 7);
    check::equal("the count is read", static_cast<long long>(parsed.threads), 6);
    check::equal("the bare flag is set", parsed.as_json, 1);
    check::text("the positional word is the file", parsed.file, "tensor.sms");
    check::equal("and a file was named", parsed.no_file, 0);

    Parsed after;
    check::text("the file may come last",
                refusal({"--json", "--anchor", "heuristic", "tensor.sms"}, after), "");
    check::text("and is still the file", after.file, "tensor.sms");
    check::text("a string value is read", after.anchor, "heuristic");

    Parsed absent;
    check::text("no file at all is accepted", refusal({"--json"}, absent), "");
    check::equal("and no_file_named says so", absent.no_file, 1);

    Parsed dash;
    check::text("a lone dash is accepted", refusal({"-", "--json"}, dash), "");
    check::equal("and no_file_named calls it unnamed", dash.no_file, 1);
    check::equal("and is not read as a flag", dash.as_json, 1);

    Parsed sized;
    check::text("a size is accepted", refusal({"--max-memory", "2G"}, sized), "");
    check::equal("and is bytes", static_cast<long long>(sized.memory), 1LL << 31);

    Parsed negative;
    check::text("a negative number is a value, not a flag",
                refusal({"--target", "-1"}, negative), "");
    check::equal("and keeps its sign", negative.target, -1);
}

/// `--help` is where three commands read a flag as a filename and left as 5,
/// "could not run at all", when the line had asked a question this header can
/// answer. Nothing about it is special: it is a flag because it looks like one,
/// and the branch that prints the usage is the command's.
void check_help_is_a_flag_and_not_a_filename() {
    Parsed parsed;
    check::text("--help is accepted", refusal({"--help"}, parsed), "");
    check::equal("and reaches its branch", parsed.wants_help, 1);
    check::text("and is not taken for the file", parsed.file, "");

    Parsed short_form;
    check::text("-h likewise", refusal({"tensor.sms", "-h"}, short_form), "");
    check::equal("and reaches the same branch", short_form.wants_help, 1);
    check::text("leaving the file alone", short_form.file, "tensor.sms");
}

/// The seam with `symmetry_argument.h`: that parser walks several words itself,
/// and the walk has to resume after the last one it took.
void check_hands_over_to_the_symmetry_parser() {
    Parsed parsed;
    check::text("a symmetry and a flag after it are both accepted",
                refusal({"--symmetry", "matmul", "2", "3", "4", "--json", "tensor.sms"}, parsed),
                "");
    check::equal("the group is read", parsed.symmetry.kind == cli::SymmetryKind::MatrixMultiplication, 1);
    check::equal("with all three dimensions",
                 static_cast<long long>(parsed.symmetry.shape.size()), 3);
    check::equal("the flag after them is still seen", parsed.as_json, 1);
    check::text("and the file after that is still the file", parsed.file, "tensor.sms");

    Parsed misspelt;
    check::text("and its own refusal is passed through unchanged",
                refusal({"-s", "matmol", "2", "2", "2"}, misspelt),
                "'matmol' is not a symmetry: expected none, auto or matmul");
}

void check_refuses_and_says_why() {
    Parsed parsed;
    check::text("a missing value names the flag", refusal({"--target"}, parsed),
                "--target needs a value");
    check::text("a value that is the next flag says so",
                refusal({"--anchor", "--json"}, parsed),
                "--anchor needs a value, and '--json' is the next flag");
    check::text("a word where a number belongs names both",
                refusal({"--target", "abc"}, parsed),
                "--target expects a whole number, not 'abc'");
    check::text("a negative count is refused as a count",
                refusal({"--threads", "-1"}, parsed),
                "--threads expects a count, not '-1'");
    check::text("a count too large to hold is not truncated",
                refusal({"--threads", "99999999999999999999"}, parsed),
                "--threads expects a count, and '99999999999999999999' is too large");
    check::text("nor is a whole number",
                refusal({"--target", "99999999999999999999"}, parsed),
                "--target expects a whole number, and '99999999999999999999' is too large");
    check::text("a count outside its range says the range",
                refusal({"--steps", "7"}, parsed),
                "--steps expects a count between 1 and 3, not '7'");
    check::text("a size that is not one keeps its own reason",
                refusal({"--max-memory", "2X"}, parsed), "--max-memory: '2X' is not a size");
    check::text("an unknown flag is still an unknown flag", refusal({"--nope"}, parsed),
                "unrecognised option: --nope");
    check::text("two files are refused rather than one ignored",
                refusal({"first.sms", "second.sms"}, parsed),
                "only one file is read, and both 'first.sms' and 'second.sms' were given");
}

}  // namespace

int main() {
    check_reads_a_line();
    check_help_is_a_flag_and_not_a_filename();
    check_hands_over_to_the_symmetry_parser();
    check_refuses_and_says_why();
    return check::report("arguments");
}
