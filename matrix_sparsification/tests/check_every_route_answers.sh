#!/bin/sh
# Every route the command offers still answers, and they agree.
#
# The flag surface was rewritten on 2026-08-22: `--exact` was removed because the
# method it selected became the default, `--operations` was added for the one
# method minimising `nnz + nns`, and `--simplex` for the route that does not
# search. Three flags changed meaning in one afternoon and nothing ran them.
#
# What this would have caught, and did: `--exact` surviving in a test script and
# in two documents after the flag was gone, and the emit test silently passing
# because an unrecognised flag was refused before anything was written.
#
# The counts are asserted to AGREE rather than to equal a number. The default is
# the proved minimum, so any route returning less would be a contradiction and
# any route returning more is a route not worth having; either way the fixture's
# own value is not the thing under test here.
set -eu
tool="$1"
fixtures="$2"
work="${TMPDIR:-/tmp}/sparsify-routes.$$"
mkdir -p "$work"
trap 'rm -rf "$work"' EXIT

count_from() { awk -v pat="$2" '$0 ~ pat {print $(NF-3)}' "$1"; }

status=0
for name in 2x2x2_7_Strassen_L 2x2x2_7_Strassen_P 1o1o2_3_Karatsuba_P; do
    operator="$fixtures/plinopt/$name.sms"
    [ -f "$operator" ] || { echo "  FAIL  no fixture $operator"; status=1; continue; }

    "$tool" "$operator"               > "$work/default" 2>&1
    "$tool" "$operator" --operations  > "$work/ops"     2>&1
    "$tool" "$operator" --simplex     > "$work/lp"      2>&1

    minimum=$(count_from "$work/default" "matroid greedy over Q")
    rescaled=$(count_from "$work/ops" "greedy, by rescaling")
    programmed=$(count_from "$work/lp" "by linear programming")

    if [ -z "$minimum" ] || [ -z "$rescaled" ] || [ -z "$programmed" ]; then
        echo "  FAIL  $name: a route printed no count (default='$minimum'" \
             "operations='$rescaled' simplex='$programmed')"
        status=1
        continue
    fi
    # The default is the minimum over every invertible V, so nothing may beat it.
    if [ "$rescaled" -lt "$minimum" ] || [ "$programmed" -lt "$minimum" ]; then
        echo "  FAIL  $name: a route beat the proved minimum $minimum" \
             "(operations $rescaled, simplex $programmed)"
        status=1
        continue
    fi
    if [ "$rescaled" = "$minimum" ] && [ "$programmed" = "$minimum" ]; then
        echo "  ok    $name: all three routes reach $minimum"
    else
        echo "  ok    $name: minimum $minimum, rescaling $rescaled, simplex $programmed"
    fi
done

# A flag that no longer exists must be refused, not ignored. `--exact` was real
# until today and a script still passing it would otherwise look like it worked.
if "$tool" "$fixtures/strassen_u.matrix" --exact > "$work/gone" 2>&1; then
    echo "  FAIL  --exact was accepted; it was removed and must be refused"
    status=1
else
    echo "  ok    --exact is refused, not ignored"
fi

[ "$status" -eq 0 ] && echo "routes: all checks passed"
exit "$status"
