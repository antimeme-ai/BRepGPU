#!/usr/bin/env python3
"""Isolated actual CPU/CUDA builder/reader and Python consumer mutants on fleet."""
import argparse
import csv
import hashlib
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
# Each changes one production expression, with the real independent test as oracle.
MUTANTS = (
    ("cpu-wrong-member", "native/topology.cpp", "cpu",
     "a.members[a.offsets[edge]+reservation]=static_cast<uint32_t>(coedge);",
     "a.members[a.offsets[edge]+reservation]=static_cast<uint32_t>((coedge+1)%desc.coedge_count);",
     "test_cylinder_seam_literal_members_and_mates"),
    ("cpu-wrong-neighbor", "native/topology.cpp", "cpu",
     "a.neighbors[left]=a.faces[right];", "a.neighbors[left]=a.faces[left];",
     "test_cylinder_seam_literal_members_and_mates"),
    ("cuda-wrong-member", "native/topology_cuda.cu", "cuda",
     "members[target]=static_cast<uint32_t>(c);", "members[target]=static_cast<uint32_t>((c+1)%count);",
     "test_cylinder_seam_literal_members_and_mates"),
    ("cuda-omitted-block-prefix", "native/topology_cuda.cu", "cuda",
     "output[index]+=prefixes[index/Tile];", "output[index]+=0;",
     "test_recursive_scan_uneven_boundaries_and_empty_tail"),
    ("cuda-wrong-neighbor", "native/topology_cuda.cu", "cuda",
     "neighbors[left]=faces[right];", "neighbors[left]=faces[left];",
     "test_cylinder_seam_literal_members_and_mates"),
    ("cuda-same-face-label-lost", "native/topology_cuda.cu", "cuda",
     "kind=faces[left]==faces[right]?BG_TOPOLOGY_SAME_FACE_PAIR:BG_TOPOLOGY_PAIRED;",
     "kind=BG_TOPOLOGY_PAIRED;", "test_cylinder_seam_literal_members_and_mates"),
    ("cuda-sign-conflict-ignored", "native/topology_cuda.cu", "cuda",
     "if(signs[left]==signs[right])kind=BG_TOPOLOGY_ORIENTATION_CONFLICT;",
     "if(false)kind=BG_TOPOLOGY_ORIENTATION_CONFLICT;", "test_all_degree_sign_and_same_face_classes"),
    ("python-mate-identity-shift", "python/brepgpu/adjacency.py", "cpu",
     "return None if slot == INVALID else self.input.coedge_sources[slot]",
     "return None if slot == INVALID else self.input.coedge_sources[(slot+1)%self.input.coedge_count]",
     "test_cylinder_seam_literal_members_and_mates"),
)


def run(command, env, log):
    start = time.perf_counter()
    with log.open("w") as stream:
        completed = subprocess.run(command, cwd=ROOT, env=env, stdout=stream,
                                   stderr=subprocess.STDOUT, check=False)
    return completed.returncode, time.perf_counter() - start


def main(args):
    if sys.platform != "linux":
        raise RuntimeError("production mutants run on the fleet")
    if args.output.exists():
        raise FileExistsError(args.output)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    private = ROOT / "context" / "adjacency" / ("mutants-" + str(time.time_ns()))
    private.mkdir(parents=True)
    paths = tuple(ROOT / name for name in (
        "CMakeLists.txt", "include/brepgpu.h", "native/topology_test_hooks.h",
        "native/topology.cpp", "native/topology_cuda.cu", "native/topology_internal.hpp",
        "include/brepgpu_topology.h", "python/brepgpu/adjacency.py",
        "python/brepgpu/__init__.py", "python/brepgpu/body.py", "python/brepgpu/body_sampling.py",
        "python/brepgpu/adjacency_oracles.py", "tests/test_adjacency.py")) + (args.library.resolve(), args.core.resolve(), args.core.resolve().parent / "libbrepgpu_cuda.so")
    original = tuple(hashlib.sha256(path.read_bytes()).hexdigest() for path in paths)

    def unchanged():
        if original != tuple(hashlib.sha256(path.read_bytes()).hexdigest() for path in paths):
            raise RuntimeError("live qualification sources or binaries changed during mutation campaign")

    rows = []
    for name, source, backend, before, after, method in MUTANTS:
        unchanged()
        env = dict(os.environ, PYTHONPATH=str(ROOT / "python") + os.pathsep + str(ROOT / "tests"),
            BREPGPU_LIBRARY=str(args.core.resolve()), BREPGPU_TOPOLOGY_LIBRARY=str(args.library.resolve()),
            BREPGPU_TEST_BACKEND=backend, BREPGPU_TEST_DEVICE="0", BREPGPU_TEST_THREADS="1")
        command = [sys.executable, "-m", "unittest", "-v", "test_adjacency.AdjacencyTests." + method]
        code, baseline_seconds = run(command, env, private / (name + "-baseline.log"))
        if code:
            raise RuntimeError("healthy baseline failed for " + name)
        mutant = private / name
        mutant.mkdir()
        shutil.copy2(ROOT / "CMakeLists.txt", mutant / "CMakeLists.txt")
        for directory in ("native", "include", "python", "tests"):
            shutil.copytree(ROOT / directory, mutant / directory,
                ignore=shutil.ignore_patterns("__pycache__", "*.pyc", ".DS_Store"))
        path = mutant / source
        text = path.read_text()
        if text.count(before) != 1:
            raise RuntimeError("mutant expression absent or ambiguous: " + name)
        path.write_text(text.replace(before, after))
        build_seconds = 0.0
        library = args.library.resolve()
        if source.startswith("native/"):
            build = mutant / "build"
            configure = ["cmake", "-S", str(mutant), "-B", str(build), "-DCMAKE_BUILD_TYPE=Release",
                         "-DBREPGPU_ENABLE_CUDA=" + ("ON" if backend == "cuda" else "OFF")]
            if backend == "cuda":
                configure += ["-DCMAKE_CUDA_COMPILER=" + str(args.cuda_compiler.resolve()),
                              "-DCUDAToolkit_ROOT=" + str(args.cuda_compiler.resolve().parents[1])]
            code, seconds = run(configure, env, private / (name + "-configure.log"))
            build_seconds += seconds
            if code:
                raise RuntimeError("mutant configuration failed: " + name)
            code, seconds = run(["cmake", "--build", str(build), "--target", "brepgpu_topology", "-j4"],
                                env, private / (name + "-build.log"))
            build_seconds += seconds
            if code:
                raise RuntimeError("mutant compilation failed: " + name)
            library = build / "libbrepgpu_topology.so"
        env.update(PYTHONPATH=str(mutant / "python") + os.pathsep + str(mutant / "tests"),
                   BREPGPU_TOPOLOGY_LIBRARY=str(library))
        log_path = private / (name + "-mutant.log")
        code, test_seconds = run(command, env, log_path)
        log = log_path.read_text()
        # Build errors, crashes, native statuses and memory-tool errors are not
        # credited as an independent assertion killing a semantic mutant.
        if code == 0 or "AssertionError" not in log or not re.search(r"FAILED \(failures=1\)", log):
            print("\n".join(log.splitlines()[-45:]), flush=True)
            raise RuntimeError("mutant did not earn an assertion kill: " + name)
        rows.append((name, source, backend, method, baseline_seconds, build_seconds, test_seconds,
                     "healthy baseline", "AssertionError", "KILLED"))
        unchanged()
        print(name, "KILLED by actual assertion", f"build={build_seconds:.3f}s", flush=True)
    with args.output.open("w", newline="") as stream:
        writer = csv.writer(stream, delimiter="\t", lineterminator="\n")
        writer.writerow(("mutant", "production_source", "backend", "direct_test", "baseline_seconds",
                         "build_seconds", "test_seconds", "baseline", "failure", "result"))
        writer.writerows(rows)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--library", type=Path, required=True)
    parser.add_argument("--core", type=Path, required=True)
    parser.add_argument("--cuda-compiler", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    main(parser.parse_args())
