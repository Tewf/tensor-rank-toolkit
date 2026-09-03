#!/bin/sh
# That the plan a run prints is the plan it carried out, and that a file replays it.
#
# The checks beside this one read `search_plan.h` on both sides, which is the
# failure `infrastructure/cli/tests/check_tunables_bound_a_run.sh` was written to end: a rule
# asserted against itself passes while nothing calls it. So this one spends
# processes. It asserts the printed lines outright, so the rules are pinned to a
# tensor and a target rather than left to be inferred; it asserts that a flag
# moves the line it names; and it asserts that a plan written out and read back
# reaches the same verdict on the same node count, which is what a run
# reproducing another machine's decisions has to mean.
set -u

command=$1
fixtures=$2
failures=0

scratch=$(mktemp -d)
trap 'rm -rf "$scratch"' EXIT

# The plan lines only, so a change to the verdict or the timing does not fail a
# check about the plan. Two spaces of indent and the word, as `plan_lines` writes
# them; the reason is left out here and asserted by name below.
plan_of() {
    "$command" "$@" 2>/dev/null |
        sed -n 's/^    \([a-z ]*\): \([a-z0-9 ]*\).*/\1: \2/p' |
        sed 's/ *$//'
}

expect_line() {
    want=$1
    shift
    if plan_of "$@" | grep -qx "$want"; then
        echo "  ok    $want"
    else
        echo "  FAIL  wanted '$want' from: $*"
        plan_of "$@" | sed 's/^/          /'
        failures=$((failures + 1))
    fi
}

expect_reason() {
    want=$1
    shift
    if "$command" "$@" 2>/dev/null | grep -qF -e "$want"; then
        echo "  ok    $want"
    else
        echo "  FAIL  wanted '$want' from: $*"
        failures=$((failures + 1))
    fi
}

tensor=$fixtures/matmul_2x2x2.tensor

# 225 maps of 4x4 against a 2 GiB budget, and 2^6 = 64 subspace elements against
# 225 pool maps. Both rules read off by hand, which is the point of pinning them.
echo "the rules, on <2,2,2> at target 6"
expect_line "pool: materialised" "$tensor" --target 6
expect_line "leaf route: walk" "$tensor" --target 6
expect_line "threads: 1" "$tensor" --target 6
expect_line "quotient: none" "$tensor" --target 6
expect_line "orbit test: full" "$tensor" --target 6
expect_line "anchor: map" "$tensor" --target 6
expect_reason "64 subspace elements at dimension 6 against 225 pool maps" \
              "$tensor" --target 6

# At dimension 8 the pool is the smaller side, so the same tensor takes the other
# route. A rule that stopped comparing would pass every check above and fail this.
echo "and the same tensor at target 8, where the comparison turns over"
expect_line "leaf route: scan" "$tensor" --target 8

echo "every field its flag moves"
expect_line "leaf route: scan" "$tensor" --target 6 --leaf-route scan
expect_line "threads: 2" "$tensor" --target 6 --threads 2
expect_line "orbit test: generators" "$tensor" --target 6 --orbit-test generators
expect_line "anchor: heuristic" "$tensor" --target 6 --anchor heuristic
expect_line "quotient: matmul 2 2 2" "$tensor" --target 6 -s matmul 2 2 2
expect_line "device: cpu" "$tensor" --target 6 --device cpu
expect_reason "--device cpu" "$tensor" --target 6 --device cpu
# The flag that controls the pool is --max-memory, and 1K holds nothing.
expect_line "pool: addressed" "$tensor" --target 6 --max-memory 1K

echo "a word no field takes is refused, naming the field and the words"
"$command" "$tensor" --target 6 --device tpu >/dev/null 2>&1
if [ $? -eq 2 ]; then
    echo "  ok    2  --device tpu"
else
    echo "  FAIL  --device tpu should leave as 2"
    failures=$((failures + 1))
fi

echo "a plan written out replays to the same verdict on the same nodes"
plan=$scratch/plan.txt
first=$("$command" "$tensor" --target 6 --plan-out "$plan" 2>/dev/null | grep 'nodes in')
again=$("$command" "$tensor" --target 6 --plan-in "$plan" 2>/dev/null | grep 'nodes in')
# The node count and not the seconds: a count is a fact about the tree and a
# timing is a fact about the afternoon, and only one of them belongs in a test.
if [ -n "$first" ] && [ "${first%% nodes*}" = "${again%% nodes*}" ]; then
    echo "  ok    ${first%% nodes*} nodes both times"
else
    echo "  FAIL  '$first' against '$again'"
    failures=$((failures + 1))
fi

echo "and every field survives the file"
for field in "pool: materialised" "leaf route: walk" "device: cpu" "threads: 1" \
             "quotient: none" "orbit test: full" "anchor: map"; do
    expect_line "$field" "$tensor" --target 6 --plan-in "$plan"
done

echo "a flag given beside a plan file still wins"
expect_line "threads: 3" "$tensor" --target 6 --plan-in "$plan" --threads 3

echo "a plan naming a field nothing has stops the run"
printf 'poool addressed\n' > "$scratch/bad.txt"
"$command" "$tensor" --target 6 --plan-in "$scratch/bad.txt" >/dev/null 2>&1
if [ $? -eq 2 ]; then
    echo "  ok    2  a field nothing has"
else
    echo "  FAIL  a plan with an unknown field should leave as 2"
    failures=$((failures + 1))
fi

if [ "$failures" -ne 0 ]; then
    echo "the plan reaches a run: $failures failed"
    exit 1
fi
echo "the plan reaches a run: all ok"
