#!/bin/sh
# Every tunable reaches show-limits, checked against the header that declares
# them rather than against a list typed here.
#
# A tunable that is added and not shown is the failure this exists to catch: the
# instrument would keep printing a complete-looking table with one number missing
# from it, and a table that is silently short is worse than no table.
set -eu
instrument="$1"
header="$2"

output=$("$instrument")
status=0

names=$(sed -n 's/.*{"\([a-z_]*\)", &Tunables::.*/\1/p' "$header" | sort -u)
if [ -z "$names" ]; then
    echo "FAIL  no tunable names could be read from $header"
    exit 1
fi

for name in $names; do
    if printf '%s\n' "$output" | grep -q "^  $name "; then
        echo "  ok    show-limits names $name"
    else
        echo "  FAIL  show-limits never names $name"
        status=1
    fi
done

for line in "cores" "physical memory" "allocation ceiling" "workers" "device order"; do
    if printf '%s\n' "$output" | grep -q "^  $line "; then
        echo "  ok    show-limits names $line"
    else
        echo "  FAIL  show-limits never names $line"
        status=1
    fi
done

if [ "$status" -eq 0 ]; then
    echo "show_limits: all checks passed"
fi
exit "$status"
