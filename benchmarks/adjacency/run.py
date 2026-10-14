#!/usr/bin/env python3
"""Serial fresh-process topology construction and full typed-reader costs."""
import argparse
import csv
import math
from pathlib import Path
import platform
import statistics
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT/"examples"))
from build_adjacency import (AdjacencyLibrary, build_and_consume, manifest_values,
                             metadata, milliseconds, prepare_case, tsv)
from brepgpu.adjacency_oracles import qualify_result

NATIVE_PHASES = ("admission_ms", "context_ms", "allocate_ms", "upload_ms", "count_ms",
                 "scan_ms", "fill_ms", "reader_ms", "download_ms", "cleanup_ms", "total_ms")
WRAPPER_PHASES = ("wrapper_allocation_ms", "native_call_ms", "publication_ms", "wrapper_wall_ms")
REUSED_PHASES = frozenset((*NATIVE_PHASES, *WRAPPER_PHASES, "external_build_ms",
                          "python_reader_ms", "actual_build_reader_ms", "independent_qualification_ms"))
SETUP_PHASES = frozenset(("body_preparation_ms", "input_packing_ms", "input_permutation_ms",
    "library_load_ms", "cold_first_build_ms", "cold_first_reader_ms", "cold_first_build_reader_ms",
    "cold_to_first_consumer_ms", "initial_independent_qualification_ms", "warmup_build_reader_ms",
    "warmup_independent_qualification_ms"))
IDENTITY_FIELDS = ("recipe", "replicas", "order", "backend", "device", "threads",
                   "edges", "faces", "coedges", "instances")


def worker(args):
    output = args.output_directory
    output.mkdir(parents=True, exist_ok=False)
    cold = time.perf_counter()
    prepared, bodies, setup = prepare_case(args.recipe, args.replicas, args.order)
    start = time.perf_counter()
    library = AdjacencyLibrary(args.library_path)
    setup.append(("library_load_ms", milliseconds(start)))
    start = time.perf_counter()
    first, consumed, build_ms, reader_ms = build_and_consume(library, prepared,
        backend=args.backend, device=args.device, threads=args.threads)
    setup.extend((("cold_first_build_ms", build_ms), ("cold_first_reader_ms", reader_ms),
                  ("cold_first_build_reader_ms", milliseconds(start)),
                  ("cold_to_first_consumer_ms", milliseconds(cold))))
    start = time.perf_counter()
    quality = qualify_result(first, literal=True)
    setup.append(("initial_independent_qualification_ms", milliseconds(start)))
    tsv(output/"cold-native.tsv", ("phase", "milliseconds"), first.timings)
    initial_consumed = consumed
    del first
    start = time.perf_counter()
    warm, warm_consumed, _, _ = build_and_consume(library, prepared,
        backend=args.backend, device=args.device, threads=args.threads)
    setup.append(("warmup_build_reader_ms", milliseconds(start)))
    start = time.perf_counter()
    if qualify_result(warm) != quality or warm_consumed != initial_consumed:
        raise AssertionError("warm consumer changed the complete qualified source relationship")
    setup.append(("warmup_independent_qualification_ms", milliseconds(start)))
    del warm
    raw = [(phase, -1, duration) for phase, duration in setup]
    for iteration in range(5):
        start = time.perf_counter()
        result, consumed, build_ms, reader_ms = build_and_consume(library, prepared,
            backend=args.backend, device=args.device, threads=args.threads)
        wall_ms = milliseconds(start)
        raw.extend((phase, iteration, duration) for phase, duration in result.timings)
        raw.extend((("external_build_ms", iteration, build_ms),
                    ("python_reader_ms", iteration, reader_ms),
                    ("actual_build_reader_ms", iteration, wall_ms)))
        start = time.perf_counter()
        if result.input is not prepared or qualify_result(result) != quality or consumed != initial_consumed:
            raise AssertionError("reused result changed source binding or complete qualified relationship")
        raw.append(("independent_qualification_ms", iteration, milliseconds(start)))
        del result
    tsv(output/"raw.tsv", ("phase", "iteration", "milliseconds"), raw)
    tsv(output/"quality.tsv", ("metric", "value"), (*quality.items(), ("qualified_results", 7)))
    tsv(output/"consumed.tsv", ("metric", "value"), initial_consumed.items())
    fields = metadata(prepared, args.recipe, args.replicas, args.order,
                      args.backend, args.device, args.threads)
    fields.update(repeats=5, warmups=1, event_profile=0, residency="per-call")
    tsv(output/"input.tsv", ("field", "value"), fields.items())
    print(f"{args.recipe} x{args.replicas} {args.order} {args.backend}:{args.device} "
          f"workers={args.threads}: {fields['edges']} edges, {fields['coedges']} uses", flush=True)


def environment(output, cuda_compiler, library):
    rows = [("python", platform.python_version()), ("system", platform.system()),
            ("kernel", platform.release()), ("machine", platform.machine()),
            ("libc", " ".join(platform.libc_ver()))]
    for label, command in (
        ("cpu", ["lscpu"]), ("g++", ["g++", "--version"]), ("cmake", ["cmake", "--version"]),
        ("cuda_compiler", [str(cuda_compiler), "--version"]),
        ("gpu", ["nvidia-smi", "--query-gpu=index,name,memory.total,driver_version",
                 "--format=csv,noheader,nounits"])):
        completed = subprocess.run(command, capture_output=True, text=True, check=False)
        if completed.returncode:
            raise RuntimeError("cannot record campaign environment: "+label)
        lines = completed.stdout.strip().splitlines()
        if label == "cpu":
            lines = [line for line in lines if line.split(":", 1)[0] in
                     ("Architecture", "CPU(s)", "On-line CPU(s) list", "Model name",
                      "Thread(s) per core", "Core(s) per socket", "Socket(s)")]
        for index, line in enumerate(lines):
            # nvcc's build metadata contains no invocation path; private host paths
            # and operational command lines are never written to public results.
            rows.append((f"{label}_{index}", line))
    cache = library.parent/"CMakeCache.txt"
    selected = {"CMAKE_BUILD_TYPE", "CMAKE_CXX_FLAGS", "CMAKE_CXX_FLAGS_RELEASE",
                "CMAKE_CUDA_FLAGS", "CMAKE_CUDA_FLAGS_RELEASE", "CMAKE_CUDA_ARCHITECTURES",
                "BREPGPU_ENABLE_CUDA", "BREPGPU_TOPOLOGY_TEST_HOOKS", "BREPGPU_ENABLE_SANITIZERS"}
    for line in cache.read_text().splitlines():
        if "=" not in line or line.startswith(("//", "#")):
            continue
        key, value = line.split("=", 1)
        key = key.split(":", 1)[0]
        if key in selected:
            rows.append(("cmake_"+key, value))
    tsv(output/"environment.tsv", ("field", "value"), rows)


def expected_metadata(recipe, replicas, order, backend, device, threads):
    E, F, C, instances = ((30, 16, 60, 3) if recipe == "mixed" else
                          (15*replicas, 7*replicas, 30*replicas, replicas))
    return {"recipe": recipe, "replicas": str(replicas), "order": order, "backend": backend,
        "device": str(device), "threads": str(threads), "edges": str(E), "faces": str(F),
        "coedges": str(C), "instances": str(instances), "host_slot_sign_input_bytes": str(12*C),
        "host_csr_reader_output_bytes": str(8*E+12*C+4),
        "cuda_slot_sign_upload_bytes": str(12*C if backend == "cuda" else 0),
        "cuda_csr_reader_download_bytes": str(8*E+12*C+4 if backend == "cuda" else 0),
        "cuda_source_identity_upload_bytes": "0", "repeats": "5", "warmups": "1",
        "event_profile": "0", "residency": "per-call"}


def read_pairs(path, key):
    with path.open(newline="") as stream:
        rows = list(csv.DictReader(stream, delimiter="\t"))
    pairs = {row[key]: row["value"] for row in rows}
    if len(pairs) != len(rows):
        raise RuntimeError("duplicate metadata field")
    return pairs


def render(summary, output):
    # Separate charts prevent Python source decoding from visually erasing the
    # native count/scan/fill/reader/transfer costs. Both are complete named scopes.
    for phase, filename, title, subtitle in (
        ("actual_build_reader_ms", "caller-costs.svg", "Complete topology caller",
         "Native rebuild + immutable wrapper publication + every typed mate/neighbor read"),
        ("total_ms", "native-costs.svg", "Native topology operation",
         "Admission + allocation + transfer + count/scan/fill + reader + cleanup; synchronous host span")):
        selected = [row for row in summary if row["phase"] == phase]
        maximum = max(float(row["max_ms"]) for row in selected)
        height = 165+32*len(selected)
        parts = [f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1160 {height}">',
            f'<rect width="1160" height="{height}" fill="#101923"/>', '<g font-family="sans-serif">',
            f'<text x="28" y="38" fill="#edf4fc" font-size="24">BRepGPU · {title}</text>',
            f'<text x="28" y="65" fill="#a9bed0" font-size="14">{subtitle}</text>']
        scale = 550/maximum if maximum else 0
        for index, row in enumerate(selected):
            y = 92+32*index
            device = f'CPU{row["threads"]}' if row["backend"] == "cpu" else f'P100 {row["device"]}'
            label = f'{device} · {row["instances"]} instances · {row["order"]}'
            color = "#f4c878" if row["backend"] == "cpu" else "#58c9e7"
            median, low, high = (float(row[name]) for name in ("median_ms", "min_ms", "max_ms"))
            parts.extend((f'<text x="28" y="{y+15}" fill="#edf4fc" font-size="13">{label}</text>',
                f'<rect x="400" y="{y}" width="{scale*median:.3f}" height="20" rx="3" fill="{color}"/>',
                f'<path d="M{400+scale*low:.3f},{y+10} H{400+scale*high:.3f}" stroke="#edf4fc" stroke-width="2"/>',
                f'<text x="970" y="{y+15}" fill="#edf4fc" font-size="14">{median:.3f} ms</text>'))
        parts.extend((f'<text x="28" y="{height-44}" fill="#a9bed0" font-size="14">Five calls after one warmup · marks show min/max · disjoint primitive instances · no resident topology allocation</text>',
            f'<text x="28" y="{height-20}" fill="#a9bed0" font-size="14">Packing/permutation, cold first call and independent qualification are reported separately in raw/summary TSV.</text>',
            '</g></svg>'))
        (output/filename).write_text("\n".join(parts)+"\n")


def controller(args):
    output = args.output_directory
    output.mkdir(parents=True, exist_ok=False)
    private = ROOT/"context"/"adjacency"/("matrix-"+str(time.time_ns()))
    private.mkdir(parents=True)
    library = args.library_path.resolve()
    identities = manifest_values(library)
    tsv(output/"sources.tsv", ("source", "sha256"), identities[0])
    tsv(output/"binaries.tsv", ("binary", "sha256"), identities[1])
    environment(output, args.cuda_compiler, library)
    raw, summary = [], []

    def unchanged():
        if identities != manifest_values(library):
            raise RuntimeError("measured source or binary changed during campaign")

    for recipe, replicas in (("mixed", 1), ("plate", 512), ("plate", 32768)):
        for order in ("ordered", "permuted"):
            for backend, device, threads in (("cpu", 0, 1), ("cpu", 0, 24),
                                             ("cuda", 0, 1), ("cuda", 1, 1)):
                key = f"{recipe}-{replicas}-{order}-{backend}-{device}-{threads}"
                case = output/"cases"/key
                command = [sys.executable, str(Path(__file__).resolve()), "--worker", "--recipe", recipe,
                    "--replicas", str(replicas), "--order", order, "--backend", backend,
                    "--device", str(device), "--threads", str(threads), "--library-path", str(library),
                    "--output-directory", str(case)]
                unchanged()
                completed = subprocess.run(command, capture_output=True, text=True, check=False)
                unchanged()
                (private/(key+"-stdout.txt")).write_text(completed.stdout)
                (private/(key+"-stderr.txt")).write_text(completed.stderr)
                if completed.returncode:
                    raise RuntimeError(f"topology case {key} failed; private logs retained")
                inputs = read_pairs(case/"input.tsv", "field")
                if inputs != expected_metadata(recipe, replicas, order, backend, device, threads):
                    raise RuntimeError("case metadata differs from requested operation: "+key)
                identity = {name: inputs[name] for name in IDENTITY_FIELDS}
                with (case/"raw.tsv").open(newline="") as stream:
                    measurements = list(csv.DictReader(stream, delimiter="\t"))
                phases, seen = {}, set()
                for row in measurements:
                    iteration = int(row["iteration"])
                    permitted = SETUP_PHASES if iteration == -1 else REUSED_PHASES
                    pair = row["phase"], iteration
                    if iteration not in (-1, 0, 1, 2, 3, 4) or row["phase"] not in permitted or pair in seen:
                        raise RuntimeError("duplicate or unexpected phase/iteration: "+key)
                    duration = float(row["milliseconds"])
                    if not math.isfinite(duration) or duration < 0:
                        raise RuntimeError("nonfinite or negative measurement: "+key)
                    seen.add(pair)
                    raw.append({**identity, **row})
                    phases.setdefault(row["phase"], []).append(duration)
                expected = {(phase, -1) for phase in SETUP_PHASES} | {
                    (phase, iteration) for phase in REUSED_PHASES for iteration in range(5)}
                if seen != expected:
                    raise RuntimeError("incomplete measurement scope: "+key)
                for phase, values in phases.items():
                    summary.append({**identity, "phase": phase, "samples": len(values),
                        "median_ms": statistics.median(values), "min_ms": min(values), "max_ms": max(values)})
                print(completed.stdout.strip(), flush=True)
    unchanged()
    tsv(output/"raw.tsv", (*IDENTITY_FIELDS, "phase", "iteration", "milliseconds"),
        ((row[name] for name in (*IDENTITY_FIELDS, "phase", "iteration", "milliseconds")) for row in raw))
    tsv(output/"summary.tsv", (*IDENTITY_FIELDS, "phase", "samples", "median_ms", "min_ms", "max_ms"),
        ((row[name] for name in (*IDENTITY_FIELDS, "phase", "samples", "median_ms", "min_ms", "max_ms"))
         for row in summary))
    render(summary, output)


if __name__ == "__main__":
    if sys.platform != "linux":
        raise RuntimeError("native benchmarks run on the fleet")
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--worker", action="store_true")
    parser.add_argument("--recipe", choices=("mixed", "plate"), default="mixed")
    parser.add_argument("--replicas", type=int, default=1)
    parser.add_argument("--order", choices=("ordered", "permuted"), default="ordered")
    parser.add_argument("--backend", choices=("cpu", "cuda"), default="cpu")
    parser.add_argument("--device", type=int, default=0)
    parser.add_argument("--threads", type=int, default=1)
    parser.add_argument("--library-path", type=Path, required=True)
    parser.add_argument("--cuda-compiler", type=Path, default=Path("nvcc"))
    parser.add_argument("--output-directory", type=Path, required=True)
    args = parser.parse_args()
    worker(args) if args.worker else controller(args)
