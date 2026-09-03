"""Which of a tool's own lines say how it was going to answer the question.

Before it opens a node, `decide-rank` prints its pool, its leaf, its device and,
under `-s`, its quotient. Those lines are the command teaching what it does, and
in a terminal they are the first thing on the screen. On a result card they were
the fourth, at the top of a stream under a verdict and two sentences, which is
where a reader stops scrolling.

So they are promoted, and the promotion is a **copy** rather than a move: the
stream underneath still holds every line the tool printed, in order and unedited.
A label that drifts out of the list below therefore costs a promotion and never a
line, which is the only reason a transcription like this one is safe to keep.
That it has not drifted is asserted against a real run in
`tests/check_web_interface.py`, not against this file.

Nothing here is reworded. A promoted line is the tool's own characters with its
indentation removed.
"""

# Exact prefixes, as the tools print them. Kept as prefixes and not patterns
# because a prefix cannot match a line it was not written for.
PLAN_PREFIXES = (
    "pool:",             # decide-rank, enumerate-subspaces: size, shape, held or addressed
    "leaf:",             # decide-rank: which leaf implementation applies
    "device:",           # decide-rank, from infrastructure/run_limits/device.h
    "route:",            # factor-over-canonical-basis: which of the four routes ran
    "symmetry:",         # factor-over-canonical-basis: the quotient, if any
    "tensor:",           # factor-over-canonical-basis: field and shape, as read
    "quotienting by ",   # decide-rank -s: the generators and the rejection rule
)


def found_in(text):
    """The plan lines of one run's stdout, in the order the tool printed them."""
    lines = []
    for raw in text.split("\n"):
        line = raw.strip()
        if line.startswith(PLAN_PREFIXES):
            lines.append(line)
    return lines
