"""Compile Main's benchmark sources against different library checkouts."""

import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from compare import compile_variant, record_main_best_if_applicable, summarize
import run as benchmark


class SharedBenchmarkSourceTest(unittest.TestCase):
    def test_main_workload_uses_each_library_version(self):
        """The development tree deliberately has no benchmark sources."""
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            main = root / "main"
            dev = root / "dev"
            source_dir = main / "benchmark" / "btree" / "vector" / "access_random"
            common_dir = main / "benchmark" / "btree"
            source_dir.mkdir(parents=True)
            dev.mkdir()
            source = source_dir / "benchmark.cpp"
            source.write_text('#include "../../benchmark_common.hpp"\n'
                              'signed main() { return version() == EXPECTED ? 0 : 1; }\n')
            (common_dir / "benchmark_common.hpp").write_text(
                '#include "data-structure/version.hpp"\n'
            )
            for checkout, version in ((main, 111), (dev, 222)):
                header = checkout / "data-structure" / "version.hpp"
                header.parent.mkdir(parents=True)
                header.write_text(f"inline long long version() {{ return {version}; }}\n")

            self.assertFalse((dev / "benchmark").exists())
            for checkout, expected in ((main, 111), (dev, 222)):
                executable = root / f"test-{expected}"
                # -I. must select the library in the compiler's cwd.
                compile_variant(
                    source, checkout, executable,
                    ["-std=c++23", "-O2", "-I.", f"-DEXPECTED={expected}"],
                )
                result = subprocess.run([str(executable)], check=False)
                self.assertEqual(result.returncode, 0)

    def test_benchmark_flags_disable_asserts(self):
        """Performance and A/B benchmarks share assert-free compilation flags."""
        self.assertIn("-DNDEBUG", benchmark.FLAGS)
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "benchmark.cpp"
            source.write_text(
                '#include <cassert>\n'
                '#ifndef NDEBUG\n'
                '#error "Benchmarks must compile with NDEBUG"\n'
                '#endif\n'
                'signed main() { assert(false); return 0; }\n'
            )
            executable = root / "benchmark"
            compile_variant(source, root, executable, benchmark.FLAGS)
            subprocess.run([str(executable)], check=True)

    def test_requires_absolute_main_source(self):
        with self.assertRaisesRegex(ValueError, "absolute"):
            compile_variant(Path("benchmark.cpp"), Path("."), Path("out"),
                            ["-I."])


    def test_two_non_main_checkouts_share_main_workload(self):
        """Both variants use main's source but independently selected headers."""
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source_root = root / "main"
            source = source_root / "benchmark" / "btree" / "vector" / "access" / "benchmark.cpp"
            source.parent.mkdir(parents=True)
            source.write_text(
                '#include "data-structure/version.hpp"\n'
                'signed main() { return version() == EXPECTED ? 0 : 1; }\n'
            )
            for name, value in (("old-feature", 111), ("new-feature", 222)):
                checkout = root / name
                header = checkout / "data-structure" / "version.hpp"
                header.parent.mkdir(parents=True)
                header.write_text(f"inline long long version() {{ return {value}; }}\n")
                self.assertFalse((checkout / "benchmark").exists())
                exe = root / f"test-{name}"
                compile_variant(
                    source, checkout, exe,
                    ["-std=c++23", "-O2", "-I.", f"-DEXPECTED={value}"],
                )
                subprocess.run([str(exe)], check=True)

    def test_only_main_baseline_updates_records(self):
        fingerprint = {
            "cpu": "test", "compiler": "g++", "flags": ["-DNDEBUG"],
            "fixture_sha256": "fixture",
        }
        records = {"schema": 1, "records": {}}
        with patch.object(benchmark, "update_best_record") as update:
            self.assertFalse(record_main_best_if_applicable(
                records, "btree/vector/access/cap8", fingerprint,
                1000, 2000, "feature-base", "main-commit",
            ))
            update.assert_not_called()
            self.assertTrue(record_main_best_if_applicable(
                records, "btree/vector/access/cap8", fingerprint,
                1000, 2000, "main-commit", "main-commit",
            ))
            update.assert_called_once_with(
                records, "btree/vector/access/cap8",
                fingerprint, 1000, 2000, commit="main-commit",
            )

    def test_non_main_summary_displays_both_refs_without_historical_best(self):
        results = [{
            "case": "btree/vector/access/cap8",
            "time_ratio": 0.9, "rss_ratio": 0.95,
            "base_ns": 1000000, "dev_ns": 900000,
            "base_rss_kib": 4096, "dev_rss_kib": 3891,
            "historical_best_ns": None, "historical_best_rss_kib": None,
            "estimated_dev_best_ns": None, "estimated_dev_best_rss_kib": None,
            "historical_best_cpu": None, "historical_best_commit": None,
        }]
        summary = summarize(
            results, {"cpu": "test CPU", "compiler": "g++"},
            "old-commit-sha", "new-commit-sha",
            base_label="feature/old", dev_label="feature/new",
            source_sha="main-commit-sha",
        )
        self.assertIn("Base: feature/old @ old-commit-s", summary)
        self.assertIn("Development: feature/new @ new-commit-s", summary)
        self.assertIn("Benchmark definitions: main @ main-commit-", summary)
        self.assertIn("benchmark-records is not updated", summary)
        self.assertIn("| Case | Base | Dev | Dev/Base |", summary)

    def test_workflow_guards_main_best_publication(self):
        workflow = (Path(__file__).resolve().parents[2] /
                    ".github/workflows/benchmark-compare.yml").read_text()
        self.assertIn("steps.comparison_refs.outputs.base_is_main == 'true'",
                      workflow)
        self.assertIn("--base-library-root", workflow)


if __name__ == "__main__":
    unittest.main()
