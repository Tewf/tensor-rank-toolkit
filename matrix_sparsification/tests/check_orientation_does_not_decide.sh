#!/bin/sh
# An operator and its transpose reach the same count.
#
# They must, because `nnz` does not care which way up a matrix is written and the
# admissible basis change for a decoding operator is the same question asked of
# the transpose. Until 2026-08-22 they did not: a matrix wider than it is tall
# went through all four methods unchanged, in microseconds, because the question
# "find invertible V minimising nnz(U V)" is **vacuous** on a wide full-rank
# matrix — every one of them has the whole space as its column space, so the
# constraint holds for anything and each method returns what it was given.
#
# Nothing caught it because every operator fixture here is 7x4, while
# `lower-the-bound --emit-operators` writes the decoding operator the other way
# up. Two halves of one pipeline, disagreeing silently about the third operator.
set -eu
tool="$1"
fixtures="$2"
work="${TMPDIR:-/tmp}/sparsify-orientation.$$"
mkdir -p "$work"
trap 'rm -rf "$work"' EXIT

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

    best_tall=$("$tool" "$tall" | awk '/nonzeros/ && !/as given/ {print $(NF-3)}' | sort -n | head -1)
    best_wide=$("$tool" "$work/$name.t.matrix" | awk '/nonzeros/ && !/as given/ {print $(NF-3)}' | sort -n | head -1)

    if [ "$best_tall" = "$best_wide" ] && [ -n "$best_tall" ]; then
        echo "  ok    $name reaches $best_tall either way up"
    else
        echo "  FAIL  $name reaches $best_tall tall and $best_wide wide"
        status=1
    fi
done

[ "$status" -eq 0 ] && echo "orientation: all checks passed"
exit "$status"
