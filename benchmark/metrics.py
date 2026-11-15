"""Measure the timed benchmark region and process peak RSS (GNU time)."""

import re
import subprocess
import tempfile
from pathlib import Path

TIME_RE = re.compile(rb"^BENCHMARK_NS=([0-9]+)$", re.MULTILINE)
GNU_TIME = Path("/usr/bin/time")


def measure(executable, input_bytes, expected, case, check_output, *, cwd):
    """Run once, validate output, and return (operation_ns, peak_rss_kib).

    Peak RSS is the maximum for the entire process lifetime, not just the
    internally timed operation. It includes inputs, answers, and allocator state.
    """
    if not GNU_TIME.is_file():
        raise RuntimeError("GNU time is required: install the 'time' package")
    with tempfile.TemporaryDirectory(prefix="bench-rss-") as directory:
        rss_file = Path(directory) / "rss.txt"
        result = subprocess.run(
            [str(GNU_TIME), "-f", "%M", "-o", str(rss_file), str(executable)],
            input=input_bytes, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
            cwd=cwd, timeout=180,
        )
        if result.returncode:
            raise RuntimeError(
                f"{case}: exit {result.returncode}: {result.stderr[-600:]!r}"
            )
        check_output(expected, result.stdout, case)
        durations = TIME_RE.findall(result.stderr)
        if len(durations) != 1 or int(durations[0]) <= 0:
            raise RuntimeError(
                f"{case}: expected one positive BENCHMARK_NS; stderr={result.stderr[:400]!r}"
            )
        try:
            rss_kib = int(rss_file.read_text().strip())
        except (OSError, ValueError) as exc:
            raise RuntimeError(f"{case}: invalid peak RSS from GNU time") from exc
        if rss_kib <= 0:
            raise RuntimeError(f"{case}: non-positive peak RSS: {rss_kib}")
        return int(durations[0]), rss_kib
