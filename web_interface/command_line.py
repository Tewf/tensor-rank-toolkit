"""Turning a chosen tool and the options a browser sent into an argv.

This is the only place a command line is built, and it builds one from the
catalogue and nothing else: a flag that is not declared for the tool being run
never reaches the argv, and no value is passed through unread. No shell is
involved anywhere, so there is no quoting to get wrong.

The refusals here are worded the way the toolkit's own are, naming the flag and
quoting the word, because `infrastructure/cli/arguments.h` established that grammar for exactly
this reason: `--target abc` reported as `stoull` names neither the flag nor the
word, and there are five numeric flags it could have been.

What this file deliberately does not do is decide whether a map is well formed.
The readers in `core/formats/` are the authority on that and their messages already
name what is wrong, so a malformed tensor reaches the binary and comes back as
that binary's own refusal rather than as a second opinion written here.
"""
import re

import catalogue

COUNT = re.compile(r"\A[0-9]+\Z")
WHOLE = re.compile(r"\A-?[0-9]+\Z")
MEMORY = re.compile(r"\A[0-9]+[KkMmGg]?\Z")       # infrastructure/cli/size_argument.h
SOLVER_NAME = re.compile(r"\A[A-Za-z0-9_.+-]{1,64}\Z")  # a name looked up on PATH
POINT = re.compile(r"\A[0-9]+:[0-9]+\Z")


class Refused(Exception):
    """A line this interface will not build, with the reason a person can act on."""


def _words_for(option, value):
    """One option's words, or none at all when it was left unset."""
    flag, kind = option["flag"], option["kind"]

    if kind == "switch":
        return [flag] if value else []
    if value is None or value == "":
        return []

    if kind == "count":
        if not COUNT.match(str(value)):
            raise Refused(flag + " expects a count, not '" + str(value) + "'")
        return [flag, str(value)]
    if kind == "whole":
        if not WHOLE.match(str(value)):
            raise Refused(flag + " expects a whole number, not '" + str(value) + "'")
        return [flag, str(value)]
    if kind == "memory":
        if not MEMORY.match(str(value)):
            raise Refused(flag + " expects a size such as 2G, not '" + str(value) + "'")
        return [flag, str(value)]
    if kind == "choice":
        if str(value) not in option["values"]:
            raise Refused(flag + " expects one of " + ", ".join(option["values"]) +
                          ", not '" + str(value) + "'")
        return [flag, str(value)]
    if kind == "text":
        if not SOLVER_NAME.match(str(value)):
            raise Refused(flag + " expects a solver name, not '" + str(value) + "'")
        return [flag, str(value)]
    if kind == "symmetry":
        return _symmetry_words(flag, value)
    if kind == "points":
        return _points_words(flag, value)
    if kind in ("emit_operators", "emit_file"):
        return [flag, str(value)]   # a path this interface chose, inside the run directory
    raise Refused("no such kind of option: " + kind)


def _symmetry_words(flag, value):
    """`-s none`, `-s auto` or `-s matmul <n> <m> <k>`, as `infrastructure/cli/symmetry_argument.h`
    reads them. The three dimensions are refused when one is missing rather than
    passed on short, because a short line there is read as a different shape."""
    mode = str(value.get("mode", "none")) if isinstance(value, dict) else str(value)
    if mode == "none":
        return []                     # the default, so the line stays as it would be typed
    if mode == "auto":
        return [flag, "auto"]
    if mode != "matmul":
        raise Refused("'" + mode + "' is not a symmetry: expected none, auto or matmul")
    shape = [str(value.get(part, "")) for part in ("n", "m", "k")]
    for dimension in shape:
        if not COUNT.match(dimension) or dimension == "0":
            raise Refused("matmul needs three dimensions <n> <m> <k>, and '" +
                          dimension + "' is not one")
    return [flag, "matmul"] + shape


def _points_words(flag, value):
    """`--points d:n`, repeated once per degree. This is step 2's output, and
    step 2 is not in this repository, so the supply is typed rather than found."""
    terms = value.split() if isinstance(value, str) else list(value)
    words = []
    for term in terms:
        if not POINT.match(term):
            raise Refused(flag + " expects terms written d:n, not '" + term + "'")
        words += [flag, term]
    return words


def build(tool_name, chosen, input_path=None, mode=None):
    """The argv for one run, without the binary, which the caller resolves.

    `chosen` is a flag-to-value mapping straight off the wire. Everything in it
    has to be declared for this tool or the whole line is refused: a browser that
    sends a flag the catalogue has never heard of is a browser this interface
    does not understand, and running a shortened version of what it asked for
    would be worse than telling it so.
    """
    tool = catalogue.BY_NAME.get(tool_name)
    if tool is None:
        raise Refused("no such tool: " + str(tool_name))

    declared = {option["flag"]: option for option in tool["options"]}
    for flag in chosen:
        if flag not in declared:
            raise Refused(tool_name + " has no option " + flag)

    words = []
    if input_path is not None:
        words.append(str(input_path))
    if tool.get("modes"):
        words += _mode_words(tool, mode)

    for option in tool["options"]:
        flag = option["flag"]
        if option.get("required") and not str(chosen.get(flag, "")).strip():
            raise Refused(tool_name + " needs " + flag)
        words += _words_for(option, chosen.get(flag))
    return words


def _mode_words(tool, mode):
    """`make-tensor` is the one tool whose question is the flag: four mutually
    exclusive builders, each with its own arity, so the words come from the mode
    rather than from the option list."""
    if not isinstance(mode, dict):
        raise Refused(tool["name"] + " needs one of " +
                      ", ".join(entry["flag"] for entry in tool["modes"]))
    declared = {entry["flag"]: entry for entry in tool["modes"]}
    entry = declared.get(mode.get("flag"))
    if entry is None:
        raise Refused("no such builder: " + str(mode.get("flag")))

    given = [str(word) for word in str(mode.get("values", "")).split()]
    for word in given:
        if not COUNT.match(word):
            raise Refused(entry["flag"] + " expects whole numbers, not '" + word + "'")
    wanted = len(entry["fields"])
    if entry.get("trailing"):
        if len(given) < wanted:
            raise Refused(entry["flag"] + " expects " + ", ".join(entry["fields"]))
    elif len(given) != wanted:
        raise Refused(entry["flag"] + " expects " + str(wanted) + " numbers: " +
                      ", ".join(entry["fields"]))
    return [entry["flag"]] + given
