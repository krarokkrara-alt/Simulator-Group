"""Offline by default. Local mock results are not ESP32/Core transport evidence."""
import argparse
import http.client
import json
import re
import socket
import sys
import threading
from dataclasses import dataclass
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import urlsplit

BODY_CAP = 256
RESPONSE_CAP = 65536
DEFAULT_VOLATILE = {"uptime", "uptimeMs", "millis", "timestamp"}
PROTECTED_FIELDS = {"running", "rpm", "ckp", "cmp", "led", "scope", "selfTest", "profile", "tps", "map", "ect", "iat", "o2"}


@dataclass(frozen=True)
class Case:
    name: str
    method: str = "POST"
    target: str = "/api/set"
    body: bytes = b"run=0&rpm=0"
    content_type: str | None = "text/plain"
    length: str | None = "auto"
    extra_headers: tuple = ()
    status: int = 400
    allow_disconnect: bool = False
    initial_stop: bool = False


CASES = (
    Case("initial STOP/RPM0", status=200, initial_stop=True),
    Case("GET cannot mutate", method="GET", target="/api/set?run=0&unknown", body=b"", content_type=None, length=None, status=405),
    Case("valid POST", status=200),
    Case("query ignored including plain collision", target="/api/set?plain=bad&run=bad&unknown", status=200),
    Case("bare body segment", body=b"run=0&unknown"),
    Case("mixed malformed value", body=b"run=0&rpm=bad"),
    Case("duplicate body field", body=b"run=0&run=0"),
    Case("trailing delimiter", body=b"run=0&"),
    Case("raw NUL", body=b"run=0\x00"),
    Case("whitespace", body=b"run=0\n"),
    Case("extra equals", body=b"run=0=0"),
    Case("percent encoding", body=b"run=%30"),
    Case("oversized zero-valued body", body=b"rpm=" + b"0" * 253),
    Case("wrong content type", content_type="application/x-www-form-urlencoded", status=415),
    Case("missing content type", content_type=None, status=415),
    Case("missing content length", body=b"", length=None),
    Case("zero content length", body=b"", length="0"),
    Case("invalid content length", body=b"", length="bad", allow_disconnect=True),
    Case("negative content length", body=b"", length="-1", allow_disconnect=True),
    Case("truncated body", body=b"run=0", length="12", allow_disconnect=True),
    Case("nonempty transfer encoding", extra_headers=(("Transfer-Encoding", "identity"),)),
)


def endpoint(base_url, allow_nonlocal=False):
    parsed = urlsplit(base_url)
    if parsed.scheme != "http" or not parsed.hostname or parsed.username or parsed.password or parsed.path not in ("", "/") or parsed.query or parsed.fragment:
        raise ValueError("Use an explicit http://host:port root URL without credentials/query.")
    if parsed.hostname != "127.0.0.1" and not allow_nonlocal:
        raise ValueError("Only 127.0.0.1 is allowed by default. Nonlocal targets need --allow-nonlocal and separate manual authorization.")
    return parsed.hostname, parsed.port or 80


def request(address, case, timeout):
    headers = [("Host", f"{address[0]}:{address[1]}"), ("Connection", "close")]
    if case.content_type is not None:
        headers.append(("Content-Type", case.content_type))
    if case.length is not None:
        headers.append(("Content-Length", str(len(case.body)) if case.length == "auto" else case.length))
    headers.extend(case.extra_headers)
    head = f"{case.method} {case.target} HTTP/1.1\r\n" + "".join(f"{key}: {value}\r\n" for key, value in headers) + "\r\n"
    payload = head.encode("ascii") + case.body
    if len(payload) > 2048:
        raise ValueError("Fixture request byte limit exceeded.")
    with socket.create_connection(address, timeout=timeout) as connection:
        connection.settimeout(timeout)
        connection.sendall(payload)
        # End the request stream; a truncated body never waits for us to send more.
        connection.shutdown(socket.SHUT_WR)
        response = http.client.HTTPResponse(connection)
        try:
            response.begin()
            body = response.read(RESPONSE_CAP + 1)
            if len(body) > RESPONSE_CAP:
                raise ValueError("Response exceeds byte limit.")
            return response.status, body
        except (http.client.RemoteDisconnected, http.client.IncompleteRead, socket.timeout, ConnectionResetError) as error:
            if case.allow_disconnect:
                return None, str(error).encode("utf-8")
            raise


def state_snapshot(address, timeout):
    code, body = request(address, Case("state", method="GET", target="/api/state", body=b"", content_type=None, length=None, status=200), timeout)
    if code != 200:
        raise AssertionError(f"State endpoint status {code}")
    state = json.loads(body)
    if not isinstance(state, dict) or "running" not in state or "rpm" not in state:
        raise AssertionError("State JSON lacks running/rpm fields.")
    if state.get("selfTest", False):
        raise AssertionError("SELF TEST is active; wait until it ends before running regression.")
    return state


def stable(state, excluded):
    return {key: value for key, value in state.items() if key not in excluded}


def run_suite(address, timeout, excluded):
    results = []
    for case in CASES:
        before = state_snapshot(address, timeout)
        code, body = request(address, case, timeout)
        after = state_snapshot(address, timeout)
        if code != case.status and not (code is None and case.allow_disconnect):
            raise AssertionError(f"{case.name}: expected {case.status}, received {code}; body={body[:160]!r}")
        before_stable, after_stable = stable(before, excluded), stable(after, excluded)
        if case.initial_stop:
            before_stable.pop("running", None)
            before_stable.pop("rpm", None)
            after_stable.pop("running", None)
            after_stable.pop("rpm", None)
        if before_stable != after_stable:
            raise AssertionError(f"{case.name}: state changed unexpectedly: before={before_stable}, after={after_stable}")
        if after["running"] is not False or after["rpm"] != 0:
            raise AssertionError(f"{case.name}: expected STOP/RPM0 snapshot.")
        results.append({"name": case.name, "status": code,
                        "snapshot_unchanged": stable(before, excluded) == stable(after, excluded),
                        "expected_fields_unchanged": True,
                        "allowed_state_changes": ["running", "rpm"] if case.initial_stop else [],
                        "disconnect_allowed": case.allow_disconnect})
    return results


class MockHandler(BaseHTTPRequestHandler):
    """A deliberately independent localhost fixture, not the Core WebServer parser."""
    state = {"running": False, "rpm": 0, "ckp": True, "cmp": True, "led": False, "scope": False, "selfTest": False, "profile": 0, "tps": 10}
    counter = 0

    def log_message(self, *_):
        pass

    def send_json(self, code, data):
        body = json.dumps(data).encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        if self.path.split("?")[0] == "/api/state":
            type(self).counter += 1
            self.send_json(200, {**self.state, "uptimeMs": self.counter})
        elif self.path.split("?")[0] == "/api/set":
            self.send_json(405, {"error": "POST required"})
        else:
            self.send_json(404, {"error": "not found"})

    def do_POST(self):
        if self.path.split("?")[0] != "/api/set":
            self.send_json(404, {"error": "not found"})
            return
        if self.headers.get("Content-Type", "").split(";")[0].strip() != "text/plain":
            self.send_json(415, {"error": "text/plain required"})
            return
        text = self.headers.get("Content-Length", "")
        if self.headers.get("Transfer-Encoding", "") or not re.fullmatch(r"[0-9]+", text) or not 1 <= int(text) <= BODY_CAP:
            self.send_json(400, {"error": "framing"})
            return
        body = self.rfile.read(int(text))
        if len(body) != int(text) or not all(33 <= value <= 126 for value in body):
            self.send_json(400, {"error": "body"})
            return
        pairs = body.split(b"&")
        fields = {}
        for pair in pairs:
            if not re.fullmatch(rb"(?:run|rpm)=0", pair):
                self.send_json(400, {"error": "fixture accepts only STOP/RPM0"})
                return
            key = pair.split(b"=")[0]
            if key in fields:
                self.send_json(400, {"error": "duplicate"})
                return
            fields[key] = 0
        if b"run" in fields:
            self.state["running"] = False
        if b"rpm" in fields:
            self.state["rpm"] = 0
        self.send_json(200, self.state)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--dry-run", action="store_true", help="Default: list planned requests; no sockets.")
    mode.add_argument("--execute", action="store_true", help="Explicitly execute against --base-url.")
    mode.add_argument("--self-test", action="store_true", help="Run only the ephemeral 127.0.0.1 fixture.")
    parser.add_argument("--base-url")
    parser.add_argument("--allow-nonlocal", action="store_true", help="Nonlocal targets require manual authorization outside this tool.")
    parser.add_argument("--timeout", type=float, default=3.0)
    parser.add_argument("--ignore-state-field", action="append", default=[])
    parser.add_argument("--report", help="Optional JSON report file; nothing is written by default.")
    args = parser.parse_args()
    exit_code = 0
    if not 0.1 <= args.timeout <= 10:
        parser.error("Timeout must be 0.1–10 seconds.")
    excluded = DEFAULT_VOLATILE | set(args.ignore_state_field)
    if excluded & PROTECTED_FIELDS:
        parser.error("Command/state fields cannot be excluded from snapshot comparisons.")
    if args.self_test and (args.base_url or args.allow_nonlocal):
        parser.error("--self-test only permits the internal 127.0.0.1 fixture.")
    if not args.execute and not args.self_test:
        report = {"mode": "dry-run", "network_access": False, "board_access": False, "cases": [{"name": c.name, "method": c.method, "target": c.target, "body_bytes": len(c.body), "expected_status": c.status} for c in CASES]}
    else:
        fixture = None
        target = args.base_url
        try:
            if args.self_test:
                fixture = ThreadingHTTPServer(("127.0.0.1", 0), MockHandler)
                threading.Thread(target=fixture.serve_forever, daemon=True).start()
                address = fixture.server_address
                target = f"http://127.0.0.1:{address[1]}"
            else:
                if not args.base_url:
                    parser.error("--execute requires an explicit --base-url.")
                address = endpoint(args.base_url, args.allow_nonlocal)
                target = args.base_url
            results = run_suite(address, args.timeout, excluded)
            report = {"mode": "localhost-mock" if args.self_test else "explicit-target", "target": target, "passed": True, "core_transport_verified": False, "hardware_verified": False, "cases": results}
        except Exception as error:
            exit_code = 1
            report = {"mode": "localhost-mock" if args.self_test else "explicit-target", "target": target,
                      "passed": False, "error": str(error), "core_transport_verified": False, "hardware_verified": False}
        finally:
            if fixture:
                fixture.shutdown()
                fixture.server_close()
    output = json.dumps(report, ensure_ascii=False, indent=2)
    print(output)
    if args.report:
        with open(args.report, "w", encoding="utf-8") as destination:
            destination.write(output + "\n")
    return exit_code


if __name__ == "__main__":
    sys.exit(main())
