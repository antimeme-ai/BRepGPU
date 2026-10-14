#!/usr/bin/env python3
"""Four direct production mutants; fleet-only, isolated owned source copies."""
import argparse
import csv
import hashlib
import os
import re
from pathlib import Path
import shutil
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
MUTANTS = (
    ("closed-contact", "native/pair_math.hpp",
     "key(l.lower[a])>key(r.upper[a])||key(r.lower[a])>key(l.upper[a])",
     "key(l.lower[a])>=key(r.upper[a])||key(r.lower[a])>=key(l.upper[a])",
     "test_closed_box_contact_is_not_lost"),
    ("page-offset", "native/pair_math.hpp", "q=start+first+bit", "q=first+bit",
     "test_asymmetric_offsets_tails_and_complete_page_walk"),
    ("tail-padding", "native/pair_math.hpp", "return mask;", "return mask|0x80000000u;",
     "test_asymmetric_offsets_tails_and_complete_page_walk"),
    ("world-origin", "python/brepgpu/pair_scene.py",
     "origins[a % 3] + scale * Fraction.from_float(value)",
     "scale * Fraction.from_float(value)",
     "test_scene_selection_exact_world_bounds_and_frame_relations"),
)


def run(command, directory, environment, log):
    with log.open("w") as stream:
        return subprocess.run(command,cwd=directory,env=environment,stdout=stream,
                              stderr=subprocess.STDOUT,check=False).returncode


def main(args):
    if sys.platform!="linux":
        raise RuntimeError("production mutants must run on the Linux fleet")
    private = ROOT/"context"/"pair"/("mutants-"+str(time.time_ns()))
    private.mkdir(parents=True)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    if args.output.exists():
        raise FileExistsError(args.output)
    baseline_env = dict(os.environ,PYTHONPATH=str(ROOT/"python")+os.pathsep+str(ROOT/"tests"),
                        BREPGPU_LIBRARY=str(args.library.resolve()),BREPGPU_TEST_BACKEND="cpu")
    rows = []
    observed = [args.library.resolve(), ROOT/"tests/test_pairs.py", ROOT/"python/brepgpu/__init__.py",
                ROOT/"python/brepgpu/pair_oracles.py", ROOT/"python/brepgpu/pair_scene.py",
                ROOT/"native/pair_math.hpp", ROOT/"native/pair_runtime.inc"]
    pinned = tuple(hashlib.sha256(path.read_bytes()).hexdigest() for path in observed)
    for name,source,before,after,test in MUTANTS:
        command = [sys.executable,"-m","unittest","test_pairs.PairTests."+test,"-v"]
        if run(command,ROOT,baseline_env,private/(name+"-baseline.log")):
            raise RuntimeError("unmutated direct test failed: "+name)
        tree = private/name
        tree.mkdir()
        for folder in ("native","include","python"):
            shutil.copytree(ROOT/folder,tree/folder,ignore=shutil.ignore_patterns("__pycache__","*.pyc"))
        shutil.copy2(ROOT/"CMakeLists.txt",tree/"CMakeLists.txt")
        target = tree/source
        original = target.read_text()
        if original.count(before)!=1:
            raise RuntimeError("mutation source site changed: "+name)
        target.write_text(original.replace(before,after))
        library = args.library.resolve()
        if source.startswith("native/"):
            if run(["cmake","-S",str(tree),"-B",str(tree/"build"),"-DCMAKE_BUILD_TYPE=Release"],
                   ROOT,os.environ,private/(name+"-configure.log")):
                raise RuntimeError("mutant configuration failed: "+name)
            if run(["cmake","--build",str(tree/"build"),"--target","brepgpu","-j8"],
                   ROOT,os.environ,private/(name+"-build.log")):
                raise RuntimeError("mutant build failed: "+name)
            library = tree/"build"/"libbrepgpu.so"
        env = dict(baseline_env,PYTHONPATH=str(tree/"python")+os.pathsep+str(ROOT/"tests"),
                   BREPGPU_LIBRARY=str(library))
        code = run(command,ROOT,env,private/(name+"-mutant.log"))
        if not code:
            raise RuntimeError("production mutant survived direct oracle: "+name)
        # Inspect a real assertion failure, rather than crediting import/build errors.
        log = (private/(name+"-mutant.log")).read_text()
        if "AssertionError" not in log or not re.search(r"FAILED \(failures=\d+\)",log):
            raise RuntimeError("mutant failed outside its direct assertion: "+name)
        rows.append((name,source,test,"PASS","KILLED",code,
                     hashlib.sha256((ROOT/source).read_bytes()).hexdigest(),
                     hashlib.sha256(target.read_bytes()).hexdigest(),
                     hashlib.sha256(library.read_bytes()).hexdigest()))
        print(name,"KILLED by direct assertion",flush=True)
    if pinned != tuple(hashlib.sha256(path.read_bytes()).hexdigest() for path in observed):
        raise RuntimeError("unmutated qualification inputs changed during campaign")
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
