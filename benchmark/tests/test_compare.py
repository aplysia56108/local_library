"""Compile Main's benchmark sources against different library checkouts."""

import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from compare import compile_variant


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

    def test_requires_absolute_main_source(self):
        with self.assertRaisesRegex(ValueError, "absolute"):
            compile_variant(Path("benchmark.cpp"), Path("."), Path("out"),
                            ["-I."])


if __name__ == "__main__":
    unittest.main()
