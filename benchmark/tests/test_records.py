"""Tests for the shared measured-best update path."""

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import run as benchmark


class BestRecordTest(unittest.TestCase):
    def test_time_and_rss_update_independently_and_preserve_provenance(self):
        baseline = {"schema": 1, "records": {}}
        case = "btree/vector/iterate/cap16"
        fingerprint_data = {
            "cpu": "CPU A",
            "compiler": "g++ 15.2",
            "flags": [*benchmark.FLAGS, "-DNODE_CAP=16"],
            "fixture_sha256": "fixed-fixture",
        }

        key, old_time, old_rss = benchmark.update_best_record(
            baseline, case, fingerprint_data, 100, 4000,
            commit="main-one", run_url="run-one",
        )
        self.assertIsNone(old_time)
        self.assertIsNone(old_rss)
        record = baseline["records"][case][key]
        self.assertEqual(record["best_ns"], 100)
        self.assertEqual(record["best_rss_kb"], 4000)
        self.assertEqual(record["commit"], "main-one")
        self.assertEqual(record["rss_commit"], "main-one")

        key_again, old_time, old_rss = benchmark.update_best_record(
            baseline, case, fingerprint_data, 110, 3500,
            commit="main-two", run_url="run-two",
        )
        self.assertEqual(key_again, key)
        self.assertEqual((old_time, old_rss), (100, 4000))
        record = baseline["records"][case][key]
        self.assertEqual(record["best_ns"], 100)
        self.assertEqual(record["best_rss_kb"], 3500)
        self.assertEqual(record["commit"], "main-one")
        self.assertEqual(record["run_url"], "run-one")
        self.assertEqual(record["rss_commit"], "main-two")
        self.assertEqual(record["rss_run_url"], "run-two")

        benchmark.update_best_record(
            baseline, case, fingerprint_data, 90, 3600,
            commit="main-three", run_url="run-three",
        )
        record = baseline["records"][case][key]
        self.assertEqual(record["best_ns"], 90)
        self.assertEqual(record["best_rss_kb"], 3500)
        self.assertEqual(record["commit"], "main-three")
        self.assertEqual(record["rss_commit"], "main-two")
        unchanged = dict(record)

        benchmark.update_best_record(
            baseline, case, fingerprint_data, 120, 4500,
            commit="main-four", run_url="run-four",
        )
        self.assertEqual(baseline["records"][case][key], unchanged)

    def test_legacy_time_only_record_adds_rss_without_changing_time(self):
        baseline = {"schema": 1, "records": {}}
        data = {"cpu": "CPU", "compiler": "gcc", "flags": ["-O2"],
                "fixture_sha256": "fixture"}
        case = "btree/set/insert/cap8"
        key, _, _ = benchmark.update_best_record(
            baseline, case, data, 100, 4000,
            commit="old-main", run_url="old-url",
        )
        legacy = baseline["records"][case][key]
        del legacy["best_rss_kb"]
        del legacy["rss_commit"]
        del legacy["rss_run_url"]

        benchmark.update_best_record(
            baseline, case, data, 120, 3000,
            commit="new-main", run_url="new-url",
        )
        record = baseline["records"][case][key]
        self.assertEqual(record["best_ns"], 100)
        self.assertEqual(record["commit"], "old-main")
        self.assertEqual(record["best_rss_kb"], 3000)
        self.assertEqual(record["rss_commit"], "new-main")


if __name__ == "__main__":
    unittest.main()
