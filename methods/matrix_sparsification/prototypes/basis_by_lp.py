"""A whole basis from the LP route, and how it compares with what is known.

One LP per coordinate gives a lightest codeword through that coordinate. Fed to
the matroid greedy in ascending weight those n candidates may or may not span;
this measures whether they do, what they weigh, and how that sits against the
bound the exact scan already licenses.

The LP's *first* weight is exact when the matroid is regular. Nothing here claims
the whole basis is: that is the open question, and this is the measurement that
says how close it gets.
"""
import sys, time, numpy as np
from scipy.optimize import linprog
from fractions import Fraction

exec(open(sys.argv[2]).read().split('path = sys.argv[1]')[0])   # reuse the readers

path = sys.argv[1]
given = read_sms(path)
tall = [list(c) for c in zip(*given)] if len(given) < len(given[0]) else given
G = [list(c) for c in zip(*tall)]
r, n = len(G), len(G[0])
H = np.array(null_space_basis(G))
m = H.shape[0]
print(f"{path.split('/')[-1]}: G {r}x{n}, H {m}x{n}")

started = time.time()
cands = []
for j in range(n):
    Aeq = np.vstack([np.hstack([H, -H]),
                     np.concatenate([np.eye(1, n, j)[0], -np.eye(1, n, j)[0]])])
    beq = np.append(np.zeros(m), 1.0)
    res = linprog(np.ones(2 * n), A_eq=Aeq, b_eq=beq,
                  bounds=[(0, None)] * (2 * n), method="highs")
    if res.status: continue
    x = res.x[:n] - res.x[n:]
    x = np.where(np.abs(x) < 1e-7, 0.0, x)
    if np.any(x): cands.append((int(np.count_nonzero(x)), x))
elapsed = time.time() - started
cands.sort(key=lambda p: p[0])
print(f"  {len(cands)} candidates from {n} LPs in {elapsed:.2f}s; "
      f"weights {sorted(set(w for w, _ in cands))}")

# The matroid greedy over just those candidates.
chosen, weights = [], []
for w, x in cands:
    M = np.array(chosen + [x])
    if np.linalg.matrix_rank(M, tol=1e-7) > len(chosen):
        chosen.append(x); weights.append(w)
    if len(chosen) == r: break
print(f"  greedy over them holds {len(chosen)}/{r} vectors, total weight {sum(weights)}")
print(f"  weights: {weights}")
if len(chosen) < r:
    print("  -> does NOT span; the LP candidates alone are not a basis")
