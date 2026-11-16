#!/usr/bin/env python3
"""Compare libraries using Main's benchmark sources and fixtures on one runner."""

import argparse
import hashlib
import json
import math
import os
import statistics
import subprocess
import sys
from pathlib import Path

import run as benchmark
from metrics import measure


def historical_best(records, case, compiler, flags, fixture_sha, cpu):
    """Compatible historical main bests; CPU may differ, other factors may not."""
    candidates = []
    for entry in records.get(case, {}).values():
        fp = entry.get("fingerprint", {})
        if (fp.get("compiler") == compiler and fp.get("flags") == flags
                and fp.get("fixture_sha256") == fixture_sha):
            candidates.append(entry)

    def select(field):
        valid = [entry for entry in candidates if entry.get(field) is not None]
        if not valid:
            return None
        # Prefer same-CPU records; use a different CPU only if necessary.
        exact = [entry for entry in valid if entry["fingerprint"].get("cpu") == cpu]
        return min(exact or valid, key=lambda entry: entry[field])

    return select("best_ns"), select("best_rss_kb")


def summarize(results, metadata, base_sha, dev_sha):
    def geometric_ratio(rows, field):
        return math.exp(sum(math.log(r[field]) for r in rows) / len(rows))

    lines = [
        "## Performance A/B comparison", "",
        f"Base: main @ {base_sha[:12]}  ",
        f"Development: {dev_sha[:12]}  ",
        f"CPU: {metadata['cpu']}  ",
        f"Compiler: {metadata['compiler']}  ", "",
        "**Below 100% is better.** Both versions were verified against the same",
        "expected output and run in alternating order on this runner.",
        "Benchmark sources and fixtures are from Main; library headers use each variant's checkout.",
        "Peak RSS measures the whole process, including buffers and allocator overhead.",
        "",
        "### Summary by structure", "",
        "| Structure | Cases | Runtime dev/main | Peak RSS dev/main | Median RSS saved |",
        "| --- | ---: | ---: | ---: | ---: |",
    ]
    for kind in sorted({r["case"].split("/")[1] for r in results}):
        subset = [r for r in results if r["case"].split("/")[1] == kind]
        saved_mib = statistics.median(
            (r["base_rss_kib"] - r["dev_rss_kib"]) / 1024 for r in subset
        )
        lines.append(
            f"| {kind} | {len(subset)} | "
            f"{geometric_ratio(subset, 'time_ratio') * 100:.1f}% | "
            f"{geometric_ratio(subset, 'rss_ratio') * 100:.1f}% | "
            f"{saved_mib:+.1f} MiB |"
        )

    def fmt_ms(v):
        return f"{v / 1e6:.3f} ms" if v is not None else "—"

    def fmt_mib(v):
        return f"{v / 1024:.2f} MiB" if v is not None else "—"

    lines += [
        "", "### Time", "",
        "| Case | Main | Dev | Dev/Main | Historical main best | Estimated dev best |",
        "| --- | ---: | ---: | ---: | ---: | ---: |",
    ]
    for r in results:
        lines.append(
            f"| {r['case']} | {fmt_ms(r['base_ns'])} | {fmt_ms(r['dev_ns'])} | "
            f"{r['time_ratio'] * 100:.1f}% | {fmt_ms(r['historical_best_ns'])} | "
            f"{fmt_ms(r['estimated_dev_best_ns'])} |"
        )

    lines += [
        "", "### Peak RSS", "",
        "| Case | Main | Dev | Dev/Main | Historical main best | Estimated dev best |",
        "| --- | ---: | ---: | ---: | ---: | ---: |",
    ]
    for r in results:
        lines.append(
            f"| {r['case']} | {fmt_mib(r['base_rss_kib'])} | "
            f"{fmt_mib(r['dev_rss_kib'])} | {r['rss_ratio'] * 100:.1f}% | "
            f"{fmt_mib(r['historical_best_rss_kib'])} | "
            f"{fmt_mib(r['estimated_dev_best_rss_kib'])} |"
        )

    foreign = sum(bool(r["historical_best_cpu"] and
                       r["historical_best_cpu"] != metadata["cpu"]) for r in results)
    old_commit = sum(bool(r["historical_best_commit"] and
                          r["historical_best_commit"] != base_sha) for r in results)
    lines += [
        "",
        "**Estimated dev best = historical main best × (measured dev / measured main).**",
        "Estimates are extrapolations, not measured records. Historical records",
        "must match compiler, flags and fixture; an exact CPU is preferred.",
        f"Estimated time based on a different CPU: {foreign} cases.",
        f"Historical time record from a different main commit: {old_commit} cases.",
        "A/B ratios may not transfer to different CPU models or code revisions.",
        "Raw samples and historical source metadata are saved in compare/results.json.",
        "This workflow never updates benchmark-records.", "",
    ]
    return "\n".join(lines)


def compile_variant(source, library_root, executable, flags):
    """Build Main's workload with headers from the selected library checkout.

    The absolute source path keeps relative benchmark includes on Main.
    The compiler's working directory and -I. resolve data-structure/
    headers to the selected checkout, not to Main's implementation.
    No benchmark files are required in the development checkout.
    """
    if not source.is_absolute():
        raise ValueError("Benchmark source must be an absolute path from Main")
    subprocess.run(
        ["g++", *flags, str(source), "-o", str(executable)],
        cwd=library_root, check=True, timeout=180,
    )


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--base-root", type=Path, required=True)
    parser.add_argument("--dev-root", type=Path, required=True)
    parser.add_argument("--target", default="all")
    parser.add_argument("--caps", default="all")
    parser.add_argument("--runs", type=int, default=5)
    parser.add_argument("--baseline", type=Path, required=True)
    args = parser.parse_args()
    if args.runs < 1 or args.runs % 2 == 0:
        parser.error("--runs must be a positive odd number")

    base_root, dev_root = args.base_root.resolve(), args.dev_root.resolve()
    if base_root != benchmark.ROOT:
        raise RuntimeError("--base-root must match the main checkout containing compare.py")
    base_sha = subprocess.check_output(
        ["git", "rev-parse", "HEAD"], cwd=base_root, text=True
    ).strip()
    dev_sha = subprocess.check_output(
        ["git", "rev-parse", "HEAD"], cwd=dev_root, text=True
    ).strip()
    baseline = json.loads(args.baseline.read_text())
    if baseline.get("schema") != 1:
        raise RuntimeError("Unsupported historical best-record format")
    compiler = subprocess.check_output(["g++", "--version"], text=True).splitlines()[0]
    metadata = {"cpu": benchmark.cpu_model(), "compiler": compiler}
    build = benchmark.BUILD / "compare"
    build.mkdir(parents=True, exist_ok=True)
    fixture_cache, results = {}, []

    for source in benchmark.choose_cases(args.target):
        relative_dir = source.parent.relative_to(benchmark.BENCHMARKS)
        input_bytes, expected = benchmark.load_fixtures(source, fixture_cache)
        fixture_sha = hashlib.sha256(input_bytes + b"\0" + expected).hexdigest()
        for cap in benchmark.parse_caps(args.caps):
            case = relative_dir.as_posix() + f"/cap{cap}"
            flags = [*benchmark.FLAGS, f"-DNODE_CAP={cap}"]
            executables = {}
            for variant, root in (("main", base_root), ("dev", dev_root)):
                exe = build / (variant + "_" + case.replace("/", "_") + ".exe")
                compile_variant(source, root, exe, flags)
                executables[variant] = exe

            samples = {
                "main": {"ns": [], "rss_kib": []},
                "dev": {"ns": [], "rss_kib": []},
            }
            for iteration in range(args.runs):
                order = ("main", "dev") if iteration % 2 == 0 else ("dev", "main")
                for variant in order:
                    ns, rss = measure(
                        executables[variant], input_bytes, expected,
                        f"{variant}/{case}", benchmark.check_output, cwd=base_root,
                    )
                    samples[variant]["ns"].append(ns)
                    samples[variant]["rss_kib"].append(rss)
                    print(
                        f"{case}: {variant} {iteration + 1}/{args.runs} verified, "
                        f"{ns / 1e6:.3f} ms, {rss / 1024:.2f} MiB", flush=True,
                    )

            main_ns = statistics.median(samples["main"]["ns"])
            dev_ns = statistics.median(samples["dev"]["ns"])
            main_rss = statistics.median(samples["main"]["rss_kib"])
            dev_rss = statistics.median(samples["dev"]["rss_kib"])
            old_time, old_mem = historical_best(
                baseline.get("records", {}), case, compiler, flags,
                fixture_sha, metadata["cpu"],
            )
            time_ratio, rss_ratio = dev_ns / main_ns, dev_rss / main_rss
            results.append({
                "case": case,
                "base_ns": main_ns, "dev_ns": dev_ns,
                "base_rss_kib": main_rss, "dev_rss_kib": dev_rss,
                "time_ratio": time_ratio, "rss_ratio": rss_ratio,
                "historical_best_ns": old_time["best_ns"] if old_time else None,
                "historical_best_rss_kib": old_mem["best_rss_kb"] if old_mem else None,
                "estimated_dev_best_ns":
                    old_time["best_ns"] * time_ratio if old_time else None,
                "estimated_dev_best_rss_kib":
                    old_mem["best_rss_kb"] * rss_ratio if old_mem else None,
                "historical_best_cpu":
                    old_time["fingerprint"]["cpu"] if old_time else None,
                "historical_best_commit": old_time.get("commit") if old_time else None,
                "historical_rss_cpu":
                    old_mem["fingerprint"]["cpu"] if old_mem else None,
                "historical_rss_commit":
                    old_mem.get("rss_commit", old_mem.get("commit")) if old_mem else None,
                "samples": samples,
            })

    (build / "results.json").write_text(json.dumps({
        "base_sha": base_sha, "dev_sha": dev_sha,
        "metadata": metadata, "runs": args.runs, "results": results,
    }, indent=2, ensure_ascii=False) + "\n")
    summary = summarize(results, metadata, base_sha, dev_sha)
    print(summary)
    if os.getenv("GITHUB_STEP_SUMMARY"):
        with open(os.environ["GITHUB_STEP_SUMMARY"], "a") as stream:
            stream.write(summary + "\n")


if __name__ == "__main__":
    try:
        main()
    except (RuntimeError, subprocess.CalledProcessError, subprocess.TimeoutExpired,
            OSError, ValueError, KeyError) as exc:
        print(f"A/B comparison failed: {exc}", file=sys.stderr)
        sys.exit(1)
