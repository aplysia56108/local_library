"""Lightweight smoke tests for the benchmark measurement and historical lookup."""

import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import compare
from metrics import measure


class MetricsTest(unittest.TestCase):
    def test_peak_rss_and_timed_output(self):
        with tempfile.TemporaryDirectory() as directory:
            executable = Path(directory) / "bench.sh"
            executable.write_text(
                "#!/bin/sh\nprintf 'answer\\n'\n"
                "printf 'BENCHMARK_NS=123456\\n' >&2\n"
            )
            executable.chmod(0o755)

            def check(expected, actual, case):
                self.assertEqual(expected, actual)

            ns, rss = measure(
                executable, b"", b"answer\n", "example", check, cwd=Path(directory)
            )
            self.assertEqual(ns, 123456)
            self.assertGreater(rss, 0)

    def test_rejects_incorrect_output(self):
        with tempfile.TemporaryDirectory() as directory:
            executable = Path(directory) / "bench.sh"
            executable.write_text(
                "#!/bin/sh\nprintf 'wrong\\n'\n"
                "printf 'BENCHMARK_NS=123456\\n' >&2\n"
            )
            executable.chmod(0o755)

            def check(expected, actual, case):
                if expected != actual:
                    raise RuntimeError("mismatch")

            with self.assertRaisesRegex(RuntimeError, "mismatch"):
                measure(executable, b"", b"answer\n", "example", check,
                        cwd=Path(directory))


class HistoricalBestTest(unittest.TestCase):
    def test_cpu_preference_and_missing_legacy_rss(self):
        flags = ["-O2", "-DNODE_CAP=16"]

        def record(cpu, ns, rss=None, fixture="abc", compiler="gcc15"):
            return {
                "fingerprint": {
                    "cpu": cpu, "compiler": compiler, "flags": flags,
                    "fixture_sha256": fixture,
                },
                "best_ns": ns,
                **({"best_rss_kb": rss} if rss is not None else {}),
            }

        records = {"btree/vector/access/cap16": {
            "a": record("CPU A", 50),
            "b": record("CPU B", 60, 40000),
            "c": record("CPU C", 20, 30000, fixture="different"),
            "d": record("CPU C", 10, 10000, compiler="wrong"),
        }}
        t, m = compare.historical_best(
            records, "btree/vector/access/cap16", "gcc15", flags, "abc", "CPU B"
        )
        self.assertEqual(t["best_ns"], 60)
        self.assertEqual(m["best_rss_kb"], 40000)
        self.assertEqual(t["fingerprint"]["cpu"], "CPU B")

        t, m = compare.historical_best(
            records, "btree/vector/access/cap16", "gcc15", flags, "abc", "CPU Z"
        )
        self.assertEqual(t["best_ns"], 50)
        self.assertEqual(m["best_rss_kb"], 40000)


if __name__ == "__main__":
    unittest.main()
