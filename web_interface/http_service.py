"""The HTTP surface: which URL reaches which action, and JSON in and out.

Only the standard library, because the toolkit's own claim is that Givaro is its
one build dependency and every solver is optional and found on PATH at run time.
A front end that needed a package installed would be the first thing here to
break that, so this one needs nothing: `python3 web_interface/serve.py` and a
browser, both of which the machine that built the toolkit already has.

Two guards, and the reason for each. The server listens on the loopback address,
so the only thing that can reach it is this machine; a page on the open internet
can still make a browser post to it, though, which is why the Host and Origin
headers are checked rather than trusted. Nothing here runs a shell and no path
in a request is ever joined onto a filesystem path: `service.py` chooses every
path a tool reads or writes.
"""
import http.server
import json
import mimetypes
import pathlib
import urllib.parse

import command_line
import repository

PAGE = pathlib.Path(__file__).resolve().parent / "page"
MOST_REQUEST_BYTES = 8 * 1024 * 1024


def make_handler(service, allowed_hosts):
    class Handler(http.server.BaseHTTPRequestHandler):
        server_version = "tensor-rank-toolkit-console"
        protocol_version = "HTTP/1.1"

        def log_message(self, form, *arguments):
            pass   # the console's own output is the run, not a request log

        # ---- the two guards -------------------------------------------------

        def from_this_machine(self):
            host = self.headers.get("Host", "").rsplit(":", 1)[0].strip("[]")
            if host and host not in allowed_hosts:
                return False
            origin = self.headers.get("Origin")
            if origin:
                named = urllib.parse.urlparse(origin).hostname or ""
                return named.strip("[]") in allowed_hosts
            return True

        # ---- answering ------------------------------------------------------

        def send(self, status, body, content_type="application/json"):
            if isinstance(body, (dict, list)):
                body = json.dumps(body).encode("utf-8")
            elif isinstance(body, str):
                body = body.encode("utf-8")
            self.send_response(status)
            self.send_header("Content-Type", content_type)
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)

        def refuse(self, status, why):
            """Every refusal is a sentence naming what is wrong, because that is
            the grammar `cli/arguments.h` set for the commands themselves."""
            self.send(status, {"refused": str(why)})

        def read_request(self):
            length = int(self.headers.get("Content-Length") or 0)
            if length > MOST_REQUEST_BYTES:
                raise ValueError("that is more than " +
                                 str(MOST_REQUEST_BYTES // (1024 * 1024)) +
                                 " MB, which is more than a map should be")
            return json.loads(self.rfile.read(length) or b"{}")

        # ---- routes ---------------------------------------------------------

        def handle_one_request(self):
            """Anything unforeseen leaves as a refusal rather than as a dropped
            connection, which a browser reports as a network fault and sends a
            reader looking in the wrong place."""
            try:
                super().handle_one_request()
            except (BrokenPipeError, ConnectionResetError):
                raise
            except Exception as why:            # noqa: BLE001 (this is the net)
                self.refuse(500, "the console failed at " + self.path + ": " +
                            repr(why))

        def do_GET(self):
            if not self.from_this_machine():
                return self.refuse(403, "this console answers this machine only")
            path = urllib.parse.urlparse(self.path).path
            if path in ("/", "/index.html"):
                return self.file(PAGE / "index.html")
            if path.startswith("/page/"):
                return self.file(PAGE / pathlib.Path(path).name)
            if path in ("/site/style.css", "/site/favicon.svg"):
                # The published page's stylesheet and icon, served from where
                # they already live so the console carries the project's tokens
                # rather than a second copy of them.
                return self.file(repository.ROOT / "site" / pathlib.Path(path).name)
            if path == "/api/setup":
                return self.send(200, service.setup())
            if path == "/api/fixture":
                asked = urllib.parse.parse_qs(urllib.parse.urlparse(self.path).query)
                try:
                    return self.send(200, {"text": repository.fixture_text(
                        (asked.get("name") or [""])[0])})
                except ValueError as why:
                    return self.refuse(404, why)
            if path == "/api/runs":
                return self.send(200, service.cards())
            if path.startswith("/api/runs/"):
                return self.run_route(path)
            if path.startswith("/api/flows/"):
                identifier = path.strip("/").split("/")[2]
                card = service.flow_card(identifier)
                if card is None:
                    return self.refuse(404, "no such flow: " + identifier)
                return self.send(200, card)
            return self.refuse(404, "no such page: " + path)

        def do_POST(self):
            if not self.from_this_machine():
                return self.refuse(403, "this console answers this machine only")
            path = urllib.parse.urlparse(self.path).path
            try:
                if path == "/api/preview":
                    return self.send(200, service.preview(self.read_request()))
                if path == "/api/runs":
                    return self.send(200, service.start(self.read_request()))
                if path == "/api/flows":
                    return self.send(200, service.start_flow(self.read_request()))
                if path.startswith("/api/runs/") and path.endswith("/stop"):
                    identifier = path.split("/")[3]
                    card = service.stop(identifier)
                    if card is None:
                        return self.refuse(404, "no such run: " + identifier)
                    return self.send(200, card)
            except command_line.Refused as why:
                return self.refuse(400, why)
            except repository.BuildNotFound as why:
                return self.refuse(400, why)
            except (ValueError, json.JSONDecodeError) as why:
                return self.refuse(400, why)
            return self.refuse(404, "no such page: " + path)

        def run_route(self, path):
            parts = path.strip("/").split("/")     # api runs <id> [files <name>]
            identifier = parts[2] if len(parts) > 2 else ""
            if len(parts) == 3:
                card = service.card(identifier)
                if card is None:
                    return self.refuse(404, "no such run: " + identifier)
                return self.send(200, card)
            if len(parts) == 5 and parts[3] == "files":
                try:
                    body = service.emitted(identifier,
                                           urllib.parse.unquote(parts[4]))
                except ValueError as why:
                    return self.refuse(404, why)
                self.send_response(200)
                self.send_header("Content-Type", "text/plain; charset=utf-8")
                self.send_header("Content-Disposition",
                                 'attachment; filename="' + parts[4] + '"')
                self.send_header("Content-Length", str(len(body)))
                self.end_headers()
                return self.wfile.write(body)
            return self.refuse(404, "no such page: " + path)

        def file(self, path):
            path = pathlib.Path(path)
            if not path.is_file():
                return self.refuse(404, "no such file: " + path.name)
            kind = mimetypes.guess_type(path.name)[0] or "text/plain"
            return self.send(200, path.read_bytes(), kind + "; charset=utf-8")

    return Handler
