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
    reproduce/measure.py --build build --check --slow  # and the 6.6 minute exhaustion
    reproduce/measure.py --build build --counts        # rewrite counts, keep every timing

`--check` is what CI runs. It re-derives every *count* and compares, and it does
not look at timings at all, because a shared cloud runner cannot reproduce a
timing and pretending otherwise is how the last wrong number got in. Timings are
rewritten only by a full run on a quiet machine, under the protocol in
MEASURING.md.

`--counts` is the rewrite a machine that is not quiet is entitled to make. It
refreshes the counts and the invocations that produced them and leaves every
`seconds` field exactly as it was measured, because MEASURING.md's own division
says a count is a fact about the problem and reproduces anywhere while a timing
is a fact about a machine on an afternoon. It stamps a `counts_provenance` block
beside the committed `provenance` rather than over it, so neither claims the
other's run. Without it, the only way to correct a drifted count is a rewrite
that overwrites protocol-grade timings with whatever the machine gives today.

Three files are covered, and every section of them that publishes a count:
`fixtures`, and the `exact_search` and `famous_tensors` blocks of the descent's
file, which `--check` did not look at until it was caught not looking. Thirty-odd
counts lived there unchecked, and seven had gone stale, one of them during the
session that improved the bound behind it. What each section is and what names its
rows is `SECTIONS`, in `questions.py`; a section missing from it goes unnoticed.

One question is not asked by default, and it is priced rather than hidden: the
GF(16) exhaustion visits 105 600 301 nodes and costs 6.6 minutes of one core,
which is longer than everything else here together, so the default run leaves it
out and `--slow` asks it. The flag it needs is written down in the
`exhaustive_command` of the row it belongs to, and this reads that field back
rather than rebuilding the line, because a line rebuilt here would carry the
default node limit and answer a different question. Two further figures are
carried rather than asked, and `questions.CARRIED` names them and their price.
Every run,
`--check` included, prints a SKIPPED line saying what it left out and why. A
number the driver does not cover must never read as one it covered and agreed
with, which is the failure these two files exist to stop.
"""
import argparse
import json
import shutil
import subprocess
import sys

# What each published number is, and how to ask for it: `questions.py`. The
# import is flat because the two files sit in one directory and Python puts a
# script's own directory first on the path, so `reproduce/measure.py` runs from
# anywhere without a package, an installer or a `sys.path` line.
from questions import (CARRIED, FIXTURES, OPERATORS, PLATEAU_QUESTIONS, REPEATS, ROOT,
                       SAT_QUESTIONS, SECTIONS, WALK_QUESTIONS, descent_of, exact_search_of,
                       famous_tensors_of, plateau_of, satisfiability_of, shown, skipped_fields,
                       sparsification_of, walk_of)


def version_in(text):
    """The version inside a tool's answer to `--version`, or None if it gave none.

    `--version` is a convention and not a guarantee. drat-trim has no such flag
    and prints its usage instead; cbc prints a welcome banner and puts the
    version on the line after it. Taking the first line either way is how
    `usage: drat-trim [INPUT] [<PROOF>] ...` came to sit in a field labelled
    version, in three committed provenance blocks, for as long as this function
    guessed.

    So a version has to look like one: it carries a digit, it does not open with
    `usage`, and past the first line it has to say `version` as well, which stops
    a copyright year from being read as a release. What this rejects is recorded
    as the marker below, never as a number nobody printed.
    """
    for index, line in enumerate(text.strip().splitlines()[:5]):
        line = line.strip()
        if not any(character.isdigit() for character in line):
            continue
        if line.lower().startswith("usage"):
            continue
        if index == 0 or "version" in line.lower():
            return line
    return None


# What is recorded for an installed tool that prints no version anywhere. It
# names the binary that answered, because that is the fact still worth having:
# a reader who needs to know which drat-trim this was can look at that path.
NO_VERSION = "reports no version; binary at"


def version_of(binary, *flags):
    """A version string, the marker for a tool that prints none, or None when it
    is not installed."""
    path = shutil.which(binary)
    if path is None:
        return None
    for flag in flags or ("--version",):
        try:
            done = subprocess.run([binary, flag], capture_output=True, text=True, timeout=10)
        except (OSError, subprocess.SubprocessError):
            continue
        found = version_in(done.stdout + done.stderr)
        if found:
            return found
    return f"{NO_VERSION} {path}"


def provenance(build, counts=False):
    """Everything a reader needs to tell why their number differs from mine.

    `counts` writes the block a `--counts` run is entitled to: it names the code
    that produced the counts and says, in `covers`, that the timings in the file
    are older than it and were not touched. It is written beside the committed
    `provenance` rather than over it, because that one describes the timings and
    is still true of them.
    """
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
    covers = {} if not counts else {"covers":
        "The counts in this file and the invocations that produced them, and nothing else. "
        "Every `seconds` field here is older than this block and was not touched: a count is "
        "exact arithmetic and comes out the same on any machine, a timing does not, so "
        "`--counts` refreshes the first and leaves the second exactly as measured. What "
        "produced the timings is the `provenance` block."}
    return {
        "commit": git("rev-parse", "HEAD"),
        "tree_clean": dirty == "" if dirty is not None else None,
        **covers,
        "compiler": version_of("c++"),
        "givaro": givaro,
        "solvers": {
            name: version_of(name)
            for name in ("kissat", "cryptominisat5", "cadical", "cvc5",
                         "drat-trim", "cbc", "glpsol")
        },
        "build_directory": shown([str(build)]),
        "repeats": 1 if counts else REPEATS,
        "protocol": ("counts only: exact arithmetic, one run each, the machine irrelevant."
                     if counts else
                     "one core, fastest of three, quiet machine. See MEASURING.md."),
        "defaults_in_force": {
            "threads": 1,
            "max_memory_bytes": 2 << 30,
            "node_limit": 5_000_000,
            "solver_timeout_seconds": 300,
        },
    }


def counts_only(block):
    """Everything but the timings and the invocations, which is what CI can check.

    A timing is any key with `seconds` in its name, which takes the second
    opinions from the other solver with it; an invocation is any key mentioning a
    command. A shared runner reproduces neither: not the wall clock, and not the
    directory a command is spelt with on the machine that ran it.
    """
    if isinstance(block, dict):
        return {k: counts_only(v) for k, v in block.items()
                if "provenance" not in k and "seconds" not in k and "command" not in k}
    if isinstance(block, list):
        return [counts_only(item) for item in block]
    return block


def rows_of(document):
    """(section, name, row) for every row of every section in SECTIONS."""
    for path, key in SECTIONS:
        block = document
        for step in path:
            block = block.get(step) if isinstance(block, dict) else None
        for row in block or ():
            yield "/".join(path), row.get(key), row


def with_timings_kept(committed, measured, timing=False):
    """`measured`, with every timing it measured replaced by the committed one.

    MEASURING.md draws this line and this walks it. A count is a fact about the
    problem, computed in exact arithmetic, and comes out the same on any machine,
    so a busy machine is no obstacle to refreshing one. A timing is a fact about
    a machine on an afternoon. `--counts` therefore rewrites the counts and the
    invocations that produced them and leaves every `seconds` field exactly as it
    was measured, rather than overwriting a figure taken under the protocol with
    one taken beside three other agents' searches.

    A measured timing of `None` is written rather than kept, because that is not a
    slower reading of the same question. It says the question no longer opens a
    node, so there is no wall clock at all, which is as machine-independent as the
    zero beside it and would be a lie to leave at 77.4 s.
    """
    if isinstance(measured, dict):
        merged = dict(committed) if isinstance(committed, dict) else {}
        for key, value in measured.items():
            merged[key] = with_timings_kept(merged.get(key), value, "seconds" in key)
        return merged
    if isinstance(measured, list) and isinstance(committed, list) \
            and len(measured) == len(committed):
        return [with_timings_kept(old, new, timing)
                for old, new in zip(committed, measured)]
    if timing and measured is not None and committed is not None:
        return committed
    return measured


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--build", default="build", help="the build directory")
    parser.add_argument("--check", action="store_true",
                        help="compare counts against the committed files and exit 1 on drift")
    parser.add_argument("--slow", action="store_true",
                        help="also ask the questions priced in exhaustive_costs, "
                             "which is 6.6 minutes of one core for the GF(16) exhaustion")
    parser.add_argument("--counts", action="store_true",
                        help="rewrite the counts and the invocations that produced them, and "
                             "leave every timing exactly as it was measured. This is the "
                             "rewrite a machine that is not quiet is entitled to make")
    arguments = parser.parse_args()

    build = (ROOT / arguments.build).resolve()
    if not build.is_dir():
        sys.exit(f"no build directory at {build}; run cmake first")

    # One run per question under --check and under --counts, three under a full
    # rewrite. The fastest of three is what a *timing* needs, and neither of those
    # two writes one: the commands are deterministic, so the second and third runs
    # would compare a count against itself at the price of tripling the sweep.
    repeats = REPEATS if not (arguments.check or arguments.counts) else 1

    descent = ROOT / "descent_search" / "results.json"
    sparsification = ROOT / "matrix_sparsification" / "results.json"
    satisfiability = ROOT / "satisfiability" / "results.json"
    flip = ROOT / "flip_graph" / "results.json"
    committed = {path: json.loads(path.read_text()) if path.exists() else {}
                 for path in (descent, sparsification, satisfiability, flip)}
    sat_rows = {row["name"]: row for row in committed[satisfiability].get("fixtures", [])}

    # Said before anything is measured, so that a reader of the output knows what
    # the run below is not going to tell them.
    skipped = [(str(satisfiability.relative_to(ROOT)), question["name"], field, why)
               for question in SAT_QUESTIONS
               for field, why in skipped_fields(question,
                                                sat_rows.get(question["name"], {}),
                                                arguments.slow).items()]
    skipped += list(CARRIED)
    for where, name, field, why in skipped:
        print(f"SKIPPED  {where}: {name} {field}, carried from the committed row. {why}")

    measured = {
        descent: {"fixtures": [descent_of(build, name, repeats) for name in FIXTURES],
                  "exact_search": exact_search_of(
                      build, committed[descent].get("exact_search", {}), repeats),
                  "famous_tensors": famous_tensors_of(
                      build, committed[descent].get("famous_tensors", {}), repeats)},
        sparsification:
            {"fixtures": [sparsification_of(build, name, repeats) for name in OPERATORS]},
        satisfiability:
            {"fixtures": [satisfiability_of(build, question,
                                            sat_rows.get(question["name"], {}), repeats,
                                            arguments.slow)
                          for question in SAT_QUESTIONS]},
        flip: {"plateau": [plateau_of(build, question, repeats)
                           for question in PLATEAU_QUESTIONS],
               "walk": [walk_of(build, question, repeats) for question in WALK_QUESTIONS]},
    }

    drifted = False
    for path, fresh in measured.items():
        existing = committed[path]
        if arguments.check:
            old_rows = {(section, name): row for section, name, row in rows_of(existing)}
            for section, name, new_row in rows_of(fresh):
                old_row = old_rows.get((section, name))
                where = f"{path.relative_to(ROOT)}: {section} {name}"
                if old_row is None:
                    print(f"NEW      {where}")
                    drifted = True
                elif counts_only(old_row) != counts_only(new_row):
                    print(f"DRIFTED  {where}")
                    print(f"  committed {counts_only(old_row)}")
                    print(f"  measured  {counts_only(new_row)}")
                    drifted = True
            continue

        if arguments.counts:
            existing = with_timings_kept(existing, fresh)
            existing["counts_provenance"] = provenance(build, counts=True)
        else:
            existing.update(fresh)
            existing["provenance"] = provenance(build)
            # A full rewrite covers what --counts covered and the timings besides,
            # so the narrower block it left behind has nothing left to say.
            existing.pop("counts_provenance", None)
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
