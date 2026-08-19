"""What a run's ending means, in the vocabulary of `cli/exit_code.h`.

That header states the distinction this whole file exists to keep: **a question
that was not answered is not a question answered no.** A budget running out
proves nothing, and folding it into a refusal turns giving up into a lower
bound. It is a mistake this repository has published once already, and a front
end is exactly where it would be made a second time, because a screen has room
for one word and the honest word is three.

So nothing here shortens. Every ending gets its own name, the three the
interface itself can cause say whose doing they were rather than passing for the
toolkit's, and no branch in this file can produce `no` from anything but exit 1.
"""

# The six the toolkit speaks, worded from the header rather than paraphrased.
# `verdict` is what the badge says; `standing` is what may be concluded from it,
# and is the sentence printed under the badge.
EXIT_CODES = {
    0: {"verdict": "yes", "standing":
        "An algorithm with that many products exists, and it was verified."},
    1: {"verdict": "no", "standing":
        "Proved that none exists."},
    2: {"verdict": "usage", "standing":
        "The arguments did not parse, so no question was asked."},
    3: {"verdict": "undecided", "standing":
        "A budget was exhausted: timeout, memory cap or node limit. "
        "Nothing is proved, in either direction."},
    4: {"verdict": "unverified", "standing":
        "An answer was produced and it failed its own check. This is worse "
        "than an error: it means a component is wrong."},
    5: {"verdict": "error", "standing":
        "Could not run at all: unreadable file, no solver, an exception."},
}

def stopped_by(reason):
    """An ending the toolkit did not choose: you pressed stop, the wall clock ran
    out, or the console shut down. All three are undecided in substance, and all
    three name who ended the run, so nobody reads one as a verdict."""
    return {"verdict": "stopped", "badge": "stopped",
            "means": "nothing is decided",
            "standing": "This run was stopped by " + reason + ". Nothing is "
                        "decided, and this is not the toolkit's answer."}


def of_exit_status(status, tool_verdicts):
    """What one finished run means. `status` is what the process returned.

    `tool_verdicts` is the tool's own reading of 0 and 1, from the catalogue,
    because the twelve do not all answer the same question: 0 is a decomposition
    for `decide-rank`, an accepted candidate for `deflate-strictly` and simply a
    completed pass for `minimise-rank`. The header's own sentence comes back
    beside it as `standing`, and `show_standing` says whether it is the faithful
    reading of this run or a claim about a different kind of tool.

    A status this file has no name for is reported as having no name, never as
    the nearest one it does have.
    """
    if status in EXIT_CODES:
        named = dict(EXIT_CODES[status])
        tools_own = tool_verdicts.get(str(status))
        if tools_own:
            # The badge may soften, because "yes" over a heuristic's exit 0 reads
            # as a rank that was decided.
            named["badge"] = tools_own["badge"]
            named["means"] = tools_own["means"]
        else:
            named["badge"] = named["verdict"]
            named["means"] = named["standing"]

        # The header's sentence for 0 and 1 is written for the tools that decide
        # a rank, and quoting it under `make-tensor` would claim a verification
        # that never happened. It is shown whenever it is the faithful reading:
        # for 2, 3, 4 and 5 always, since those four mean the same from all
        # twelve and are the ones that must never be softened, and for 0 and 1
        # when the tool declares no reading of its own.
        named["show_standing"] = status not in (0, 1) or not tools_own
        return named
    return {"verdict": "unknown", "badge": "unknown",
            "standing": "The tool left with status " + str(status) +
                        ", which is not one of the six codes in cli/exit_code.h. "
                        "Nothing is decided.",
            "means": "no verdict"}


def of_signal(number):
    """A run killed by something other than this interface: a crash, or an
    outside `kill`. It is not exit 1 and it is not exit 3; it is neither."""
    return {"verdict": "signalled", "badge": "signalled",
            "standing": "The tool was ended by signal " + str(number) +
                        " from outside this interface. Nothing is decided.",
            "means": "no verdict"}
