"""One flow, a step at a time, and never two of them at once.

`runner.py` is one process and `registry.py` is all of them; this is the third
thing, which is an order between two of them. It runs on this side and not in
the browser for the reason `service.py` gives about the preview: a chain
assembled in a page would be a second implementation of the chain the checks
drive, and the first time the two disagreed the console would be showing a
pipeline nobody ran.

**Nothing a step reads comes from the request.** A later step reads a file an
earlier step wrote, and the path to it is the run directory this console chose,
carried here in Python rather than sent back through a browser that could name
any file on the machine.

**A flow that has been stopped starts nothing else.** The step is begun under
the same lock the stop takes, so a flow cannot slip a run past
`registry.stop_everything` between two of its steps: that run is exactly the
orphan `infrastructure/run_limits/child_process.h` was written about, one level up again.
"""
import threading
import time

# How long a finished step waits before the next one starts. Nothing is timed
# through this and no measurement passes it; it is only the cost of asking.
BETWEEN_ASKS_SECONDS = 0.1


class Flow:
    def __init__(self, identifier, declaration, console, request,
                 wall_clock_seconds):
        self.identifier = identifier
        self.declaration = declaration
        self.console = console
        self.request = request
        self.wall_clock_seconds = wall_clock_seconds
        self.runs = []                  # the steps started, in the order started
        self.stopped_because = None     # why it ended early, and never a verdict
        self.running = True
        self.lock = threading.Lock()

    def start(self):
        threading.Thread(target=self._run, daemon=True).start()

    def _run(self):
        try:
            self._walk()
        except Exception as why:        # noqa: BLE001 (a thread of our own)
            self._stop_because("the console failed between steps: " + repr(why))
        finally:
            self.running = False

    def _walk(self):
        """Every step in order, each on what the step before it wrote."""
        steps = self.declaration["steps"]
        reads = [None]              # the first step reads the map in the request
        for number, step in enumerate(steps):
            wrote = []
            for path in reads:
                run = self._start(step, path)
                if run is None:
                    return
                written = self._files_if_it_reached(run, step)
                if written is None:
                    return
                wrote += [run.directory / name for name in written]
            if number + 1 == len(steps):
                return
            following = steps[number + 1]
            reads = [path for path in wrote
                     if path.name.endswith(following["takes"])]
            if not reads:
                return self._stop_because(
                    step["tool"] + " wrote no " + following["takes"] + " for " +
                    following["tool"] + " to read, so the flow stops here "
                    "rather than on a file that is not there.")

    def _start(self, step, path):
        """One step, unless this flow was stopped between two of them."""
        with self.lock:
            if self.stopped_because is not None:
                return None
            run = self.console.start_step(self.request, step, path,
                                          self.wall_clock_seconds)
            self.runs.append(run)
            return run

    def _files_if_it_reached(self, run, step):
        """What the step wrote, once it has ended with the badge its flow names.

        The badge is read off the run's own card, so the word compared here is
        the word the reader is shown and `outcome.py` is still the only place a
        run's ending is decided.
        """
        while run.is_running():
            time.sleep(BETWEEN_ASKS_SECONDS)
        card = self.console.card(run.identifier)
        badge = (card.get("outcome") or {}).get("badge")
        if badge != step["expect"]:
            return self._stop_because(
                step["tool"] + " ended as " + str(badge) + ", and this flow goes "
                "on only from " + step["expect"] + ". Its own card says what it "
                "decided, and nothing after it was started.")
        return card["files"]

    def _stop_because(self, reason):
        """Ends the flow, keeping the first reason: a step that failed says more
        than the shutdown that arrived a moment later."""
        with self.lock:
            if self.stopped_because is None:
                self.stopped_because = reason
        return None

    def is_running(self):
        return self.running

    def stop(self, by="you"):
        """End the flow and the step it is on, so nothing of it is left going."""
        with self.lock:
            if self.stopped_because is None:
                self.stopped_because = "stopped by " + by
            last = self.runs[-1] if self.runs else None
        if last is not None:
            last.stop(by)

    def card(self):
        """What a reader is shown about the flow itself. Everything a step did
        is on that step's own card, and none of it is repeated here."""
        return {
            "id": self.identifier,
            "flow": self.declaration["name"],
            "running": self.running,
            "runs": [run.identifier for run in self.runs],
            # Not a verdict, and worded so it cannot be read as one: every step
            # ran and ended where its own tool says it ends.
            "badge": ("running" if self.running else
                      "complete" if self.stopped_because is None else "cut short"),
            "stopped_because": self.stopped_because,
        }
