"""Tool safety/assertion tests only; no Core parser, socket or board required."""
import contextlib
import io
import json
import unittest
from unittest import mock
import http_regression as tool


class ToolTests(unittest.TestCase):
    def test_default_never_opens_socket(self):
        output = io.StringIO()
        with mock.patch("sys.argv", ["http_regression.py"]), mock.patch.object(tool.socket, "create_connection", side_effect=AssertionError("network forbidden")), contextlib.redirect_stdout(output):
            self.assertEqual(tool.main(), 0)
        self.assertIs(json.loads(output.getvalue())["network_access"], False)

    def test_execute_needs_explicit_url(self):
        with mock.patch("sys.argv", ["http_regression.py", "--execute"]), mock.patch.object(tool.socket, "create_connection", side_effect=AssertionError("network forbidden")), contextlib.redirect_stderr(io.StringIO()):
            with self.assertRaises(SystemExit) as error:
                tool.main()
            self.assertEqual(error.exception.code, 2)

    def test_nonlocal_denied_by_default(self):
        with self.assertRaises(ValueError):
            tool.endpoint("http://192.0.2.1")  # Documentation-only address; no connection.

    def test_rejection_with_changed_state_is_failure(self):
        before = {"running": False, "rpm": 0, "tps": 10}
        after = {**before, "tps": 11}
        case = tool.Case("invalid body", body=b"run=0&rpm=bad")
        with mock.patch.object(tool, "CASES", (case,)), mock.patch.object(tool, "state_snapshot", side_effect=[before, after]), mock.patch.object(tool, "request", return_value=(400, b"{}")):
            with self.assertRaisesRegex(AssertionError, "state changed unexpectedly"):
                tool.run_suite(("127.0.0.1", 1), 1, tool.DEFAULT_VOLATILE)

    def test_accepted_invalid_body_is_failure(self):
        state = {"running": False, "rpm": 0}
        case = tool.Case("invalid body", body=b"run=0&rpm=bad")
        with mock.patch.object(tool, "CASES", (case,)), mock.patch.object(tool, "state_snapshot", return_value=state), mock.patch.object(tool, "request", return_value=(200, b"{}")):
            with self.assertRaisesRegex(AssertionError, "expected 400"):
                tool.run_suite(("127.0.0.1", 1), 1, tool.DEFAULT_VOLATILE)

    def test_failed_execution_returns_nonzero_and_failed_json(self):
        output = io.StringIO()
        with mock.patch("sys.argv", ["http_regression.py", "--execute", "--base-url", "http://127.0.0.1:1"]), mock.patch.object(tool, "run_suite", side_effect=AssertionError("injected tool failure")), contextlib.redirect_stdout(output):
            self.assertEqual(tool.main(), 1)
        self.assertIs(json.loads(output.getvalue())["passed"], False)


if __name__ == "__main__":
    unittest.main()
