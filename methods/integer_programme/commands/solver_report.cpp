/// The retired spelling of `curve-bounds --solvers`, kept so a script says why.
///
/// The ranking is only ever read to decide what `curve-bounds --route chain`
/// will reach, so it moved beside that flag rather than standing as a command of
/// its own; [`../../../OPTIONS/one-question-per-command.md`](../../../OPTIONS/one-question-per-command.md)
/// is the audit that says so.
///
/// This file is what is left, and it is here rather than deleted for one reason:
/// a shell answering `list-solvers: command not found` names no replacement, and
/// somebody's `list-solvers | grep gurobi` would go quiet without saying what to
/// type instead. It prints the new line and leaves as **2**, so a caller that
/// checks its exit code stops rather than reading an empty ranking as "no
/// backends installed", the one wrong answer this file could give.
#include "exit_code.h"
#include "report.h"

int main() {
    cli::note() << "list-solvers is now `curve-bounds --solvers`, which prints the same\n"
                   "ranking from the same tunable, ilp_backend_order. Nothing else about it\n"
                   "changed: same order, same present/absent column, same exit 0.\n"
                   "\n"
                   "usage: curve-bounds --solvers";
    return cli::exit_status(cli::ExitCode::Usage);
}
