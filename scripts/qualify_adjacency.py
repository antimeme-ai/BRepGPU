#!/usr/bin/env python3
"""Serial direct CPU/P100 incidence and original geometry compatibility checks."""
import argparse
import csv
import os
from pathlib import Path
import re
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]


def main(args):
    if sys.platform != "linux":
        raise RuntimeError("product qualification runs on the fleet")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    if args.output.exists():
        raise FileExistsError(args.output)
    private = ROOT / "context" / "adjacency" / ("qualification-" + str(time.time_ns()))
    private.mkdir(parents=True)
    rows = []
    cases = (("cpu1-new-and-compatibility", "cpu", 0, 1,
              ("test_adjacency", "test_body", "test_pairs", "test_intersections")),
             ("cpu24-adjacency", "cpu", 0, 24, ("test_adjacency",)),
             ("p100-0-adjacency", "cuda", 0, 1, ("test_adjacency",)),
             ("p100-1-adjacency", "cuda", 1, 1, ("test_adjacency",)))
    for key, backend, device, threads, suites in cases:
        env = dict(os.environ, PYTHONPATH=str(ROOT / "python") + os.pathsep + str(ROOT / "tests"),
            BREPGPU_LIBRARY=str(args.core.resolve()), BREPGPU_TOPOLOGY_LIBRARY=str(args.library.resolve()),
            BREPGPU_TEST_BACKEND=backend, BREPGPU_TEST_DEVICE=str(device), BREPGPU_TEST_THREADS=str(threads))
        command = [sys.executable, "-m", "unittest", "-v", *suites]
        if backend == "cuda":
            command = [str(args.compute_sanitizer.resolve()), "--tool", "memcheck", "--leak-check", "full",
                       "--error-exitcode", "86", *command]
        start = time.perf_counter()
        log_path = private / (key + ".log")
        with log_path.open("w") as stream:
            completed = subprocess.run(command, cwd=ROOT, env=env, stdout=stream,
                                       stderr=subprocess.STDOUT, check=False)
        wall = time.perf_counter() - start
        log = log_path.read_text()
        if completed.returncode:
            print("\n".join(log.splitlines()[-70:]), flush=True)
            raise RuntimeError("qualification failed: " + key)
        match = re.search(r"Ran (\d+) tests? in ([0-9.]+)s", log)
        if not match:
            raise RuntimeError("missing unittest result: " + key)
        skips = re.search(r"OK \(skipped=(\d+)\)", log)
        errors = leaks = "not-run"
        if backend == "cuda":
            if "ERROR SUMMARY: 0 errors" not in log or "LEAK SUMMARY: 0 bytes leaked in 0 allocations" not in log:
                raise RuntimeError("device memory report failed: " + key)
            errors = leaks = 0
        rows.append((key, backend, device, threads, ",".join(suites), int(match[1]),
                     int(skips[1]) if skips else 0, float(match[2]), wall, errors, leaks, "PASS"))
        print(key, match[1] + " tests", f"{wall:.3f}s", "PASS", flush=True)
    with args.output.open("w", newline="") as stream:
        writer = csv.writer(stream, delimiter="\t")
        writer.writerow(("case", "backend", "device", "threads", "suites", "tests", "skips",
                         "test_seconds", "process_seconds", "device_errors", "device_leaked_bytes", "result"))
        writer.writerows(rows)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--library", type=Path, required=True)
    parser.add_argument("--core", type=Path, required=True)
    parser.add_argument("--compute-sanitizer", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    main(parser.parse_args())
