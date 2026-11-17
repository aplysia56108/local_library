"""The A/B workflow must persist only measured main baselines."""

import hashlib
import io
import json
import os
import sys
import tempfile
import unittest
from contextlib import redirect_stdout
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import compare
import run as benchmark


class ABMainRecordTest(unittest.TestCase):
    def test_main_medians_recorded_not_development_or_estimated_bests(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            main_root = root / "main"
            dev_root = root / "dev"
            source = main_root / "benchmark/btree/vector/iterate/benchmark.cpp"
            source.parent.mkdir(parents=True)
            source.write_text("// placeholder benchmark source\n")
            dev_root.mkdir()
            build = main_root / "out/benchmark"
            baseline_path = root / "baseline.json"
            baseline_path.write_text('{"schema":1,"records":{}}\n')

            samples = {
                "main": iter([(120, 3000), (100, 3200), (110, 3100)]),
                "dev": iter([(60, 1500), (70, 1400), (50, 1600)]),
            }

            def fake_measure(executable, input_bytes, expected, case,
                             check_output, *, cwd):
                self.assertEqual(cwd, main_root)
                self.assertEqual(input_bytes, b"input")
                self.assertEqual(expected, b"output")
                return next(samples[case.split("/", 1)[0]])

            def fake_check_output(command, *, cwd=None, text=False):
                if command[0] == "git":
                    return "a" * 40 if cwd == main_root else "b" * 40
                if command[:2] == ["g++", "--version"]:
                    return "g++ (GCC) 15.2.0\n"
                raise AssertionError(command)

            argv = [
                "compare.py", "--base-root", str(main_root),
                "--dev-root", str(dev_root),
                "--target", "btree/vector/iterate",
                "--caps", "8", "--runs", "3",
                "--baseline", str(baseline_path),
            ]
            with (
                patch.multiple(
                    benchmark, ROOT=main_root, BENCHMARKS=main_root / "benchmark",
                    BUILD=build, choose_cases=lambda target: [source],
                    parse_caps=lambda caps: [8],
                    load_fixtures=lambda source, cache: (b"input", b"output"),
                    cpu_model=lambda: "Test CPU",
                ),
                patch.object(compare, "compile_variant") as compile_mock,
                patch.object(compare, "measure", side_effect=fake_measure),
                patch.object(compare.subprocess, "check_output",
                             side_effect=fake_check_output),
                patch.object(sys, "argv", argv),
                patch.dict(os.environ, {
                    "GITHUB_REPOSITORY": "example/local_library",
                    "GITHUB_RUN_ID": "123",
                }, clear=True),
                redirect_stdout(io.StringIO()),
            ):
                compare.main()

            self.assertEqual(compile_mock.call_count, 2)
            saved = json.loads((build / "best-updated.json").read_text())
            records = saved["records"]["btree/vector/iterate/cap8"]
            self.assertEqual(len(records), 1)
            record = next(iter(records.values()))
            self.assertEqual(record["best_ns"], 110)
            self.assertEqual(record["best_rss_kb"], 3100)
            self.assertEqual(record["commit"], "a" * 40)
            self.assertEqual(record["rss_commit"], "a" * 40)
            self.assertEqual(
                record["run_url"], "https://github.com/example/local_library/actions/runs/123"
            )
            self.assertEqual(record["fingerprint"]["cpu"], "Test CPU")
            self.assertEqual(record["fingerprint"]["flags"],
                             [*benchmark.FLAGS, "-DNODE_CAP=8"])
            self.assertEqual(
                record["fingerprint"]["fixture_sha256"],
                hashlib.sha256(b"input\0output").hexdigest(),
            )
            results = json.loads((build / "compare/results.json").read_text())
            self.assertEqual(results["results"][0]["base_ns"], 110)
            self.assertEqual(results["results"][0]["dev_ns"], 60)
            self.assertEqual(results["results"][0]["base_rss_kib"], 3100)
            self.assertEqual(results["results"][0]["dev_rss_kib"], 1500)

if __name__ == "__main__":
    unittest.main()
