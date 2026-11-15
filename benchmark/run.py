#!/usr/bin/env python3
"""Run manually selected benchmarks and compare valid runs with saved bests."""

import argparse
import hashlib
import json
import os
import statistics
import subprocess
import sys
from pathlib import Path

from metrics import measure

ROOT = Path(__file__).resolve().parent.parent
BENCHMARKS = ROOT / "benchmark"
BUILD = ROOT / "out" / "benchmark"
FLAGS = ["-std=c++23", "-O2", "-Wall", "-Wextra", "-I.", "-Idebug", "-I.deps/ac-library"]


def cpu_model():
    try:
        for line in Path("/proc/cpuinfo").read_text().splitlines():
            if line.startswith("model name"):
                return line.split(":", 1)[1].strip()
    except OSError:
        pass
    return "unknown CPU"


def choose_cases(target):
    """Accept all, a directory prefix, or a specific operation directory."""
    cases = sorted(BENCHMARKS.rglob("benchmark.cpp"))
    if not cases:
        raise RuntimeError("No benchmark.cpp found")
    if target != "all":
        cases = [
            source for source in cases
            if (path := source.parent.relative_to(BENCHMARKS).as_posix()) == target
            or path.startswith(target + "/")
        ]
        if not cases:
            raise RuntimeError(f"Unknown benchmark target: {target}")
    return cases


def parse_caps(value):
    if value == "all":
        return [8, 16, 32, 64]
    try:
        caps = [int(part) for part in value.split(",")]
    except ValueError:
        raise ValueError(f"Invalid capacities: {value}") from None
    if not caps or any(cap not in (8, 16, 32, 64) for cap in caps):
        raise ValueError("Capacities must be 8, 16, 32, or 64")
    return list(dict.fromkeys(caps))


def check_output(expected, actual, case):
    if actual == expected:
        return
    want = expected.splitlines()
    got = actual.splitlines()
    for i, (a, b) in enumerate(zip(want, got), 1):
        if a != b:
            raise RuntimeError(f"{case}: output mismatch at line {i}: expected {a[:80]!r}, got {b[:80]!r}")
    raise RuntimeError(f"{case}: output mismatch (expected {len(want)} lines, got {len(got)} lines)")


def load_fixtures(source, cache):
    """Use committed fixtures, or generate both when omitted for large cases."""
    case_dir = source.parent
    if case_dir in cache:
        return cache[case_dir]
    fixture_in = case_dir / "in.txt"
    fixture_out = case_dir / "out.txt"
    generator = case_dir / "generator.cpp"
    if not generator.is_file():
        raise RuntimeError(f"{case_dir}: missing generator.cpp")
    if fixture_in.is_file() != fixture_out.is_file():
        raise RuntimeError(f"{case_dir}: in.txt and out.txt must both exist or both be absent")
    if fixture_in.is_file():
        data = (fixture_in.read_bytes(), fixture_out.read_bytes())
    else:
        relative = case_dir.relative_to(BENCHMARKS)
        generated_dir = BUILD / "generated-fixtures" / relative
        generated_dir.mkdir(parents=True, exist_ok=True)
        executable = BUILD / ("generator_" + "_".join(relative.parts) + ".exe")
        subprocess.run(
            ["g++", "-std=c++23", "-O2", str(generator.relative_to(ROOT)), "-o", str(executable)],
            cwd=ROOT, check=True, timeout=180,
        )
        subprocess.run([str(executable)], cwd=generated_dir, check=True, timeout=180)
        data = ((generated_dir / "in.txt").read_bytes(),
                (generated_dir / "out.txt").read_bytes())
        print(f"{relative}: generated deterministic fixtures", flush=True)
    cache[case_dir] = data
    return data



def run_case(source, cap, runs, baseline, metadata, fixture_cache):
    case = source.parent.relative_to(BENCHMARKS).as_posix() + f"/cap{cap}"
    input_bytes, expected = load_fixtures(source, fixture_cache)
    fixture_sha = hashlib.sha256(input_bytes + b"\\0" + expected).hexdigest()
    flags = [*FLAGS, f"-DNODE_CAP={cap}"]
    fingerprint_data = {
        **metadata,
        "flags": flags,
        "fixture_sha256": fixture_sha,
    }
    fingerprint = hashlib.sha256(json.dumps(fingerprint_data, sort_keys=True).encode()).hexdigest()[:16]

    executable = BUILD / (case.replace("/", "_") + ".exe")
    compile_command = ["g++", *flags, str(source.relative_to(ROOT)), "-o", str(executable)]
    subprocess.run(compile_command, cwd=ROOT, check=True, timeout=180)

    durations = []
    rss_peaks = []
    for iteration in range(runs):
        ns, rss_kib = measure(executable, input_bytes, expected, case, check_output, cwd=ROOT)
        durations.append(ns)
        rss_peaks.append(rss_kib)
        print(
            f"{case}: {iteration + 1}/{runs} verified, "
            f"{ns / 1e6:.3f} ms, {rss_kib / 1024:.2f} MiB peak RSS",
            flush=True,
        )

    measured = statistics.median(durations)
    rss_measured = statistics.median(rss_peaks)
    per_case = baseline.setdefault("records", {}).setdefault(case, {})
    old = per_case.get(fingerprint)
    best = old.get("best_ns") if old else None
    best_rss = old.get("best_rss_kb") if old else None
    record = dict(old or {})
    run_url = (
        f"https://github.com/{os.environ['GITHUB_REPOSITORY']}/actions/runs/{os.environ['GITHUB_RUN_ID']}"
        if os.getenv("GITHUB_REPOSITORY") and os.getenv("GITHUB_RUN_ID") else "local"
    )
    commit = os.getenv("GITHUB_SHA", "local")
    if best is None or measured < best:
        record.update({
            "best_ns": measured,
            "fingerprint": fingerprint_data,
            "commit": commit,
            "run_url": run_url,
        })
    if best_rss is None or rss_measured < best_rss:
        record.update({
            "best_rss_kb": rss_measured,
            "fingerprint": fingerprint_data,
            "rss_commit": commit,
            "rss_run_url": run_url,
        })
    if record != (old or {}):
        per_case[fingerprint] = record

    return {
        "case": case,
        "median_ns": measured,
        "samples_ns": durations,
        "previous_best_ns": best,
        "ratio_percent": measured / best * 100 if best else None,
        "median_rss_kb": rss_measured,
        "samples_rss_kb": rss_peaks,
        "previous_best_rss_kb": best_rss,
        "rss_ratio_percent": rss_measured / best_rss * 100 if best_rss else None,
        "fingerprint": fingerprint,
    }


def render_summary(results, metadata):
    def status(ratio):
        if ratio is None:
            return "🆕 New baseline"
        if ratio > 150:
            return "🔴 Regression"
        if ratio > 120:
            return "🟡 Caution"
        return "✅ OK"

    def fmt_ms(value):
        return f"{value / 1e6:.3f} ms" if value is not None else "—"

    def fmt_rss(value):
        return f"{value / 1024:.2f} MiB" if value is not None else "—"

    def fmt_ratio(value):
        return f"{value:.1f}%" if value is not None else "—"

    lines = [
        "## Performance benchmarks",
        "",
        f"CPU: \`{metadata['cpu']}\`  ",
        f"Compiler: \`{metadata['compiler']}\`  ",
        "The median of successful runs is measured inside benchmark.cpp (without input/output).",
        "Peak RSS is for the entire benchmark process, including input and output buffers.",
        "",
        "| Case | Previous time best | Time median | Time/best | Time status | "
        "Previous RSS best | Peak RSS median | RSS/best | RSS status |",
        "| --- | ---: | ---: | ---: | --- | ---: | ---: | ---: | --- |",
    ]
    for result in results:
        lines.append(
            f"| \`{result['case']}\` | "
            f"{fmt_ms(result['previous_best_ns'])} | "
            f"{fmt_ms(result['median_ns'])} | "
            f"{fmt_ratio(result['ratio_percent'])} | "
            f"{status(result['ratio_percent'])} | "
            f"{fmt_rss(result['previous_best_rss_kb'])} | "
            f"{fmt_rss(result['median_rss_kb'])} | "
            f"{fmt_ratio(result['rss_ratio_percent'])} | "
            f"{status(result['rss_ratio_percent'])} |"
        )
    lines += [
        "",
        "Time and RSS bests are tracked independently; each run stores both sample arrays.",
        "A baseline is only comparable when CPU, compiler, flags and fixture contents match.",
        "Red results are warnings, not workflow failures. Invalid outputs fail the workflow.",
        "",
    ]
    return "\\n".join(lines)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--target", default="all")
    parser.add_argument("--runs", type=int, default=5)
    parser.add_argument("--caps", default="all", help="all or comma-separated capacities (8,16,32,64)")
    parser.add_argument("--baseline", type=Path, default=BUILD / "best.json")
    parser.add_argument("--updated", type=Path, default=BUILD / "best-updated.json")
    args = parser.parse_args()
    if args.runs < 1 or args.runs % 2 == 0:
        parser.error("--runs must be a positive odd number")

    BUILD.mkdir(parents=True, exist_ok=True)
    if args.baseline.exists():
        baseline = json.loads(args.baseline.read_text())
    else:
        baseline = {"schema": 1, "records": {}}
    if baseline.get("schema") != 1:
        raise RuntimeError("Unsupported benchmark best record format")

    compiler = subprocess.check_output(["g++", "--version"], text=True).splitlines()[0]
    metadata = {"cpu": cpu_model(), "compiler": compiler, "flags": FLAGS}
    caps = parse_caps(args.caps)
    # Keep each operation's four capacity variants on the same runner.
    fixture_cache = {}
    results = [
        run_case(source, cap, args.runs, baseline, metadata, fixture_cache)
        for source in choose_cases(args.target)
        for cap in caps
    ]

    args.updated.parent.mkdir(parents=True, exist_ok=True)
    args.updated.write_text(json.dumps(baseline, indent=2, ensure_ascii=False, sort_keys=True) + "\n")
    (BUILD / "results.json").write_text(json.dumps(results, indent=2, ensure_ascii=False) + "\n")
    summary = render_summary(results, metadata)
    print(summary)
    if os.getenv("GITHUB_STEP_SUMMARY"):
        with open(os.environ["GITHUB_STEP_SUMMARY"], "a") as f:
            f.write(summary + "\n")


if __name__ == "__main__":
    try:
        main()
    except (RuntimeError, subprocess.CalledProcessError, subprocess.TimeoutExpired, OSError, ValueError) as exc:
        print(f"Benchmark failed: {exc}", file=sys.stderr)
        sys.exit(1)
