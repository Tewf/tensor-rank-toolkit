#!/usr/bin/env python3
"""Ask every backend the same rank question and tabulate what each one cost.

Three instruments answer "is there an algorithm with k products", and they answer
it in different currencies: a tree search counts nodes, a SAT solver counts
conflicts. The only currency they share is wall-clock on one machine, so that is
what this measures, and it measures it on the same tensors with the same targets.

A fourth instrument was here and is not: the rank as a MILP lost by two to three
orders of magnitude on every row and was retired. Why, with the numbers:
state-of-the-art.md.

**The orbit reduction is a column, not a separate backend.** The whole claim of
this repository's symmetry work is that quotienting a search by the map's own
automorphisms is worth more than the term ordering the published encodings break
symmetry with, and a claim of that shape is only readable if the two numbers sit
side by side on one row. The term ordering gets its own row instead, because it is
a different constraint and not a variant of the same one.

    tools/compare_backends.py --build build --fixtures fixtures --repeats 3

Wall-clock is noisy upward and not downward, so `--repeats` reports the fastest of
N rather than a mean: a slow run means the machine was busy, and averaging that in
measures the machine.

Nothing here proves anything. A FOUND is checked by the command that produced it
against the map it must compute; a `no` is only as good as the search that
exhausted, which is why a timeout is reported as itself and never as a `no`.
"""
import argparse
import os
import re
import signal
import subprocess
import time
from pathlib import Path

# (tensor, target, matmul shape or None, the answer). One row either side of every
# known rank, so a wrong verdict is visible rather than merely slow. Ranks are
# from descent_search/README.md and fixtures/README.md.
QUESTIONS = [
    ("f2_2x2", 3, None, "yes"),
    ("f2_2x2", 2, None, "no"),
    ("f2_2x3", 5, None, "yes"),
    ("f2_2x3", 4, None, "no"),
    ("gf4_multiplication", 3, None, "yes"),
    ("gf4_multiplication", 2, None, "no"),
    ("gf8_multiplication", 6, None, "yes"),
    ("gf8_multiplication", 5, None, "no"),
    # Everything above is a correctness control rather than a comparison: the tree
    # search and both SAT rows answer all of it in under 0.02 s, and ranking
    # millisecond measurements is the error this repository has already published
    # twice. Everything below discriminates, cost being concentrated in the
    # refusal just below the rank.
    ("matmul_2x2x2", 7, (2, 2, 2), "yes"),
    ("matmul_2x2x2", 6, (2, 2, 2), "no"),
    ("matmul_2x2x3", 11, (2, 2, 3), "yes"),
    ("f2_5x5", 14, None, "yes"),
    ("f2_5x5", 11, None, "no"),
]

# (label, binary, extra arguments, which orbit reductions it takes).
#
# `any` accepts `--symmetry auto` as well, since those commands can build a
# stabiliser for a map that is not a matrix product. The SAT rows are `matmul`:
# their orbit reduction is a cube split, and a cube *is* a representative of the
# closed-form orbits of <n,m,k>, so there is no map whose own stabiliser would name
# one. Those cells read `n/a` rather than `refused`, because it is a boundary of the
# method and not a group too large to build. `--break-symmetry` is the term
# ordering, which is a different reduction and composes with the cubes.
BACKENDS = [
    ("tree search", "exhaustive_search/decide-rank", [], "any"),
    ("SAT", "satisfiability/decide-rank-by-sat", [], "matmul"),
    ("SAT + term ordering", "satisfiability/decide-rank-by-sat", ["--break-symmetry"], "matmul"),
]


def symmetry_flags(shape):
    """The largest group the commands will build for this map.

    A named matmul shape gets its closed-form group, which needs no group built at
    all. Anything else gets `auto`, which enumerates the stabiliser, and that is
    affordable only while the shape is small: on `f2_5x5` the commands refuse it
    outright, naming the 9.99872e13 automorphisms they would have to construct. So
    the orbit column is filled for every small row and legitimately empty for the
    large non-matmul ones, and `refused` in the table means exactly that.
    """
    if shape is None:
        return ["--symmetry", "auto"]
    return ["--symmetry", "matmul", *map(str, shape)]


# The solvers the SAT rows shell out to. They are launched under `timeout` on
# purpose, so an orphan dies within that cap rather than running until reboot, but
# a cap is not a cleanup: for those seconds the orphan holds a core and every later
# cell reads slow. Measured while the retired MILP row was still here, whose orphan
# `cbc` cost that row 6.62 s against a stray where a clean run read 3.34 s. The
# names are the SAT strand's because those are the only children left.
EXTERNAL_SOLVERS = ("kissat", "cryptominisat", "cryptominisat5", "cadical", "cvc5", "drat-trim")


def solver_pids():
    """The external solvers alive right now, by pid."""
    try:
        listing = subprocess.run(["ps", "-eo", "pid=,comm="], capture_output=True, text=True,
                                 timeout=10).stdout
    except (OSError, subprocess.SubprocessError):
        return set()
    found = set()
    for line in listing.splitlines():
        parts = line.split(None, 1)
        if len(parts) == 2 and parts[1].strip() in EXTERNAL_SOLVERS:
            found.add(int(parts[0]))
    return found


def reap(before):
    """Kill the external solvers that appeared since `before` and are still alive."""
    for pid in solver_pids() - before:
        try:
            os.kill(pid, signal.SIGKILL)
        except OSError:
            pass


def run(command, timeout):
    """(verdict, seconds). One of 'yes', 'no', 'timeout', 'refused' or 'error'.

    The whole process group is killed on timeout, not just the command. A backend
    that shells out to a solver leaves that solver running when only its parent
    is killed, and five such orphans once held two cores at full tilt for half an
    hour and quietly spoiled another session's timings.

    Then anything this cell started and the group kill missed is reaped by pid.
    Only pids that appeared during this cell are touched, so a solver somebody
    else is running is left alone.
    """
    before = solver_pids()
    started = time.monotonic()
    process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                               text=True, stdin=subprocess.DEVNULL, start_new_session=True)
    try:
        out, complaint = process.communicate(timeout=timeout)
        done = subprocess.CompletedProcess(command, process.returncode, out, complaint)
    except subprocess.TimeoutExpired:
        os.killpg(os.getpgid(process.pid), signal.SIGKILL)
        # Reap before draining, and never drain without a bound. `communicate()`
        # waits for end-of-file on the pipes, not for the child to die, so anything
        # that inherited them and survived the group kill holds it open forever.
        # That is not hypothetical: this harness sat in `poll` for ten minutes on
        # one cell whose command had already exited.
        reap(before)
        try:
            process.communicate(timeout=10)
        except subprocess.TimeoutExpired:
            pass
        return "timeout", timeout
    elapsed = time.monotonic() - started
    reap(before)
    # The verdict is read before the exit code, because `decide-rank` reports a
    # negative answer *through* a non-zero exit and would otherwise look broken.
    text = done.stdout
    if re.search(r"\bFOUND\b", text):
        return "yes", elapsed
    if re.search(r"\bNO\b|:\s*no\b", text):
        return "no", elapsed
    # A refusal is a result here, not a fault: these commands decline a
    # combination they cannot answer soundly and say why on stderr. Reporting that
    # as `error` would read as a broken backend, which is the opposite of what it
    # is. `--symmetry auto` on `f2_5x5` is the case that made this necessary.
    if done.stderr.strip():
        return "refused", elapsed
    return "error", elapsed


# Above this many seconds one run is enough. Repeats exist to stop scheduler noise
# deciding a ranking between two fast cells, and that noise is milliseconds; a cell
# measured in tens of seconds is not going to change rank because of it. Without
# this the matrix costs `repeats` times its slowest cells and does not finish.
LONG_ENOUGH_SECONDS = 5.0


def fastest_of(command, timeout, repeats):
    """The quickest run, and the verdict, which must not change between runs.

    A backend that answers differently on two identical runs is reported as
    `unstable` rather than having one of its answers picked.
    """
    best, verdict = None, None
    for _ in range(repeats):
        this_verdict, seconds = run(command, timeout)
        if verdict is not None and this_verdict != verdict:
            return "unstable", best
        verdict = this_verdict
        best = seconds if best is None else min(best, seconds)
        if this_verdict == "timeout" or best > LONG_ENOUGH_SECONDS:
            break
    return verdict, best


def cell(verdict, seconds, expected):
    """What to print, and whether it contradicts the known rank."""
    if verdict in ("timeout", "error", "unstable", "refused"):
        return verdict, 0
    if verdict != expected:
        return f"{seconds:.2f} !", 1
    return f"{seconds:.2f}", 0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", default="build")
    parser.add_argument("--fixtures", default="fixtures")
    parser.add_argument("--timeout", type=float, default=300.0)
    parser.add_argument("--repeats", type=int, default=3)
    parser.add_argument("--only", default=None, help="substring filter on the backend label")
    parser.add_argument("--question", default=None,
                        help="substring filter on the tensor name, so the expensive rows can be "
                             "run at a different timeout from the cheap ones")
    options = parser.parse_args()

    print(f"fastest of {options.repeats}, timeout {options.timeout:g} s. "
          "A `!` marks a verdict that disagrees with the known rank.")
    wrong = 0
    for name, target, shape, expected in QUESTIONS:
        if options.question and options.question not in name:
            continue
        tensor = str(Path(options.fixtures) / f"{name}.tensor")
        print(f"\n{name} at k = {target}   (the answer is {expected})")
        print(f"  {'backend':<22} {'plain':>10} {'with orbits':>12}")
        for label, binary, extra, orbits in BACKENDS:
            if options.only and options.only not in label:
                continue
            path = Path(options.build) / binary
            if not path.exists():
                print(f"  {label:<22} {'absent':>10}")
                continue
            base = [str(path), tensor, "--target", str(target), *extra]
            variants = [base]
            quotients = orbits == "any" or (orbits == "matmul" and shape is not None)
            variants.append(base + symmetry_flags(shape) if quotients else None)
            columns = []
            for arguments in variants:
                if arguments is None:
                    columns.append("n/a")
                    continue
                verdict, seconds = fastest_of(arguments, options.timeout, options.repeats)
                text, bad = cell(verdict, seconds, expected)
                wrong += bad
                columns.append(text)
            print(f"  {label:<22} {columns[0]:>10} {columns[1]:>12}")

    if wrong:
        print(f"\n{wrong} cell(s) disagreed with the known rank.")
    return 1 if wrong else 0


if __name__ == "__main__":
    raise SystemExit(main())
