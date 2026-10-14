#!/usr/bin/env python3
"""Exercise actual preserved v1 artifacts and the current projection boundary.

Each combination runs in a fresh process. This reads preserved binaries and
extracts the committed old adapter only into ignored context; it never builds
or overwrites a baseline. The TSV contains source/binary identities and direct
geometry outcomes, without workstation paths or host identifiers.
"""
import argparse
from array import array
import csv
from fractions import Fraction
import hashlib
import importlib.util
import io
import math
from pathlib import Path
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"python"))
LEGACY_REVISION="1bc2d22f0cd3f977993906ad105e24aadc4bb56a"
COLUMNS=("scenario","backend","device","reported_backend","adapter_revision",
    "adapter_sha256","core_generation","core_sha256","plugin_generation","plugin_sha256",
    "requests_before","requests_after","component_checks","query_ok_count",
    "projection_status","projection_attempts","maximum_E0_error","maximum_E1_error",
    "maximum_E2_error","outcome")

def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def demand(condition,message):
    if not condition:raise AssertionError(message)

def load_adapter(path):
    spec=importlib.util.spec_from_file_location("brepgpu_compatibility_v1",path)
    module=importlib.util.module_from_spec(spec)
    sys.modules[spec.name]=module
    spec.loader.exec_module(module)
    return module

def run_case(args):
    import brepgpu
    from brepgpu.fixtures import ERROR_TARGETS,analytic_jet,exact_component_error,exact_jet,plane,rational_cylinder

    old_adapter=args.worker=="old-adapter-new-core"
    adapter=load_adapter(args.legacy_adapter) if old_adapter else brepgpu
    old_core=args.worker=="new-adapter-old-core"
    old_plugin=old_core or args.worker=="new-core-old-plugin"
    core=Path(args.baseline_dir if old_core else args.current_dir)/"libbrepgpu.so"
    plugin=Path(args.baseline_dir if old_plugin else args.current_dir)/"libbrepgpu_cuda.so"
    adapter_file=Path(args.legacy_adapter) if old_adapter else Path(brepgpu.__file__)
    fixtures=(plane(),rational_cylinder())
    uv=tuple((s,t) for s in (0.,.25,.5,.75,1.) for t in (0.,.25,.5,.75,1.))
    uv+=((.37,.61),(math.nextafter(0.,1.),.5),(math.nextafter(1.,0.),.5))
    requests=[(slot,s,t) for slot in range(2) for s,t in uv]
    requests=requests[::2]+list(reversed(requests[1::2]))
    slots=array("I",(slot for slot,s,t in requests))
    parameters=array("d",(x for slot,s,t in requests for x in (s,t)))
    maxima=[Fraction(0)]*3
    components=ok_count=before=after=0
    reported=-1
    with adapter.Engine(backend=args.backend,device=args.device,library_path=str(core),
                        plugin_path=str(plugin) if args.backend=="cuda" else None) as engine:
        with engine.create_patch_set([adapter.Patch(f.degree_u,f.degree_v,f.controls) for f in fixtures]) as patches:
            with engine.create_query_set(patches,slots,parameters) as queries:
                def check_evaluation():
                    nonlocal components,ok_count,reported
                    for order in (0,1,2):
                        with engine.evaluate(queries,order) as result:
                            info=result.info()
                            demand(info.backend==(1 if args.backend=="cuda" else 0),"selected backend was substituted")
                            reported=int(info.backend)
                            jets=result.download()
                        demand(jets.count==len(requests) and all(status==0 for status in jets.statuses),
                               "evaluator returned invalid size/status")
                        ok_count+=len(requests)
                        for i,(slot,s,t) in enumerate(requests):
                            exact=exact_jet(fixtures[slot],s,t,order)
                            analytic=analytic_jet(fixtures[slot],s,t,order)
                            demand(exact==analytic,"fixture control bits do not represent the independently known map")
                            for term,vector in enumerate(exact):
                                for axis,truth in enumerate(vector):
                                    value=jets.data[(term*3+axis)*jets.count+i]
                                    error=exact_component_error(value,truth)
                                    grade=0 if term==0 else (1 if term<3 else 2)
                                    demand(error is not None and error<=ERROR_TARGETS[grade],
                                           "preserved/current evaluator differs from direct rational geometry")
                                    for cumulative in range(grade,3):maxima[cumulative]=max(maxima[cumulative],error)
                                    components+=1
                    return 3*len(requests)
                before=check_evaluation()
                projection_status="not_attempted_old_adapter"
                attempts=0
                if not old_adapter:
                    # A known local target/seed: unsupported is capability
                    # negotiation, not rejection of malformed numerical input.
                    for repeat in range(2):
                        try:
                            with engine.create_projection_query_set(patches,[0],(.25,.75,.4375),(.25,.75)):
                                raise AssertionError("old interface unexpectedly accepted projection")
                        except adapter.BgError as error:
                            demand(error.code==adapter.BG_UNSUPPORTED and error.flags==0,
                                   "projection refusal has wrong status or poisoned CUDA")
                            projection_status=str(error.code);attempts+=1
                    after=check_evaluation()
    return dict(zip(COLUMNS,(args.worker,args.backend,args.device,reported,
        LEGACY_REVISION if old_adapter else args.current_revision,digest(adapter_file),
        "preserved_v1" if old_core else "current_v2",digest(core),
        ("preserved_v1" if old_plugin else "current_v2") if args.backend=="cuda" else "not_loaded",
        digest(plugin) if args.backend=="cuda" else "not_loaded",before,after,components,ok_count,
        projection_status,attempts,*(format(float(error),".17g") for error in maxima),"PASS")))

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline-dir",type=Path,default=ROOT/"context/projection/v1-baseline")
    parser.add_argument("--current-dir",type=Path,default=ROOT/"build/projection-release")
    parser.add_argument("--output",type=Path,default=ROOT/"results/projection-2026-10-14/compatibility.tsv")
    parser.add_argument("--devices",type=int,nargs="+",default=(0,1))
    parser.add_argument("--current-revision",default="working_tree")
    parser.add_argument("--worker",choices=("new-adapter-old-core","new-core-old-plugin","old-adapter-new-core"))
    parser.add_argument("--backend",choices=("cpu","cuda"),default="cpu")
    parser.add_argument("--device",type=int,default=0)
    parser.add_argument("--legacy-adapter",type=Path)
    args=parser.parse_args()
    if args.worker:
        row=run_case(args)
        writer=csv.DictWriter(sys.stdout,fieldnames=COLUMNS,delimiter="\t",lineterminator="\n")
        writer.writeheader();writer.writerow(row)
        return

    # Immutable baseline inputs must exist before context extraction or runs.
    for folder in (args.baseline_dir,args.current_dir):
        for name in ("libbrepgpu.so","libbrepgpu_cuda.so"):
            demand((folder/name).is_file(),"required actual binary absent")
    identities={path:digest(path) for path in (args.baseline_dir/"libbrepgpu.so",args.baseline_dir/"libbrepgpu_cuda.so")}
    context=ROOT/"context/projection-compatibility"
    context.mkdir(parents=True,exist_ok=True)
    legacy=context/("adapter-"+LEGACY_REVISION+".py")
    old_bytes=subprocess.run(["git","show",LEGACY_REVISION+":python/brepgpu/__init__.py"],
                             cwd=ROOT,check=True,stdout=subprocess.PIPE).stdout
    if legacy.exists():demand(legacy.read_bytes()==old_bytes,"existing legacy adapter extraction differs")
    else:legacy.write_bytes(old_bytes)
    current_revision=args.current_revision
    if current_revision=="working_tree":
        base=subprocess.run(["git","rev-parse","HEAD"],cwd=ROOT,check=True,
                            stdout=subprocess.PIPE,text=True).stdout.strip()
        committed=subprocess.run(["git","show",base+":python/brepgpu/__init__.py"],cwd=ROOT,
                                 check=True,stdout=subprocess.PIPE).stdout
        current_revision=base if committed==(ROOT/"python/brepgpu/__init__.py").read_bytes() else "working_tree_above_"+base
    combinations=[("new-adapter-old-core","cpu",0)]
    combinations += [("new-adapter-old-core","cuda",device) for device in args.devices]
    combinations += [("new-core-old-plugin","cuda",device) for device in args.devices]
    combinations += [("old-adapter-new-core","cpu",0)]
    combinations += [("old-adapter-new-core","cuda",device) for device in args.devices]
    rows=[]
    for scenario,backend,device in combinations:
        command=[sys.executable,str(Path(__file__).resolve()),"--worker",scenario,"--backend",backend,
            "--device",str(device),"--baseline-dir",str(args.baseline_dir.resolve()),
            "--current-dir",str(args.current_dir.resolve()),"--legacy-adapter",str(legacy),
            "--current-revision",current_revision]
        completed=subprocess.run(command,cwd=ROOT,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
        (context/(scenario+"-"+backend+str(device)+".log")).write_text(completed.stdout+completed.stderr)
        demand(completed.returncode==0,scenario+"/"+backend+str(device)+" failed; see ignored worker log")
        parsed=list(csv.DictReader(io.StringIO(completed.stdout),delimiter="\t"))
        demand(len(parsed)==1 and parsed[0]["outcome"]=="PASS","fresh compatibility process did not return a passing row")
        rows.append(parsed[0]);print(scenario,backend,device,"PASS",flush=True)
    demand(all(digest(path)==value for path,value in identities.items()),"preserved binary was changed")
    args.output.parent.mkdir(parents=True,exist_ok=True)
    with args.output.open("w",newline="") as stream:
        writer=csv.DictWriter(stream,fieldnames=COLUMNS,delimiter="\t",lineterminator="\n")
        writer.writeheader();writer.writerows(rows)
    print("compatibility rows:",len(rows),"component checks:",sum(int(row["component_checks"]) for row in rows))

if __name__=="__main__":main()
