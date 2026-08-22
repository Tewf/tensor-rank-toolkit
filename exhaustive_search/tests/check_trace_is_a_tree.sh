#!/bin/sh
# That a trace says exactly what the search did, and that asking for one changes
# nothing about what the search does.
#
# Three claims, and none of them is visible from reading the code that writes a
# trace. **It counts what the search counts:** a node is opened where the budget
# consumes one, so the two totals are one number arrived at twice, and the
# search's total is what this repository publishes. **It is a tree:** every node
# is opened once, names a parent opened before it one level up, and is settled
# exactly once. The close is a destructor so that a `return` added later cannot
# leave a node open, and this is what would catch it if one did. **And the hook
# is free:** the node count and the verdict must be identical with the flag and
# without it, which is the property a pointer threaded through a hot recursion is
# likeliest to break.
#
# Both searches are asked, because they are different functions: the plain one in
# `exhaustive_search.cpp` and the quotiented one in
# `../../orbit_reduction/orbit_search.cpp`. A trace that worked for one and
# silently wrote nothing for the other is exactly what happened while this was
# being built.
set -u

command=$1
fixtures=$2
work=${TMPDIR:-/tmp}/trace-check-$$
mkdir -p "$work"
failures=0

report() {
    if [ "$2" -eq 0 ]; then
        printf '  ok    %s\n' "$1"
    else
        printf '  FAIL  %s\n' "$1"
        failures=$((failures + 1))
    fi
}

# One question, asked with a trace and without, on each search.
for quotient in "" "-s matmul 2 2 2"; do
    label=$([ -z "$quotient" ] && echo "plain" || echo "quotiented")
    trace="$work/$label.jsonl"

    # shellcheck disable=SC2086
    traced=$("$command" "$fixtures/matmul_2x2x2.tensor" --target 6 $quotient --trace "$trace" 2>&1)
    # shellcheck disable=SC2086
    plain=$("$command" "$fixtures/matmul_2x2x2.tensor" --target 6 $quotient 2>&1)

    traced_nodes=$(printf '%s' "$traced" | sed -n 's/.*  \([0-9]*\) nodes in.*/\1/p')
    plain_nodes=$(printf '%s' "$plain" | sed -n 's/.*  \([0-9]*\) nodes in.*/\1/p')
    [ "$traced_nodes" = "$plain_nodes" ]
    report "$label: the flag does not change the node count ($plain_nodes)" $?

    printf '%s' "$traced" | grep -q "NO: there is no algorithm"
    report "$label: the flag does not change the verdict" $?

    written=$(sed -n 's/.*trace: \([0-9]*\) nodes.*/\1/p' <<EOT
$traced
EOT
)
    [ "$written" = "$plain_nodes" ]
    report "$label: the trace holds every node the search visited ($written)" $?

    python3 - "$trace" "$label" <<'PY'
import json, sys

path, label = sys.argv[1], sys.argv[2]
with open(path) as handle:
    lines = [json.loads(line) for line in handle if line.strip()]
head, events = lines[0], lines[1:]

if head.get("schema") != "trace/1" or head.get("world") != "search":
    sys.exit(f"{label}: the header is not a trace/1 in the search world: {head}")

depth, settled = {}, set()
for event in events:
    name = event["ids"][0]
    attrs = event.get("attrs", {})
    if event["op"] == "open":
        if name in depth:
            sys.exit(f"{label}: {name} opened twice")
        parent = attrs["parent"]
        if parent is None:
            if attrs["depth"] != 0:
                sys.exit(f"{label}: the root sits at depth {attrs['depth']}")
        else:
            if parent not in depth:
                sys.exit(f"{label}: {name} names an unopened parent {parent}")
            if attrs["depth"] != depth[parent] + 1:
                sys.exit(f"{label}: {name} does not sit one below its parent")
        depth[name] = attrs["depth"]
    else:
        if name not in depth:
            sys.exit(f"{label}: {name} spoken about before it was opened")
        if event["op"] in ("prune", "adopt", "close"):
            if name in settled:
                sys.exit(f"{label}: {name} settled twice")
            settled.add(name)
if settled != set(depth):
    sys.exit(f"{label}: {len(set(depth) - settled)} nodes opened and never settled")
PY
    report "$label: the trace is a tree, every node settled exactly once" $?
done

# A trace on more than one worker is refused, not written badly.
"$command" "$fixtures/matmul_2x2x2.tensor" --target 6 --threads 4 \
    --trace "$work/never.jsonl" > /dev/null 2>&1
[ $? -eq 2 ] && [ ! -f "$work/never.jsonl" ]
report "more than one worker is refused as usage, and nothing is written" $?

rm -rf "$work"
if [ "$failures" -eq 0 ]; then
    printf 'trace: all checks passed\n'
    exit 0
fi
printf 'trace: %d check(s) failed\n' "$failures"
exit 1
