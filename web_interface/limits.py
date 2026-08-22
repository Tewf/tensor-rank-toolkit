"""What bounds a run on this machine, as the toolkit itself reports it.

`show-limits` already answers this and is the authority: it reads the machine,
derives the ceilings and applies `tunables.conf` with exactly the precedence
every command applies. So this module runs it and hands the text on **verbatim**.

**It parses nothing and decides nothing**, for the same reason the rest of this
interface refuses to validate a map: a second opinion written in Python could
disagree with the authority, and two answers to one question is worse than one.
What the page shows is the instrument's own output and the line that produced it,
so a reader can retype it in a terminal and get the same thing.

Editing the limits is deliberately not offered here. `tunables.conf` belongs to
the checkout and a console that wrote to it would change what every other run on
this machine is bounded by, invisibly and from a browser. Per-run limits are
already flags, they are already in the options panel, and `OPTIONS.md`'s
precedence — a flag, then the file, then the compiled default — stays the one
rule.
"""
import subprocess

import repository

INSTRUMENT = "run_limits/show-limits"


def resolved(build, timeout_seconds=10):
    """`show-limits` output and the command that produced it.

    A build without the instrument is not an error: this interface predates it
    and an older build is a reasonable thing to be pointed at. The pane then says
    what is missing and how to get it, which is more use than an empty box.
    """
    binary = build / INSTRUMENT
    command = repository.command_as_typed([str(binary)])
    if not binary.is_file():
        return {"command": command,
                "text": "show-limits is not in this build. Rebuild the toolkit to "
                        "see what a run here is bounded by."}
    try:
        finished = subprocess.run([str(binary)], capture_output=True, text=True,
                                  timeout=timeout_seconds, check=False)
    except (OSError, subprocess.SubprocessError) as refusal:
        return {"command": command, "text": "show-limits could not be run: " + str(refusal)}
    return {"command": command, "text": (finished.stdout + finished.stderr).strip()}
