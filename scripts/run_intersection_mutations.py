#!/usr/bin/env python3
"""Four direct production-geometry mutants, isolated on the Linux fleet."""
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

ROOT=Path(__file__).resolve().parents[1]
SOURCE="python/brepgpu/intersections.py"
MUTANTS=(
    ("missing-component", '("components", tuple(components))', '("components", tuple(components[:1]))',
     "test_four_clipped_arcs_complete_exact_endpoints"),
    ("ignored-hole", 'answer.append((side+":hole", polynomial))',
     'answer.append((side+":hole", (Fraction(1),)))', "test_small_wall_inside_plate_hole_is_empty"),
    ("split-seam-span", 'row["span_id"], row["kind"]', 'row["id"], row["kind"]',
     "test_cardinal_generators_one_physical_span_per_segment"),
    ("wrong-source-uv", 'left_map=plane_uv if plane_left else wall_uv', 'left_map=wall_uv if plane_left else wall_uv',
     "test_body_entry_point_complete_circle_and_coupled_maps"),
    ("reversed-component-order", 'components.append(Component(cid, kind, closed, self.tangent, tuple(order)))',
     'components.append(Component(cid, kind, closed, self.tangent, tuple(reversed(order))))',
     "test_hole_boundary_circle_and_crossing_arc"),
)


def run(command,env,log):
    with log.open("w") as stream:
        return subprocess.run(command,cwd=ROOT,env=env,stdout=stream,stderr=subprocess.STDOUT,check=False).returncode


def main(args):
    if sys.platform!="linux":
        raise RuntimeError("production mutants run on the fleet")
    args.output.parent.mkdir(parents=True,exist_ok=True)
    if args.output.exists():
        raise FileExistsError(args.output)
    private=ROOT/"context"/"intersection"/("mutants-"+str(time.time_ns()))
    private.mkdir(parents=True)
    paths=(args.library.resolve(),ROOT/SOURCE,ROOT/"tests/test_intersections.py",
           ROOT/"python/brepgpu/intersection_oracles.py",ROOT/"python/brepgpu/intersection_sampling.py")
    pinned=tuple(hashlib.sha256(p.read_bytes()).hexdigest() for p in paths)
    baseline_env=dict(os.environ,PYTHONPATH=str(ROOT/"python")+os.pathsep+str(ROOT/"tests"),
                      BREPGPU_LIBRARY=str(args.library.resolve()),BREPGPU_TEST_BACKEND="cpu")
    rows=[]
    for name,before,after,test in MUTANTS:
        command=[sys.executable,"-m","unittest","test_intersections.IntersectionTests."+test,"-v"]
        if run(command,baseline_env,private/(name+"-baseline.log")):
            raise RuntimeError("unmutated direct contract failed: "+name)
        tree=private/name
        shutil.copytree(ROOT/"python",tree/"python",ignore=shutil.ignore_patterns("__pycache__","*.pyc"))
        target=tree/SOURCE
        original=target.read_text()
        if original.count(before)!=1:
            raise RuntimeError("mutation source site changed: "+name)
        target.write_text(original.replace(before,after))
        env=dict(baseline_env,PYTHONPATH=str(tree/"python")+os.pathsep+str(ROOT/"tests"))
        status=run(command,env,private/(name+"-mutant.log"))
        log=(private/(name+"-mutant.log")).read_text()
        if not status or "AssertionError" not in log or not re.search(r"FAILED \(failures=\d+\)",log):
            raise RuntimeError("mutant was not killed by its direct assertion: "+name)
        rows.append((name,SOURCE,test,"PASS","KILLED",status,
                     hashlib.sha256((ROOT/SOURCE).read_bytes()).hexdigest(),
                     hashlib.sha256(target.read_bytes()).hexdigest(),pinned[0]))
        print(name,"KILLED by direct assertion",flush=True)
    if pinned!=tuple(hashlib.sha256(p.read_bytes()).hexdigest() for p in paths):
        raise RuntimeError("unmutated inputs changed during mutation campaign")
    with args.output.open("w",newline="") as stream:
        writer=csv.writer(stream,delimiter="\t",lineterminator="\n")
        writer.writerow(("mutant","source","direct_test","baseline","mutant_result","exit_code",
                         "baseline_source_sha256","mutant_source_sha256","used_core_sha256"))
        writer.writerows(rows)


if __name__=="__main__":
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--library",type=Path,required=True)
    parser.add_argument("--output",type=Path,required=True)
    main(parser.parse_args())
