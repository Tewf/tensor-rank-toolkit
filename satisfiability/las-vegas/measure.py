#!/usr/bin/env python3
"""Price a solver that can only find against kissat, on the same files.

A stochastic local search is a Las Vegas algorithm: right whenever it answers,
random in how long it takes. So its cost is a distribution and not a number, and
this driver reports it as one: every seed's wall clock, how many seeds finished
within the cap, and the median where more than half did. kissat is asked the
same file beside it, fastest of three, which is the protocol `MEASURING.md`
sets for a deterministic run. Two files come out: `results.json`, with every run
and its command, and `results.md`, the table read off it.

    satisfiability/las-vegas/measure.py --build build --seeds 5 --timeout 60
    satisfiability/las-vegas/measure.py --build build --only challenges \\
        --challenges build/third_party/matrix-challenges/challenge1

This is not a row in `reproduce/measure.py`, and deliberately: that driver takes
the fastest of three deterministic runs and re-derives its counts in CI, while
"finished within the cap" is a fact about a machine on an afternoon that no
shared runner can check. It borrows that driver's `provenance` so the stamp has
one definition.

`--jobs` runs that many solver processes at once. The protocol is one core and
a quiet machine, so a number taken at `--jobs 3` is a finish-or-not within the
cap and the seconds beside it carry the chassis's thermal band; the stamp
records the count so nobody quotes a two-digit ratio off it.
"""
import argparse
import concurrent.futures
import json
import os
import pathlib
import re
import shutil
import statistics
import subprocess
import sys
import time

HERE = pathlib.Path(__file__).resolve().parent
ROOT = HERE.parent.parent
sys.path.insert(0, str(ROOT / "reproduce"))
from measure import provenance  # noqa: E402
from questions import shown  # noqa: E402
from results_table import table  # noqa: E402
from solver_runs import FINDS_ONLY, READS_XOR_LINES, read_constraints, run_on_formula  # noqa: E402

# The fixtures, the k a decomposition is found at, and what that k is. The first
# five are controls every solver answers in milliseconds; the last four are the
# brief's, and `f2_5x5` at 14 is the heuristic's ceiling and not the rank, which
# is 13.
QUESTIONS = [
    ("f2_2x2", 3, "rank"),
    ("w_state", 3, "rank"),
    ("gf4_multiplication", 3, "rank"),
    ("gf8_multiplication", 6, "rank"),
    ("f2_2x3", 5, "rank"),
    ("matmul_2x2x2", 7, "rank, Strassen"),
    ("cyclic_f2_7", 13, "rank, established here on both sides"),
    ("f2_5x5", 14, "an upper bound the heuristic reaches; the rank is 13"),
    ("matmul_3x3x3", 23, "Laderman, the best known; what Heule's instances ask"),
]
KISSAT_REPEATS = 3
# The two solvers that are here as controls, one row each with numbers: the
# field already records that a WalkSAT-type solver fails on expanded parities,
# and the continuous solver has no parity support yet. They get fewer seeds.
CONTROLS = ("probSAT", "multilinear-sat")


def portable(command):
    """A command line without this machine's home directory in it: relative to
    the repository where it can be, `~` where it cannot. Every published
    `results.json` once carried an absolute home path, and this is the same
    fix `reproduce/measure.py` made, at the source."""
    return shown(command).replace(str(pathlib.Path.home()), "~")


def solvers_from(arguments):
    """Name to binary, for every solver this run can reach. A finds-only
    solver is given by flag or found on PATH; kissat is PATH only.

    A given path that does not exist is refused rather than dropped: on
    2026-09-01 a deleted `build/third_party/` turned 34 overnight runs into
    `timeout: failed to run command` logs that were read as negatives. A
    missing binary must end the run, never thin the roster."""
    found = {}
    for name, given in (("yalsat", arguments.yalsat), ("xnfsat", arguments.xnfsat),
                        ("probSAT", arguments.probsat),
                        ("multilinear-sat", arguments.multilinear_sat), ("kissat", None)):
        if given and not os.access(given, os.X_OK):
            raise SystemExit(f"--{name.lower().replace('-', '_')} {given}: "
                             "not an executable file")
        path = given or shutil.which(name)
        if path:
            found[name] = str(pathlib.Path(path).resolve())
    return found


def fixture_run(build, fixture, target, break_symmetry, name, binary, seed, timeout):
    """One `decide-rank-by-sat` question, read off its exit code and its own
    clock. 0 is a find the command has already checked, 3 is the third answer."""
    command = [str(build / "satisfiability" / "decide-rank-by-sat"),
               str(ROOT / "fixtures" / f"{fixture}.tensor"), "--target", str(target),
               "--solver", binary, "--seed", str(seed), "--timeout", str(timeout)]
    # A named solver decides the file's shape by what it reads, so the flag is
    # recorded only where it describes the file: xnfsat is handed the x lines.
    if name not in READS_XOR_LINES:
        command.append("--plain-cnf")
    if break_symmetry:
        command.append("--break-symmetry")
    started = time.perf_counter()
    try:
        done = subprocess.run(command, capture_output=True, text=True, timeout=timeout + 60)
    except subprocess.TimeoutExpired:
        # The command enforces the cap itself; a minute past it is a hang, and
        # one hung cell must not take the sweep down with it.
        return {"seed": seed, "verdict": "error", "seconds": round(timeout + 60.0, 4),
                "command": portable(command), "note": "the command outlived its own cap"}
    wall = time.perf_counter() - started
    clock = re.search(r"\(([\d.]+) s\)|gave up after ([\d.]+) s", done.stdout)
    seconds = float(clock.group(1) or clock.group(2)) if clock else wall
    verdict = {0: "found", 1: "refuted", 3: "unknown"}.get(done.returncode, "error")
    return {"seed": seed, "verdict": verdict, "seconds": round(seconds, 4),
            "command": portable(command)}


def summary(runs, timeout):
    """How many seeds finished, the mean over those that did, which is
    `[nawrocki2021]`'s measure, and the median over all of them where more than
    half did. A seed that hit the cap is not a time and is not averaged in."""
    finished = [run["seconds"] for run in runs if run["verdict"] == "found"]
    block = {"seeds": runs, "finished_within_cap": len(finished), "of": len(runs),
             "cap_seconds": timeout,
             "mean_seconds_of_finished": round(statistics.mean(finished), 4) if finished else None}
    if len(finished) * 2 > len(runs):
        block["median_seconds"] = round(statistics.median(
            finished + [timeout] * (len(runs) - len(finished))), 4)
    else:
        block["median_seconds"] = None
        block["median_note"] = "half or more of the seeds hit the cap, so the median is above it"
    return block


def measure_fixtures(build, solvers, seeds, control_seeds, timeout, jobs, only=None):
    """Every question, with and without the term ordering, under every solver.
    `only` names the fixtures wanted, for a rerun of one row."""
    jobs_list = []
    for fixture, target, meaning in QUESTIONS:
        if only and fixture not in only:
            continue
        for break_symmetry in (False, True):
            for name, binary in solvers.items():
                runs = [1] * KISSAT_REPEATS
                if name in FINDS_ONLY:
                    runs = range(1, (control_seeds if name in CONTROLS else seeds) + 1)
                for seed in runs:
                    jobs_list.append((fixture, target, break_symmetry, name, binary, seed))
    with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as pool:
        results = list(pool.map(lambda job: (job, fixture_run(build, *job[:5], job[5], timeout)),
                                jobs_list))
    rows = {}
    for (fixture, target, break_symmetry, name, _, _), run in results:
        key = (fixture, target, break_symmetry)
        row = rows.setdefault(key, {"fixture": fixture, "target": target,
                                    "what_the_target_is": dict(
                                        (q[0], q[2]) for q in QUESTIONS)[fixture],
                                    "break_symmetry": break_symmetry, "solvers": {}})
        row["solvers"].setdefault(name, []).append(run)
    for row in rows.values():
        for name, runs in row["solvers"].items():
            if name in FINDS_ONLY:
                row["solvers"][name] = summary(runs, timeout)
            else:
                fastest = min(runs, key=lambda run: run["seconds"])
                row["solvers"][name] = {"verdict": fastest["verdict"],
                                        "seconds": fastest["seconds"],
                                        "fastest_of": len(runs), "command": fastest["command"]}
    return list(rows.values())


def xnf_of(cnf, cnf2xnf):
    """The XNF `cnf2xnf` recovers from a challenge CNF, written once beside it.
    The instances carry no licence, so the derived file stays in the same
    ignored directory."""
    xnf = cnf.with_suffix(".xnf")
    if not xnf.exists():
        subprocess.run([cnf2xnf, str(cnf), str(xnf)], check=True, capture_output=True)
    return xnf


def measure_challenges(directory, solvers, seeds, control_seeds, timeout, jobs, cnf2xnf):
    """Heule's satisfiable instances, run directly, every find checked against
    the file's own clauses and parities. xnfsat is handed the XNF that cnf2xnf
    recovers from each CNF, which is how `[nawrocki2021]` ran it."""
    given = pathlib.Path(directory)
    instances = [given] if given.is_file() else sorted(given.glob("*.cnf"))
    if any(name in READS_XOR_LINES for name in solvers) and not cnf2xnf:
        sys.exit("xnfsat reads XNF, so a challenge run needs --cnf2xnf to recover it")
    formula_of = {(cnf, name): xnf_of(cnf, cnf2xnf) if name in READS_XOR_LINES else cnf
                  for cnf in instances for name in solvers}
    constraints = {path: read_constraints(path) for path in set(formula_of.values())}
    jobs_list = [(cnf, name, binary, seed)
                 for cnf in instances for name, binary in solvers.items()
                 for seed in (range(1, (control_seeds if name in CONTROLS else seeds) + 1)
                              if name in FINDS_ONLY else [1])]
    with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as pool:
        results = list(pool.map(
            lambda job: (job, run_on_formula(job[1], job[2], str(formula_of[(job[0], job[1])]),
                                             job[3], timeout,
                                             constraints=constraints[formula_of[(job[0], job[1])]])),
            jobs_list))
    rows = {}
    for (cnf, name, binary, seed), run in results:
        row = rows.setdefault(cnf, {"instance": cnf.name, "solvers": {}})
        run["command"] = portable([binary, formula_of[(cnf, name)].name, str(seed)])
        row["solvers"].setdefault(name, []).append(run)
    for row in rows.values():
        for name, runs in row["solvers"].items():
            row["solvers"][name] = summary(runs, timeout)
    return list(rows.values())


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--build", default="build")
    parser.add_argument("--seeds", type=int, default=5)
    parser.add_argument("--control-seeds", type=int, default=3,
                        help="seeds for probSAT and multilinear-sat, the two control rows; "
                             "0 drops them, which a challenge run wants since the fixtures "
                             "already price them")
    parser.add_argument("--timeout", type=int, default=60, help="cap per fixture question")
    parser.add_argument("--jobs", type=int, default=1)
    parser.add_argument("--only", choices=("fixtures", "challenges"))
    parser.add_argument("--fixture", action="append",
                        help="measure this fixture only; repeatable")
    parser.add_argument("--solvers", help="comma-separated names to measure; the other "
                        "solvers' cells in a rerun row are kept. For a longer cap on one "
                        "solver, whose cell then records it in its own command")
    parser.add_argument("--challenges", help="a directory of challenge CNF files, or one file")
    parser.add_argument("--challenge-timeout", type=int, default=300)
    parser.add_argument("--challenge-seeds", type=int, default=3)
    parser.add_argument("--yalsat")
    parser.add_argument("--xnfsat")
    parser.add_argument("--cnf2xnf", help="the extractor, needed for xnfsat on a challenge CNF")
    parser.add_argument("--probsat")
    parser.add_argument("--multilinear-sat")
    arguments = parser.parse_args()

    build = (ROOT / arguments.build).resolve()
    solvers = solvers_from(arguments)
    if arguments.solvers:
        solvers = {name: path for name, path in solvers.items()
                   if name in arguments.solvers.split(",")}
    if not solvers:
        sys.exit("no solver was given or found; see --help")
    output = HERE / "results.json"
    document = json.loads(output.read_text()) if output.exists() else {}
    document.update({
        "task": "What a solver that can only find, a stochastic local search, costs on the "
                "GF(2) encodings against kissat on the same file.",
        "reading": "A finds-only solver never refutes: its 'unknown' is the third answer and "
                   "never a no. Its cost is a distribution over seeds, reported as every seed, "
                   "the count finished within the cap, and the median where more than half "
                   "finished. kissat is the fastest of three deterministic runs.",
    })
    if arguments.only != "challenges":
        fresh = measure_fixtures(build, solvers, arguments.seeds, arguments.control_seeds,
                                 arguments.timeout, arguments.jobs, arguments.fixture)
        # A rerun of one fixture replaces its rows and keeps the others; a
        # rerun of some solvers replaces their cells and keeps the rest of the row.
        kept = [row for row in document.get("fixtures", [])
                if arguments.fixture and row["fixture"] not in arguments.fixture]
        if arguments.solvers:
            by_key = {(row["fixture"], row["target"], row["break_symmetry"]): row
                      for row in document.get("fixtures", [])}
            for row in fresh:
                old = by_key.get((row["fixture"], row["target"], row["break_symmetry"]))
                if old is not None:
                    old["solvers"].update(row["solvers"])
                    row["solvers"] = old["solvers"]
        document["fixtures"] = kept + fresh
    if arguments.only != "fixtures" and arguments.challenges:
        document["challenges"] = measure_challenges(arguments.challenges, solvers,
                                                    arguments.challenge_seeds,
                                                    arguments.control_seeds,
                                                    arguments.challenge_timeout, arguments.jobs,
                                                    arguments.cnf2xnf)
    # The shared block names the tree and the tools; each section that ran gets
    # its own arguments beside it, because the two halves are separate runs and
    # one block claiming both would misstate whichever ran with other numbers.
    document["provenance"] = provenance(build)
    document["provenance"]["solvers_run_directly"] = {
        name: portable([path]) for name, path in solvers.items() if name in FINDS_ONLY}
    arguments_run = {"jobs": arguments.jobs, "solvers": sorted(solvers),
                     "commit": document["provenance"]["commit"],
                     "finished": time.strftime("%Y-%m-%d %H:%M")}
    if arguments.only != "challenges":
        document["fixtures_provenance"] = {
            **arguments_run, "seeds": arguments.seeds, "control_seeds": arguments.control_seeds,
            "cap_seconds": arguments.timeout, "rerun_of": arguments.fixture}
    if arguments.only != "fixtures" and arguments.challenges:
        document["challenges_provenance"] = {
            **arguments_run, "seeds": arguments.challenge_seeds,
            "control_seeds": arguments.control_seeds, "cap_seconds": arguments.challenge_timeout,
            "cnf2xnf": portable([arguments.cnf2xnf]) if arguments.cnf2xnf else None}
    output.write_text(json.dumps(document, indent=2) + "\n")
    (HERE / "results.md").write_text(table(document))
    print(f"wrote {output.relative_to(ROOT)} and results.md")
    return 0


if __name__ == "__main__":
    sys.exit(main())
