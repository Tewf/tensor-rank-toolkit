# cli/

The plumbing every command shares: how a command line is read, what the exit
codes mean, why commentary goes to stderr with a `#` and results to stdout,
and the tunables file. Nothing in here answers a question about a tensor.

The full statement of what belongs here and what never will is at the top of
[`CMakeLists.txt`](CMakeLists.txt); each header opens with its own rules.
A script author should read [`exit_code.h`](exit_code.h) first — here, a
proved *no* is exit 1 and giving up is exit 3, and neither is a crash
([`../OPTIONS.md`](../OPTIONS.md) before wrapping anything in `set -e`).
