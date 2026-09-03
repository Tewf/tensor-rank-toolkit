"""Is the column matroid regular? If it is, this whole problem is polynomial.

[tillmann2019, Thm 5]: for a unimodular matrix the spark is computable in
polynomial time, because the l0 problem collapses to l1 on basic solutions, and
the vector matroids of totally unimodular matrices are the regular ones, which
are closed under duality. Our operators are 0/+-1, so the necessary condition
holds and the question is worth ten seconds before anything else is built.

Refuting is cheap and confirming is not: one basis with |det| >= 2 refutes it, so
sample first and only then consider the C(n, r) full test.
"""
import sys, random, itertools
from fractions import Fraction

def read_sms(path):
    with open(path) as h:
        head = h.readline().split()
        while head and head[0].startswith('#'): head = h.readline().split()
        rows, cols = int(head[0]), int(head[1])
        g = [[0]*cols for _ in range(rows)]
        for line in h:
            p = line.split()
            if len(p) < 3 or p[0] == '0': continue
            g[int(p[0])-1][int(p[1])-1] = int(p[2])
    return g

def det_int(m):
    """Bareiss: exact integer determinant, no fractions."""
    m = [row[:] for row in m]; n = len(m); prev = 1; sign = 1
    for k in range(n-1):
        if m[k][k] == 0:
            swap = next((i for i in range(k+1, n) if m[i][k] != 0), None)
            if swap is None: return 0
            m[k], m[swap] = m[swap], m[k]; sign = -sign
        for i in range(k+1, n):
            for j in range(k+1, n):
                m[i][j] = (m[i][j]*m[k][k] - m[i][k]*m[k][j]) // prev
        prev = m[k][k]
    return sign * m[n-1][n-1]

def check(path, samples=20000):
    given = read_sms(path)
    tall = [list(c) for c in zip(*given)] if len(given) < len(given[0]) else given
    G = [list(c) for c in zip(*tall)]          # r x n, the same matrix the oracles take
    r, n = len(G), len(G[0])
    cols = [[G[i][j] for i in range(r)] for j in range(n)]
    seen, worst, bad = set(), 1, None
    random.seed(0)
    for _ in range(samples):
        pick = tuple(sorted(random.sample(range(n), r)))
        if pick in seen: continue
        seen.add(pick)
        d = abs(det_int([[cols[j][i] for j in pick] for i in range(r)]))
        if d > worst: worst, bad = d, pick
        if d > 1: break
    total = len(list(itertools.combinations(range(n), r))) if n < 30 else None
    print(f"{path.split('/')[-1]}: {r}x{n}, sampled {len(seen)} of "
          f"{total if total else 'C(%d,%d), too many' % (n, r)} column bases")
    print(f"  largest |det| seen: {worst}" + (f"  at columns {bad}" if bad else ""))
    print("  -> NOT unimodular; Tillmann's polynomial route does not apply"
          if worst > 1 else
          "  -> every sampled basis is +-1; worth the full test")

for p in sys.argv[1:]:
    check(p); print()
