#!/bin/sh
# The exit codes are an interface, so they are asserted rather than described.
#
# The one that matters is 3. A question nobody answered must never leave as 1,
# because a script reading 1 as "proved impossible" would turn a timeout into a
# lower bound. That is not hypothetical here: this repository published an
# F2 5x5 result built on exactly that confusion, and the last case below is the
# same command that produced it.
#
# It lives under satisfiability/ because that is where the convention was first
# kept, but the vocabulary belongs to every command, so every command is
# asserted here rather than each strand growing a copy of this file.
set -u

command=$1
fixtures=$2
failures=0

# The other commands sit beside this one in the build tree, one directory per
# strand, so each path is the one ctest passes with the strand swapped. Passing
# seven more paths in would only restate that.
binaries=$(dirname "$(dirname "$command")")

expect() {
    want=$1
    shift
    "$command" "$@" >/dev/null 2>&1
    got=$?
    if [ "$got" -ne "$want" ]; then
        echo "  FAIL  wanted $want, got $got:  $*"
        failures=$((failures + 1))
    else
        echo "  ok    $want  $*"
    fi
}

echo "decide-rank-by-sat"
expect 0 "$fixtures/f2_2x2.tensor"
expect 0 "$fixtures/f2_2x2.tensor" --target 3
expect 1 "$fixtures/f2_2x2.tensor" --target 2
expect 2 --nonsense
expect 2 -h
expect 5 "$fixtures/no_such_file.tensor"

# Undecided, and deliberately the question this repository got wrong: five
# seconds cannot settle twelve products for F2 5x5, and saying so is the point.
expect 3 "$fixtures/f2_5x5.tensor" --target 12 --timeout 5

command=$binaries/exhaustive_search/decide-rank
echo "decide-rank"
expect 2
expect 2 "$fixtures/f2_2x2.tensor" --nonsense
# A target under the polynomial floor, which is a proof and not a search.
expect 1 "$fixtures/f2_2x2.tensor" --target 1
expect 5 "$fixtures/no_such_file.tensor"

# The same confusion as above, in the other decider, and the reason this file
# grew. One node settles nothing about twelve products for F2 5x5, and this
# used to leave as 2, which is what a mistyped flag leaves as. 3 and not 2 is
# the assertion: a retry loop reading 2 would have called the invocation wrong,
# and a caller reading it as a refusal would have had a lower bound out of a
# budget that expired after a single node.
expect 3 "$fixtures/f2_5x5.tensor" --target 12 --node-limit 1

# A pool too large to hold must refuse the routes that read a held pool,
# and this is the worst thing this file guards. Only the plain search with an
# explicit target walks an addressed pool, and so does the quotient now. What
# was fatal was reading the *materialised* pool when it did not fit, because
# that one is **empty** then, and an empty pool is not a small pool: `--symmetry`
# on <4,4,4> returned "NO ... exhaustive" for 47 products after one node, and
# AlphaTensor exhibits 47. A false refutation, leaving as 1.
#
# The quotient no longer reads it. Its candidate list is an index and its orbits
# a question rather than a table, so it takes the addressed pool where the held
# one is refused, and this line now asserts the answer rather than the refusal:
# 648 nodes and a genuine NO, the same count the held pool gives.
expect 1 "$fixtures/matmul_2x2x2.tensor" --target 6 --max-memory 1 -s matmul 2 2 2

# The sweep is the one route left reading a held pool, so it is still refused.
expect 5 "$fixtures/matmul_2x2x2.tensor" --max-memory 1
# And the plain route with a target still runs, because it too has an addressed
# form: undecided at one node, never a refusal.
expect 3 "$fixtures/matmul_2x2x2.tensor" --target 7 --max-memory 1 --node-limit 1

command=$binaries/descent_search/minimise-rank
echo "minimise-rank"
expect 2
expect 2 "$fixtures/f2_2x2.tensor" --nonsense
expect 0 "$fixtures/f2_2x2.tensor"
expect 5 "$fixtures/no_such_file.tensor"

command=$binaries/flip_graph/walk-scheme
echo "walk-scheme"
expect 2
expect 2 "$fixtures/f2_2x2.tensor" --nonsense
# --from 1 asks to start from a scheme the heuristic does not reach, so the
# argument is unavailable rather than the map refuted.
expect 2 "$fixtures/f2_2x2.tensor" --flips 50 --seeds 1 --from 1
expect 0 "$fixtures/f2_2x2.tensor" --flips 50 --seeds 1
expect 5 "$fixtures/no_such_file.tensor"

command=$binaries/map_construction/make-tensor
echo "make-tensor"
expect 2
expect 2 --nonsense 2
expect 0 --matmul 2 2 2 2

command=$binaries/matrix_sparsification/sparsify-operator
echo "sparsify-operator"
expect 2
expect 0 "$fixtures/strassen_u.matrix"
expect 5 "$fixtures/no_such_file.matrix"

command=$binaries/curve_bounds/curve-bounds
echo "curve-bounds"
expect 2
expect 2 --nonsense
expect 0 --table
expect 0 --degree 4 --points 2:4
# An odd degree out of degree-2 points only: infeasible as a fact about the
# supply, which is a refusal and not a budget that ran out.
expect 1 --degree 3 --points 2:4
expect 5 --degree 4 --points bogus

command=$binaries/integer_programme/list-solvers
echo "list-solvers"
expect 0

command=$binaries/pencil_rank/decide-rank-by-pencil
echo "decide-rank-by-pencil"
expect 2
# 0 when the rank is settled, 3 when only bounded. This command is the one place
# Undecided means "the mathematics stopped here", not "a budget ran out": the
# closure formula is exact over an algebraically closed field and short over a
# small one, so reporting it as a rank would be wrong.
expect 0 "$fixtures/pencil_split_f3_3.tensor"
expect 3 "$fixtures/pencil_irreducible_f2_4.tensor"
# More than two slices is not a pencil, which is a bad invocation.
expect 2 "$fixtures/matmul_2x2x2.tensor"
expect 5 "$fixtures/no_such_file.tensor"

command=$binaries/canonical_factorisation/factor-over-canonical-basis
echo "factor-over-canonical-basis"
expect 2
expect 2 --route bogus
expect 2 "$fixtures/f2_2x2.tensor" --route bogus
expect 0 "$fixtures/f2_2x2.tensor"
expect 5 "$fixtures/no_such_file.tensor"

if [ "$failures" -ne 0 ]; then
    echo "exit codes: $failures failed"
    exit 1
fi
echo "exit codes: all ok"
