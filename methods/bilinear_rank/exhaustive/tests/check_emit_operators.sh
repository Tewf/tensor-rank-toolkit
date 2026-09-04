#!/bin/sh
# That `decide-rank --emit-operators` leaves behind a recipe worth trusting:
# the three SMS files exist only after a yes, and reading them back through
# `operators-to-tensor` rebuilds the input map entry for entry. The library
# test of the recovery cannot see any of this: files, suffixes and the round
# trip through a second command are the interface, not the algebra.
set -u

decide=$1
fixtures=$2
to_tensor=$3
failures=0

scratch=$(mktemp -d)
trap 'rm -rf "$scratch"' EXIT

echo "a bare sweep with --emit-operators writes the three files and exits 0"
"$decide" "$fixtures/f2_2x2.tensor" --emit-operators "$scratch/out" \
    >"$scratch/log" 2>&1
got=$?
if [ "$got" -ne 0 ]; then
    echo "  FAIL  exit $got: $(tail -1 "$scratch/log")"
    failures=$((failures + 1))
fi
for suffix in L R P; do
    if [ ! -f "$scratch/out_${suffix}.sms" ]; then
        echo "  FAIL  out_${suffix}.sms was not written"
        failures=$((failures + 1))
    fi
done

echo "the emitted triple rebuilds the input map entry for entry"
"$to_tensor" "$scratch/out_L.sms" "$scratch/out_R.sms" "$scratch/out_P.sms" \
    -q 2 >"$scratch/rebuilt" 2>"$scratch/err"
if [ $? -ne 0 ]; then
    echo "  FAIL  operators-to-tensor refused: $(head -1 "$scratch/err")"
    failures=$((failures + 1))
fi
grep -v '^#' "$scratch/rebuilt" >"$scratch/theirs"
grep -v '^#' "$fixtures/f2_2x2.tensor" >"$scratch/ours"
if ! diff -q "$scratch/theirs" "$scratch/ours" >/dev/null; then
    echo "  FAIL  the rebuilt map differs from the input"
    failures=$((failures + 1))
fi

echo "a proved no with the flag set writes nothing"
"$decide" "$fixtures/f2_2x2.tensor" --target 2 --emit-operators "$scratch/no" \
    >"$scratch/log2" 2>&1
got=$?
if [ "$got" -ne 1 ]; then
    echo "  FAIL  expected the proved no's exit 1, got $got"
    failures=$((failures + 1))
fi
if [ -f "$scratch/no_L.sms" ]; then
    echo "  FAIL  a refutation left an operator file behind"
    failures=$((failures + 1))
fi

exit "$failures"
