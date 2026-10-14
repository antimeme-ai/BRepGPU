#!/usr/bin/env python3
"""Measure a small serial body-sampling matrix using the actual Python caller."""
import argparse
import csv
import hashlib
import os
from pathlib import Path
import statistics
import subprocess
import sys
import time

from brepgpu import Engine
from brepgpu.body import make_body
from brepgpu.body_sampling import BodySampler, prepare_grid
from brepgpu.body_oracles import qualify_batch

REUSED_PHASES = ("gather_ms", "query_ms", "evaluate_ms", "download_ms", "convert_ms",
                 "normal_scatter_ms", "child_cleanup_ms", "reused_wall_ms", "actual_reused_caller_ms")


def tsv(path, header, rows):
    with path.open("w", newline="") as stream:
        writer = csv.writer(stream, delimiter="\t", lineterminator="\n")
        writer.writerow(header)
        writer.writerows(rows)


def measured(start):
    return 1000*(time.perf_counter()-start)


def worker(args):
    output = args.output_directory
    output.mkdir(parents=True, exist_ok=False)
    singles = []
    start = time.perf_counter()
    body_start = time.perf_counter()
    body = make_body(args.shape)
    singles.append(("body_prepare_ms", measured(body_start)))
    request_start = time.perf_counter()
    request = prepare_grid(body, args.resolution)
    singles.append(("request_prepare_ms", measured(request_start)))
    setup_start = time.perf_counter()
    with Engine(backend=args.backend, device=args.device, threads=args.threads,
                library_path=args.library_path, plugin_path=args.plugin_path) as engine:
        singles.append(("cold_engine_ms", measured(setup_start)))
        setup_start = time.perf_counter()
        with BodySampler(engine, body) as sampler:
            singles.append(("cold_patchset_ms", measured(setup_start)))
            cold = sampler.sample(request)
            cleanup_start = time.perf_counter()
    singles.append(("cold_cleanup_ms", measured(cleanup_start)))
    singles.append(("cold_wall_ms", measured(start)))
    qualify_start = time.perf_counter()
    quality = qualify_batch(cold)
    singles.append(("initial_qualification_ms", measured(qualify_start)))
    rows = [(phase, -1, duration) for phase, duration in singles]
    with Engine(backend=args.backend, device=args.device, threads=args.threads,
                library_path=args.library_path, plugin_path=args.plugin_path) as engine:
        with BodySampler(engine, body) as sampler:
            sampler.sample(request)  # One excluded warmup on the retained engine/patch set.
            for iteration in range(5):
                call_start = time.perf_counter()
                batch = sampler.sample(request)
                actual_wall = measured(call_start)
                if batch.count != cold.count or batch.evaluated_count != cold.evaluated_count:
                    raise RuntimeError("repeated body sample counts changed")
                rows.extend((phase, iteration, duration) for phase, duration in batch.timings)
                rows.append(("actual_reused_caller_ms", iteration, actual_wall))
    tsv(output/"raw.tsv", ("phase", "iteration", "milliseconds"), rows)
    tsv(output/"quality.tsv", ("metric", "value"), quality.items())
    patch_count = len(body.patches)
    control_bytes = sum(len(patch.controls)*8 for patch in body.patches)
    tsv(output/"input.tsv", ("field", "value"), (
        ("shape", args.shape), ("backend", args.backend), ("device", args.device),
        ("requested_cpu_workers", args.threads), ("resolution", args.resolution),
        ("body_id", body.body_id), ("revision", body.revision),
        ("candidates", cold.count), ("evaluated", cold.evaluated_count),
        ("patch_count", patch_count), ("control_bytes", control_bytes),
        ("body_table_bytes", sum(len(data) for name, data in body.packed_tables)),
        ("body_table_upload_bytes", 0),
        ("cuda_patch_upload_bytes", control_bytes+16*patch_count if args.backend == "cuda" else 0),
        ("cuda_query_upload_bytes", 20*cold.evaluated_count if args.backend == "cuda" else 0),
        ("cuda_jet_status_download_bytes", 76*cold.evaluated_count if args.backend == "cuda" else 0),
        ("host_source_metadata_bytes", 136*patch_count+20*cold.evaluated_count),
    ))
    print(f"{args.shape} {args.backend}:{args.device} workers={args.threads} resolution={args.resolution} "
          f"rows={cold.count}/{cold.evaluated_count} samples=5")


def read_pairs(path, key="field"):
    with path.open(newline="") as stream:
        return {row[key]: row["value"] for row in csv.DictReader(stream, delimiter="\t")}


def render(summary, path):
    selected = [row for row in summary if row["phase"] == "actual_reused_caller_ms" and row["resolution"] == 65]
    maximum = max(float(row["max_ms"]) for row in selected)
    parts = ['<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 850 720">',
             '<rect width="850" height="720" fill="#101923"/>',
             '<text x="28" y="38" fill="#edf4fc" font-family="sans-serif" font-size="23">BRepGPU · complete reused body sampling</text>',
             '<text x="28" y="65" fill="#a9bed0" font-family="sans-serif" font-size="14">Resolution 65 · five calls per case · retained body, grid, engine and patch set</text>']
    for i, row in enumerate(selected):
        y = 103+43*i
        label = f'{row["shape"]} · CPU{row["threads"]}' if row["backend"] == "cpu" else f'{row["shape"]} · P100 {row["device"]}'
        median = float(row["median_ms"])
        minimum, upper = float(row["min_ms"]), float(row["max_ms"])
        width = 430*median/maximum
        color = "#f4c878" if row["backend"] == "cpu" else "#58c9e7"
        parts.append(f'<text x="28" y="{y+15}" fill="#edf4fc" font-family="sans-serif" font-size="14">{label}</text>')
        parts.append(f'<rect x="205" y="{y}" width="{width:.3f}" height="22" rx="3" fill="{color}"><title>{minimum:.6f}–{upper:.6f} ms; evaluated {row["evaluated"]} / {row["candidates"]}</title></rect>')
        parts.append(f'<path d="M{205+430*minimum/maximum:.3f},{y+11} H{205+430*upper/maximum:.3f}" stroke="#edf4fc" stroke-width="2"/>')
        parts.append(f'<text x="665" y="{y+16}" fill="#edf4fc" font-family="sans-serif" font-size="14">{median:.3f} ms</text>')
    parts.extend(('<text x="28" y="656" fill="#a9bed0" font-family="sans-serif" font-size="14">Includes query packing/upload, native jets, download/provenance, CPU conversion/normals and cleanup.</text>',
                  '<text x="28" y="682" fill="#a9bed0" font-family="sans-serif" font-size="14">Grid/classification preparation, first use, qualification and export are reported separately.</text>',
                  '<text x="28" y="706" fill="#a9bed0" font-family="sans-serif" font-size="14">Shared host; range marks min/max. No mesh, Boolean or GPU topology speedup is implied.</text>', '</svg>'))
    path.write_text("\n".join(parts)+"\n")


def controller(args):
    output = args.output_directory
    output.mkdir(parents=True, exist_ok=False)
    repository = Path(__file__).resolve().parents[2]
    private = repository/"context"/"body"/("benchmark-"+str(time.time_ns()))
    private.mkdir(parents=True)
    sources = ["python/brepgpu/body.py", "python/brepgpu/body_sampling.py", "python/brepgpu/body_oracles.py",
               "python/brepgpu/__init__.py", "benchmarks/body/run.py", "native/cuda_backend.cu", "native/runtime.cpp", "native/evaluator_cpu.cpp"]
    identities = [(name, hashlib.sha256((repository/name).read_bytes()).hexdigest()) for name in sources]
    tsv(output/"sources.tsv", ("source", "sha256"), identities)
    native_core = args.library_path or Path(os.environ.get("BREPGPU_LIBRARY", "build/discovery-release/libbrepgpu.so"))
    binaries = [("core", hashlib.sha256(native_core.read_bytes()).hexdigest())]
    plugin = args.plugin_path or native_core.parent/"libbrepgpu_cuda.so"
    binaries.append(("cuda_plugin", hashlib.sha256(plugin.read_bytes()).hexdigest()))
    tsv(output/"binaries.tsv", ("binary", "sha256"), binaries)
    all_rows, summary = [], []
    for shape in ("box", "cylinder", "plate"):
        for resolution in (17, 65):
            for backend, device, threads in (("cpu", 0, 1), ("cpu", 0, 24), ("cuda", 0, 1), ("cuda", 1, 1)):
                key = f"{shape}-{resolution}-{backend}-{device}-{threads}"
                case = output/"cases"/key
                command = [sys.executable, str(Path(__file__).resolve()), "--worker", "--shape", shape,
                           "--resolution", str(resolution), "--backend", backend, "--device", str(device),
                           "--threads", str(threads), "--output-directory", str(case),
                           "--library-path", str(native_core.resolve()), "--plugin-path", str(plugin.resolve())]
                completed = subprocess.run(command, capture_output=True, text=True, check=False)
                (private/(key+"-stdout.txt")).write_text(completed.stdout)
                (private/(key+"-stderr.txt")).write_text(completed.stderr)
                if completed.returncode:
                    raise RuntimeError(f"body case {key} failed; working log in ignored context/body")
                metadata = read_pairs(case/"input.tsv")
                with (case/"raw.tsv").open(newline="") as stream:
                    raw = list(csv.DictReader(stream, delimiter="\t"))
                phases = {}
                for row in raw:
                    duration = float(row["milliseconds"])
                    if duration < 0 or not duration < float("inf"):
                        raise ValueError("invalid measured duration")
                    phases.setdefault(row["phase"], []).append(duration)
                    all_rows.append((key, shape, resolution, backend, device, threads, metadata["candidates"],
                                     metadata["evaluated"], row["phase"], row["iteration"], row["milliseconds"]))
                for phase, values in phases.items():
                    samples = 5 if phase in REUSED_PHASES else 1
                    if len(values) != samples:
                        raise ValueError(f"unexpected sample count for {key}:{phase}")
                    summary.append(dict(case=key, shape=shape, resolution=resolution, backend=backend,
                                        device=device, threads=threads, candidates=metadata["candidates"],
                                        evaluated=metadata["evaluated"], phase=phase, samples=len(values),
                                        median_ms=statistics.median(values), min_ms=min(values), max_ms=max(values)))
                print(completed.stdout.strip(), flush=True)
    for name, expected in identities:
        if hashlib.sha256((repository/name).read_bytes()).hexdigest() != expected:
            raise RuntimeError("source changed during performance trials")
    if hashlib.sha256(native_core.read_bytes()).hexdigest() != binaries[0][1] or hashlib.sha256(plugin.read_bytes()).hexdigest() != binaries[1][1]:
        raise RuntimeError("native binary changed during trials")
    tsv(output/"raw.tsv", ("case", "shape", "resolution", "backend", "device", "threads", "candidates", "evaluated", "phase", "iteration", "milliseconds"), all_rows)
    header = tuple(summary[0])
    tsv(output/"summary.tsv", header, ([row[name] for name in header] for row in summary))
    render(summary, output/"reused-body-sampling.svg")
    (output/"METHODS.md").write_text(
        "# Actual body-sampling timing scope\n\n"
        "24 cases, separate fresh process per case, fixed serial CPU1/CPU24/P1000/P1001 "
        "order. One cold/preparation observation, independent exact initial qualification, "
        "one excluded warmup and five reused calls. Every reused call checks native status, "
        "source provenance and finite nondegenerate normals. Exact initial qualification "
        "is outside reused timing; no exported point files are written in timed calls.\n\n"
        "Reused wall retains the immutable body, prepared grid/classification, Engine and "
        "uploaded PatchSet. Fresh query packing/upload, evaluate, Python jet/provenance "
        "download, host conversion/normal/scatter and child cleanup are included. CPU24 "
        "changes native workers, not the serial Python stages. actual_reused_caller_ms is "
        "measured outside sampler.sample and includes final batch publication; the internal "
        "reused_wall_ms ends through child cleanup. Cold wall includes body/grid, "
        "first Engine/PatchSet and cleanup. Case first use is not a cold filesystem/driver "
        "cache claim. Independent phase medians are not additive.\n\n"
        "CUDA input20/output76 bytes per evaluated row are RAM/VRAM payloads, not network "
        "traffic or total host copies. Native geometry upload is 16-byte DevicePatch plus "
        "homogeneous controls per patch; body adjacency tables remain on CPU. Download "
        "also reads retained host provenance136 bytes/descriptor plus20 bytes/query. "
        "Conversion and Python output allocation add host traffic.\n\n"
        "No owned copy/build/qualification/performance work runs concurrently with this "
        "serial matrix. The host is shared, unreserved and clock-uncontrolled; five-sample "
        "ranges are descriptive. This caller does not change reference-kernel Booleans. "
        "Operation logs remain in ignored context; raw numerical samples are public.\n")


def main():
    parser = argparse.ArgumentParser(__doc__)
    parser.add_argument("--output-directory", type=Path, required=True)
    parser.add_argument("--library-path", type=Path)
    parser.add_argument("--plugin-path", type=Path)
    parser.add_argument("--worker", action="store_true", help=argparse.SUPPRESS)
    parser.add_argument("--shape", choices=("box", "cylinder", "plate"), default="plate")
    parser.add_argument("--resolution", type=int, default=17)
    parser.add_argument("--backend", choices=("cpu", "cuda"), default="cpu")
    parser.add_argument("--device", type=int, default=0)
    parser.add_argument("--threads", type=int, default=1)
    args = parser.parse_args()
    worker(args) if args.worker else controller(args)


if __name__ == "__main__":
    main()
