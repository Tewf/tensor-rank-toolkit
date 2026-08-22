"""What happens when somebody presses Run, with no HTTP in it.

Kept apart from the HTTP surface so that the checks in `tests/` drive the same
code a browser drives. Two decisions live here and nowhere else.

**Every path a tool reads or writes is chosen on this side**, inside the run's
own directory, and never taken from the request.

**The command shown before a run and the command run are assembled once.** The
browser's preview is not a second implementation guessing at what will happen;
it is this function with the run directory not yet made. A preview that could
drift from the run would be worse than no preview, because the whole claim of
this interface is that the line under an answer is the line that produced it.
"""
import pathlib

import catalogue
import command_line
import limits
import repository
import registry
import runner
import worked_examples
import workspace

# A backstop, not a budget. Where a tool has a budget flag of its own the browser
# fills that flag in with this number, so the limit is named in the command; this
# one only ends a run that outlives it, and ending a run decides nothing.
DEFAULT_WALL_CLOCK_SECONDS = 120
LONGEST_WALL_CLOCK_SECONDS = 86_400

# What the preview stands in for while there is no run yet.
NOT_YET = "<this run>"


def _slices_in(text):
    """The slice count off the `shape` line, or None if there is not one.

    The same two words `map_box.js` restates under the box, read here so the
    warning and the reading cannot disagree. It judges nothing else:
    `formats/tensor_file.h` is the authority on whether a map is well formed and
    its refusal is what a run shows.
    """
    for raw in (text or "").split("\n"):
        line = raw.split("#")[0].strip()
        words = line.split()
        if len(words) >= 4 and words[0] == "shape":
            try:
                return int(words[1])
            except ValueError:
                return None
    return None


def _shape_warning(request, tool):
    """A tool that takes only so many slices, said before Run rather than after.

    `decide-rank-by-pencil` is the one tool with a shape it cannot take, and
    until this existed the console offered it for any map, let you press Run, and
    handed back the binary's refusal. The refusal was right and the press was
    wasted. `most_slices` is declared in `catalogue.py` beside the question, so
    this stays one rule rather than a name checked here.
    """
    most = tool.get("most_slices")
    if most is None:
        return []
    slices = _slices_in((request.get("input") or {}).get("text"))
    if slices is None or slices <= most:
        return []
    return [tool["name"] + " decides pencils, and this map says " + str(slices) +
            " slices. Two is the whole method: a pencil A + xB has a complete "
            "invariant and three slices do not, which is where deciding rank "
            "becomes NP-complete. decide-rank takes this map, and takes the "
            "pencil route itself on the maps where it applies."]


class Service:
    def __init__(self, build, runs_root, wall_clock_seconds=DEFAULT_WALL_CLOCK_SECONDS):
        self.build = pathlib.Path(build).resolve()
        self.runs_root = pathlib.Path(runs_root).resolve()
        self.runs_root.mkdir(parents=True, exist_ok=True)
        self.wall_clock_seconds = wall_clock_seconds
        self.registry = registry.Registry()

    def setup(self):
        """Everything the page needs to draw itself once."""
        return {
            "tools": catalogue.TOOLS,
            "examples": worked_examples.EXAMPLES,
            "fixtures": repository.fixtures(),
            "root": str(repository.ROOT),
            "build": str(self.build),
            "wall_clock_seconds": self.wall_clock_seconds,
            "longest_wall_clock_seconds": LONGEST_WALL_CLOCK_SECONDS,
            # What this run will be bounded by, before it is started rather than
            # inferred afterwards from how it went. Verbatim from `show-limits`:
            # `limits.py` parses none of it and decides none of it.
            "limits": limits.resolved(self.build),
        }

    def preview(self, request):
        """The line that pressing Run would produce, assembled by the same code."""
        plan = self._assemble(request, self.runs_root / NOT_YET, write=False)
        return {"command": plan["shown"], "reader": plan["reader"],
                "warnings": self._warnings(request, plan["tool"])}

    def _warnings(self, request, tool):
        """What is worth knowing before pressing Run, and nothing that is not.

        One thing, and it is the one `not-production-ready-yet.md` records. A
        solver started by `run_limits/child_process.h` is in a process group of
        its own, so stopping this run does not reach it; what ends it is the
        alarm it carries, set from the tool's own timeout. This interface fills
        that flag in with the console's wall clock, which closes the window, and
        clearing it or raising it above the clock reopens it. It used to reopen
        without saying so, which is the part that was fixable.
        """
        chosen = request.get("options") or {}
        seconds = self._wall_clock(request)
        said = _shape_warning(request, tool)
        for option in tool["options"]:
            if not option.get("carries_wall_clock"):
                continue
            given = str(chosen.get(option["flag"], "")).strip()
            if given.isdigit() and int(given) <= seconds:
                continue
            if given.isdigit():
                stated = (option["flag"] + " is " + given + " s, above this "
                          "console's " + str(seconds) + " s wall clock.")
            else:
                stated = (option["flag"] + " is not set here, so it is whatever "
                          "tunables.conf says; this console's wall clock is " +
                          str(seconds) + " s.")
            said.append(stated + " A solver this run starts carries an alarm set "
                        "from that flag, and stopping the run does not reach it: "
                        "it can hold a core until the alarm fires. Set the flag "
                        "at or below the wall clock and that window closes.")
        return said

    def start(self, request):
        # The tool is resolved before a directory is made, so a request naming
        # one that does not exist leaves nothing on the disk to clean up.
        tool = _named(request)
        directory = workspace.new_run_directory(self.runs_root, tool["name"])
        plan = self._assemble(request, directory, write=True)

        run = self.registry.add(lambda identifier: runner.Run(
            identifier, plan["tool"], plan["argv"], directory, plan["input_path"],
            self._wall_clock(request), repository.ROOT, plan["result_name"],
            plan["verdicts"]))
        run.shown_command = plan["shown"]
        run.reader = plan["reader"]
        run.start()
        return {"id": run.identifier, "command": plan["shown"],
                "reader": plan["reader"]}

    # ---- the one assembly ---------------------------------------------------

    def _assemble(self, request, directory, write):
        tool = _named(request)
        binary = repository.binary(self.build, tool["binary"])

        input_path, reader = self._input_for(tool, directory, request, write)
        stem = (input_path.stem if input_path is not None
                else _clean(request.get("name") or tool["name"]))

        chosen = dict(request.get("options") or {})
        self._place_output_paths(tool, chosen, directory, stem)
        argv = [binary] + command_line.build(tool["name"], chosen, input_path,
                                             request.get("mode"))

        result_name, shown = "stdout.log", repository.command_as_typed(argv)
        if tool.get("produces") == "tensor":
            # This tool's answer is a file, so stdout is that file and the line a
            # reader retypes has to say where it went.
            result_name = stem + ".tensor"
            shown += " > " + repository.command_as_typed([directory / result_name])
        return {"tool": tool, "argv": argv, "shown": shown, "reader": reader,
                "input_path": input_path, "result_name": result_name,
                "verdicts": _verdicts_for(tool, chosen)}

    def _input_for(self, tool, directory, request, write):
        """The map or the operator, as a path.

        An unmodified fixture is run where it already is, so the command under
        the answer is the same line the repository's own documents and
        `reproduce/questions.py` use, and the answer can be set beside a
        published one without a reader having to believe two paths hold the same
        bytes. Anything else is written out, because a map that was pasted has to
        exist somewhere before it can be reproduced.
        """
        if tool.get("input") in (None, "none"):
            return None, None
        given = request.get("input") or {}
        text, name = given.get("text", ""), given.get("name", "")

        fixture = given.get("fixture")
        if fixture:
            try:
                if repository.fixture_text(fixture) == text:
                    return repository.fixture_path(fixture), pathlib.Path(fixture).suffix
            except ValueError as why:
                raise command_line.Refused(str(why))

        if not write:
            if not text.strip():
                raise command_line.Refused(
                    "there is nothing in the box: paste a map, load a fixture, "
                    "or build one.")
            suffix = (".tensor" if tool["input"] == "tensor"
                      else workspace.operator_suffix(text))
            return directory / (_clean(name).rsplit(".", 1)[0] + suffix), suffix
        try:
            return workspace.write_input(directory, tool["input"], text, name)
        except ValueError as why:
            raise command_line.Refused(str(why))

    def _place_output_paths(self, tool, chosen, directory, stem):
        """Every file a tool is asked to write lands in this run's own directory.

        The request says whether to write one; it never says where. That is the
        difference between a flag and a filesystem, and the request is on the
        wrong side of it.
        """
        for option in tool["options"]:
            flag = option["flag"]
            if flag not in chosen or not chosen[flag]:
                continue
            if option["kind"] == "emit_operators":
                chosen[flag] = str(directory / stem)
            elif option["kind"] == "emit_file":
                chosen[flag] = str(directory / (stem + option["suffix"]))

    def _wall_clock(self, request):
        try:
            seconds = int(request.get("wall_clock_seconds") or self.wall_clock_seconds)
        except (TypeError, ValueError):
            raise command_line.Refused(
                "the wall clock expects a count of seconds, not '" +
                str(request.get("wall_clock_seconds")) + "'")
        return max(1, min(seconds, LONGEST_WALL_CLOCK_SECONDS))

    # ---- reading a run back -------------------------------------------------

    def card(self, identifier):
        run = self.registry.get(identifier)
        if run is None:
            return None
        card = run.snapshot(run.shown_command)
        card["reader"] = run.reader
        card["files"] = workspace.emitted_files(run.directory, self._own_files(run))
        card["directory"] = repository.command_as_typed([run.directory])
        return card

    def cards(self):
        """Every run this console has started, newest first.

        A page reloaded in the middle of a two hour search should find that
        search rather than an empty column: the run belongs to the server, not to
        the tab that started it.
        """
        with self.registry.lock:
            identifiers = list(self.registry.runs)
        return [self.card(identifier) for identifier in reversed(identifiers)]

    def stop(self, identifier):
        run = self.registry.get(identifier)
        if run is None:
            return None
        run.stop("you")
        return self.card(identifier)

    def emitted(self, identifier, name):
        """One file the run wrote, by the name it wrote it under. The name is
        matched against what is there rather than joined onto a path, so nothing
        outside the run's directory can be asked for."""
        run = self.registry.get(identifier)
        if run is None:
            raise ValueError("no such run")
        if name not in workspace.emitted_files(run.directory, self._own_files(run)):
            raise ValueError("no such file in that run: " + name)
        return (run.directory / name).read_bytes()

    def _own_files(self, run):
        """The three this interface made itself, which are not results."""
        keep = {"stdout.log", "stderr.log", run.result_name}
        if run.input_path is not None:
            keep.add(run.input_path.name)
        return keep


def _verdicts_for(tool, chosen):
    """What this run's exit codes mean, which a flag may change.

    `decide-rank-by-sat --emit-cnf` writes the question and stops, so its exit 0
    is a file that got written and not a decomposition that was found. A flag
    that changes the claim carries the replacement beside itself in the
    catalogue, and this is where the two are put together.
    """
    verdicts = dict(tool.get("verdicts", {}))
    for option in tool["options"]:
        if chosen.get(option["flag"]) and option.get("verdicts"):
            verdicts.update(option["verdicts"])
    return verdicts


def _named(request):
    """The tool the request asked for, refused by name when there is no such one."""
    tool = catalogue.BY_NAME.get(request.get("tool"))
    if tool is None:
        raise command_line.Refused("no such tool: " + str(request.get("tool")))
    return tool


def _clean(name):
    kept = "".join(letter if letter.isalnum() or letter in "._-" else "_"
                   for letter in name)
    return kept.strip("._") or "map"
