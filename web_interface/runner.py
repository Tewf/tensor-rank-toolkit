"""One run: started in its own process group, bounded, stoppable, and leaving
nothing of ours behind when it ends.

`infrastructure/run_limits/child_process.h` is the launcher inside the toolkit and it exists
because orphaned solvers once held cores and corrupted every measurement taken
afterwards. This module is the same problem one level up, so it keeps the same
two rules.

**The group is what must be killed, not the process.** A tool that forks a
solver leaves it behind when only its parent is signalled, which is the same
leak with an extra step. `start_new_session` puts the child in a group of its
own with no terminal attached, and the signal goes to the group.

**SIGTERM first, then SIGKILL.** `infrastructure/cli/interrupt_cleanup.h` catches SIGINT,
SIGTERM and SIGHUP to unlink the scratch CNF a killed sweep would otherwise
leave in /tmp, then restores the default and re-raises, so a terminated run
still dies of the signal it was sent. SIGKILL cannot be caught, so opening with
it would leave those files behind. The grace period is short because the handler
only calls `unlink`.

**What this still cannot reach, stated rather than glossed.** A solver started
by `infrastructure/run_limits/child_process.h` calls `setpgid(0, 0)` and is therefore in a
group of its own, not in the tool's, so killing the tool's group does not reach
it. What ends it is the `alarm` it carries, set from the tool's own timeout, so
it holds a core for up to that many seconds after a stop. Set the tool's budget
flag at or below the wall clock here and that window closes; the interface fills
that flag in for you and shows it in the command rather than bounding the run
from outside where a reader cannot see it.
"""
import os
import pathlib
import signal
import subprocess
import threading
import time

import outcome
import plan_lines

# Long enough for a handler whose whole body is a few `unlink` calls, short
# enough that a stop still feels like a stop.
GRACE_SECONDS = 2.0

# A log is shown in full until it is silly to, and then by its tail, because the
# end of a search log is the part that says how it ended.
MOST_LOG_BYTES = 400_000


class Run:
    def __init__(self, identifier, tool, argv, directory, input_path,
                 wall_clock_seconds, working_directory, result_name="stdout.log",
                 verdicts=None):
        self.identifier = identifier
        self.tool = tool
        # Where stdout goes. It is a parameter because `make-tensor` writes a
        # tensor there rather than a report, and a file called stdout.log is not
        # a file anybody would keep: the command shown ends `> map.tensor`, so
        # that has to be the name it really has.
        self.result_name = result_name
        # What exit 0 and exit 1 mean for this run, which is not always what they
        # mean for the tool: a flag that makes a run write a file and stop
        # changes the claim its exit 0 carries.
        self.verdicts = tool.get("verdicts", {}) if verdicts is None else verdicts
        self.argv = [str(word) for word in argv]
        self.directory = pathlib.Path(directory)
        self.input_path = input_path
        self.wall_clock_seconds = wall_clock_seconds
        self.working_directory = working_directory
        self.started_at = None
        self.ended_at = None
        self.process = None
        self.stopped_by = None          # who ended it, and never a verdict
        self.lock = threading.Lock()

    def start(self):
        self.started_at = time.time()
        # The parent's copies of the two files are closed as soon as the child
        # has them, which is what the `with` is for; the child keeps its own.
        with open(self.directory / self.result_name, "wb") as result, \
             open(self.directory / "stderr.log", "wb") as commentary:
            self.process = subprocess.Popen(
                self.argv,
                cwd=str(self.working_directory),
                stdin=subprocess.DEVNULL,
                stdout=result,
                stderr=commentary,
                # A session of its own, so the whole group can be signalled and
                # so a terminal signal meant for the server never reaches a run.
                start_new_session=True)
        threading.Thread(target=self._wait_out_the_clock, daemon=True).start()

    def _wait_out_the_clock(self):
        try:
            self.process.wait(timeout=self.wall_clock_seconds)
        except subprocess.TimeoutExpired:
            self.stop("the wall clock")
        self.ended_at = time.time()

    def stop(self, by="you"):
        """End the group, and wait for it, so this returns to a machine with
        nothing of this run still on it."""
        with self.lock:
            if self.stopped_by is not None or self.process.poll() is not None:
                return
            self.stopped_by = by
        self._signal_group(signal.SIGTERM)
        try:
            self.process.wait(timeout=GRACE_SECONDS)
        except subprocess.TimeoutExpired:
            self._signal_group(signal.SIGKILL)
            self.process.wait()

    def _signal_group(self, number):
        try:
            os.killpg(self.process.pid, number)
        except (ProcessLookupError, PermissionError):
            pass   # already gone, which is the state the call was asking for

    def is_running(self):
        return self.process is not None and self.process.poll() is None

    def seconds(self):
        if self.started_at is None:
            return 0.0
        return (self.ended_at or time.time()) - self.started_at

    def _log(self, name):
        path = self.directory / name
        if not path.is_file():
            return ""
        size = path.stat().st_size
        with open(path, "rb") as reading:
            if size > MOST_LOG_BYTES:
                reading.seek(size - MOST_LOG_BYTES)
                head = "[" + str(size - MOST_LOG_BYTES) + " earlier bytes not shown]\n"
            else:
                head = ""
            return head + reading.read().decode("utf-8", "replace")

    def snapshot(self, command_as_typed):
        """Everything a result card shows. The verdict is computed here and
        nowhere else, and a run this interface stopped never gets one."""
        status = self.process.poll()
        card = {
            "id": self.identifier,
            "tool": self.tool["name"],
            # The process group's number, which is the child's own since it was
            # started in a session of its own. It is here so a reader can find
            # the run in `ps`, and so a check can ask the operating system
            # whether the group is gone rather than scanning for a name, which
            # would answer about somebody else's solver.
            "process_group": self.process.pid,
            "command": command_as_typed,
            "running": status is None,
            "seconds": round(self.seconds(), 3),
            "wall_clock_seconds": self.wall_clock_seconds,
            "result": self._log(self.result_name),
            "commentary": self._log("stderr.log"),
            "files": [],
        }
        # The lines the tool printed about how it was going to answer. Taken
        # while the run is still going too, because those lines are printed
        # before the first node and are the only thing on the card for the hour
        # a search says nothing else.
        card["plan"] = plan_lines.found_in(card["result"])
        if status is None:
            return card

        if self.stopped_by is not None:
            card["outcome"] = outcome.stopped_by(self.stopped_by)
        elif status < 0:
            card["outcome"] = outcome.of_signal(-status)
        else:
            card["outcome"] = outcome.of_exit_status(status, self.verdicts)
        card["exit_status"] = status
        return card
