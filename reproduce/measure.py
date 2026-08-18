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

# Fastest of this many, because wall clock is noisy upward and not downward: a
# slow run means the machine was busy, and averaging that in measures the
# machine rather than the code.
REPEATS = 3

FIXTURES = ["f2_5x5", "f2_3x8", "f2_4x7", "f3_3x6"]
OPERATORS = ["strassen_u", "strassen_v", "alternative_basis_u"]


def output_of(command):
    """Run and return stdout+stderr, refusing to guess when a command fails."""
    done = subprocess.run(command, capture_output=True, text=True)
    if done.returncode != 0:
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
        "build_directory": str(build),
        "repeats": REPEATS,
        "protocol": "one core, fastest of three, quiet machine. See MEASURING.md.",
        "defaults_in_force": {
            "threads": 1,
            "max_memory_bytes": 2 << 30,
            "node_limit": 5_000_000,
            "solver_timeout_seconds": 300,
        },
    }


def fastest(command, repeats=REPEATS):
    """(text of the fastest run, its seconds). Every run must agree in output."""
    best_seconds = None
    best_text = None
    for _ in range(repeats):
        started = time.perf_counter()
        text = output_of(command)
        seconds = time.perf_counter() - started
        if best_seconds is None or seconds < best_seconds:
            best_seconds, best_text = seconds, text
    return best_text, round(best_seconds, 4)


def descent_of(build, name):
    """One fixture through minimise-rank, per step."""
    command = [str(build / "descent_search" / "minimise-rank"),
               str(ROOT / "fixtures" / f"{name}.tensor"), "--steps", "3"]
    text, seconds = fastest(command)

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
            "command": " ".join(command),
            **steps}


def sparsification_of(build, name):
    """One operator through sparsify-operator, per method."""
    command = [str(build / "matrix_sparsification" / "sparsify-operator"),
               str(ROOT / "fixtures" / f"{name}.matrix")]
    text, seconds = fastest(command)

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
            "command": " ".join(command),
            "seconds": seconds,
            **counts}


def counts_only(block):
    """Everything but the timings, which is the part CI can check."""
    if isinstance(block, dict):
        return {k: counts_only(v) for k, v in block.items()
                if k not in ("seconds", "provenance", "command")}
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

    measured = {
        ROOT / "descent_search" / "results.json":
            {"fixtures": [descent_of(build, name) for name in FIXTURES]},
        ROOT / "matrix_sparsification" / "results.json":
            {"fixtures": [sparsification_of(build, name) for name in OPERATORS]},
    }

    drifted = False
    for path, fresh in measured.items():
        existing = json.loads(path.read_text()) if path.exists() else {}
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
        print("every published count still reproduces")
    return 0


if __name__ == "__main__":
    sys.exit(main())
