"""Where the repository, the built binaries and the fixtures are.

Everything this interface runs is a binary that already exists, so the only
questions here are which directory it was built into and how the line that
started it is written down. Both matter for the same reason: a result is worth
something to a reader only if the reader can retype the command that produced it.

The command is rendered by `reproduce/questions.py`, which already had to solve
this for the published numbers, and it is imported rather than reimplemented so
that the two never drift. Anything inside the repository comes out relative to
it and a solver found on PATH keeps its absolute path, because that one really
is a fact about the machine.
"""
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent

# `reproduce/` is imported flat, the way its own two files import each other, so
# no package, no installer and no `__init__.py` anywhere.
sys.path.insert(0, str(ROOT / "reproduce"))
from questions import shown  # noqa: E402  (the path has to be set first)


class BuildNotFound(Exception):
    """The build directory has no binaries in it, which is not this layer's fault.

    Raised rather than reported, because there is exactly one cure and the
    caller is the only place that can print it: build the toolkit first.
    """


def binary(build, relative_path):
    """The built tool, as an absolute path, checked to exist before it is run."""
    path = pathlib.Path(build).resolve() / relative_path
    if not path.is_file():
        raise BuildNotFound(
            "no binary at " + str(path) + ". Build the toolkit first:\n"
            "    cmake -B build -G Ninja && cmake --build build")
    return path


def command_as_typed(argv):
    """The argv as a line to retype, relative to the repository where it can be.

    The child runs with the repository root as its working directory, which is
    also what makes `tunables.conf` reach it, so this line is literally what you
    type after cloning and building.
    """
    return shown([str(word) for word in argv])


def fixtures():
    """The maps and operators shipped here, so nobody has to type one to start.

    Grouped by what reads them: a tensor file is a bilinear map and ten of the
    twelve tools take one, a matrix or SMS file is a single operator and
    `sparsify-operator` takes that.
    """
    tensors = sorted(path.name for path in (ROOT / "fixtures").glob("*.tensor"))
    operators = sorted(path.name for path in (ROOT / "fixtures").glob("*.matrix"))
    plinopt = sorted("plinopt/" + path.name
                     for path in (ROOT / "fixtures" / "plinopt").glob("*.sms"))
    return {"tensor": tensors, "operator": operators + plinopt}


def fixture_path(name):
    """One fixture's path, refused unless it is inside `fixtures/`.

    The name arrives over HTTP, so it is resolved and then checked to be under
    the fixtures directory. A path that climbs out of it is refused rather than
    normalised into something else, because a reader of this function should not
    have to decide which of the two it does.
    """
    directory = (ROOT / "fixtures").resolve()
    path = (directory / name).resolve()
    if directory not in path.parents or not path.is_file():
        raise ValueError("no such fixture: " + name)
    return path


def fixture_text(name):
    """One fixture's contents, for the box in the browser."""
    return fixture_path(name).read_text(encoding="utf-8")
