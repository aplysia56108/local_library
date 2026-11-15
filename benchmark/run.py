#!/usr/bin/env python3
"""Run manually selected benchmarks and compare valid runs with saved bests."""

import argparse
import hashlib
import json
import os
import re
import statistics
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BENCHMARKS = ROOT / "benchmark"
BUILD = ROOT / "out" / "benchmark"
FLAGS = ["-std=c++23", "-O2", "-Wall", "-Wextra", "-I.", "-Idebug", "-I.deps/ac-library"]
TIME_RE = re.compile(rb"^BENCHMARK_NS=([0-9]+)$", re.MULTILINE)


def cpu_model():
    try:
        for line in Path("/proc/cpuinfo").read_text().splitlines():
            if line.startswith("model name"):
                return line.split(":", 1)[1].strip()
    except OSError:
        pass
    return "unknown CPU"


def choose_cases(target):
    cases = sorted(BENCHMARKS.rglob("benchmark.cpp"))
    if not cases:
        raise RuntimeError("No benchmark.cpp found")
    if target != "all":
        cases = [source for source in cases if source.parent.relative_to(BENCHMARKS).as_posix() == target]
        if not cases:
            raise RuntimeError(f"Unknown benchmark target: {target}")
    return cases


def check_output(expected, actual, case):
    if actual == expected:
        return
    want = expected.splitlines()
    got = actual.splitlines()
    for i, (a, b) in enumerate(zip(want, got), 1):
        if a != b:
            raise RuntimeError(f"{case}: output mismatch at line {i}: expected {a[:80]!r}, got {b[:80]!r}")
    raise RuntimeError(f"{case}: output mismatch (expected {len(want)} lines, got {len(got)} lines)")


def run_case(source, runs, baseline, metadata):
    case = source.parent.relative_to(BENCHMARKS).as_posix()
    fixture_in = source.parent / "in.txt"
    fixture_out = source.parent / "out.txt"
    fixture_generator = source.parent / "generator.cpp"
    for path in [fixture_in, fixture_out, fixture_generator]:
        if not path.is_file():
            raise RuntimeError(f"{case}: missing {path.name}")
    input_bytes = fixture_in.read_bytes()
    expected = fixture_out.read_bytes()
    fixture_sha = hashlib.sha256(input_bytes + b"\0" + expected).hexdigest()
    fingerprint_data = {
        **metadata,
        "fixture_sha256": fixture_sha,
    }
    fingerprint = hashlib.sha256(json.dumps(fingerprint_data, sort_keys=True).encode()).hexdigest()[:16]

    executable = BUILD / (case.replace("/", "_") + ".exe")
    compile_command = ["g++", *FLAGS, str(source.relative_to(ROOT)), "-o", str(executable)]
    subprocess.run(compile_command, cwd=ROOT, check=True, timeout=180)

    durations = []
    for iteration in range(runs):
        run = subprocess.run(
            [str(executable)], input=input_bytes, stdout=subprocess.PIPE,
            stderr=subprocess.PIPE, cwd=ROOT, timeout=180, check=True,
        )
        check_output(expected, run.stdout, case)
        times = TIME_RE.findall(run.stderr)
        if len(times) != 1 or int(times[0]) <= 0:
            raise RuntimeError(f"{case}: expected one positive BENCHMARK_NS on stderr; got {run.stderr[:400]!r}")
        durations.append(int(times[0]))
        print(f"{case}: {iteration + 1}/{runs} verified, {durations[-1] / 1e6:.3f} ms", flush=True)

    measured = statistics.median(durations)
    per_case = baseline.setdefault("records", {}).setdefault(case, {})
    old = per_case.get(fingerprint)
    best = old["best_ns"] if old else None
    if best is None or measured < best:
        per_case[fingerprint] = {
            "best_ns": measured,
            "fingerprint": fingerprint_data,
            "commit": os.getenv("GITHUB_SHA", "local"),
            "run_url": (
                f"https://github.com/{os.environ['GITHUB_REPOSITORY']}/actions/runs/{os.environ['GITHUB_RUN_ID']}"
                if os.getenv("GITHUB_REPOSITORY") and os.getenv("GITHUB_RUN_ID") else "local"
            ),
        }
    ratio = measured / best * 100 if best else None
    return {
        "case": case,
        "median_ns": measured,
        "samples_ns": durations,
        "previous_best_ns": best,
        "ratio_percent": ratio,
        "fingerprint": fingerprint,
    }


def render_summary(results, metadata):
    lines = [
        "## Performance benchmarks",
        "",
        f"CPU: `{metadata['cpu']}`  ",
        f"Compiler: `{metadata['compiler']}`  ",
        "The median of successful runs is measured inside benchmark.cpp (without input/output).",
        "",
        "| Case | Previous best | Current median | Best ratio | Status |",
        "| --- | ---: | ---: | ---: | --- |",
    ]
    for result in results:
        best = result["previous_best_ns"]
        ratio = result["ratio_percent"]
        if best is None:
            label = "🆕 New baseline"
            ratio_text = "—"
        elif ratio > 150:
            label = "🔴 Regression"
            ratio_text = f"{ratio:.1f}%"
        elif ratio > 120:
            label = "🟡 Caution"
            ratio_text = f"{ratio:.1f}%"
        else:
            label = "✅ OK"
            ratio_text = f"{ratio:.1f}%"
        lines.append(
            f"| `{result['case']}` | "
            f"{f'{best / 1e6:.3f} ms' if best is not None else '—'} | "
            f"{result['median_ns'] / 1e6:.3f} ms | {ratio_text} | {label} |"
        )
    lines += [
        "",
        "A baseline is only comparable when CPU, compiler, flags and fixture contents match.",
        "Red results are warnings, not workflow failures. Invalid outputs fail the workflow.",
        "",
    ]
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--target", default="all")
    parser.add_argument("--runs", type=int, default=5)
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
    results = [run_case(source, args.runs, baseline, metadata) for source in choose_cases(args.target)]

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
