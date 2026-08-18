#!/usr/bin/env python3
"""Regenerate every published number, and stamp what produced it.

The results files this writes used to be maintained by hand. That is how a
figure once entered them that no run had produced: a node count and a wall clock
were transcribed into a table headed "Measured", and survived in five documents
until somebody went looking for the command. There was no command.

So the rule here is that a published number and the invocation that produced it
are written by the same process, in the same pass, or neither is written. Every
file this emits carries a `provenance` block naming the commit, the compiler, the
Givaro version, the solvers found on PATH, and the flags actually passed. A
reader who disagrees with a number can rerun exactly what made it.

    reproduce/measure.py --build build                 # rewrite the results files
    reproduce/measure.py --build build --check         # counts only, exit 1 on drift

`--check` is what CI runs. It re-derives every *count* and compares, and it does
not look at timings at all, because a shared cloud runner cannot reproduce a
timing and pretending otherwise is how the last wrong number got in. Timings are
rewritten only by a full run on a quiet machine, under the protocol in
MEASURING.md.

Three files are covered: the descent, the sparsification and the satisfiability
strand. One figure inside the third is not, and that is stated rather than
arranged: the flags behind it were never written down, `satisfiability/results.json`
says so beside it in a `_not_reproducible` key, and every run of this prints a
SKIPPED line naming it. A number the driver does not cover must never read as one
it covered and agreed with, which is the failure this whole file exists to stop.
"""
import argparse
import json
import pathlib
import re
import shutil
import subprocess
import sys
import time

ROOT = pathlib.Path(__file__).resolve().parent.parent


def shown(command):
    """A command line as a reader on another machine can run it.

    The commands recorded here used to be absolute, which made every one of
    them a fact about this laptop: they carried a home directory, a username
    and a directory layout, none of which a reader has and none of which the
    numbers depend on. Anything under the repository is rendered relative to
    it, so the recorded line is what you type after cloning; a solver found on
    PATH keeps its absolute path, because that one really is a fact about the
    machine and pretending otherwise would hide which binary answered.
    """
    words = []
    for word in command:
        try:
            words.append(str(pathlib.Path(word).resolve().relative_to(ROOT)))
        except ValueError:
            words.append(word)
    return " ".join(words)

# Fastest of this many, because wall clock is noisy upward and not downward: a
# slow run means the machine was busy, and averaging that in measures the
# machine rather than the code.
REPEATS = 3

FIXTURES = ["f2_5x5", "f2_3x8", "f2_4x7", "f3_3x6"]
OPERATORS = ["strassen_u", "strassen_v", "alternative_basis_u"]

# The satisfiability strand's published questions, written as invocations rather
# than as prose: the fixture, the k a decomposition is found at, the k that is
# refused, and whether the tree search was asked that same refusal. A fixture
# with neither target has no published answer, so nothing is run for it and its
# row is carried whole.
SAT_QUESTIONS = [
    {"name": "f2_2x2", "found_at": 3, "ruled_out_at": 2},
    {"name": "f2_2x3", "found_at": 5, "ruled_out_at": 4},
    {"name": "gf4_multiplication", "found_at": 3, "ruled_out_at": 2},
    {"name": "gf8_multiplication", "found_at": 6, "ruled_out_at": 5},
    {"name": "w_state", "found_at": 3, "ruled_out_at": 2},
    {"name": "matmul_2x2x2", "found_at": 7, "ruled_out_at": 6, "exhaustive": True},
    {"name": "matmul_2x2x3", "ruled_out_at": 8, "exhaustive": True},
    {"name": "gf16_multiplication", "found_at": 9, "ruled_out_at": 8, "exhaustive": True},
    {"name": "f2_5x5"},
]

# The flags every published satisfiability number was taken under. They are here
# once, and `results.json` names them in its `source`, because a figure taken
# under other flags is a figure of something else.
SAT_FLAGS = ["--break-symmetry", "--plain-cnf"]

# The suffix a results file marks an orphaned figure with. `<field>_not_reproducible`
# holds the reason, and the field it names is carried rather than re-derived.
NOT_REPRODUCIBLE = "_not_reproducible"


def output_of(command, expect=(0,)):
    """Run and return stdout+stderr, refusing to guess when a command fails.

    `expect` is the exit codes the question may answer with, because here a
    refusal is an answer and not a failure: both deciders exit 0 for "here is a
    decomposition" and 1 for "there is none". Anything else, a spent budget or a
    misread file, is still a failure and still stops the run.
    """
    done = subprocess.run(command, capture_output=True, text=True)
    if done.returncode not in expect:
        raise RuntimeError(f"{' '.join(command)} exited {done.returncode}\n{done.stderr}")
    return done.stdout + done.stderr


def version_of(binary, *flags):
    """A version string, or None when the tool is not installed."""
    path = shutil.which(binary)
    if path is None:
        return None
    for flag in flags or ("--version",):
        try:
            done = subprocess.run([binary, flag], capture_output=True, text=True, timeout=10)
            first = (done.stdout + done.stderr).strip().splitlines()
            if first:
                return first[0].strip()
        except (OSError, subprocess.SubprocessError):
            continue
    return path


def provenance(build):
    """Everything a reader needs to tell why their number differs from mine."""
    def git(*args):
        try:
            return subprocess.run(["git", *args], cwd=ROOT, capture_output=True,
                                  text=True).stdout.strip() or None
        except OSError:
            return None

    givaro = None
    if shutil.which("pkg-config"):
        givaro = subprocess.run(["pkg-config", "--modversion", "givaro"],
                                capture_output=True, text=True).stdout.strip() or None

    dirty = git("status", "--porcelain")
    return {
        "commit": git("rev-parse", "HEAD"),
        "tree_clean": dirty == "" if dirty is not None else None,
        "compiler": version_of("c++"),
        "givaro": givaro,
        "solvers": {
            name: version_of(name)
            for name in ("kissat", "cryptominisat5", "cadical", "cvc5",
                         "drat-trim", "cbc", "glpsol")
        },
        "build_directory": shown([str(build)]),
        "repeats": REPEATS,
        "protocol": "one core, fastest of three, quiet machine. See MEASURING.md.",
        "defaults_in_force": {
            "threads": 1,
            "max_memory_bytes": 2 << 30,
            "node_limit": 5_000_000,
            "solver_timeout_seconds": 300,
        },
    }


def fastest(command, repeats=REPEATS, expect=(0,)):
    """(text of the fastest run, its seconds). Every run must agree in output."""
    best_seconds = None
    best_text = None
    for _ in range(repeats):
        started = time.perf_counter()
        text = output_of(command, expect)
        seconds = time.perf_counter() - started
        if best_seconds is None or seconds < best_seconds:
            best_seconds, best_text = seconds, text
    return best_text, round(best_seconds, 4)


def descent_of(build, name, repeats=REPEATS):
    """One fixture through minimise-rank, per step."""
    command = [str(build / "descent_search" / "minimise-rank"),
               str(ROOT / "fixtures" / f"{name}.tensor"), "--steps", "3"]
    text, seconds = fastest(command, repeats)

    steps = {}
    for step, label in ((1, "step_1"), (2, "step_2"), (3, "step_3")):
        found = re.search(rf"step {step}: (\d+) multiplications.*?([\d.e+-]+) s cumulative", text)
        if not found:
            raise RuntimeError(f"{name}: no 'step {step}' line in\n{text}")
        steps[label] = {"multiplications": int(found.group(1)),
                        "seconds": round(float(found.group(2)), 4)}
    naive = re.search(r"naive: (\d+) multiplications", text)
    return {"name": name,
            "naive": int(naive.group(1)) if naive else None,
            "command": shown(command),
            **steps}


def sparsification_of(build, name, repeats=REPEATS):
    """One operator through sparsify-operator, per method."""
    command = [str(build / "matrix_sparsification" / "sparsify-operator"),
               str(ROOT / "fixtures" / f"{name}.matrix")]
    text, seconds = fastest(command, repeats)

    counts = {}
    for key, pattern in (
            ("as_given", r"as given: (\d+) nonzeros"),
            ("row_basis_heuristic", r"row-basis heuristic: (\d+) nonzeros"),
            ("oracle_bottom_up", r"exact oracle, bottom-up: (\d+) nonzeros"),
            ("oracle_top_down", r"exact oracle, top-down: (\d+) nonzeros")):
        found = re.search(pattern, text)
        if not found:
            raise RuntimeError(f"{name}: no '{key}' line in\n{text}")
        counts[key] = int(found.group(1))

    shape = re.search(r"as given: \d+ nonzeros, (\d+x\d+)", text)
    return {"name": name,
            "shape": shape.group(1) if shape else None,
            "command": shown(command),
            "seconds": seconds,
            **counts}


def unreproducible(row):
    """`{field: why}` for the figures in a committed row this refuses to re-derive.

    A number whose flags were never written down cannot be reproduced by anybody,
    so the results file states that beside the number itself, in a
    `<field>_not_reproducible` key, and this reads the file rather than keeping a
    second list of exceptions in code. The caller prints one line per entry on
    every run, `--check` included.
    """
    return {key[:-len(NOT_REPRODUCIBLE)]: why for key, why in row.items()
            if key.endswith(NOT_REPRODUCIBLE)}


def satisfiability_of(build, question, committed, repeats=REPEATS):
    """One fixture's published questions, re-asked of the solver and of the tree.

    What this does not measure is carried from the committed row untouched: the
    field, the rank the literature gives, the prose, and the second opinions from
    the other solver are not measurements this makes, and restating them here
    would put them in two files at once.
    """
    name = question["name"]
    tensor = str(ROOT / "fixtures" / f"{name}.tensor")
    solver = str(build / "satisfiability" / "decide-rank-by-sat")
    row = dict(committed)

    def solved(target, expect, wanted):
        command = [solver, tensor, "--target", str(target), *SAT_FLAGS]
        text, seconds = fastest(command, repeats, expect=(expect,))
        if wanted not in text:
            raise RuntimeError(f"{name}: k={target} no longer answers {wanted!r}\n{text}")
        return shown(command), seconds

    if "found_at" in question:
        target = question["found_at"]
        command, seconds = solved(target, 0, "FOUND a decomposition")
        row.update({"found_at": target, "found_seconds": seconds,
                    "found_command": command})

    if "ruled_out_at" in question:
        target = question["ruled_out_at"]
        command, seconds = solved(target, 1, "NO, rank is more than")
        row.update({"ruled_out_at": target, "ruled_out_seconds": seconds,
                    "ruled_out_command": command})

    if question.get("exhaustive") and "exhaustive_nodes" not in unreproducible(committed):
        target = question["ruled_out_at"]
        command = [str(build / "exhaustive_search" / "decide-rank"), tensor,
                   "--target", str(target)]
        text, seconds = fastest(command, repeats, expect=(1,))
        # No node line at all means the polynomial lower bound refused the target
        # before the search opened a node. That is a count of zero and a question
        # with no search time to report, not a measurement that went missing.
        visited = re.search(r"(\d+) nodes in", text)
        row.update({"exhaustive_nodes": int(visited.group(1)) if visited else 0,
                    "exhaustive_ruled_out_seconds": seconds if visited else None,
                    "exhaustive_command": shown(command)})
    return row


def counts_only(block):
    """Everything but the timings and the invocations, which is what CI can check.

    A timing is any key with `seconds` in its name, which takes the second
    opinions from the other solver with it; an invocation is any key named
    `command` or ending in `_command`. A shared runner reproduces neither: not
    the wall clock, and not the absolute paths a command is spelt with on the
    machine that ran it.
    """
    if isinstance(block, dict):
        return {k: counts_only(v) for k, v in block.items()
                if k != "provenance" and "seconds" not in k
                and not k.endswith("command")}
    if isinstance(block, list):
        return [counts_only(item) for item in block]
    return block


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--build", default="build", help="the build directory")
    parser.add_argument("--check", action="store_true",
                        help="compare counts against the committed files and exit 1 on drift")
    arguments = parser.parse_args()

    build = (ROOT / arguments.build).resolve()
    if not build.is_dir():
        sys.exit(f"no build directory at {build}; run cmake first")

    # One run per question under --check, three under a rewrite. The fastest of
    # three is what a *timing* needs, and --check reads no timing: the commands
    # are deterministic, so the second and third runs would compare a count
    # against itself, at the price of tripling the two-minute solver sweep.
    repeats = 1 if arguments.check else REPEATS

    descent = ROOT / "descent_search" / "results.json"
    sparsification = ROOT / "matrix_sparsification" / "results.json"
    satisfiability = ROOT / "satisfiability" / "results.json"
    committed = {path: json.loads(path.read_text()) if path.exists() else {}
                 for path in (descent, sparsification, satisfiability)}
    sat_rows = {row["name"]: row for row in committed[satisfiability].get("fixtures", [])}

    # Said before anything is measured, so that a reader of the output knows what
    # the run below is not going to tell them.
    skipped = [(question["name"], field, why) for question in SAT_QUESTIONS
               for field, why in unreproducible(sat_rows.get(question["name"], {})).items()]
    for name, field, why in skipped:
        print(f"SKIPPED  {satisfiability.name}: {name} {field}, which this driver "
              f"does not cover. {why}")

    measured = {
        descent: {"fixtures": [descent_of(build, name, repeats) for name in FIXTURES]},
        sparsification:
            {"fixtures": [sparsification_of(build, name, repeats) for name in OPERATORS]},
        satisfiability:
            {"fixtures": [satisfiability_of(build, question,
                                            sat_rows.get(question["name"], {}), repeats)
                          for question in SAT_QUESTIONS]},
    }

    drifted = False
    for path, fresh in measured.items():
        existing = committed[path]
        if arguments.check:
            for new_row in fresh["fixtures"]:
                old_row = next((r for r in existing.get("fixtures", [])
                                if r.get("name") == new_row["name"]), None)
                if old_row is None:
                    print(f"NEW      {path.name}: {new_row['name']}")
                    drifted = True
                elif counts_only(old_row) != counts_only(new_row):
                    print(f"DRIFTED  {path.name}: {new_row['name']}")
                    print(f"  committed {counts_only(old_row)}")
                    print(f"  measured  {counts_only(new_row)}")
                    drifted = True
            continue

        existing.update(fresh)
        existing["provenance"] = provenance(build)
        path.write_text(json.dumps(existing, indent=2) + "\n")
        print(f"wrote {path.relative_to(ROOT)}")

    if arguments.check:
        if drifted:
            sys.exit("a published count no longer matches what the code produces")
        print("every published count still reproduces" +
              (f", bar the {len(skipped)} printed SKIPPED above" if skipped else ""))
    return 0


if __name__ == "__main__":
    sys.exit(main())
