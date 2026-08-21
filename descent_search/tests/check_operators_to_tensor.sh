#!/bin/sh
# That `operators-to-tensor` run the way a collaborator will run it produces the
# fixture, and refuses the ways his files can fail to be an algorithm.
#
# `test_operators_to_tensor.cpp` asserts the convention against his bytes, and
# says nothing about the command: it reads the three files itself, so it would
# pass unchanged if the command handed them over as P, R, L. The order of three
# positional filenames is exactly the kind of interface no library test sees.
set -u

command=$1
fixtures=$2
failures=0

plinopt=$fixtures/plinopt
scratch=$(mktemp -d)
trap 'rm -rf "$scratch"' EXIT

# The invocation from the collaborator page, word for word. Comments differ by
# construction, since ours name the files they came from; everything a reader
# parses must not.
echo "his published Strassen operators produce our matrix multiplication fixture"
"$command" "$plinopt/2x2x2_7_Strassen_L.sms" "$plinopt/2x2x2_7_Strassen_R.sms" \
           "$plinopt/2x2x2_7_Strassen_P.sms" -q 2 >"$scratch/out" 2>"$scratch/err"
got=$?
grep -v '^#' "$scratch/out" >"$scratch/theirs"
grep -v '^#' "$fixtures/matmul_2x2x2.tensor" >"$scratch/ours"
if [ "$got" -ne 0 ]; then
    echo "  FAIL  left as $got: $(head -1 "$scratch/err")"
    failures=$((failures + 1))
elif ! diff -q "$scratch/theirs" "$scratch/ours" >/dev/null; then
    echo "  FAIL  the tensor differs from fixtures/matmul_2x2x2.tensor"
    diff "$scratch/ours" "$scratch/theirs" | head -8
    failures=$((failures + 1))
else
    echo "  ok    identical to fixtures/matmul_2x2x2.tensor, comments aside"
fi

# Exit code, and a refusal that names what is wrong with the files rather than
# naming the function that threw.
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
        echo "  FAIL  wanted '$said' on stderr, said: $(head -1 "$scratch/err")"
        failures=$((failures + 1))
        return
    fi
    echo "  ok    $want  $said"
}

echo "and the ways three files fail to be one algorithm are named"
# L where P belongs. The shapes no longer meet, which is the same refusal
# PLinOpt makes and in the same words.
refuses 5 "inner dimension mismatch" \
        "$command" "$plinopt/2x2x2_7_Strassen_P.sms" "$plinopt/2x2x2_7_Strassen_R.sms" \
        "$plinopt/2x2x2_7_Strassen_L.sms" -q 2
# SMS carries no field, so there is nothing to fall back on when -q is absent.
refuses 2 "--field expects a prime" \
        "$command" "$plinopt/2x2x2_7_Strassen_L.sms" "$plinopt/2x2x2_7_Strassen_R.sms" \
        "$plinopt/2x2x2_7_Strassen_P.sms"
refuses 2 "reads three files, L then R then P, and was given 2" \
        "$command" "$plinopt/2x2x2_7_Strassen_L.sms" "$plinopt/2x2x2_7_Strassen_R.sms" -q 2
# A rational algorithm has a modulus it does not survive: MMchecker on this same
# triple with -q 2 says "not a 2x2x2 MM algorithm", which is true and does not
# say that a denominator was the reason.
smallrat=$plinopt/2x2x2_7_DPS-smallrat-12.2034
refuses 5 "whose denominator vanishes modulo 2" \
        "$command" "${smallrat}_L.sms" "${smallrat}_R.sms" "${smallrat}_P.sms" -q 2

# The same triple at a prime it does survive is the 2x2x2 map again, which is
# what MMchecker says about it too: SUCCESS at -q 7 and -q 11, ERROR at 2, 3
# and 5. Read as an algorithm over GF(7), the ninths are ninths.
echo "and a rational algorithm at a prime it survives is the same map"
"$command" "${smallrat}_L.sms" "${smallrat}_R.sms" "${smallrat}_P.sms" -q 7 \
    >"$scratch/out" 2>"$scratch/err"
grep -v '^#' "$scratch/out" | sed 's/^field 7$/field 2/' >"$scratch/theirs"
if diff -q "$scratch/theirs" "$scratch/ours" >/dev/null; then
    echo "  ok    the rational algorithm rebuilds the same map over GF(7)"
else
    echo "  FAIL  over GF(7) it is not matmul_2x2x2"
    diff "$scratch/ours" "$scratch/theirs" | head -8
    failures=$((failures + 1))
fi

if [ "$failures" -ne 0 ]; then
    echo "operators-to-tensor: $failures failed"
    exit 1
fi
echo "operators-to-tensor: all ok"
