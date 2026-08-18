#!/bin/sh
# That a number in a file actually bounds a run, and that a flag beats the file.
#
# The check this replaces compared tunables.conf against the defaults compiled
# into tunables.h. Both sides of that comparison came from the same header, so it
# passed while nothing outside cli/ included the header at all: every command was
# still bounded by its own literal and the file bounded nothing. A test that
# cannot fail when the wiring is absent is worse than no test, because it is
# reported as coverage.
#
# So this one spends a process. It writes a tunables.conf with a node limit of
# one, runs decide-rank under it, and asserts the search stops and leaves as 3.
# Three is the assertion and not merely the observation: a budget that expired
# decides nothing, and this repository has already published a lower bound that
# was one.
set -u

command=$1
fixtures=$2
failures=0

scratch=$(mktemp -d)
trap 'rm -rf "$scratch"' EXIT
conf=$scratch/tunables.conf

# Named through the environment rather than left in the working directory: the
# repository's own tunables.conf sits in the tree the tests run from, and a test
# whose result depends on which directory ctest chose is not a test.
expect() {
    want=$1
    shift
    BILINEAR_TUNABLES=$conf "$command" "$@" >/dev/null 2>&1
    got=$?
    if [ "$got" -ne "$want" ]; then
        echo "  FAIL  wanted $want, got $got:  $*"
        failures=$((failures + 1))
    else
        echo "  ok    $want  $*"
    fi
}

# Without any file, so the compiled default stands and the fixture settles. This
# is the control: every refusal below has to be the file's doing and not the
# question's.
echo "no file: the compiled default stands"
unset BILINEAR_TUNABLES
"$command" "$fixtures/matmul_2x2x2.tensor" --target 7 >/dev/null 2>&1
got=$?
if [ "$got" -ne 0 ]; then
    echo "  FAIL  wanted 0 without a tunables file, got $got"
    failures=$((failures + 1))
else
    echo "  ok    0  matmul_2x2x2.tensor --target 7"
fi

echo "search_node_limit = 1 in the file"
printf 'search_node_limit = 1\n' > "$conf"
# A question the search cannot settle in one node, which is every question.
expect 3 "$fixtures/matmul_2x2x2.tensor" --target 7
# And the one this repository got wrong, for the same reason, on a fixture no
# budget of any size settles quickly.
expect 3 "$fixtures/f2_5x5.tensor" --target 12

echo "and an explicit flag overrides the file"
expect 0 "$fixtures/matmul_2x2x2.tensor" --target 7 --node-limit 5000000

# The refusal has to reach the command, not stop at the parser's own test. A name
# nothing has is the failure mode the file's format exists to prevent: a run
# bounded by a number the file appears to have changed and did not.
echo "a typo in the file stops the run"
printf 'search_node_limit_typo = 1\n' > "$conf"
expect 2 "$fixtures/matmul_2x2x2.tensor" --target 7

if [ "$failures" -ne 0 ]; then
    echo "tunables bound a run: $failures failed"
    exit 1
fi
echo "tunables bound a run: all ok"
