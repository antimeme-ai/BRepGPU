#!/usr/bin/env python3
"""Focused new Python operation and unchanged native/body/pair regression checks."""
import argparse
import csv
import os
from pathlib import Path
import re
import subprocess
import sys
import time

ROOT=Path(__file__).resolve().parents[1]


def main(args):
    if sys.platform != "linux":
        raise RuntimeError("native qualification runs on the fleet")
    args.output.parent.mkdir(parents=True,exist_ok=True)
    if args.output.exists():
        raise FileExistsError(args.output)
    private=ROOT/"context"/"intersection"/("qualification-"+str(time.time_ns()))
    private.mkdir(parents=True)
    rows=[]
    for key,backend,device,threads,suites in (
        ("cpu1-new-and-regression","cpu",0,1,("test_intersections","test_body","test_pairs")),
        ("cpu24-intersections","cpu",0,24,("test_intersections",)),
        ("p100-0-intersections","cuda",0,1,("test_intersections",)),
        ("p100-1-intersections","cuda",1,1,("test_intersections",))):
        if args.sampling_classes_only:
            key=key.split("-")[0]+("-"+str(device) if backend=="cuda" else "")+"-sampling-classes"
            suites=("test_intersections.IntersectionTests.test_sampling_consumes_generators_points_and_framed_faces",)
        env=dict(os.environ,PYTHONPATH=str(ROOT/"python")+os.pathsep+str(ROOT/"tests"),
            BREPGPU_LIBRARY=str(args.core.resolve()),BREPGPU_TEST_BACKEND=backend,
            BREPGPU_TEST_DEVICE=str(device),BREPGPU_TEST_THREADS=str(threads),
            BREPGPU_CUDART=str(args.cudart.resolve()))
        command=[sys.executable,"-m","unittest","-v",*suites]
        if backend == "cuda":
            command=[str(args.compute_sanitizer.resolve()),"--tool","memcheck","--leak-check","full",
                     "--error-exitcode","86",*command]
        start=time.perf_counter()
        with (private/(key+".log")).open("w") as stream:
            completed=subprocess.run(command,cwd=ROOT,env=env,stdout=stream,stderr=subprocess.STDOUT,check=False)
        wall=time.perf_counter()-start
        log=(private/(key+".log")).read_text()
        if completed.returncode:
            print("\n".join(log.splitlines()[-50:]),flush=True)
            raise RuntimeError("qualification failed: "+key)
        match=re.search(r"Ran (\d+) tests? in ([0-9.]+)s",log)
        if not match:
            raise RuntimeError("unittest summary absent: "+key)
        skips=re.search(r"OK \(skipped=(\d+)\)",log)
        errors=leaks="not-run"
        if backend == "cuda":
            if "ERROR SUMMARY: 0 errors" not in log or "LEAK SUMMARY: 0 bytes leaked in 0 allocations" not in log:
                raise RuntimeError("device memory summary not clean: "+key)
            errors=leaks=0
        rows.append((key,backend,device,threads,",".join(suites),int(match[1]),int(skips[1]) if skips else 0,
                     float(match[2]),wall,errors,leaks,"PASS"))
        print(key,match[1]+" tests",f"{wall:.3f}s","PASS",flush=True)
    with args.output.open("w",newline="") as stream:
        writer=csv.writer(stream,delimiter="\t",lineterminator="\n")
        writer.writerow(("case","backend","device","threads","suites","tests","skipped",
            "suite_seconds","process_seconds","device_errors","device_leaked_bytes","status"))
        writer.writerows(rows)


if __name__ == "__main__":
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--core",type=Path,required=True)
    parser.add_argument("--compute-sanitizer",type=Path,required=True)
    parser.add_argument("--cudart",type=Path,required=True)
    parser.add_argument("--output",type=Path,required=True)
    parser.add_argument("--sampling-classes-only",action="store_true",
                        help="Run only the new consumer class/frame test after previous families passed")
    main(parser.parse_args())
