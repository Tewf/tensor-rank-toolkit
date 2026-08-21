#!/bin/sh
# `decide-rank` must take the polynomial route when a pencil settles the rank,
# and must not take it when the pencil only bounds it.
#
# Both halves matter. Taking the shortcut where the form is exact is the win: the
# pool for a two-slice tensor over a large field is unbuildable and the answer is
# microseconds away. Refusing it where the form is inexact is the correctness
# half: `pencil_rank_of` still returns a proved *bound* there, that bound loses to
# `flattening_floor` on every fixture measured, and reporting it as the rank would
# turn a lower bound into an answer. This asserts the shortcut is taken exactly
# where it is sound, which prose next to the code cannot.
set -u

command=$1
fixtures=$2
failures=0

report() {
    if [ "$2" -eq 0 ]; then
        printf '  ok   %s\n' "$1"
    else
        printf '  FAIL %s\n' "$1"
        failures=$((failures + 1))
    fi
}

# Exact: no minimal indices and distinct linear divisors, so the form is the rank.
said=$("$command" "$fixtures/pencil_split_f3_3.tensor" 2>&1)
echo "$said" | grep -q 'pencil: two slices'; report "an exact pencil takes the polynomial route" $?
echo "$said" | grep -q 'rank: 3 (exact'; report "and reports the rank the pencil module reports" $?
# Still a decomposition, because that is what `decide-rank` without `--target`
# has always returned and the pencil has none to give: it reads a rank off a
# canonical form and never builds one. The rank becomes the target instead, so
# the sweep asks one question at the value already proved rather than climbing
# to it. `reproduce/measure.py` caught the version that returned the number.
echo "$said" | grep -q 'FOUND: 3 products'; report "and still returns the decomposition" $?
echo "$said" | grep -q 'verified'; report "which is verified against the map" $?

# A target under the rank needs no pool at all: the form has already refuted it.
said=$("$command" "$fixtures/pencil_split_f3_3.tensor" --target 2 2>&1)
echo "$said" | grep -q 'NO: there is no algorithm with 2'; report "a target under the rank is refused" $?
echo "$said" | grep -q 'pool:' && report "without building a pool" 1 || report "without building a pool" 0

# Inexact: the module proves a bound and not the rank, so the search must run.
said=$("$command" "$fixtures/pencil_irreducible_f2_4.tensor" 2>&1)
echo "$said" | grep -q 'pencil: two slices' && report "an inexact pencil does not take it" 1 \
                                            || report "an inexact pencil does not take it" 0
echo "$said" | grep -q 'pool:'; report "and falls through to the search" $?
echo "$said" | grep -q 'FOUND: 6 products'; report "reaching 6, which the pencil bound of 5 misses" $?

# More than two slices is a different problem and must never reach the shortcut.
said=$("$command" "$fixtures/matmul_2x2x2.tensor" --target 7 2>&1)
echo "$said" | grep -q 'pencil: two slices' && report "four slices never take it" 1 \
                                            || report "four slices never take it" 0

if [ "$failures" -ne 0 ]; then
    printf '%d check(s) failed\n' "$failures"
    exit 1
fi
