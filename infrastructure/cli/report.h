#pragma once

#include <iostream>
#include <ostream>
#include <sstream>
#include <string>

/// Which stream a line goes to, which is the difference between a command that
/// can be piped and one that cannot.
///
/// The convention is one sentence long: **a result goes to stdout, and
/// everything else goes to stderr with a `#` in front of it.** Progress, pool
/// sizes, generator counts, warnings, what was written where, why a search gave
/// up: all commentary, all stderr, all commented.
///
/// The `#` is not decoration. It is the comment character of the SMS format and
/// of its readers, so a run whose commentary is commented can have its stdout
/// redirected straight into a file that external optimisation tools read. Writing
/// output that is readable by external tools is the point of this layer, and
/// this is the half of it that costs one character.
///
/// The writers in [`core/formats/`](../../core/formats/) have always done this, commenting
/// every header line they emit (`sms_file.cpp`, `tensor_file.cpp`,
/// `dense_matrix_file.cpp`). The commands did not: `minimise-rank` split the
/// streams and wrote its commentary bare, and that half was the fault. Merging
/// the two streams (`2>&1`, which is what a CI log does) then produced a file no
/// reader can parse, because it stops at the first line that does not look like
/// data, and a bare progress line looks exactly like data. So the convention is
/// not new here, it is the file writers' practice now kept by every command
/// here: every `std::cerr` in a `*/commands/*_main.cpp` comes through
/// `note()`, usage blocks included, and every result through `result()`.
/// `infrastructure/cli/tests/check_argument_grammar.sh` asks the built commands rather than
/// this header, because a check on the header passed throughout the period when
/// nothing called it.
namespace cli {

/// One piece of commentary, on stderr, with `#` in front of every line of it.
///
/// Built up and written at the end of the statement rather than straight
/// through, because the prefix has to survive a message containing its own
/// newlines: the commands print two-line explanations of why they stopped, and
/// half a commented message is worse than none, since a reader stops at the
/// first line that parses as data rather than as a comment.
class Note {
   public:
    Note() = default;
    Note(const Note&) = delete;
    Note& operator=(const Note&) = delete;

    ~Note() {
        const std::string written = text_.str();
        if (written.empty()) {
            std::cerr << "#\n";
            return;
        }
        std::istringstream lines(written);
        std::string line;
        while (std::getline(lines, line)) {
            std::cerr << (line.empty() ? "#" : "# " + line) << "\n";
        }
    }

    template <class Written>
    Note& operator<<(const Written& value) {
        text_ << value;
        return *this;
    }

   private:
    std::ostringstream text_;
};

/// Commentary: `cli::note() << "pool: " << pool.size();` and the newline follows
/// from the end of the statement. A trailing newline written by hand costs
/// nothing, so a line moved here from a `std::cerr` needs no editing.
inline Note note() { return Note(); }

/// The result, which is what the next program in the pipe reads. Nothing that is
/// not a result may be written here, and that is the whole rule.
inline std::ostream& result() { return std::cout; }

}  // namespace cli
