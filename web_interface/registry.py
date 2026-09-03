"""Everything this console has started, and the promise that none outlives it.

Separate from `runner.py` because that file is about one run and this one is
about all of them. The promise is the whole reason it exists: a console that
exits over the top of a running search leaves the orphan
`infrastructure/run_limits/child_process.h` was written about, so shutting down goes through
here and waits for each group to be gone.

`service.py` holds two of these, one of runs and one of flows, because a flow
starts runs and holds no process of its own. Neither is named here: what is
kept is anything that can say whether it is still going and be told to stop.
"""
import threading


class Registry:
    def __init__(self):
        self.runs = {}
        self.lock = threading.Lock()
        self.counter = 0

    def add(self, make_run):
        with self.lock:
            self.counter += 1
            identifier = str(self.counter)
        run = make_run(identifier)
        with self.lock:
            self.runs[identifier] = run
        return run

    def get(self, identifier):
        with self.lock:
            return self.runs.get(identifier)

    def stop_everything(self):
        """Called when the server is shutting down. Anything left behind here is
        the leak `infrastructure/run_limits/child_process.h` was written about."""
        with self.lock:
            running = [run for run in self.runs.values() if run.is_running()]
        for run in running:
            run.stop("the console shutting down")
        return len(running)
