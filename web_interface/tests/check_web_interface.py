#!/usr/bin/env python3
"""Drive the console over HTTP and check what comes back.

    python3 web_interface/tests/check_web_interface.py [--build build]

Not registered with ctest, deliberately: the suite's count is quoted in the
README and in CI, and a front end's checks arriving inside it would move a
number that means something else. This runs on its own and says so.

The check that matters most is the one on exit 3. A budget that ran out has to
arrive at a reader as undecided and never as a refutation, because that
confusion has produced a wrong published result in this repository once already.
So it is asserted here against a real run of a real binary rather than against a
table of codes, which would pass whatever the interface did with them.
"""
import argparse
import json
import os
import pathlib
import re
import sys
import threading
import time
import urllib.error
import urllib.request

HERE = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))

import http.server            # noqa: E402
import http_service           # noqa: E402
import plan_lines             # noqa: E402
import repository             # noqa: E402
import service                # noqa: E402
import worked_examples        # noqa: E402

PASSED, FAILED = [], []


def check(what, held):
    (PASSED if held else FAILED).append(what)
    print(("  ok   " if held else "  FAIL ") + what)


def ask(port, method, path, body=None):
    request = urllib.request.Request(
        "http://127.0.0.1:" + str(port) + path, method=method,
        data=json.dumps(body).encode() if body is not None else None,
        headers={"Content-Type": "application/json"} if body is not None else {})
    try:
        with urllib.request.urlopen(request) as answer:
            return answer.status, json.loads(answer.read())
    except urllib.error.HTTPError as refused:
        return refused.code, json.loads(refused.read())


def served(port, path):
    """One file exactly as the browser receives it, status and text."""
    request = urllib.request.Request("http://127.0.0.1:" + str(port) + path)
    try:
        with urllib.request.urlopen(request) as answer:
            return answer.status, answer.read().decode("utf-8")
    except urllib.error.HTTPError as refused:
        return refused.code, refused.read().decode("utf-8")


def until_finished(port, identifier, most_seconds=120):
    limit = time.time() + most_seconds
    while time.time() < limit:
        status, card = ask(port, "GET", "/api/runs/" + identifier)
        if not card["running"]:
            return card
        time.sleep(0.2)
    raise AssertionError("run " + identifier + " never finished")


def run(port, tool, options, fixture=None, text=None, wall_clock=120):
    body = {"tool": tool, "options": options, "wall_clock_seconds": wall_clock,
            "input": {"text": text if text is not None
                      else (repository.fixture_text(fixture) if fixture else ""),
                      "name": fixture or "typed.tensor", "fixture": fixture}}
    status, started = ask(port, "POST", "/api/runs", body)
    if status != 200:
        return status, started
    return status, until_finished(port, started["id"])


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--build", default=str(repository.ROOT / "build"))
    chosen = parser.parse_args()

    console = service.Service(chosen.build, HERE.parent / "runs", 120)
    handler = http_service.make_handler(console, {"127.0.0.1", "localhost", "::1"})
    server = http.server.ThreadingHTTPServer(("127.0.0.1", 0), handler)
    port = server.server_address[1]
    threading.Thread(target=server.serve_forever, daemon=True).start()
    print("console on 127.0.0.1:" + str(port))

    try:
        checks(port, pathlib.Path(chosen.build))
    finally:
        console.registry.stop_everything()
        server.shutdown()
        server.server_close()

    print("\n" + str(len(PASSED)) + " passed, " + str(len(FAILED)) + " failed")
    return 1 if FAILED else 0


def checks(port, build):
    print("\nthe catalogue")
    status, setup = ask(port, "GET", "/api/setup")
    check("and every starter names a tool that is there",
          all(example["tool"] in {tool["name"] for tool in setup["tools"]}
              for example in setup["examples"]))
    catalogue_matches_the_build(setup, build)

    page_is_whole(port)

    print("\nthe command shown is the command run")
    body = {"tool": "decide-rank", "options": {"--target": "7"},
            "input": {"text": repository.fixture_text("matmul_2x2x2.tensor"),
                      "name": "matmul_2x2x2.tensor",
                      "fixture": "matmul_2x2x2.tensor"}}
    _, seen = ask(port, "POST", "/api/preview", body)
    wanted = ("build/exhaustive_search/decide-rank "
              "fixtures/matmul_2x2x2.tensor --target 7")
    check("the preview is the retypable line", seen["command"] == wanted)
    _, card = run(port, "decide-rank", {"--target": "7"}, "matmul_2x2x2.tensor")
    check("and the run shows the same line", card["command"] == wanted)

    print("\nthe plan the tool prints, promoted and not reworded")
    check("the plan lines are the tool's own characters",
          all(line in card["result"] for line in card["plan"]))
    named = " ".join(card["plan"])
    check("and they are the pool, the leaf and the device",
          "pool:" in named and "leaf:" in named and "device:" in named)
    _, quotiented = run(port, "decide-rank",
                        {"--target": "6",
                         "-s": {"mode": "matmul", "n": "2", "m": "2", "k": "2"}},
                        "matmul_2x2x2.tensor")
    # The transcription in `plan_lines.py` is only safe while it is still true of
    # what the tools print, so it is asserted against a run and not against
    # itself. A quotient is the line most recently reworded.
    check("a quotient reaches the plan too",
          any(line.startswith("quotienting by ") for line in quotiented["plan"]))

    print("\nthe six exit codes, from real runs")
    check("exit 0 is yes", card["exit_status"] == 0 and
          card["outcome"]["verdict"] == "yes")
    _, refuted = run(port, "decide-rank", {"--target": "6"}, "matmul_2x2x2.tensor")
    check("exit 1 is no", refuted["exit_status"] == 1 and
          refuted["outcome"]["verdict"] == "no")

    _, gave_up = run(port, "decide-rank",
                     {"--target": "13", "--node-limit": "1000"}, "f2_5x5.tensor")
    check("exit 3 is undecided", gave_up["exit_status"] == 3 and
          gave_up["outcome"]["verdict"] == "undecided")
    check("exit 3 is never reported as no",
          "no" != gave_up["outcome"]["verdict"] and
          "Nothing is proved" in gave_up["outcome"]["standing"])

    _, unreadable = run(port, "decide-rank", {},
                        text="field 4\nshape 1 1 1\n\n1\n")
    check("exit 5 is an error", unreadable["exit_status"] == 5)
    check("and it carries the reader's own words",
          "must be a prime" in unreadable["commentary"])

    print("\nfound, proved, and nothing proved: three, and never two")
    check("a decomposition is found", card["outcome"]["decides"] == "found")
    check("a refutation is proved", refuted["outcome"]["decides"] == "proved")
    check("an exhausted budget proves nothing",
          gave_up["outcome"]["decides"] == "nothing proved")
    check("and so does a run that could not start at all",
          unreadable["outcome"]["decides"] == "nothing proved")

    print("\na flag that changes what exit 0 claims")
    _, wrote = run(port, "decide-rank-by-sat",
                   {"--target": "5", "--emit-cnf": True}, "f2_2x3.tensor")
    check("--emit-cnf leaves with 0 and claims no decomposition",
          wrote["exit_status"] == 0 and
          wrote["outcome"]["badge"] == "written" and
          "No solver was asked" in wrote["outcome"]["means"])
    _, asked = run(port, "decide-rank-by-sat", {"--target": "5"}, "f2_2x3.tensor")
    check("and without it the same code is a decomposition",
          asked["exit_status"] == 0 and asked["outcome"]["badge"] == "yes")

    print("\nwhat a run would leave behind, said before it starts")
    box = {"text": repository.fixture_text("f2_2x3.tensor"),
           "name": "f2_2x3.tensor", "fixture": "f2_2x3.tensor"}
    _, filled = ask(port, "POST", "/api/preview",
                    {"tool": "decide-rank-by-sat", "options": {"--timeout": "60"},
                     "wall_clock_seconds": 120, "input": box})
    check("a solver bounded under the wall clock is not warned about",
          filled["warnings"] == [])
    _, cleared = ask(port, "POST", "/api/preview",
                     {"tool": "decide-rank-by-sat", "options": {},
                      "wall_clock_seconds": 120, "input": box})
    check("and clearing its budget says what that reopens",
          len(cleared["warnings"]) == 1 and
          "--timeout" in cleared["warnings"][0] and
          "hold a core" in cleared["warnings"][0])

    print("\nrefusals, before anything is started")
    status, why = ask(port, "POST", "/api/runs",
                      {"tool": "decide-rank", "options": {"--nonsense": "1"},
                       "input": {"text": "field 2\nshape 1 1 1\n\n1\n"}})
    check("an undeclared flag is refused", status == 400 and
          "no option --nonsense" in why["refused"])
    status, why = ask(port, "POST", "/api/runs",
                      {"tool": "decide-rank", "options": {"--target": "abc"},
                       "input": {"text": "field 2\nshape 1 1 1\n\n1\n"}})
    check("a bad value names the flag and quotes the word",
          status == 400 and "--target expects a count, not 'abc'" == why["refused"])
    status, why = ask(port, "POST", "/api/runs",
                      {"tool": "decide-rank", "options": {}, "input": {"text": "  "}})
    check("an empty box is refused", status == 400 and "nothing in the box" in why["refused"])
    status, why = ask(port, "GET", "/api/fixture?name=../../etc/passwd")
    check("a fixture outside fixtures/ is refused", status == 404)

    print("\nstopping, and leaving nothing running")
    body = {"tool": "enumerate-subspaces",
            "options": {"--target": "7",
                        "-s": {"mode": "matmul", "n": "2", "m": "2", "k": "2"}},
            "wall_clock_seconds": 600,
            "input": {"text": repository.fixture_text("matmul_2x2x2.tensor"),
                      "name": "matmul_2x2x2.tensor",
                      "fixture": "matmul_2x2x2.tensor"}}
    _, started = ask(port, "POST", "/api/runs", body)
    time.sleep(1.5)
    _, mid = ask(port, "GET", "/api/runs/" + started["id"])
    check("a long run reports itself running", mid["running"])
    _, stopped = ask(port, "POST", "/api/runs/" + started["id"] + "/stop")
    check("a stopped run is stopped, not refuted",
          stopped["outcome"]["verdict"] == "stopped" and
          "stopped by you" in stopped["outcome"]["standing"] and
          "not the toolkit's answer" in stopped["outcome"]["standing"])
    check("and nothing of it is left running", group_is_gone(mid["process_group"]))

    body["wall_clock_seconds"] = 2
    _, timed = ask(port, "POST", "/api/runs", body)
    ended = until_finished(port, timed["id"], most_seconds=30)
    check("the wall clock ends a run and decides nothing",
          ended["outcome"]["verdict"] == "stopped" and
          "stopped by the wall clock" in ended["outcome"]["standing"])
    check("and nothing of that one is left either",
          group_is_gone(ended["process_group"]))

    print("\nthe worked examples, each run rather than read")
    for example in worked_examples.EXAMPLES:
        _, ended = run(port, example["tool"], example["options"],
                       example["fixture"])
        check(example["title"] + " -> " + example["expect"],
              ended.get("outcome", {}).get("badge") == example["expect"])

    print("\nthis machine only")
    request = urllib.request.Request("http://127.0.0.1:" + str(port) + "/api/setup",
                                     headers={"Origin": "http://elsewhere.example"})
    try:
        urllib.request.urlopen(request)
        check("a request from another origin is refused", False)
    except urllib.error.HTTPError as refused:
        check("a request from another origin is refused", refused.code == 403)


# Binaries the build produces that are deliberately not offered here, each with
# the reason, because a name on this list is a decision and not an oversight.
# `OPTIONS/one-question-per-command.md` is where the decisions are argued.
NOT_TOOLS = {
    # An instrument: its output is nanoseconds, and MEASURING.md's line is that
    # counts reproduce anywhere and timings do not. `measure-leaf` is its model
    # and is CUDA-only, so it is usually not in the build tree at all.
    "oracle_guided_search/price-canonical-route",
    "gpu_leaf/measure-leaf",
}


def catalogue_matches_the_build(setup, build):
    """The offered tools against the binaries that exist, in both directions.

    A count asserted as a literal is what let this drift: two commands shipped
    without ever reaching the catalogue and one stayed in it after it stopped
    being a command, and `len(tools) == 12` was true throughout. So the question
    asked here is the one that was actually wrong — does the list *correspond* —
    and the number falls out of it.
    """
    offered = {tool["binary"] for tool in setup["tools"]}
    missing = sorted(name for name in offered if not (build / name).is_file())
    check("every tool offered is a binary that is there" +
          (" (" + ", ".join(missing) + ")" if missing else ""), not missing)

    built = {str(path.relative_to(build)) for path in build.glob("*/*")
             if path.is_file() and os.access(path, os.X_OK)}
    unoffered = sorted(built - offered - NOT_TOOLS)
    check("and every command in the build tree is offered or named as not a tool" +
          (" (" + ", ".join(unoffered) + ")" if unoffered else ""), not unoffered)


def page_is_whole(port):
    """The page the browser is actually given, against the scripts it loads.

    The interface is five plain scripts reaching into the markup by identifier,
    which is the arrangement that costs nothing until a pane is renamed and one
    `$("...")` in one file goes on returning null. So the served HTML and the
    served scripts are read back from the running console and compared: every
    identifier a script reaches for has to be in the page it was sent with.
    """
    print("\nthe page the browser is given")
    status, html = served(port, "/")
    check("the console page is served", status == 200 and "<main" in html)

    asked_for = re.findall(r'(?:src|href)="(/page/[^"]+)"', html)
    missing = [path for path in asked_for if served(port, path)[0] != 200]
    check("every stylesheet and script it links is served too",
          len(asked_for) >= 2 and not missing)

    in_page = set(re.findall(r'\bid="([^"]+)"', html))
    reached_for = set()
    for path in asked_for:
        if path.endswith(".js"):
            reached_for |= set(re.findall(r'\$\("([^"]+)"\)', served(port, path)[1]))
    astray = sorted(reached_for - in_page)
    check("and every element the scripts reach for is in it" +
          (" (" + ", ".join(astray) + ")" if astray else ""),
          len(reached_for) > 10 and not astray)


def group_is_gone(process_group):
    """Signal 0 to the group, which asks the kernel whether it exists and sends
    nothing. Asked of the group rather than of a process name, because a name
    scan answers about every copy on the machine including somebody else's."""
    try:
        os.killpg(process_group, 0)
    except ProcessLookupError:
        return True
    return False


if __name__ == "__main__":
    sys.exit(main())
