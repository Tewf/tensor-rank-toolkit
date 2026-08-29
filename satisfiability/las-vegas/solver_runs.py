"""Running one solver on one DIMACS or XNF file, and checking what it says.

The fixtures go through `decide-rank-by-sat`, which checks a yes by multiplying
the decomposition out. Heule's challenge instances are their own CNF, with no
tensor behind them that this repository can read back, so they are run directly
and a yes is checked the only way a formula allows: every clause holds and every
parity has the right value under the model. A solver's own `s SATISFIABLE` is
never taken on its word.

The command shapes mirror `satisfiability/local_search_solver.cpp`, which is the
copy that ships; this one exists because the C++ has no CNF-in command, and it
says so here so that the day one is added this file goes.
"""
import os
import resource
import signal
import subprocess
import time

# What a solver that can only find may say, and what is believed of it. A
# refutation from one is discarded into `unknown`, exactly as `run_solver` does
# in C++: yalsat prints `s UNSATISFIABLE` when unit propagation alone closes a
# formula, and that is not this class's to assert.
FINDS_ONLY = ("yalsat", "xnfsat", "probSAT", "multilinear-sat")
# Which of them takes a parity as one `x` line and is handed the XNF.
READS_XOR_LINES = ("xnfsat",)


def command_for(name, binary, formula, seed, timeout):
    """The argument vector, per solver. kissat takes no seed here: its runs are
    deterministic and the protocol for it is the fastest of three."""
    if name == "yalsat":
        return [binary, formula, str(seed)]
    if name == "xnfsat":
        # Its witness is off by default, and a yes without a model is unchecked.
        return [binary, "--witness=1", formula, str(seed)]
    if name == "probSAT":
        return [binary, "-a", formula, str(seed)]
    if name == "multilinear-sat":
        # One core, as the C++ does: its CPU backend is OpenMP over the batch
        # and was measured at twelve threads per process.
        return ["env", "OMP_NUM_THREADS=1", binary, formula, "--time-limit", str(timeout),
                "--seed", str(seed), "--backend", "cpu"]
    if name == "kissat":
        return [binary, formula]
    raise ValueError(f"{name} is not a solver this driver knows how to run")


def read_constraints(path):
    """(clauses, parities) of a DIMACS or XNF file. A parity is the literals of
    an `x` line, which must xor to true, the convention of cnf2xnf, xnfsat,
    cryptominisat and this repository's writer alike."""
    clauses, parities = [], []
    with open(path) as file:
        for line in file:
            if line.startswith(("c", "p")) or not line.strip():
                continue
            is_parity = line.startswith("x")
            literals = [int(word) for word in line.lstrip("x").split()]
            if literals and literals[-1] == 0:
                literals.pop()
            if literals:
                (parities if is_parity else clauses).append(literals)
    return clauses, parities


def model_satisfies(constraints, literals):
    """Whether every clause and every parity holds under the assignment the
    `v` lines gave."""
    clauses, parities = constraints
    true = set(literals)
    return (all(any(literal in true for literal in clause) for clause in clauses) and
            all(sum(literal in true for literal in parity) % 2 == 1 for parity in parities))


def cap_address_space(megabytes):
    """The same RLIMIT_AS the C++ launcher sets, so a solver that grows is
    stopped by the cap rather than by the machine."""
    limit = megabytes << 20
    return lambda: resource.setrlimit(resource.RLIMIT_AS, (limit, limit))


def run_on_formula(name, binary, formula, seed, timeout, memory_megabytes=2048,
                   constraints=None):
    """One run: `verdict` in found, unknown, refuted, unverified or error, with
    `seconds` of wall clock and, on a find, `model_checked` against the file.

    The process group is killed on timeout, not just the child, for the reason
    `tools/compare_backends.py` gives: a parent killed alone leaves its solver
    holding a core, and every later cell then reads slow.
    """
    command = command_for(name, binary, formula, seed, timeout)
    started = time.perf_counter()
    process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
                               stdin=subprocess.DEVNULL, text=True, start_new_session=True,
                               preexec_fn=cap_address_space(memory_megabytes))
    # One second past the cap: multilinear-sat stops itself at `--time-limit`
    # and prints `s UNKNOWN`; yalsat, xnfsat and probSAT have no clock and are
    # killed.
    try:
        output, _ = process.communicate(timeout=timeout + 1)
    except subprocess.TimeoutExpired:
        os.killpg(os.getpgid(process.pid), signal.SIGKILL)
        output, _ = process.communicate(timeout=10)
    seconds = time.perf_counter() - started

    status = None
    literals = []
    for line in output.splitlines():
        if line.startswith("s "):
            status = line[2:].strip()
        elif line.startswith("v "):
            literals.extend(int(word) for word in line[2:].split() if word != "0")

    result = {"seed": seed, "seconds": round(seconds, 3), "verdict": "unknown"}
    if status == "SATISFIABLE":
        checked = model_satisfies(
            constraints if constraints is not None else read_constraints(formula), literals)
        result["verdict"] = "found" if checked else "unverified"
        result["model_checked"] = checked
    elif status == "UNSATISFIABLE":
        result["verdict"] = "unknown" if name in FINDS_ONLY else "refuted"
        if name in FINDS_ONLY:
            result["discarded"] = "the solver claimed unsatisfiable; a finds-only no is nothing"
    elif status not in (None, "UNKNOWN"):
        result["verdict"] = "error"
        result["said"] = status
    return result
