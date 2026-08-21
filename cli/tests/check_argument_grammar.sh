#!/bin/sh
# That the commands read a command line the way `cli/arguments.h` says they do.
#
# `test_arguments.cpp` asserts the walker against a loop written inside the test,
# which is a check on the header and not on any command. It passed throughout the
# period when no command included the header: `--steps abc` left as 5 saying
# `stoi`, `--steps` with nothing after it was reported as an unrecognised option,
# `--steps 7` ran three steps and answered 0, and `--help` was read as a filename.
# None of that is visible from inside the header, so this spends processes.
#
# The same argument covers the `#`: `test_report.cpp` asserts that `cli::note()`
# comments what it is given, which says nothing about whether any command sends
# its commentary there. The last section asks the built commands.
set -u

command=$1
fixtures=$2
failures=0

# The others sit beside this one, one directory per strand, so each path is the
# one ctest passes with the strand swapped. Passing the rest in as arguments
# would only restate that, and would need editing every time the list moves.
binaries=$(dirname "$(dirname "$command")")
minimise=$binaries/descent_search/minimise-rank

scratch=$(mktemp -d)
trap 'rm -rf "$scratch"' EXIT

# Exit code, and the refusal names the flag and quotes the word. Both are the
# interface: a message naming the function that threw instead of the argument
# that was wrong is the fault this file exists to keep out.
refuses() {
    want=$1
    said=$2
    shift 2
    "$@" >"$scratch/out" 2>"$scratch/err"
    got=$?
    if [ "$got" -ne "$want" ]; then
        echo "  FAIL  wanted exit $want, got $got:  $*"
        failures=$((failures + 1))
        return
    fi
    if ! grep -qF -- "$said" "$scratch/err"; then
        echo "  FAIL  wanted '$said' on stderr:  $*"
        echo "        said instead: $(head -2 "$scratch/err")"
        failures=$((failures + 1))
        return
    fi
    echo "  ok    $want  $said"
}

echo "a bad value names the flag and the word, and is usage rather than error"
refuses 2 "--target expects a whole number, not 'abc'" \
        "$command" "$fixtures/f2_2x2.tensor" --target abc
refuses 2 "--threads expects a count, not 'abc'" \
        "$minimise" "$fixtures/f2_2x2.tensor" --threads abc
refuses 2 "--max-memory: '2X' is not a size" \
        "$minimise" "$fixtures/f2_2x2.tensor" --max-memory 2X

echo "a missing value says so, rather than that the flag was not recognised"
refuses 2 "--target needs a value" "$command" "$fixtures/f2_2x2.tensor" --target
refuses 2 "--max-memory needs a value" "$minimise" "$fixtures/f2_2x2.tensor" --max-memory
refuses 2 "--emit-operators needs a value, and '--json' is the next flag" \
        "$minimise" "$fixtures/f2_2x2.tensor" --emit-operators --json

# `--symmetry` reads several words through its own parser, and that parser threw
# a plain runtime_error, which every command turned into 5: could not run at all.
refuses 2 "--symmetry needs none, auto, or matmul <n> <m> <k>" \
        "$command" "$fixtures/f2_2x2.tensor" --symmetry
refuses 2 "matmul needs three dimensions" \
        "$command" "$fixtures/f2_2x2.tensor" -s matmul 2 2
refuses 2 "'matmol' is not a symmetry" \
        "$command" "$fixtures/f2_2x2.tensor" -s matmol 2 2 2

echo "a count outside its range is refused, not run at the nearest value"
# Usage says 1|2|3. Seven ran three steps and zero ran one, and both answered 0.
refuses 2 "--steps expects a count between 1 and 3, not '7'" \
        "$minimise" "$fixtures/f2_2x2.tensor" --steps 7
refuses 2 "--steps expects a count between 1 and 3, not '0'" \
        "$minimise" "$fixtures/f2_2x2.tensor" --steps 0

echo "and the file is read once: a second one is a glob nobody meant"
refuses 2 "only one file is read" \
        "$command" "$fixtures/f2_2x2.tensor" "$fixtures/f2_2x3.tensor"

# Every command, because `--help` is the flag a reader tries first and it used to
# be read as a tensor file by three of them, which left as 5: could not run at
# all. `looks_like_flag` calls it a flag, so `exit_code.h` makes it Usage.
echo "--help prints the usage and leaves as 2, on every command"
for relative in \
    descent_search/minimise-rank \
    descent_search/operators-to-tensor \
    exhaustive_search/decide-rank \
    flip_graph/walk-scheme \
    map_construction/make-tensor \
    matrix_sparsification/sparsify-operator \
    satisfiability/decide-rank-by-sat \
    curve_bounds/curve-bounds \
    incumbent_search/lower-the-bound \
    oracle_guided_search/deflate-strictly \
    oracle_guided_search/enumerate-subspaces \
    oracle_guided_search/price-canonical-route \
    pencil_rank/decide-rank-by-pencil \
    canonical_factorisation/factor-over-canonical-basis
do
    tool=$binaries/$relative
    name=$(basename "$tool")
    "$tool" --help >"$scratch/out" 2>"$scratch/err"
    got=$?
    if [ "$got" -ne 2 ]; then
        echo "  FAIL  $name --help left as $got, wanted 2"
        failures=$((failures + 1))
        continue
    fi
    if ! grep -q "usage: $name" "$scratch/err"; then
        echo "  FAIL  $name --help printed no usage naming itself"
        failures=$((failures + 1))
        continue
    fi
    # The flag has to appear in the text, or a reader has no way to find it.
    if ! grep -q -- "--help" "$scratch/err"; then
        echo "  FAIL  $name's usage does not mention --help"
        failures=$((failures + 1))
        continue
    fi
    # `report.h`'s rule, asserted where it is kept rather than where it is
    # written: a usage block is not a result, so it is commented and stdout is
    # untouched. A caller redirecting stdout into a file gets nothing to parse.
    if [ -s "$scratch/out" ]; then
        echo "  FAIL  $name --help wrote to stdout, which is for results only"
        failures=$((failures + 1))
        continue
    fi
    if grep -qv '^#' "$scratch/err"; then
        echo "  FAIL  $name --help wrote an uncommented line to stderr:"
        grep -nv '^#' "$scratch/err" | head -2
        failures=$((failures + 1))
        continue
    fi
    echo "  ok    2  $name --help"
done

# A spelling that has been published and then merged away has one job left: to
# name what to type instead. Silence would be worse than the old command, because
# `list-solvers | grep gurobi` going quiet reads as "no backends installed".
echo "a retired spelling names its replacement rather than going quiet"
refuses 2 "curve-bounds --solvers" "$binaries/integer_programme/list-solvers"
refuses 2 "curve-bounds --solvers" "$binaries/integer_programme/list-solvers" --help

if [ "$failures" -ne 0 ]; then
    echo "argument grammar: $failures failed"
    exit 1
fi
echo "argument grammar: all ok"
