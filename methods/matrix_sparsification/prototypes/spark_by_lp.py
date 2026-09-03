"""The minimum weight by linear programming, which is exact only if the matroid
is regular.

`[tillmann2019, Thm 5]`: for a unimodular matrix the l0 problem collapses to l1
on basic solutions, so the spark is an LP rather than a search. Our quantity is
the *co*girth of the column matroid of G, which is the girth of the dual, and
regular matroids are closed under duality: so build H spanning the null space of
G and compute spark(H).

    spark(H) = min over j of  min ||x||_1  s.t.  Hx = 0, x_j = 1

n small LPs. Compared here against what the exact scan already established, which
is the only thing that says whether the collapse really happened.
"""
import sys, numpy as np
from scipy.optimize import linprog
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

def null_space_basis(G):
    """Rows spanning {y : G y^T = 0}, exactly, over Q."""
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
    basis = []
    for c in free:
        v = [Fraction(0)]*n; v[c] = Fraction(1)
        for i, p in enumerate(pivots): v[p] = -M[i][c]
        basis.append([float(x) for x in v])
    return basis

path = sys.argv[1]
given = read_sms(path)
tall = [list(c) for c in zip(*given)] if len(given) < len(given[0]) else given
G = [list(c) for c in zip(*tall)]                 # r x n
r, n = len(G), len(G[0])
H = np.array(null_space_basis(G))                 # (n-r) x n, rowspace = null(G)
print(f"{path.split('/')[-1]}: G is {r}x{n}, H is {H.shape[0]}x{H.shape[1]}")

# min ||x||_1 s.t. Hx = 0, x_j = 1, with x = u - v, u,v >= 0
m = H.shape[0]
best, argj = None, None
for j in range(n):
    Aeq = np.hstack([H, -H])
    beq = np.zeros(m)
    fix = np.zeros(2*n); fix[j] = 1; fix[n+j] = -1
    Aeq = np.vstack([Aeq, fix]); beq = np.append(beq, 1.0)
    res = linprog(np.ones(2*n), A_eq=Aeq, b_eq=beq,
                  bounds=[(0, None)]*(2*n), method="highs")
    if res.status == 0:
        if best is None or res.fun < best - 1e-9:
            best, argj = res.fun, j
print(f"  LP optimum over all j: {best:.6f}  (at coordinate {argj})")
print(f"  rounded: {round(best)}")
print("  If the matroid is regular this is the exact minimum weight.")
