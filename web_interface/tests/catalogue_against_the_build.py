"""The console's catalogue against the build, at both depths and in both directions.

`catalogue.py` is a curated tool model: it offers twelve of the thirteen commands
and a chosen set of each one's flags. Curation and drift look identical from
inside the file, so what makes them different is written down here. A binary the
build produces is either offered or on `NOT_TOOLS` with its reason; a flag that
binary's own `--help` prints is either an option in the catalogue or on
`NOT_OFFERED` with its reason. Anything else is drift and fails loudly.

**The flags are read off the binaries rather than transcribed.** `--help` is run
and its output parsed, for the reason `infrastructure/cli/tests/check_argument_grammar.sh` gives
for asking the commands themselves: a table of flags kept beside a table of flags
agrees with itself whatever either one says about the tools. This costs twelve
processes that print a usage block and stop.

The depth is what was missing. The tool half of this check shipped on 2026-08-22
and held; underneath it twenty-one flags had reached the binaries without ever
reaching the catalogue, `sparsify-operator --field p` among them, and every check
here was green throughout.
"""
import os
import re
import subprocess

# Every flag a usage block prints, plus `-s`, which is the one the catalogue
# offers under its short spelling. Read off the whole block and not off the
# `usage:` lines, because three commands document flags only underneath them.
FLAG = re.compile(r"(?<![\w-])(--[a-z][a-z0-9-]*|-s)(?![\w-])")

# Binaries the build produces that are deliberately not offered, each with the
# reason, because a name on this list is a decision and not an oversight.
# `OPTIONS/one-question-per-command.md` is where the decisions are argued.
NOT_TOOLS = {
    # An instrument: its output is nanoseconds, and MEASURING.md's line is that
    # counts reproduce anywhere and timings do not. `measure-leaf` is its model
    # and is CUDA-only, so it is usually not in the build tree at all.
    "oracle_guided_search/price-canonical-route",
    "infrastructure/gpu_leaf/measure-leaf",
    # Retired into `curve-bounds --solvers`. It prints the line to type and
    # leaves as 2, so offering it here would offer a refusal.
    "integer_programme/list-solvers",
    # A tool, and on the list in OPTIONS/one-question-per-command.md, but it
    # reads three files at once and this console offers one: `repository.py`
    # hands a tool a single fixture name and `workspace.py` writes a single
    # typed map. Offering it with two of its three operands missing would be
    # worse than not offering it. A limit of the console, named here so that it
    # is a decision rather than a drift.
    "descent_search/operators-to-tensor",
    # An instrument that takes no map: it prints what this machine and this
    # working directory bound a run to. The console shows its output in the run
    # pane instead of offering it as a tool, because there is no question to ask
    # it and nothing to choose.
    "infrastructure/run_limits/show-limits",
}

# Two flags every usage block prints that no tool's panel carries, for reasons
# that are the same on all twelve and are therefore written once.
EVERY_COMMAND = {
    # It prints the usage and leaves as exit 2 (OPTIONS.md), so a console
    # offering it would be offering a refusal: the argument that took
    # `list-solvers` off the tool list above, one level down.
    "--help": "prints the usage and leaves as 2, so offering it offers a refusal",
    # `infrastructure/cli/symmetry_argument.h` reads one flag under two spellings. The
    # catalogue offers `-s` and `command_line.py` builds `-s`, so offering the
    # long one too would put one idea in the panel twice.
    "--symmetry": "the long spelling of -s, which is what the panel offers",
}

# Flags a binary takes that this console deliberately does not offer. One line
# each, and the line is the reason: a flag here is a decision about what a
# browser can ask, and a flag that is merely forgotten belongs in the catalogue.
NOT_OFFERED = {
    "decide-rank": {
        # The console has exactly one input slot and the map is in it:
        # `repository.py` hands a tool one fixture name and `workspace.py`
        # writes one typed map. That is the same limit that keeps
        # `operators-to-tensor` off the tool list above, and it is why the
        # writing half of the pair, `--plan-out`, is offered and this one is
        # not: a plan leaves on a card and is replayed at a terminal.
        "--plan-in": "names a second file to read, and this console offers one box",
    },
    "walk-scheme": {
        # OPTIONS/one-idea-several-spellings.md: an accepted older spelling of
        # `--flips`, which is offered. The same word means pipeline stages on
        # `minimise-rank`, so a panel carrying both would offer one idea twice
        # under the name that means something else next door.
        "--steps": "an accepted older spelling of --flips, which is offered",
    },
}


def findings(setup, build):
    """Every claim this file makes, as (what was asked, whether it held).

    Returned rather than reported, so that the one place a check prints is
    `check_web_interface.py`, which is where the count that the README quotes is
    kept. This file decides what is true and not how it is said.
    """
    return _the_tools(setup, build) + _the_flags(setup, build)


def _the_tools(setup, build):
    """The offered tools against the binaries that exist, in both directions.

    A count asserted as a literal is what let this drift: two commands shipped
    without ever reaching the catalogue and one stayed in it after it stopped
    being a command, and `len(tools) == 12` was true throughout. So the question
    asked here is the one that was actually wrong, does the list *correspond*,
    and the number falls out of it.
    """
    offered = {tool["binary"] for tool in setup["tools"]}
    missing = sorted(name for name in offered if not (build / name).is_file())

    built = {str(path.relative_to(build)) for path in build.glob("*/*")
             if path.is_file() and os.access(path, os.X_OK)}
    unoffered = sorted(built - offered - NOT_TOOLS)
    return [
        ("every tool offered is a binary that is there" + _naming(missing),
         not missing),
        ("and every command in the build tree is offered or named as not a tool" +
         _naming(unoffered), not unoffered),
    ]


def _the_flags(setup, build):
    """The offered flags against the flags the binaries print, in both directions.

    Four ways this can be wrong and all four are asked. A flag reached the binary
    and not the panel, which is the drift this was written for. A flag left the
    binary and stayed in the panel, which builds a line the tool refuses. An
    exclusion outlived the flag it excused, which is a reason with nothing left
    to be the reason for. And an exclusion carries no reason at all, which is how
    a failing check gets silenced in one line by the person it was warning.
    """
    astray, vanished, stale, unexplained = [], [], [], []
    for tool in setup["tools"]:
        binary = build / tool["binary"]
        if not binary.is_file():
            continue                  # already a failure above, and once is enough
        printed = _flags_printed_by(binary)
        offered = ({option["flag"] for option in tool["options"]} |
                   {mode["flag"] for mode in tool.get("modes", [])})
        excused = dict(EVERY_COMMAND, **NOT_OFFERED.get(tool["name"], {}))

        astray += _named(tool, printed - offered - set(excused))
        vanished += _named(tool, offered - printed)
        # Staleness is asked of the per-tool list only. `--symmetry` is excused
        # on all twelve and printed by seven, and an exclusion that holds for a
        # command which never had the flag is unused rather than stale.
        stale += _named(tool, set(excused) - printed - set(EVERY_COMMAND))
        unexplained += _named(tool, {flag for flag, why in excused.items()
                                     if not str(why).strip()})
    return [
        ("every flag a binary's --help prints is offered or named as not offered" +
         _naming(astray), not astray),
        ("and every flag offered is one that binary still takes" +
         _naming(vanished), not vanished),
        ("and nothing is excused that the binary stopped printing" +
         _naming(stale), not stale),
        ("and each flag left off says in one line why" + _naming(unexplained),
         not unexplained),
    ]


def _flags_printed_by(binary):
    """The flags one command's own usage block names.

    `--help` leaves as exit 2 and writes to stderr, both asserted for every
    command in `infrastructure/cli/tests/check_argument_grammar.sh`, so neither is re-asserted
    here and stdout is not read.
    """
    said = subprocess.run([str(binary), "--help"], capture_output=True,
                          text=True, timeout=30).stderr
    return set(FLAG.findall(said))


def _named(tool, flags):
    return [tool["name"] + " " + flag for flag in sorted(flags)]


def _naming(wrong):
    return " (" + ", ".join(wrong) + ")" if wrong else ""
