/// That commentary is commented and results are not, which is the whole of the
/// stdout/stderr contract.
///
/// The rule `report.h` states is one sentence long: a result goes to stdout, and
/// everything else goes to stderr with a `#` in front of it. The `#` is not
/// decoration, it is the comment character of the SMS format and of PLinOpt's
/// readers, so a run whose commentary is commented can have its stdout redirected
/// straight into a file `PMchecker` reads. The case that makes this worth a test
/// rather than a convention is a message containing its own newline: half a
/// commented message is worse than none, because a reader stops at the first line
/// that parses as data.
#include <iostream>
#include <sstream>
#include <string>

#include "check.h"
#include "report.h"

namespace {

/// What `write` sends to stderr, with the stream put back afterwards.
std::string commentary(void (*write)()) {
    std::ostringstream caught;
    std::streambuf* held = std::cerr.rdbuf(caught.rdbuf());
    write();
    std::cerr.rdbuf(held);
    return caught.str();
}

void one_line() { cli::note() << "step 3 pool: " << 225 << " rank-one maps"; }

void two_lines() { cli::note() << "gave up after 300 s\nnothing is proved by that"; }

void a_trailing_newline() { cli::note() << "wrote out.sms\n"; }

void nothing_at_all() { cli::note(); }

void check_every_line_is_commented() {
    check::text("one line is commented once", commentary(one_line),
                "# step 3 pool: 225 rank-one maps\n");
    check::text("a message with its own newline is commented on both lines", commentary(two_lines),
                "# gave up after 300 s\n# nothing is proved by that\n");
    check::text("a trailing newline does not produce a bare line",
                commentary(a_trailing_newline), "# wrote out.sms\n");
    check::text("and an empty note is still a comment", commentary(nothing_at_all), "#\n");
}

void check_a_result_is_not_commented() {
    std::ostringstream caught;
    std::streambuf* held = std::cout.rdbuf(caught.rdbuf());
    cli::result() << "7 products\n";
    std::cout.rdbuf(held);
    check::text("a result goes to stdout bare", caught.str(), "7 products\n");
}

}  // namespace

int main() {
    check_every_line_is_commented();
    check_a_result_is_not_commented();
    return check::report("report");
}
