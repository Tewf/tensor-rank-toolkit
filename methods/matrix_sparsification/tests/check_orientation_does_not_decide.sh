#!/bin/sh
# An operator and its transpose reach the same count.
#
# They must, because `nnz` does not care which way up a matrix is written and the
# admissible basis change for a decoding operator is the same question asked of
# the transpose. Until 2026-08-22 they did not: a matrix wider than it is tall
# went through every method then shipped unchanged, in microseconds, because
# "find invertible V minimising nnz(U V)" is **vacuous** on a wide full-rank
# matrix: every one of them has the whole space as its column space, so the
# constraint holds for anything and every route returns what it was given.
#
# Nothing caught it because every operator fixture here is 7x4, while
# `tighten-rank-bound --emit-operators` writes the decoding operator the other way
# up. Two halves of one pipeline, disagreeing silently about the third operator.
set -eu
tool="$1"
fixtures="$2"
work="${TMPDIR:-/tmp}/sparsify-orientation.$$"
mkdir -p "$work"
trap 'rm -rf "$work"' EXIT

# The nonzero count of every route, named rather than counted to. This read
# `awk '{print $(NF-3)}'` until the result line grew an operations count between
# the nonzeros and the seconds: that moves the field this wants without moving
# the field it reads, so the check would have gone on passing while comparing
# operations to operations. A pattern naming `nonzeros` cannot drift that way.
route_counts() { grep -v 'as given' | sed -n 's/^.*: \([0-9][0-9]*\) nonzeros.*$/\1/p'; }

status=0
for name in strassen_u strassen_v strassen_w alternative_basis_u; do
    tall="$fixtures/$name.matrix"
    [ -f "$tall" ] || { echo "  FAIL  no fixture $tall"; status=1; continue; }

    # The transpose, written in the same dense format the reader takes.
    awk '
      /^#/ { next }
      /^shape/ { rows = $2; cols = $3; next }
      NF { for (c = 1; c <= NF; c++) entry[NR "," c] = $c; seen++ }
      END {
        printf "# transpose of %s, written by the orientation check\n", FILENAME
        printf "shape %d %d\n", cols, rows
        n = 0
        for (r = 1; r <= NR; r++) if (length(entry[r ",1"])) { n++; keep[n] = r }
        for (c = 1; c <= cols; c++) {
          line = ""
          for (i = 1; i <= n; i++) line = line (i > 1 ? " " : "") entry[keep[i] "," c]
          print line
        }
      }' "$tall" > "$work/$name.t.matrix"

    best_tall=$("$tool" "$tall" | route_counts | sort -n | head -1)
    best_wide=$("$tool" "$work/$name.t.matrix" | route_counts | sort -n | head -1)

    if [ "$best_tall" = "$best_wide" ] && [ -n "$best_tall" ]; then
        echo "  ok    $name reaches $best_tall either way up"
    else
        echo "  FAIL  $name reaches $best_tall tall and $best_wide wide"
        status=1
    fi
done

[ "$status" -eq 0 ] && echo "orientation: all checks passed"
exit "$status"
