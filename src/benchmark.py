#!/usr/bin/env python3

import argparse
import os
import re
import shutil
import subprocess
from pathlib import Path
from statistics import mean, stdev

THREAD_COUNTS = [1, 2, 4, 8, 16, 24, 32, 40, 48, 56, 64]
RUNS_PER_CASE = 8

PROFILES = {
    "all_sequential": {
        "comp_zorder": False,
        "sort_zorder": False,
        "cons_radix": False,
    },
    "all_parallel": {
        "comp_zorder": True,
        "sort_zorder": True,
        "cons_radix": True,
    },
    "parallel_cons_radix_only": {
        "comp_zorder": False,
        "sort_zorder": False,
        "cons_radix": True,
    },
    "parallel_sort_zorder_only": {
        "comp_zorder": False,
        "sort_zorder": True,
        "cons_radix": False,
    },
    "parallel_comp_zorder_only": {
        "comp_zorder": True,
        "sort_zorder": False,
        "cons_radix": False,
    },
}

TIME_RE = re.compile(
    r"(?P<stage>comp_zorder|cons_radix)_(?P<mode>SEQ|PAR) complete:\s*(?P<us>[0-9.]+)us"
)


def require_tool(tool):
    if shutil.which(tool) is None:
        raise RuntimeError(f"Required tool not found: {tool}")


def compile_cpp(source, exe, compiler):
    require_tool(compiler)

    cmd = [
        compiler,
        "-O3",
        "-std=c++17",
        "-fopenmp",
        "-mavx2",
        "-march=native",
        str(source),
        "-o",
        str(exe),
    ]

    print("Compiling:", " ".join(cmd))
    subprocess.run(cmd, check=True)


def build_command(exe, filename, threads, profile):
    cmd = [
        str(exe),
        "-f", filename,
        "-t", str(threads),

        # Disable logging for benchmark timing.
        "-l",
    ]

    # In lbvh.cpp, -c/-s/-r toggle that stage to FALSE, i.e. sequential.
    if not profile["comp_zorder"]:
        cmd.append("-c")
    if not profile["sort_zorder"]:
        cmd.append("-s")
    if not profile["cons_radix"]:
        cmd.append("-r")

    return cmd


def parse_times(stdout):
    times = {}

    for match in TIME_RE.finditer(stdout):
        stage = match.group("stage")
        mode = match.group("mode")
        us = float(match.group("us"))
        times[f"{stage}_{mode}_us"] = us

    return times


def run_once(cmd, threads):
    env = os.environ.copy()
    env["OMP_NUM_THREADS"] = str(threads)
    env["OMP_PROC_BIND"] = "close"
    env["OMP_PLACES"] = "cores"

    proc = subprocess.run(
        cmd,
        env=env,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )

    if proc.returncode != 0:
        raise RuntimeError(
            "Command failed:\n"
            f"{' '.join(cmd)}\n\n"
            f"STDOUT:\n{proc.stdout}\n\n"
            f"STDERR:\n{proc.stderr}"
        )

    return parse_times(proc.stdout), proc.stdout


def avg_or_blank(values):
    return mean(values) if values else None


def sd_or_zero(values):
    return stdev(values) if len(values) > 1 else 0.0


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=Path, default=Path("lbvh.cpp"))
    parser.add_argument("--exe", type=Path, default=Path("./lbvh_benchmark"))
    parser.add_argument("--filename", required=True, help="OBJ filename inside models/")
    parser.add_argument("--compiler", default="g++")
    parser.add_argument("--no-compile", action="store_true")
    parser.add_argument("--out", type=Path, default=Path("benchmark.txt"))
    args = parser.parse_args()

    if not args.no_compile:
        compile_cpp(args.source, args.exe, args.compiler)

    lines = []
    lines.append("LBVH Benchmark Results")
    lines.append("======================")
    lines.append(f"source: {args.source}")
    lines.append(f"executable: {args.exe}")
    lines.append(f"model: models/{args.filename}")
    lines.append(f"runs per case: {RUNS_PER_CASE}")
    lines.append(f"threads: {THREAD_COUNTS}")
    lines.append("compile flags: -O3 -std=c++17 -fopenmp -mavx2 -march=native")
    lines.append("logging disabled with -l")
    lines.append("")

    for profile_name, profile in PROFILES.items():
        lines.append(f"Profile: {profile_name}")
        lines.append("-" * (9 + len(profile_name)))
        lines.append(
            "threads | "
            "comp_zorder_avg_us | comp_zorder_stdev_us | "
            "cons_radix_avg_us | cons_radix_stdev_us"
        )
        lines.append("-" * 86)

        for threads in THREAD_COUNTS:
            comp_times = []
            cons_times = []

            print(f"Running {profile_name}, threads={threads}")

            for run_idx in range(RUNS_PER_CASE):
                cmd = build_command(args.exe, args.filename, threads, profile)
                parsed, stdout = run_once(cmd, threads)

                for key, value in parsed.items():
                    if key.startswith("comp_zorder"):
                        comp_times.append(value)
                    elif key.startswith("cons_radix"):
                        cons_times.append(value)

            comp_avg = avg_or_blank(comp_times)
            comp_sd = sd_or_zero(comp_times)
            cons_avg = avg_or_blank(cons_times)
            cons_sd = sd_or_zero(cons_times)

            lines.append(
                f"{threads:7d} | "
                f"{comp_avg:18.3f} | {comp_sd:20.3f} | "
                f"{cons_avg:17.3f} | {cons_sd:19.3f}"
            )

        lines.append("")

    args.out.write_text("\n".join(lines))
    print(f"Wrote benchmark results to {args.out}")


if __name__ == "__main__":
    main()
