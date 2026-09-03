#!/bin/sh
# That the three limits reach the commands, which no test inside run_limits can see.
#
# `test_device.cpp` and `test_parallel.cpp` read `run_limits` on both sides, so
# both would pass throughout a period when not one command offered `--threads` or
# `--max-memory`, and that period was most of this repository's history. The same
# argument `infrastructure/cli/tests/check_argument_grammar.sh` and
# `methods/bilinear_rank/search_plan/tests/check_the_plan_reaches_a_run.sh` make, so this one spends
# processes too.
#
# Three things are asserted, and the middle one is the reason for the file:
#
#  1. **A bulk allocation that cannot fit says so and names the number.** Every
#     command here has one that is exponential or cubic in something read off the
#     command line or the tensor file, and an out-of-memory kill is not a result.
#     Asserted as exit 5 with the words `over the` and `budget`, which is what
#     `require_room` writes.
#  2. **The flag its own refusal names exists.** The sentence ends "Raise it with
#     --max-memory if the machine has the room", and printing that from a command
#     that then refuses the flag is worse than never mentioning it. So each is
#     asked with `--max-memory 1` (a budget that holds nothing), and the refusal
#     is the proof that the flag arrived.
#  3. **`--threads` changes the clock and not the answer.** `lower-the-bound` is
#     asked at 1 and at 4 and the whole count line has to match, because the
#     children of one node are prepared in parallel and entered in the same order.
#     A node count is a fact about the tree; a timing is a fact about the
#     afternoon, and only one of them belongs in a test.
set -u

command=$1
fixtures=$2
failures=0

# One binary per strand, all siblings of the one ctest passes. Derived rather
# than listed, so moving the build layout moves one line.
binaries=${3:?the build root, passed by the add_test registration}
minimise=$binaries/methods/bilinear_rank/greedy_heuristic/minimise-rank
lower=$binaries/methods/bilinear_rank/branch_and_bound/lower-the-bound
walk=$binaries/methods/bilinear_rank/flip_graph/walk-scheme
sparsify=$binaries/matrix_sparsification/sparsify-operator
pencil=$binaries/pencil_rank/decide-rank-by-pencil
curve=$binaries/curve_bounds/curve-bounds
maker=$binaries/methods/bilinear_rank/map_construction/make-tensor
sat=$binaries/satisfiability/decide-rank-by-sat

scratch=$(mktemp -d)
trap 'rm -rf "$scratch"' EXIT

# Exit 5 and a refusal that prices the allocation. Both halves matter: a command
# that died would leave 134 or 137, and one that quietly carried on would leave 0.
refuses_with_a_number() {
    what=$1
    shift
    "$@" >"$scratch/out" 2>"$scratch/err"
    got=$?
    if [ "$got" -ne 5 ]; then
        echo "  FAIL  $what: wanted exit 5, got $got"
        head -2 "$scratch/err" | sed 's/^/          /'
        failures=$((failures + 1))
        return
    fi
    if ! grep -q 'over the .* budget' "$scratch/err"; then
        echo "  FAIL  $what: the refusal did not price the allocation"
        head -2 "$scratch/err" | sed 's/^/          /'
        failures=$((failures + 1))
        return
    fi
    if ! grep -q -- '--max-memory' "$scratch/err"; then
        echo "  FAIL  $what: the refusal did not name the flag that moves it"
        failures=$((failures + 1))
        return
    fi
    echo "  ok    $what"
}

# The flag is accepted at all: not "unrecognised option", not a usage exit.
accepts() {
    what=$1
    shift
    "$@" >"$scratch/out" 2>"$scratch/err"
    got=$?
    if [ "$got" -eq 2 ] || grep -q 'unrecognised option' "$scratch/err"; then
        echo "  FAIL  $what: the flag was refused"
        head -2 "$scratch/err" | sed 's/^/          /'
        failures=$((failures + 1))
        return
    fi
    echo "  ok    $what"
}

tensor=$fixtures/matmul_2x2x2.tensor
small=$fixtures/f2_2x2.tensor
operator=$fixtures/strassen_u.matrix

# **`decide-rank` is deliberately not in this list, either way round.** A budget
# that holds nothing does not refuse `--target k`: the pool goes addressed and the
# search runs, which is what `methods/bilinear_rank/search_plan/tests/check_the_plan_reaches_a_run.sh`
# asserts and what `satisfiability/tests/check_exit_codes.sh` asserts the exit
# code of. **`--max-memory` is two things in this repository**: a ceiling on one
# allocation, and the knob that chooses the cheaper representation. Wherever a
# cheaper representation exists, the second meaning wins. The sweep, which has no
# addressed form, does refuse, but with a sentence of its own that explains which
# route to take instead rather than with `require_room`'s, so asserting it here
# would be asserting that command's prose in this file.
echo "a budget that holds nothing is refused with the number, on every command"
refuses_with_a_number "minimise-rank" "$minimise" "$tensor" --max-memory 1
refuses_with_a_number "lower-the-bound" "$lower" "$tensor" --max-memory 1
refuses_with_a_number "walk-scheme --from" "$walk" "$tensor" --from 7 --max-memory 1
refuses_with_a_number "sparsify-operator" "$sparsify" "$operator" --max-memory 1
refuses_with_a_number "decide-rank-by-pencil" \
    "$pencil" "$fixtures/pencil_split_f3_3.tensor" --max-memory 1
refuses_with_a_number "curve-bounds --route enumeration" \
    "$curve" --degree 5 --points 1:8 --route enumeration --max-memory 1
refuses_with_a_number "make-tensor" "$maker" --matmul 2 2 2 2 --max-memory 1

echo "and a shape no machine holds is refused rather than killed, at the default"
# 7.2 TiB of slices. Before this it was a reserve and the kernel answered.
refuses_with_a_number "make-tensor --matmul 2 100 100 100" \
    "$maker" --matmul 2 100 100 100

echo "--threads is accepted where a parallel_for is waiting for it"
accepts "decide-rank-by-sat --threads" "$sat" "$small" --target 4 --threads 2
accepts "lower-the-bound --threads" "$lower" "$tensor" --threads 2

echo "and it moves the clock rather than the answer"
# The whole commentary line: nodes, children costed, moves offered, improvements,
# branches bounded, depth. Every one of them is a fact about the tree.
#
# **On <2,2,3> and not on <2,2,2>, because <2,2,2> cannot tell.** Written against
# the smaller fixture this check passed with the children deliberately landing in
# completion order instead of in their own slots. 21 nodes is too few and too
# shallow for two equal-cost children ever to race. <2,2,3> is 341 nodes and
# 159 860 children, and the same sabotage moves it to 160 102: a beam that entered
# a different child at one node, which is exactly the failure the slots prevent.
identity=$fixtures/matmul_2x2x3.tensor
one=$("$lower" "$identity" --threads 1 2>&1 | grep '^# ')
four=$("$lower" "$identity" --threads 4 2>&1 | grep '^# ')
if [ -n "$one" ] && [ "$one" = "$four" ]; then
    echo "  ok    ${one}"
else
    echo "  FAIL  one worker: '$one'"
    echo "        four:       '$four'"
    failures=$((failures + 1))
fi

# And the answer itself, which is the line a reader quotes.
one=$("$lower" "$identity" --threads 1 2>/dev/null | grep '^best:')
four=$("$lower" "$identity" --threads 4 2>/dev/null | grep '^best:')
if [ -n "$one" ] && [ "$one" = "$four" ]; then
    echo "  ok    $one, at either count"
else
    echo "  FAIL  '$one' against '$four'"
    failures=$((failures + 1))
fi

if [ "$failures" -ne 0 ]; then
    echo "the limits reach the commands: $failures failed"
    exit 1
fi
echo "the limits reach the commands: all ok"
