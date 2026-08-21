"""Where one run's files live, under a path a reader can retype.

A map that arrives pasted into a browser has to become a file before any of the
tools can read it, and the name it gets is not a detail. Two reasons.

The first is reproducibility: the command this interface shows is worth
something only if the file it names is still there afterwards, so the map is
written out under the repository rather than piped through a temporary that
disappears.

The second is that `sparsify-operator` picks its reader by the literal four
characters `.sms`, which `formats/plinopt_interoperability/four-false-failures.md`
records as a way to make that layer look broken when it is not: any other name
silently gets the dense reader. So the suffix is chosen here from what the file
actually contains, and the choice is reported to the caller rather than made
quietly.
"""
import datetime
import pathlib
import re

SMS_HEADER = re.compile(r"\A[0-9]+\s+[0-9]+\s+[MmIiRrPp]\s*\Z")


def new_run_directory(runs_root, tool_name):
    """One directory per run, named so the order they were made in is the order
    they sort in, and so a directory can be found from the result card."""
    stamp = datetime.datetime.now().strftime("%Y%m%d-%H%M%S-%f")
    directory = pathlib.Path(runs_root) / (stamp + "-" + tool_name)
    directory.mkdir(parents=True)
    return directory


def operator_suffix(text):
    """`.sms` or `.matrix`, decided by the first line that is neither blank nor a
    comment. Both formats skip `#`, and PLinOpt's shipped matrices put their
    comment before the header, so the scan cannot start at line one.

    This reads three fields of one line to choose a reader. It does not parse the
    file: `formats/sms_file.h` and `formats/dense_matrix_file.h` do that, and
    whichever one runs will refuse what it does not understand in its own words.
    """
    for line in text.splitlines():
        stripped = line.strip()
        if not stripped or stripped.startswith("#"):
            continue
        return ".sms" if SMS_HEADER.match(stripped) else ".matrix"
    return ".matrix"


def write_input(directory, kind, text, name_hint=""):
    """The map or the operator, written out under the name its reader expects.

    Returns the path and the suffix, so a caller can say which reader will run
    rather than leaving the user to find out from the result.
    """
    if not text.strip():
        raise ValueError("there is nothing in the box: paste a map, load a "
                         "fixture, or build one.")
    suffix = ".tensor" if kind == "tensor" else operator_suffix(text)
    stem = re.sub(r"[^A-Za-z0-9_.-]", "_", name_hint).strip("._") or "input"
    stem = stem.rsplit(".", 1)[0] if "." in stem else stem
    path = pathlib.Path(directory) / (stem + suffix)
    path.write_text(text if text.endswith("\n") else text + "\n", encoding="utf-8")
    return path, suffix


def emitted_files(directory, keep):
    """Everything the run left behind that is not its own input or its two logs,
    which is what a result card offers for download."""
    return sorted(path.name for path in pathlib.Path(directory).iterdir()
                  if path.is_file() and path.name not in keep)
