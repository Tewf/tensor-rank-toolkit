#!/usr/bin/env python3
"""Serve the console, and leave nothing running when it stops.

    python3 web_interface/serve.py                 # then open the URL it prints
    python3 web_interface/serve.py --build build   # a build directory elsewhere
    python3 web_interface/serve.py --port 8770 --no-browser

Standard library only, so this needs nothing installed that building the toolkit
did not already need. It binds the loopback address, so nothing off this machine
can reach it; `--host` is there for a deliberate exception and says so when used.

Ctrl-C stops every run that is still going before the process leaves, because a
console that exits over the top of a running search is the orphan problem
`run_limits/child_process.h` was written about.
"""
import argparse
import http.server
import pathlib
import sys
import webbrowser

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

import http_service      # noqa: E402  (the path has to be set first)
import repository        # noqa: E402
import service           # noqa: E402

LOOPBACK = {"127.0.0.1", "localhost", "::1"}


def arguments():
    parser = argparse.ArgumentParser(
        description="A browser console for the toolkit's commands.")
    parser.add_argument("--build", default=str(repository.ROOT / "build"),
                        help="the directory the toolkit was built into")
    parser.add_argument("--runs", default=str(pathlib.Path(__file__).resolve().parent / "runs"),
                        help="where each run's files are kept")
    parser.add_argument("--port", type=int, default=8770)
    parser.add_argument("--host", default="127.0.0.1",
                        help="loopback by default; anything else is reachable "
                             "from the network and is announced when used")
    parser.add_argument("--wall-clock", type=int,
                        default=service.DEFAULT_WALL_CLOCK_SECONDS,
                        help="seconds after which the console stops a run. It is "
                             "a backstop and decides nothing")
    parser.add_argument("--no-browser", action="store_true")
    return parser.parse_args()


def check_the_build(build):
    """Every binary the console offers, before it offers any of them.

    A console that lists its tools and fails on the first press is worse than
    one that will not start, because the failure looks like the tool's.
    """
    import catalogue
    missing = [tool["name"] for tool in catalogue.TOOLS
               if not (pathlib.Path(build) / tool["binary"]).is_file()]
    if missing:
        print("Nothing to drive: " + str(len(missing)) + " of " +
              str(len(catalogue.TOOLS)) + " tools are not in " + str(build) +
              " (" + ", ".join(missing[:3]) +
              ("..." if len(missing) > 3 else "") + ").\n\n"
              "Build the toolkit first, from the repository root:\n"
              "    cmake -B build -G Ninja && cmake --build build\n",
              file=sys.stderr)
        return False
    return True


def kept_already(runs):
    """What earlier sessions left in the runs directory, counted on the way past.

    Nothing prunes it and nothing here will: a run's directory holds its input,
    its logs and anything it emitted, and deleting somebody's evidence to save a
    few megabytes is the wrong trade. What was fixable is that it grew unseen, so
    it is now on the line above the URL, with the one command that clears it.
    """
    directory = pathlib.Path(runs)
    if not directory.is_dir():
        return ""
    kept = [path for path in directory.iterdir() if path.is_dir()]
    if not kept:
        return ""
    bytes_held = sum(file.stat().st_size for path in kept
                     for file in path.rglob("*") if file.is_file())
    return ("\n              " + str(len(kept)) + " earlier run(s) already there, " +
            str(round(bytes_held / 1e6, 1)) + " MB. Nothing prunes them:\n" +
            "              rm -rf " + str(directory))


def main():
    chosen = arguments()
    if not check_the_build(chosen.build):
        return 1

    console = service.Service(chosen.build, chosen.runs, chosen.wall_clock)
    handler = http_service.make_handler(console, LOOPBACK | {chosen.host})
    try:
        server = http.server.ThreadingHTTPServer((chosen.host, chosen.port), handler)
    except OSError as why:
        # Almost always a console already open on that port, which is a thing to
        # say rather than a traceback to read.
        print("cannot listen on " + chosen.host + ":" + str(chosen.port) +
              ": " + str(why) + "\n\nA console may already be open there. "
              "Use it, or start this one on another port:\n"
              "    python3 web_interface/serve.py --port " +
              str(chosen.port + 1) + "\n", file=sys.stderr)
        return 1

    address = "http://" + ("localhost" if chosen.host in LOOPBACK else chosen.host)
    address += ":" + str(server.server_address[1]) + "/"
    print("tensor-rank-toolkit console on " + address)
    print("  build:      " + str(pathlib.Path(chosen.build).resolve()))
    print("  runs kept:  " + str(pathlib.Path(chosen.runs).resolve()) +
          kept_already(chosen.runs))
    print("  wall clock: " + str(chosen.wall_clock) +
          " s per run, a backstop that decides nothing")
    if chosen.host not in LOOPBACK:
        print("  reachable from the network, because --host " + chosen.host +
              " was given")
    print("Ctrl-C to stop.")
    if not chosen.no_browser:
        webbrowser.open(address)

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        stopped = console.registry.stop_everything()
        print("\nstopped " + str(stopped) + " run(s) still going, and left.")
    finally:
        server.server_close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
