#!/usr/bin/env python3
"""Qualify actual preserved v1/v2 artifacts against additive discovery.

Fresh process per row; no builds or writes to preserved directories. Public TSV
records identities and direct output checks, never operational paths. Transient
factory caching is covered separately by the existing modeled native scenario
(discovery-extension-transient); absent old v3 factories cannot supply that fault.
"""
import argparse
from array import array
import csv
from fractions import Fraction
import io
from pathlib import Path
import subprocess
import sys

sys.dont_write_bytecode=True

from check_projection_compatibility import digest, demand, load_adapter, LEGACY_REVISION

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"python"))
V2_REVISION="c3f70613526fc2d150ef992663ba373374f87759"
V2_ADAPTER_SHA256="ae708f5cf7138706bff4350a4a5515bea6dc3e90e7c45f6b49fcedec366d9fd0"
COLUMNS=("scenario","backend","device","reported_backend","adapter_generation",
    "adapter_revision","adapter_sha256","core_generation","core_sha256",
    "plugin_generation","plugin_sha256","evaluator_requests_before",
    "evaluator_requests_after","jet_component_checks","query_ok_count",
    "projection_status","projection_attempts","projection_component_checks",
    "discovery_status","discovery_attempts","discovery_certificate_checks",
    "maximum_E0_error","maximum_E1_error","maximum_E2_error","outcome")


def artifact_paths(args):
    return {1:args.v1_dir,2:args.v2_dir,3:args.current_dir}


def worker(args):
    import brepgpu as current
    from brepgpu.fixtures import ERROR_TARGETS,analytic_jet,exact_component_error,exact_jet,plane,rational_cylinder
    adapter=current if args.adapter_generation==3 else load_adapter(args.v1_adapter if args.adapter_generation==1 else args.v2_adapter)
    folders=artifact_paths(args)
    core=folders[args.core_generation]/"libbrepgpu.so"
    plugin=folders[args.plugin_generation]/"libbrepgpu_cuda.so"
    adapter_file=Path(current.__file__) if args.adapter_generation==3 else (args.v1_adapter if args.adapter_generation==1 else args.v2_adapter)
    adapter_revision={1:LEGACY_REVISION,2:V2_REVISION,3:args.current_revision}[args.adapter_generation]
    fixtures=(plane(),rational_cylinder())
    uv=tuple((s,t) for s in (0.,.25,.5,.75,1.) for t in (0.,.25,.5,.75,1.))
    import math
    uv+=((.37,.61),(math.nextafter(0.,1.),.5),(math.nextafter(1.,0.),.5))
    requests=[(slot,s,t) for slot in range(2) for s,t in uv]
    requests=requests[::2]+list(reversed(requests[1::2]))
    slots=array("I",(slot for slot,s,t in requests));parameters=array("d",(x for slot,s,t in requests for x in (s,t)))
    maxima=[Fraction(0)]*3;components=ok_count=reported=0
    projection_status=discovery_status="not_exposed_old_adapter"
    projection_attempts=projection_checks=discovery_attempts=discovery_checks=0
    target=(.25,.75,.4375)
    projection_supported=args.adapter_generation>=2 and args.core_generation>=2 and (args.backend=="cpu" or args.plugin_generation>=2)
    discovery_supported=args.adapter_generation==3 and args.core_generation==3 and (args.backend=="cpu" or args.plugin_generation==3)
    with adapter.Engine(backend=args.backend,device=args.device,library_path=str(core),
                        plugin_path=str(plugin) if args.backend=="cuda" else None) as engine:
        with engine.create_patch_set([adapter.Patch(f.degree_u,f.degree_v,f.controls) for f in fixtures]) as patches:
            with engine.create_query_set(patches,slots,parameters) as queries:
                def evaluator():
                    nonlocal components,ok_count,reported
                    for order in (0,1,2):
                        with engine.evaluate(queries,order) as result:
                            info=result.info();reported=int(info.backend)
                            demand(reported==(args.backend=="cuda"),"requested backend was substituted")
                            jets=result.download()
                        demand(jets.count==len(requests) and all(s==0 for s in jets.statuses),"evaluator size/status failure")
                        ok_count+=len(requests)
                        for i,(slot,s,t) in enumerate(requests):
                            exact=exact_jet(fixtures[slot],s,t,order)
                            demand(exact==analytic_jet(fixtures[slot],s,t,order),"fixture exact map differs")
                            for term,vector in enumerate(exact):
                                for axis,truth in enumerate(vector):
                                    error=exact_component_error(jets.data[(term*3+axis)*jets.count+i],truth)
                                    grade=0 if term==0 else 1 if term<3 else 2
                                    demand(error is not None and error<=ERROR_TARGETS[grade],"old/new evaluator geometry failure")
                                    for cumulative in range(grade,3):maxima[cumulative]=max(maxima[cumulative],error)
                                    components+=1
                    return len(requests)*3
                before=evaluator()
                if args.adapter_generation>=2:
                    for repeat in range(2):
                        if projection_supported:
                            with engine.create_projection_query_set(patches,[0],target,(.25,.75)) as pq:
                                with engine.project(pq) as pr:
                                    demand(pr.info().backend==(args.backend=="cuda"),"projection backend substituted")
                                    batch=pr.download().require_stationary()
                            demand(batch.count==1,"projection size failure")
                            exact=exact_jet(fixtures[0],*batch.uv(0),0)[0]
                            for axis,truth in enumerate(exact):
                                error=exact_component_error(batch.point(0)[axis],truth)
                                demand(error is not None and error<=ERROR_TARGETS[0],"projection point differs from original carrier")
                                projection_checks+=1
                            demand(tuple(batch.uv(0))==(.25,.75) and batch.distance_squared(0)==0 and batch.residual(0)==0,
                                   "exact seeded plane projection changed")
                            projection_status="STATIONARY"
                        else:
                            try:
                                with engine.create_projection_query_set(patches,[0],target,(.25,.75)):
                                    raise AssertionError("old interface unexpectedly accepts projection")
                            except adapter.BgError as error:
                                demand(error.code==adapter.BG_UNSUPPORTED and error.flags==0,"projection refusal was fatal or wrong code")
                                projection_status="UNSUPPORTED"
                        projection_attempts+=1
                if args.adapter_generation==3:
                    for repeat in range(2):
                        if discovery_supported:
                            with engine.create_hierarchy(patches,adapter.HierarchyOptions(depth=0)) as h:
                                with engine.create_discovery_query_set(h,[0],target) as dq:
                                    with engine.discover(dq) as dr:
                                        demand(dr.info().backend==(args.backend=="cuda"),"discovery backend substituted")
                                        batch=dr.download().require_gap()
                            demand(batch.count==1 and batch.flags[0]&adapter.BG_DISC_CANDIDATE_VALID,"discovery candidate absent")
                            exact=exact_jet(fixtures[0],*batch.uv(0),0)[0]
                            distance=sum((truth-Fraction.from_float(target[axis]))**2 for axis,truth in enumerate(exact))
                            lower,upper=map(Fraction.from_float,batch.bracket(0))
                            demand(lower<=0<=upper and distance<=upper,"discovery excludes exact minimum/witness")
                            demand(tuple(batch.targets)==target and batch.hierarchy_options.depth==0 and batch.hierarchy_options.mode==0,
                                   "discovery provenance/options changed")
                            discovery_checks+=2;discovery_status="GAP_SATISFIED"
                        else:
                            try:
                                with engine.create_hierarchy(patches,adapter.HierarchyOptions(depth=0)):
                                    raise AssertionError("old interface unexpectedly accepts discovery")
                            except adapter.BgError as error:
                                demand(error.code==adapter.BG_UNSUPPORTED and error.flags==0,"discovery refusal was fatal or wrong code")
                                discovery_status="UNSUPPORTED"
                        discovery_attempts+=1
                after=evaluator()
    return dict(zip(COLUMNS,(args.scenario,args.backend,args.device,reported,
        args.adapter_generation,adapter_revision,digest(adapter_file),args.core_generation,digest(core),
        args.plugin_generation if args.backend=="cuda" else "not_loaded",digest(plugin) if args.backend=="cuda" else "not_loaded",
        before,after,components,ok_count,projection_status,projection_attempts,projection_checks,
        discovery_status,discovery_attempts,discovery_checks,*(format(float(e),".17g") for e in maxima),"PASS")))


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--v1-dir",type=Path,default=ROOT/"context/projection/v1-baseline")
    parser.add_argument("--v2-dir",type=Path,default=ROOT/"context/discovery/v2-baseline")
    parser.add_argument("--v1-adapter",type=Path)
    parser.add_argument("--v2-adapter",type=Path,default=ROOT/"context/discovery/v2-baseline/python/brepgpu/__init__.py")
    parser.add_argument("--current-dir",type=Path,default=ROOT/"build/discovery-release")
    parser.add_argument("--output",type=Path,default=ROOT/"results/discovery-2026-10-14/compatibility.tsv")
    parser.add_argument("--devices",type=int,nargs="+",default=(0,1))
    parser.add_argument("--current-revision",default="working_tree")
    parser.add_argument("--worker",action="store_true")
    parser.add_argument("--scenario")
    parser.add_argument("--adapter-generation",type=int,choices=(1,2,3),default=3)
    parser.add_argument("--core-generation",type=int,choices=(1,2,3),default=3)
    parser.add_argument("--plugin-generation",type=int,choices=(1,2,3),default=3)
    parser.add_argument("--backend",choices=("cpu","cuda"),default="cpu")
    parser.add_argument("--device",type=int,default=0)
    args=parser.parse_args()
    if args.worker:
        row=worker(args);writer=csv.DictWriter(sys.stdout,fieldnames=COLUMNS,delimiter="\t",lineterminator="\n")
        writer.writeheader();writer.writerow(row);return
    demand(args.devices and len(set(args.devices))==len(args.devices) and all(d>=0 for d in args.devices),"invalid/duplicate device list")
    baseline_dirs=(args.v1_dir.resolve(),args.v2_dir.resolve())
    context=ROOT/"context/discovery-compatibility"
    demand(all(not context.resolve().is_relative_to(folder) for folder in baseline_dirs),"ignored worker context overlaps a protected baseline")
    demand(all(not args.output.resolve().is_relative_to(folder) for folder in baseline_dirs),"output overlaps a protected baseline")
    for folder in artifact_paths(args).values():
        for name in ("libbrepgpu.so","libbrepgpu_cuda.so"):demand((folder/name).is_file(),"required actual binary absent")
    demand(args.v2_adapter.is_file() and digest(args.v2_adapter)==V2_ADAPTER_SHA256,"actual preserved v2 adapter absent or changed")
    old_bytes=subprocess.run(["git","show",LEGACY_REVISION+":python/brepgpu/__init__.py"],cwd=ROOT,check=True,stdout=subprocess.PIPE).stdout
    if args.v1_adapter is None:
        preserved=args.v1_dir/"python/brepgpu/__init__.py"
        if preserved.is_file():args.v1_adapter=preserved
        else:
            context.mkdir(parents=True,exist_ok=True);args.v1_adapter=context/("adapter-"+LEGACY_REVISION+".py")
            if args.v1_adapter.exists():demand(args.v1_adapter.read_bytes()==old_bytes,"existing owned v1 adapter extraction changed")
            else:
                with args.v1_adapter.open("xb") as stream:stream.write(old_bytes)
    demand(args.v1_adapter.is_file() and args.v1_adapter.read_bytes()==old_bytes,"v1 adapter differs from recorded exact source revision")
    protected=[folder/name for folder in baseline_dirs for name in ("libbrepgpu.so","libbrepgpu_cuda.so")]+[args.v1_adapter,args.v2_adapter]
    demand(args.output.resolve() not in {p.resolve() for p in protected},"output aliases a protected input")
    identities={path:digest(path) for path in protected}
    context.mkdir(parents=True,exist_ok=True)
    if args.current_revision=="working_tree":
        base=subprocess.run(["git","rev-parse","HEAD"],cwd=ROOT,check=True,stdout=subprocess.PIPE,text=True).stdout.strip()
        committed=subprocess.run(["git","show",base+":python/brepgpu/__init__.py"],cwd=ROOT,check=True,stdout=subprocess.PIPE).stdout
        args.current_revision=base if committed==(ROOT/"python/brepgpu/__init__.py").read_bytes() else "working_tree_above_"+base
    combinations=[]
    for generation in (1,2):
        combinations.append((f"new-adapter-v{generation}-core","cpu",0,3,generation,generation))
        combinations.extend((f"new-adapter-v{generation}-core","cuda",d,3,generation,generation) for d in args.devices)
        combinations.extend((f"new-core-v{generation}-plugin","cuda",d,3,3,generation) for d in args.devices)
        combinations.extend((f"v{generation}-core-new-plugin","cuda",d,3,generation,3) for d in args.devices)
        combinations.append((f"v{generation}-adapter-new-libraries","cpu",0,generation,3,3))
        combinations.extend((f"v{generation}-adapter-new-libraries","cuda",d,generation,3,3) for d in args.devices)
    combinations.append(("current-positive-control","cpu",0,3,3,3))
    combinations.extend(("current-positive-control","cuda",d,3,3,3) for d in args.devices)
    rows=[]
    try:
        for scenario,backend,device,adapter,core,plugin in combinations:
            command=[sys.executable,"-B",str(Path(__file__).resolve()),"--worker","--scenario",scenario,"--backend",backend,"--device",str(device),
                "--adapter-generation",str(adapter),"--core-generation",str(core),"--plugin-generation",str(plugin),
                "--v1-dir",str(args.v1_dir.resolve()),"--v2-dir",str(args.v2_dir.resolve()),"--v1-adapter",str(args.v1_adapter.resolve()),
                "--v2-adapter",str(args.v2_adapter.resolve()),"--current-dir",str(args.current_dir.resolve()),"--current-revision",args.current_revision]
            completed=subprocess.run(command,cwd=ROOT,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,timeout=60)
            (context/(scenario+"-"+backend+str(device)+".log")).write_text(completed.stdout+completed.stderr)
            demand(completed.returncode==0,scenario+"/"+backend+str(device)+" failed; see ignored worker log")
            parsed=list(csv.DictReader(io.StringIO(completed.stdout),delimiter="\t"))
            demand(len(parsed)==1 and parsed[0]["outcome"]=="PASS","compatibility worker did not emit one passing row")
            rows.append(parsed[0]);print(scenario,backend,device,"PASS",flush=True)
    finally:
        demand(all(digest(path)==value for path,value in identities.items()),"protected baseline input changed")
    args.output.parent.mkdir(parents=True,exist_ok=True)
    with args.output.open("w",newline="") as out:
        writer=csv.DictWriter(out,fieldnames=COLUMNS,delimiter="\t",lineterminator="\n");writer.writeheader();writer.writerows(rows)
    print("compatibility rows:",len(rows),"jet components:",sum(int(r["jet_component_checks"]) for r in rows),
          "discovery attempts:",sum(int(r["discovery_attempts"]) for r in rows))


if __name__=="__main__":main()
