#!/usr/bin/env python3
"""Attack four body-host fault classes with the independent direct fleet suite."""
import argparse
import ast
import csv
import hashlib
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time

MUTANTS = (
    ("hole-membership", "body.py", "if sign < 0: return REJECTED", "if sign > 0: return REJECTED"),
    ("normal-orientation", "body_sampling.py",
     "normal = tuple(face.orientation * value / length for value in cross)",
     "normal = tuple(value / length for value in cross)"),
    ("internal-chart-boundary", "body.py",
     "(chart.quadrant == 0 and u == 0) or (chart.quadrant == 3 and u == 1)",
     "(u == 0) or (u == 1)"),
    ("scatter-order", "body_sampling.py",
     "for native_index, index in enumerate(request.admitted_indices):",
     "for native_index, index in enumerate(reversed(request.admitted_indices)):"),
)


def main():
    parser = argparse.ArgumentParser(__doc__)
    parser.add_argument("--library-path", type=Path, required=True)
    parser.add_argument("--output-directory", type=Path, required=True)
    args = parser.parse_args()
    if sys.platform != "linux":
        parser.error("native product mutation work runs on the Linux fleet")
    repository = Path(__file__).resolve().parents[1]
    args.output_directory.mkdir(parents=True, exist_ok=False)
    working = repository/"context"/"body"/("mutations-"+str(time.time_ns()))
    working.mkdir(parents=True)
    def digest(path):
        return hashlib.sha256(path.read_bytes()).hexdigest()
    sources = ("python/brepgpu/body.py", "python/brepgpu/body_sampling.py",
               "python/brepgpu/body_oracles.py", "tests/test_body.py", "scripts/run_body_mutations.py")
    identities = tuple((name, digest(repository/name)) for name in sources)
    core_sha = digest(args.library_path)
    base_environment = dict(os.environ)
    base_environment.update(BREPGPU_LIBRARY=str(args.library_path.resolve()),
                            BREPGPU_TEST_BACKEND="cpu", BREPGPU_TEST_THREADS="1")
    base_environment.pop("BREPGPU_TEST_PLUGIN", None)
    command = [sys.executable, "-m", "unittest", "test_body", "-v"]

    def run(name, python):
        environment = dict(base_environment)
        environment["PYTHONPATH"] = str(python)+os.pathsep+str(repository/"tests")
        result = subprocess.run(command, cwd=repository, env=environment,
                                capture_output=True, text=True, check=False)
        (working/(name+"-stdout.txt")).write_text(result.stdout)
        (working/(name+"-stderr.txt")).write_text(result.stderr)
        return result

    baseline = run("baseline", repository/"python")
    if baseline.returncode:
        raise RuntimeError("unmutated direct body suite failed; working logs in ignored context/body")
    match = re.search(r"Ran (\d+) tests? in", baseline.stderr)
    if not match:
        raise RuntimeError("missing baseline direct test count")
    test_count = int(match.group(1))
    rows = []
    for name, module, old, new in MUTANTS:
        private_python = working/name/"python"
        shutil.copytree(repository/"python", private_python, ignore=shutil.ignore_patterns("__pycache__", "*.pyc"))
        source_path = private_python/"brepgpu"/module
        original = source_path.read_text()
        if original.count(old) != 1:
            raise RuntimeError("mutation source anchor changed: "+name)
        changed = original.replace(old, new)
        ast.parse(changed, filename=module)
        source_path.write_text(changed)
        result = run(name, private_python)
        infrastructure = any(token in result.stderr for token in ("ModuleNotFoundError", "ImportError:", "SyntaxError:", "_FailedTest"))
        killed = result.returncode != 0 and "AssertionError" in result.stderr and not infrastructure
        rows.append((name, module, hashlib.sha256(original.encode()).hexdigest(),
                     hashlib.sha256(changed.encode()).hexdigest(), test_count, baseline.returncode,
                     result.returncode, "killed" if killed else "unqualified"))
        print(name+": "+rows[-1][-1], flush=True)
    if digest(args.library_path) != core_sha or any(digest(repository/name) != sha for name, sha in identities):
        raise RuntimeError("source or actual native core changed during mutation trials")
    with (args.output_directory/"source-manifest.tsv").open("w", newline="") as stream:
        writer = csv.writer(stream, delimiter="\t", lineterminator="\n")
        writer.writerow(("source", "sha256"))
        writer.writerows((*identities, ("native_core", core_sha)))
    with (args.output_directory/"mutations.tsv").open("w", newline="") as stream:
        writer = csv.writer(stream, delimiter="\t", lineterminator="\n")
        writer.writerow(("mutation", "module", "original_sha256", "mutant_sha256", "baseline_tests",
                         "baseline_exit", "mutant_exit", "verdict"))
        writer.writerows(rows)
    (args.output_directory/"METHODS.md").write_text(
        "# Direct body-host mutations\n\n"
        "Four actual production host-code changes attack exact hole membership, "
        "face-normal orientation, internal chart/seam labels and admitted-row scatter. "
        "One unmutated CPU direct suite must pass first. Mutants run in private copies "
        "against the same actual native core, then the same independent test suite. "
        "A killed result requires an assertion witness and excludes import/syntax "
        "infrastructure failures. Source copies and operational logs remain in ignored "
        "context; native baseline/product libraries are never overwritten.\n")
    if any(row[-1] != "killed" for row in rows):
        raise SystemExit("one or more body mutants lacked a direct qualified kill")


if __name__ == "__main__":
    main()
