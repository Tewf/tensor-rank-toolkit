#!/bin/sh
# What `--emit` writes goes back the way up it came in, and is already minimal.
#
# The count does not care which way up a matrix is written, so nothing about the
# reported number would notice this being wrong. A *program* does. A decoding
# operator is `n² x R` and computes `n²` outputs from `R` products; written tall
# it is the transposed map, which is a different straight-line program with a
# different addition count, and the tool reading the file next has no way to tell.
# Written the wrong way up, `bin/optimizer` on our 9x23 answer for `Grey-221_P`
# reports 15 additions where the program that is actually run takes 29.
#
# Two claims, because either alone would pass while the file was useless:
# the shape that comes out is the shape that went in, and feeding the answer back
# in reaches the same count, since a minimum is a fixed point of a method that
# returns minima.
set -eu
tool="$1"
fixtures="$2"
work="${TMPDIR:-/tmp}/sparsify-emit.$$"
mkdir -p "$work"
trap 'rm -rf "$work"' EXIT

shape_of() { grep -v '^#' "$1" | head -1 | awk '{print $1 "x" $2}'; }
count_of() { awk '/matroid greedy over Q/ {print $(NF-3)}' "$1"; }

status=0
for name in 2x2x2_7_Strassen_P 2x2x2_7_Strassen_L 1o1o2_3_Karatsuba_P; do
    source="$fixtures/plinopt/$name.sms"
    [ -f "$source" ] || { echo "  FAIL  no fixture $source"; status=1; continue; }

    "$tool" "$source" --emit "$work/$name.sms" > "$work/$name.out" 2>&1
    [ -f "$work/$name.sms" ] || { echo "  FAIL  $name: nothing emitted"; status=1; continue; }

    want=$(shape_of "$source")
    got=$(shape_of "$work/$name.sms")
    if [ "$want" != "$got" ]; then
        echo "  FAIL  $name went in $want and came out $got"
        status=1
        continue
    fi

    "$tool" "$work/$name.sms" > "$work/$name.again" 2>&1
    first=$(count_of "$work/$name.out")
    again=$(count_of "$work/$name.again")
    if [ "$first" = "$again" ] && [ -n "$first" ]; then
        echo "  ok    $name emits $want at $first nonzeros, and stays there"
    else
        echo "  FAIL  $name emitted $first and re-reading it reached $again"
        status=1
    fi
done

[ "$status" -eq 0 ] && echo "emit: all checks passed"
exit "$status"
