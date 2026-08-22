"""Two independent tests of regularity for 4x4x4_49_156_L.

(1) More basis determinants. One |det| >= 2 refutes.
(2) Tutte: a matroid is regular iff it is binary AND ternary, so the matroid of
    the columns over Q, over GF(2) and over GF(3) must agree. Disagreement on any
    subset refutes, and is a far more sensitive probe than determinants of full
    bases because it tests every rank, not just the top one.
"""
import random, sys
sys.setrecursionlimit(10000)

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
    m = [r[:] for r in m]; n = len(m); prev = 1; sign = 1
    for k in range(n-1):
        if m[k][k] == 0:
            sw = next((i for i in range(k+1, n) if m[i][k] != 0), None)
            if sw is None: return 0
            m[k], m[sw] = m[sw], m[k]; sign = -sign
        for i in range(k+1, n):
            for j in range(k+1, n):
                m[i][j] = (m[i][j]*m[k][k] - m[i][k]*m[k][j]) // prev
        prev = m[k][k]
    return sign * m[n-1][n-1]

def rank_mod(vectors, p):
    basis = []
    for v in vectors:
        v = [x % p for x in v]
        for b in basis:
            lead = next((i for i,e in enumerate(b) if e), None)
            if v[lead]:
                f = v[lead] * pow(b[lead], p-2, p) % p
                v = [(a - f*c) % p for a, c in zip(v, b)]
        if any(v): basis.append(v)
    return len(basis)

def rank_q(vectors):
    from fractions import Fraction
    basis = []
    for v in vectors:
        v = [Fraction(x) for x in v]
        for b in basis:
            lead = next((i for i,e in enumerate(b) if e), None)
            if v[lead]:
                f = v[lead]/b[lead]
                v = [a - f*c for a, c in zip(v, b)]
        if any(v): basis.append(v)
    return len(basis)

path = sys.argv[1]
given = read_sms(path)
tall = [list(c) for c in zip(*given)] if len(given) < len(given[0]) else given
G = [list(c) for c in zip(*tall)]
r, n = len(G), len(G[0])
cols = [[G[i][j] for i in range(r)] for j in range(n)]
random.seed(1)

print(f"{path.split('/')[-1]}: {r}x{n}")
worst = 1
for k in range(200000):
    pick = random.sample(range(n), r)
    d = abs(det_int([[cols[j][i] for j in pick] for i in range(r)]))
    if d > worst:
        worst = d
        print(f"  (1) determinants: |det| = {d} after {k+1} samples -> NOT unimodular")
        break
else:
    print(f"  (1) determinants: 200000 bases sampled, all |det| in {{0,1}}")

bad = 0
for k in range(20000):
    size = random.randint(2, r)
    pick = random.sample(range(n), size)
    vs = [cols[j] for j in pick]
    rq, r2, r3 = rank_q(vs), rank_mod(vs, 2), rank_mod(vs, 3)
    if not (rq == r2 == r3):
        bad += 1
        if bad == 1:
            print(f"  (2) Tutte: ranks disagree on {sorted(pick)} -> "
                  f"Q={rq} GF(2)={r2} GF(3)={r3}  -> NOT regular")
        if bad > 3: break
if bad == 0:
    print("  (2) Tutte: 20000 random subsets, Q / GF(2) / GF(3) ranks all agree")
else:
    print(f"  (2) Tutte: {bad} disagreements found")
