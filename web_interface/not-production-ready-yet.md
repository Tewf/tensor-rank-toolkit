# What is not finished

Written down rather than left to be found. Nothing here is a defect in the
toolkit; all of it is this interface.

**A killed run can leave one solver behind, for up to its own timeout.**
`run_limits/child_process.h` starts a solver in a process group of its own, so
killing the tool's group does not reach it; what ends it is the `alarm` it
carries, set from the tool's own timeout. Stopping a `decide-rank-by-sat` run
whose `--timeout` is 300 can therefore leave kissat holding a core for up to 301
seconds. The interface fills that flag in with its own wall clock so the window
matches what you chose, but clearing the flag by hand reopens it. Closing it
properly means killing the group from a signal handler, which that header
explains it declined to do for a reason that still holds.

**Progress is what the tool prints, and no more.** There is no percentage and no
node counter, because none of the twelve emits one while it works. What a card
shows is both streams as they arrive, the elapsed clock and the wall clock it is
running against. For a search that prints nothing for an hour, that is an
elapsed clock and a stop button.

**One console, one user.** There is no accounting of who started what and no
limit on how many runs may be going at once, so twelve heavy searches will
oversubscribe the machine exactly as twelve terminals would. It listens on
loopback because it assumes the person driving it is the person sitting there.

**Runs accumulate.** Each keeps a directory under `web_interface/runs/` holding
its input, its two logs and anything it emitted. Nothing prunes them. They are
ignored by git and can be deleted whole.

**`curve-bounds --table` prints a transcription and takes no map**, so it sits in
the same list as the questions that do. It is correct and it is untidy.

**Nothing here has been driven by anybody but its author.** The checks pass and
the worked example reproduces, and that is a different claim from a colleague
having sat in front of it.

## Things it deliberately does not do

**It does not validate a map itself.** The readers in `formats/` already refuse
what they do not understand and their messages name the line, so a malformed
tensor reaches the binary and comes back as that binary's refusal. A second
opinion written in Python could disagree with the authority, and the only thing
worse than a refusal is two of them.

**It does not measure.** The seconds on a card are a wall clock on a machine
that is also running a browser. `MEASURING.md` is the protocol a published
timing is taken under, and this is not it, which every card says.
