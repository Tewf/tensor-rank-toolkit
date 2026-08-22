"""Is the LP's basis the same operator? Checked in exact rational arithmetic.

The LP runs in floating point. A basis that spans a slightly different space is
worthless however light it is, so the vectors come back to exact rationals here
and the three ranks that matter are recomputed: the original, the answer, and the
two stacked.
"""
import sys, numpy as np
from fractions import Fraction
from scipy.optimize import linprog

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

def rref_rank(rows):
    M = [[Fraction(x) for x in r] for r in rows]
    if not M: return 0
    h, w = len(M), len(M[0]); piv = 0
    for c in range(w):
        t = next((k for k in range(piv, h) if M[k][c] != 0), None)
        if t is None: continue
        M[piv], M[t] = M[t], M[piv]
        for o in range(h):
            if o != piv and M[o][c] != 0:
                f = M[o][c]/M[piv][c]
                M[o] = [a - f*b for a, b in zip(M[o], M[piv])]
        piv += 1
    return piv

def null_space_basis(G):
    r, n = len(G), len(G[0])
    M = [[Fraction(x) for x in row] for row in G]
    pivots, row = [], 0
    for col in range(n):
        t = next((k for k in range(row, r) if M[k][col] != 0), None)
        if t is None: continue
        M[row], M[t] = M[t], M[row]
        M[row] = [e / M[row][col] for e in M[row]]
        for o in range(r):
            if o != row and M[o][col] != 0:
                f = M[o][col]; M[o] = [a - f*b for a, b in zip(M[o], M[row])]
        pivots.append(col); row += 1
    free = [c for c in range(n) if c not in pivots]
    out = []
    for c in free:
        v = [Fraction(0)]*n; v[c] = Fraction(1)
        for i, p in enumerate(pivots): v[p] = -M[i][c]
        out.append(v)
    return out

path = sys.argv[1]
given = read_sms(path)
tall = [list(c) for c in zip(*given)] if len(given) < len(given[0]) else given
G = [list(c) for c in zip(*tall)]
r, n = len(G), len(G[0])
Hq = null_space_basis(G)
H = np.array([[float(x) for x in row] for row in Hq])

cands = []
for j in range(n):
    Aeq = np.vstack([np.hstack([H, -H]),
                     np.concatenate([np.eye(1, n, j)[0], -np.eye(1, n, j)[0]])])
    beq = np.append(np.zeros(H.shape[0]), 1.0)
    res = linprog(np.ones(2*n), A_eq=Aeq, b_eq=beq, bounds=[(0, None)]*(2*n), method="highs")
    if res.status: continue
    x = res.x[:n] - res.x[n:]
    # Back to exact rationals. These come out on a small lattice, so rounding to
    # a denominator bound and then verifying is honest rather than hopeful.
    xq = [Fraction(round(v * 2520), 2520) for v in x]
    if any(xq): cands.append((sum(1 for v in xq if v != 0), xq))
cands.sort(key=lambda p: p[0])

chosen, weights = [], []
for w, v in cands:
    if rref_rank(chosen + [v]) > len(chosen):
        chosen.append(v); weights.append(w)
    if len(chosen) == r: break

print(f"{path.split('/')[-1]}: {r}x{n}")
print(f"  LP basis: {len(chosen)} vectors, total weight {sum(weights)}")
print(f"  weights: {weights}")
print("  --- exact rational receipt ---")
print(f"  rank(original row space)     = {rref_rank(G)}")
print(f"  rank(LP basis)               = {rref_rank(chosen)}")
print(f"  rank(both stacked)           = {rref_rank(G + chosen)}")
ok = rref_rank(G) == rref_rank(chosen) == rref_rank(G + chosen)
print(f"  same space: {ok}")
# And every LP vector really is in the row space: G^T y = v solvable.
print(f"  every vector lies in the row space: "
      f"{all(rref_rank(G + [v]) == rref_rank(G) for v in chosen)}")
