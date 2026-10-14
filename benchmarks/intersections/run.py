#!/usr/bin/env python3
"""Serial complete-consumer comparisons; run native work on the Linux fleet."""
import argparse
import csv
import hashlib
import os
from pathlib import Path
import statistics
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[2]
SETUP_PHASES = frozenset(("body_preparation_ms","exact_construction_ms","request_preparation_ms",
    "cold_engine_ms","geometry_setup_ms","cold_first_sample_ms","cold_to_first_batch_ms",
    "initial_independent_qualification_ms","sampler_cleanup_ms","engine_cleanup_ms"))
REUSED_PHASES = frozenset(("gather_ms","query_ms","evaluate_ms","download_ms","convert_ms",
    "world_scatter_ms","grade_ms","child_cleanup_ms","reused_wall_ms","actual_sample_caller_ms"))
sys.path.insert(0,str(ROOT/"examples"))
from intersect_faces import make_inputs, engine_scope, milliseconds, tsv
from brepgpu import Engine
from brepgpu.intersection_sampling import prepare_samples, IntersectionSampler
from brepgpu.intersection_oracles import recipe_counts, qualify_batch


def worker(args):
    output = args.output_directory
    output.mkdir(parents=True,exist_ok=False)
    cold = time.perf_counter()
    raw,setup = [],[]
    start = time.perf_counter()
    plane,wall = make_inputs(args.recipe)
    setup.append(("body_preparation_ms",milliseconds(start)))
    start = time.perf_counter()
    result = plane.intersect_faces(1,wall,3)
    setup.append(("exact_construction_ms",milliseconds(start)))
    start = time.perf_counter()
    request = prepare_samples(result,args.resolution)
    setup.append(("request_preparation_ms",milliseconds(start)))
    start = time.perf_counter()
    with engine_scope(Engine(backend=args.backend,device=args.device,threads=args.threads,
                library_path=args.library_path,plugin_path=args.plugin_path)) as engine:
        setup.append(("cold_engine_ms",milliseconds(start)))
        start = time.perf_counter()
        with IntersectionSampler(engine,result) as sampler:
            setup.append(("geometry_setup_ms",milliseconds(start)))
            start = time.perf_counter()
            first = sampler.sample(request)
            setup.append(("cold_first_sample_ms",milliseconds(start)))
            setup.append(("cold_to_first_batch_ms",milliseconds(cold)))
            start = time.perf_counter()
            quality = qualify_batch(first,request)
            quality.update(recipe_counts(result,args.recipe))
            setup.append(("initial_independent_qualification_ms",milliseconds(start)))
            sampler.sample(request)  # One excluded warmup, no event profile.
            for iteration in range(5):
                start = time.perf_counter()
                batch = sampler.sample(request)
                external = milliseconds(start)
                if batch.count != first.count or batch.result is not result:
                    raise AssertionError("reused consumer changed the bound result/request")
                raw.extend((phase,iteration,duration) for phase,duration in batch.timings)
                raw.append(("actual_sample_caller_ms",iteration,external))
            cleanup = time.perf_counter()
        setup.append(("sampler_cleanup_ms",milliseconds(cleanup)))
        cleanup = time.perf_counter()
    setup.append(("engine_cleanup_ms",milliseconds(cleanup)))
    raw = [(phase,-1,duration) for phase,duration in setup]+raw
    tsv(output/"raw.tsv",("phase","iteration","milliseconds"),raw)
    tsv(output/"quality.tsv",("metric","value"),quality.items())
    tsv(output/"input.tsv",("field","value"),(("recipe",args.recipe),("resolution",args.resolution),
        ("backend",args.backend),("device",args.device),("threads",args.threads),
        ("components",len(result.components)),("pieces",len(result.pieces)),
        ("candidate_samples",first.count),("native_rows",2*first.count),("repeats",5),("profile",0),
        ("cuda_geometry_upload_bytes",976 if args.backend=="cuda" else 0),
        ("cuda_query_upload_bytes",40*first.count if args.backend=="cuda" else 0),
        ("cuda_position_status_download_bytes",56*first.count if args.backend=="cuda" else 0),
        ("host_source_descriptor_bytes",len(first.source_descriptors)),
        ("host_source_slot_uv_bytes",len(first.native_slots)+len(first.native_uv)),("topology_upload_bytes",0)))
    print(f"{args.recipe} res={args.resolution} {args.backend}:{args.device} workers={args.threads}: "
          f"{first.count} samples, {2*first.count} native rows",flush=True)


def render(rows,path):
    selected = [r for r in rows if r["phase"]=="actual_sample_caller_ms"]
    maximum = max(float(r["max_ms"]) for r in selected)
    height = 170+33*len(selected)
    parts = ['<svg xmlns="http://www.w3.org/2000/svg" '+f'viewBox="0 0 1080 {height}">',
        f'<rect width="1080" height="{height}" fill="#101923"/>','<g font-family="sans-serif">',
        '<text x="28" y="38" fill="#edf4fc" font-size="24">BRepGPU · two-sided section consumer costs</text>',
        '<text x="28" y="65" fill="#a9bed0" font-size="14">Five reused calls · exact CPU construction/request prep reported separately · every call includes model grades</text>']
    for i,row in enumerate(selected):
        y = 93+33*i
        label = f'CPU{row["threads"]}' if row["backend"]=="cpu" else f'P100 {row["device"]}'
        label += f' · {row["recipe"]} · {row["candidate_samples"]} samples'
        scale = 460/maximum
        color = "#f4c878" if row["backend"]=="cpu" else "#58c9e7"
        parts.extend((f'<text x="28" y="{y+15}" fill="#edf4fc" font-size="13">{label}</text>',
            f'<rect x="360" y="{y}" width="{scale*float(row["median_ms"]):.3f}" height="20" rx="3" fill="{color}"/>',
            f'<path d="M{360+scale*float(row["min_ms"]):.3f},{y+10} H{360+scale*float(row["max_ms"]):.3f}" stroke="#edf4fc" stroke-width="2"/>',
            f'<text x="850" y="{y+15}" fill="#edf4fc" font-size="14">{float(row["median_ms"]):.3f} ms</text>'))
    parts.extend((f'<text x="28" y="{height-46}" fill="#a9bed0" font-size="14">Raw iterations and native evaluation host spans accompany this chart. Shared host; marks show min/max.</text>',
        f'<text x="28" y="{height-20}" fill="#a9bed0" font-size="14">CUDA evaluates supplied curves. Exact intersection construction and topology remain CPU work.</text>',
        '</g></svg>'))
    path.write_text("\n".join(parts)+"\n")


def controller(args):
    output = args.output_directory
    output.mkdir(parents=True,exist_ok=False)
    private = ROOT/"context"/"intersection"/("matrix-"+str(time.time_ns()))
    private.mkdir(parents=True)
    sources = ("python/brepgpu/body.py","python/brepgpu/intersections.py",
        "python/brepgpu/intersection_sampling.py","python/brepgpu/intersection_oracles.py",
        "python/brepgpu/__init__.py","python/brepgpu/body_oracles.py",
        "python/brepgpu/pair_scene.py","python/brepgpu/pair_oracles.py",
        "examples/discover_pairs.py","examples/intersect_faces.py","benchmarks/intersections/run.py")
    source_paths = tuple(ROOT/name for name in sources)
    initial_sources = tuple(hashlib.sha256(path.read_bytes()).hexdigest() for path in source_paths)
    tsv(output/"sources.tsv",("source","sha256"),
        zip(sources,initial_sources))
    core = args.library_path.resolve()
    plugin = (args.plugin_path or core.parent/"libbrepgpu_cuda.so").resolve()
    initial_binaries = tuple(hashlib.sha256(path.read_bytes()).hexdigest() for path in (core,plugin))
    tsv(output/"binaries.tsv",("binary","sha256"),zip(("core","cuda_plugin"),initial_binaries))
    def unchanged():
        if initial_sources != tuple(hashlib.sha256(path.read_bytes()).hexdigest() for path in source_paths) \
                or initial_binaries != tuple(hashlib.sha256(path.read_bytes()).hexdigest() for path in (core,plugin)):
            raise RuntimeError("measured source or binary changed during campaign")
    identity_fields = ("recipe","resolution","backend","device","threads","components","pieces","candidate_samples","native_rows")
    raw,summary = [],[]
    for recipe in ("circle","clipped"):
        for resolution in (65,4097):
            for backend,device,threads in (("cpu",0,1),("cpu",0,24),("cuda",0,1),("cuda",1,1)):
                key = f"{recipe}-{resolution}-{backend}-{device}-{threads}"
                case = output/"cases"/key
                command = [sys.executable,str(Path(__file__).resolve()),"--worker","--recipe",recipe,
                    "--resolution",str(resolution),"--backend",backend,"--device",str(device),"--threads",str(threads),
                    "--library-path",str(core),"--plugin-path",str(plugin),"--output-directory",str(case)]
                unchanged()
                completed = subprocess.run(command,capture_output=True,text=True,check=False)
                unchanged()
                (private/(key+"-stdout.txt")).write_text(completed.stdout)
                (private/(key+"-stderr.txt")).write_text(completed.stderr)
                if completed.returncode:
                    raise RuntimeError(f"consumer case {key} failed; private logs retained")
                with (case/"input.tsv").open(newline="") as stream:
                    inputs = list(csv.DictReader(stream,delimiter="\t"))
                fields = {row["field"]:row["value"] for row in inputs}
                if len(fields) != len(inputs):
                    raise RuntimeError("duplicate input field: "+key)
                expected_inputs = dict(recipe=recipe,resolution=str(resolution),backend=backend,
                    device=str(device),threads=str(threads),components=str(1 if recipe=="circle" else 4),
                    pieces="4",candidate_samples=str(4*resolution),native_rows=str(8*resolution),
                    repeats="5",profile="0",cuda_geometry_upload_bytes=str(976 if backend=="cuda" else 0),
                    cuda_query_upload_bytes=str(160*resolution if backend=="cuda" else 0),
                    cuda_position_status_download_bytes=str(224*resolution if backend=="cuda" else 0),
                    host_source_descriptor_bytes="680",host_source_slot_uv_bytes=str(160*resolution),topology_upload_bytes="0")
                if fields != expected_inputs:
                    raise RuntimeError("case metadata differs from requested qualified scope: "+key)
                identity = {name:fields[name] for name in identity_fields}
                with (case/"raw.tsv").open(newline="") as stream:
                    measurements = list(csv.DictReader(stream,delimiter="\t"))
                phases,seen = {},set()
                for row in measurements:
                    iteration = int(row["iteration"])
                    permitted = SETUP_PHASES if iteration == -1 else REUSED_PHASES
                    pair = (row["phase"],iteration)
                    if iteration not in (-1,0,1,2,3,4) or row["phase"] not in permitted or pair in seen:
                        raise RuntimeError("duplicate or unexpected phase/iteration: "+key)
                    seen.add(pair)
                    duration = float(row["milliseconds"])
                    if not 0 <= duration < float("inf"):
                        raise RuntimeError("nonfinite or negative measurement")
                    raw.append({**identity,**row})
                    phases.setdefault(row["phase"],[]).append(duration)
                expected_rows = {(phase,-1) for phase in SETUP_PHASES} | \
                                {(phase,i) for phase in REUSED_PHASES for i in range(5)}
                if seen != expected_rows:
                    raise RuntimeError("incomplete measurement phases: "+key)
                for phase,values in phases.items():
                    summary.append({**identity,"phase":phase,"samples":len(values),
                        "median_ms":statistics.median(values),"min_ms":min(values),"max_ms":max(values)})
                print(completed.stdout.strip(),flush=True)
    unchanged()
    tsv(output/"raw.tsv",(*identity_fields,"phase","iteration","milliseconds"),
        ((r[name] for name in (*identity_fields,"phase","iteration","milliseconds")) for r in raw))
    tsv(output/"summary.tsv",(*identity_fields,"phase","samples","median_ms","min_ms","max_ms"),
        ((r[name] for name in (*identity_fields,"phase","samples","median_ms","min_ms","max_ms")) for r in summary))
    render(summary,output/"costs.svg")


if __name__ == "__main__":
    if sys.platform != "linux":
        raise RuntimeError("native benchmarks run on the fleet")
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--worker",action="store_true")
    parser.add_argument("--recipe",choices=("circle","clipped"),default="circle")
    parser.add_argument("--resolution",type=int,default=65)
    parser.add_argument("--backend",choices=("cpu","cuda"),default="cpu")
    parser.add_argument("--device",type=int,default=0)
    parser.add_argument("--threads",type=int,default=1)
    parser.add_argument("--library-path",type=Path,required=True)
    parser.add_argument("--plugin-path",type=Path)
    parser.add_argument("--output-directory",type=Path,required=True)
    args = parser.parse_args()
    worker(args) if args.worker else controller(args)
