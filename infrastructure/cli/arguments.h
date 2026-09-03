#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>

#include "size_argument.h"

/// Walking a command line once, so that no command hand-rolls the walk again.
///
/// Every command here wrote the same loop, and the eight of them between them
/// wrote about forty copies of its branches. Two faults are in every copy.
///
/// The first is `&& argument + 1 < argc`, repeated on each branch that takes a
/// value. Left out, the command reads past the end of `argv`; put in, a flag
/// whose value is missing falls through to the final `else`, so `--target` with
/// nothing after it is reported as though `--target` were misspelt. Neither is
/// what happened.
///
/// The second is `std::stoull`, whose exception says the name of the function
/// that threw and nothing else. `decide-rank tensor --target abc` printed
/// `decide-rank: stoull`, which names neither the flag nor the word that was
/// wrong, and there are five numeric flags it could have been.
///
/// So the walk lives here once, and every refusal names the flag and quotes the
/// word. Reading one typed value stays where it already is: `size_argument.h`
/// reads `2G`, `symmetry_argument.h` reads `matmul 2 2 2`. This header drives
/// the loop those two are called from and puts the flag's name in front of what
/// they say; it does not replace them.
namespace cli {

/// Thrown for anything wrong on the command line, so a caller can tell a bad
/// argument from a failed run and exit differently for the two. A run that never
/// started is not a question answered no.
class ArgumentError : public std::runtime_error {
   public:
    explicit ArgumentError(const std::string& what) : std::runtime_error(what) {}
};

/// Whether a word is a flag rather than a value.
///
/// The fault this header removes has a quieter form than a missing value:
/// `--anchor --json` is a value that is the next flag, and a command taking the
/// word as given searches from an anchor named `--json` and reports nothing. A
/// lone `-` is a filename (one every command here refuses with its usage,
/// because nothing reads stdin) and `-1` is a number, so neither counts as a
/// flag.
inline bool looks_like_flag(const std::string& word) {
    if (word.size() < 2 || word.front() != '-') return false;
    return word.find_first_not_of("0123456789", 1) != std::string::npos;
}

/// A count, refused rather than guessed at. `named` is the flag it came from, or
/// wherever else it came from, and it goes in front so the reader knows which
/// argument to fix.
inline std::size_t parse_count(const std::string& named, const std::string& text) {
    if (text.empty() || text.find_first_not_of("0123456789") != std::string::npos) {
        throw ArgumentError(named + " expects a count, not '" + text + "'");
    }
    try {
        return static_cast<std::size_t>(std::stoull(text));
    } catch (const std::out_of_range&) {
        throw ArgumentError(named + " expects a count, and '" + text + "' is too large");
    }
}

/// A memory size, with the flag's name in front of the reason it was refused.
///
/// `size_argument.h` throws a `std::runtime_error`, which every command that
/// walks its own line turned into exit 5: a bad word on the command line
/// reported as a tool that could not run. The wrapping lives here once so that
/// `--max-memory bogus` leaves as 2 wherever it is written.
inline std::size_t parse_memory_size(const std::string& named, const std::string& text) {
    try {
        return parse_size(text);
    } catch (const std::exception& problem) {
        throw ArgumentError(named + ": " + problem.what());
    }
}

/// A whole number, which may be negative because `--target -1` is how three
/// commands spell "no target given".
inline long long parse_whole_number(const std::string& named, const std::string& text) {
    const std::string digits = (!text.empty() && text.front() == '-') ? text.substr(1) : text;
    if (digits.empty() || digits.find_first_not_of("0123456789") != std::string::npos) {
        throw ArgumentError(named + " expects a whole number, not '" + text + "'");
    }
    try {
        return std::stoll(text);
    } catch (const std::out_of_range&) {
        throw ArgumentError(named + " expects a whole number, and '" + text + "' is too large");
    }
}

/// The command line, walked once: the flags in the order they were written, and
/// the one positional word, which is a filename.
///
/// The shape a command's loop takes, and the reason the cursor is a member:
///
/// ```
/// cli::Arguments arguments(argc, argv);
/// while (arguments.next_flag()) {
///     if (arguments.is("--json")) as_json = true;
///     else if (arguments.is("--target")) target = arguments.whole_number();
///     else if (arguments.is("--max-memory")) budget = arguments.memory_size();
///     else if (arguments.is("--symmetry", "-s")) symmetry = arguments.parsed_by(parse_symmetry);
///     else arguments.refuse();
/// }
/// const std::string path = arguments.filename();  // empty or "-": none named
/// ```
class Arguments {
   public:
    /// `argv[0]` is the program's name and is skipped, as everywhere else.
    Arguments(int argc, char** argv) : word_count_(argc), words_(argv) {}

    /// Step to the next flag, collecting the positional word on the way. False
    /// when the line is used up.
    bool next_flag() {
        while (++cursor_ < word_count_) {
            const std::string word = words_[cursor_];
            if (looks_like_flag(word)) {
                flag_ = word;
                return true;
            }
            take_filename(word);
        }
        return false;
    }

    /// The flag the walk is standing on.
    const std::string& flag() const { return flag_; }

    /// Whether it is spelled any of these ways, so that a long form and its
    /// short alias are one branch rather than two.
    template <class... Spellings>
    bool is(Spellings... spellings) const {
        return ((flag_ == spellings) || ...);
    }

    /// The word after the flag, refused when it is missing or is the next flag.
    std::string text() {
        if (cursor_ + 1 >= word_count_) throw ArgumentError(flag_ + " needs a value");
        const std::string word = words_[cursor_ + 1];
        if (looks_like_flag(word)) {
            throw ArgumentError(flag_ + " needs a value, and '" + word + "' is the next flag");
        }
        ++cursor_;
        return word;
    }

    std::size_t count() { return parse_count(flag_, text()); }

    /// A count that has to land in a range, because `--steps 7` running three
    /// steps is a run that answers a question nobody asked.
    std::size_t count(std::size_t least, std::size_t most) {
        const std::string word = text();
        const std::size_t value = parse_count(flag_, word);
        if (value < least || value > most) {
            throw ArgumentError(flag_ + " expects a count between " + std::to_string(least) +
                                " and " + std::to_string(most) + ", not '" + word + "'");
        }
        return value;
    }

    long long whole_number() { return parse_whole_number(flag_, text()); }

    /// `2G`, `2048K` or a count of bytes, read by `size_argument.h` and refused
    /// with the flag's name in front of its reason.
    std::size_t memory_size() { return parse_memory_size(flag_, text()); }

    /// Hand the rest of the line to a parser that reads several words of its
    /// own: `arguments.parsed_by(cli::parse_symmetry)`. It leaves the cursor on
    /// the last word it consumed, which is what `symmetry_argument.h` already
    /// does for a hand-rolled loop, so that parser is used here unchanged.
    ///
    /// Its refusal is passed on with its wording untouched, because that parser
    /// names the flag and quotes the word already and a prefix here would say
    /// `--symmetry` twice. What does change is the type, and with it the exit
    /// code: it throws `std::runtime_error`, which every command turns into 5,
    /// so `-s` with nothing after it and `-s matmol 2 2 2` were both reported as
    /// tools that could not run rather than as lines that did not parse. That is
    /// the same fault `parse_memory_size` above exists to remove.
    template <class Parser>
    auto parsed_by(Parser parser) {
        try {
            return parser(word_count_, words_, cursor_);
        } catch (const ArgumentError&) {
            throw;
        } catch (const std::exception& problem) {
            throw ArgumentError(problem.what());
        }
    }

    /// What the loop's last branch does. The wording is the one the commands
    /// already print, so nothing a user reads changes.
    [[noreturn]] void refuse() const { throw ArgumentError("unrecognised option: " + flag_); }

    /// The positional word. Empty when none was given.
    const std::string& filename() const { return filename_; }

    /// No input file was named: the word is missing, or is the lone `-`.
    /// Every command answers that with its usage. This was `no_file_named`, a
    /// name that promised a route nothing implements: no command here reads a
    /// map from stdin, and a reader who piped one in got usage and exit 2.
    bool no_file_named() const { return filename_.empty() || filename_ == "-"; }

   private:
    /// One file per run. A second one is a mistake worth naming: it is what a
    /// shell glob does when a fixture directory holds more matches than the
    /// writer expected, and the old loops read the first and ignored the rest.
    void take_filename(const std::string& word) {
        if (!filename_.empty()) {
            throw ArgumentError("only one file is read, and both '" + filename_ + "' and '" +
                                word + "' were given");
        }
        filename_ = word;
    }

    int word_count_;
    char** words_;
    int cursor_ = 0;  // sits on the flag while a branch reads its value
    std::string flag_;
    std::string filename_;
};

}  // namespace cli
