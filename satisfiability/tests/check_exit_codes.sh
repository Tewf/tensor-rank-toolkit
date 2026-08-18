#!/bin/sh
# The exit codes are an interface, so they are asserted rather than described.
#
# The one that matters is 3. A question nobody answered must never leave as 1,
# because a script reading 1 as "proved impossible" would turn a timeout into a
# lower bound. That is not hypothetical here: this repository published an
# F2 5x5 result built on exactly that confusion, and the last case below is the
# same command that produced it.
set -u

command=$1
fixtures=$2
failures=0

expect() {
    want=$1
    shift
    "$command" "$@" >/dev/null 2>&1
    got=$?
    if [ "$got" -ne "$want" ]; then
        echo "  FAIL  wanted $want, got $got:  $*"
        failures=$((failures + 1))
    else
        echo "  ok    $want  $*"
    fi
}

expect 0 "$fixtures/f2_2x2.tensor"
expect 0 "$fixtures/f2_2x2.tensor" --target 3
expect 1 "$fixtures/f2_2x2.tensor" --target 2
expect 2 --nonsense
expect 2 -h
expect 5 "$fixtures/no_such_file.tensor"

# Undecided, and deliberately the question this repository got wrong: five
# seconds cannot settle twelve products for F2 5x5, and saying so is the point.
expect 3 "$fixtures/f2_5x5.tensor" --target 12 --timeout 5

if [ "$failures" -ne 0 ]; then
    echo "exit codes: $failures failed"
    exit 1
fi
echo "exit codes: all ok"
