# What is not finished

Written down rather than left to be found. Nothing here is a defect in the
toolkit; all of it is this interface. What has since been closed is at the
bottom, with what closing it did and did not reach.

**Progress is what the tool prints, and no more.** There is no percentage and no
node counter, because no tool here emits one while it works. What a card
shows is the plan the tool printed before it started, both streams as they
arrive, the elapsed clock and the wall clock it is running against. For a search
that prints nothing for an hour, that is a plan, an elapsed clock and a stop
button.

**One console, one user.** There is no accounting of who started what and no
limit on how many runs may be going at once, so twelve heavy searches will
oversubscribe the machine exactly as twelve terminals would. It listens on
loopback because it assumes the person driving it is the person sitting there.

**Nothing here has been driven by any person but its author.** The checks pass
and the worked example reproduces, and that is a different claim from a
colleague having sat in front of it. What has been done, on 2026-09-03, is a
simulated first-time user: a language model handed only "run serve.py", driving
the console over HTTP with no access to this directory's source. It started
the cheapest search on a shipped fixture, followed the output, stopped a run
and found where results are kept — evidence the pages carry enough to act on,
and still not a person in the chair.

## Things it deliberately does not do

**It does not validate a map itself.** The readers in `formats/` already refuse
what they do not understand and their messages name the line, so a malformed
tensor reaches the binary and comes back as that binary's refusal. A second
opinion written in Python could disagree with the authority, and the only thing
worse than a refusal is two of them. Filtering the fixture menu by the chosen
tool's declared input is not that: it reads the catalogue, not the file.

**It does not measure.** The seconds on a card are a wall clock on a machine
that is also running a browser. `MEASURING.md` is the protocol a published
timing is taken under, and this is not it, which every card says.

## Closed since, and what each one left standing

**A killed run could leave one solver behind without saying so.**
`run_limits/child_process.h` starts a solver in a process group of its own, so
killing the tool's group does not reach it; what ends it is the `alarm` it
carries, set from the tool's own timeout. The interface fills that flag in with
its own wall clock, which closes the window, and clearing it or raising it
reopens it. **What was fixable was the silence**: the preview now says so, before
the run, naming the flag and the two numbers. The window itself is still open
where you clear the flag, and closing it properly means killing the group from a
signal handler, which that header explains it declined to do for a reason that
still holds.

**Runs accumulated unseen.** Each keeps a directory under `web_interface/runs/`
holding its input, its two logs and anything it emitted. **Nothing prunes them
and nothing here will**, because that is a run's evidence; what `serve.py` now
prints at startup is how many are there, how large, and the one command that
clears them.

**`curve-bounds --table` sat among the questions.** It is grouped under "on
nothing but its own arguments", and its exit 0 carries its own reading — the
table was printed and nothing was minimised — rather than the tool's. What
remains untidy is that it is a mode wearing the shape of a flag, in the same list
as the flags that tune a minimisation, and that is the tool's shape rather than
this interface's. `--solvers`, which arrived when `list-solvers` stopped being a
command of its own, is the same shape and carries the same kind of reading.
