#!/usr/bin/env python3
"""Serial fresh-process CPU/P100 comparisons of the complete shortlist caller."""
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
sys.path.insert(0,str(ROOT/"examples"))
from discover_pairs import engine_scope, prepare_inputs, milliseconds, tsv
from brepgpu import Engine
from brepgpu.pair_scene import PairDiscovery
from brepgpu.pair_oracles import qualify_scene, qualify_page


def worker(args):
    output = args.output_directory
    output.mkdir(parents=True,exist_ok=False)
    cold_start = time.perf_counter()
    left,right,setup = prepare_inputs(args.size,args.library_path)
    singles = list(setup)
    start = time.perf_counter()
    raw = []
    with engine_scope(Engine(backend=args.backend,device=args.device,threads=args.threads,
                library_path=args.library_path,plugin_path=args.plugin_path)) as engine:
        singles.append(("cold_engine_ms",milliseconds(start)))
        start = time.perf_counter()
        with PairDiscovery(engine,left,right) as discovery:
            singles.append(("cold_index_upload_ms",milliseconds(start)))
            start = time.perf_counter()
            first = discovery.page()
            singles.append(("cold_page_ms",milliseconds(start)))
            singles.append(("cold_to_first_page_ms",milliseconds(cold_start)))
            start = time.perf_counter()
            actual_left,actual_right = discovery.download_boxes()
            singles.append(("qualification_input_download_ms",milliseconds(start)))
            start = time.perf_counter()
            quality = {}
            quality.update(("left_"+key,value) for key,value in qualify_scene(left,actual_left).items())
            quality.update(("right_"+key,value) for key,value in qualify_scene(right,actual_right).items())
            quality.update(qualify_page(first,actual_left,actual_right))
            singles.append(("initial_qualification_ms",milliseconds(start)))
            if not first.info.flags & 2:
                raise RuntimeError("benchmark recipe exceeds the full-page cap")
            expected_survivors = sum(word.bit_count() for word in first.words)
            discovery.page()  # One excluded warmup; no event-profile overhead.
            for iteration in range(5):
                start = time.perf_counter()
                page = discovery.page()
                decode_start = time.perf_counter()
                shortlist = tuple(page.survivors())
                decode_ms = milliseconds(decode_start)
                actual_wall = milliseconds(start)
                if len(shortlist)!=expected_survivors or page.words!=first.words or page.left is not left or page.right is not right:
                    raise RuntimeError("reused page changed from the exact-qualified immutable inputs")
                raw.extend((phase,iteration,duration) for phase,duration in page.timings)
                raw.extend((("survivor_decode_ms",iteration,decode_ms),
                            ("actual_shortlist_caller_ms",iteration,actual_wall)))
    raw = [(phase,-1,duration) for phase,duration in singles]+raw
    tsv(output/"raw.tsv",("phase","iteration","milliseconds"),raw)
    tsv(output/"quality.tsv",("metric","value"),quality.items())
    tsv(output/"input.tsv",("field","value"), (("size",args.size),("backend",args.backend),
        ("device",args.device),("threads",args.threads),("left_charts",len(left.sources)),
        ("right_charts",len(right.sources)),("pairs",first.info.count),
        ("survivors",expected_survivors),("repeats",5),("profile",0),
        ("cuda_index_upload_bytes",48*(len(left.sources)+len(right.sources)) if args.backend=="cuda" else 0),
        ("cuda_mask_download_bytes",4*len(first.words) if args.backend=="cuda" else 0),
        ("topology_upload_bytes",0)))
    print(f"{args.backend}:{args.device} workers={args.threads} size={args.size} "
          f"pairs={first.info.count} survivors={expected_survivors}")


def render(rows,path):
    selected = [row for row in rows if row["phase"]=="actual_shortlist_caller_ms"]
    maximum = max(row["max_ms"] for row in selected)
    parts = ['<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 900 560">',
        '<rect width="900" height="560" fill="#101923"/>',
        '<g font-family="sans-serif">',
        '<text x="28" y="38" fill="#edf4fc" font-size="24">BRepGPU · resident pair shortlist costs</text>',
        '<text x="28" y="68" fill="#a9bed0" font-size="14">Five calls · retained common-world indexes · actual page, download, cleanup and Python decoding</text>']
    for index,row in enumerate(selected):
        y = 103+43*index
        label = f'CPU{row["threads"]}' if row["backend"]=="cpu" else f'P100 {row["device"]}'
        label += f' · {row["size"]} bodies/side'
        scale = 430/maximum
        color = "#f4c878" if row["backend"]=="cpu" else "#58c9e7"
        parts.extend((f'<text x="28" y="{y+16}" fill="#edf4fc" font-size="14">{label}</text>',
            f'<rect x="240" y="{y}" width="{scale*row["median_ms"]:.3f}" height="22" rx="3" fill="{color}"/>',
            f'<path d="M{240+scale*row["min_ms"]:.3f},{y+11} H{240+scale*row["max_ms"]:.3f}" stroke="#edf4fc" stroke-width="2"/>',
            f'<text x="720" y="{y+16}" fill="#edf4fc" font-size="14">{row["median_ms"]:.3f} ms</text>'))
    parts.extend(('<text x="28" y="486" fill="#a9bed0" font-size="14">Cold body/bounds preparation, upload and independent qualification reported separately.</text>',
        '<text x="28" y="513" fill="#a9bed0" font-size="14">Shared host; ranges are min/max. Closed-AABB survivors, not actual intersections.</text>',
        '<text x="28" y="541" fill="#a9bed0" font-size="14">No event timing in this matrix. Kernel-only speed does not establish whole-CAD speed.</text>',
        '</g></svg>'))
    path.write_text("\n".join(parts)+"\n")


def controller(args):
    output = args.output_directory
    output.mkdir(parents=True,exist_ok=False)
    private = ROOT/"context"/"pair"/("matrix-"+str(time.time_ns()))
    private.mkdir(parents=True)
    sources = ["CMakeLists.txt","native/runtime.cpp","native/cuda_backend.cu","native/pair_math.hpp",
               "include/brepgpu_pairs.h","native/pair_backend_api.h",
               "python/brepgpu/__init__.py","python/brepgpu/body.py","python/brepgpu/pair_scene.py",
               "native/discovery_cpu.cpp","native/discovery_cpu.h","native/discovery_math.hpp",
               "python/brepgpu/pair_oracles.py","examples/discover_pairs.py","benchmarks/pairs/run.py"]
    # Include owned implementation fragments if the native owner uses them.
    sources.extend(str(path.relative_to(ROOT)) for path in sorted((ROOT/"native").glob("pair*.inc")))
    tsv(output/"sources.tsv",("source","sha256"),
        ((name,hashlib.sha256((ROOT/name).read_bytes()).hexdigest()) for name in sources))
    core = args.library_path
    plugin = args.plugin_path or core.parent/"libbrepgpu_cuda.so"
    tsv(output/"binaries.tsv",("binary","sha256"),
        ((name,hashlib.sha256(path.read_bytes()).hexdigest()) for name,path in (("core",core),("cuda_plugin",plugin))))
    raw,summary = [],[]
    columns = ("size","backend","device","threads","pairs","survivors")
    for size in (4,128):
        for backend,device,threads in (("cpu",0,1),("cpu",0,24),("cuda",0,1),("cuda",1,1)):
            key = f"{size}-{backend}-{device}-{threads}"
            case = output/"cases"/key
            command = [sys.executable,str(Path(__file__).resolve()),"--worker","--size",str(size),
                "--backend",backend,"--device",str(device),"--threads",str(threads),
                "--library-path",str(core.resolve()),"--plugin-path",str(plugin.resolve()),
                "--output-directory",str(case)]
            completed = subprocess.run(command,capture_output=True,text=True,check=False)
            (private/(key+"-stdout.txt")).write_text(completed.stdout)
            (private/(key+"-stderr.txt")).write_text(completed.stderr)
            if completed.returncode:
                raise RuntimeError(f"pair case {key} failed; private logs retained")
            with (case/"input.tsv").open(newline="") as stream:
                fields = {row["field"]:row["value"] for row in csv.DictReader(stream,delimiter="\t")}
            identity = {name:fields[name] for name in columns}
            with (case/"raw.tsv").open(newline="") as stream:
                measurements = list(csv.DictReader(stream,delimiter="\t"))
            phases = {}
            for row in measurements:
                duration = float(row["milliseconds"])
                if not 0<=duration<float("inf"):
                    raise RuntimeError("nonfinite or negative measurement")
                raw.append({**identity,**row})
                phases.setdefault(row["phase"],[]).append(duration)
            for phase,values in phases.items():
                summary.append({**identity,"phase":phase,"samples":len(values),
                    "median_ms":statistics.median(values),"min_ms":min(values),"max_ms":max(values)})
            print(completed.stdout.strip(),flush=True)
    tsv(output/"raw.tsv",(*columns,"phase","iteration","milliseconds"),
        ((row[name] for name in (*columns,"phase","iteration","milliseconds")) for row in raw))
    tsv(output/"summary.tsv",(*columns,"phase","samples","median_ms","min_ms","max_ms"),
        ((row[name] for name in (*columns,"phase","samples","median_ms","min_ms","max_ms")) for row in summary))
    render(summary,output/"costs.svg")


if __name__=="__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--worker",action="store_true")
    parser.add_argument("--size",type=int,default=4)
    parser.add_argument("--backend",choices=("cpu","cuda"),default="cpu")
    parser.add_argument("--device",type=int,default=0)
    parser.add_argument("--threads",type=int,default=1)
    parser.add_argument("--library-path",type=Path,required=True)
    parser.add_argument("--plugin-path",type=Path)
    parser.add_argument("--output-directory",type=Path,required=True)
    arguments = parser.parse_args()
    worker(arguments) if arguments.worker else controller(arguments)
