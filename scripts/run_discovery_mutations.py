#!/usr/bin/env python3
"""Isolated production discovery mutations; execute only on the qualified fleet.

No authoritative source is edited. Outputs/logs/copies are confined to a fresh
repository context/discovery directory. Build/loader failures are not kills.
"""
import argparse
import csv
import hashlib
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
STRICT = ("-std=c++17", "-O2", "-fno-fast-math", "-ffp-contract=off", "-fPIC", "-pthread")
# (name, file, unique old anchor, replacement, exact case, mode)
MUTATIONS = (
    ("subdivision-inward", "native/discovery_math.hpp",
     "return {down_mul(down_add(a.lo,b.lo),.5),up_mul(up_add(a.hi,b.hi),.5)};",
     "return {up_mul(up_add(a.hi,b.hi),.5),down_mul(down_add(a.lo,b.lo),.5)};",
     "degree_3_3_asymmetric", 0),
    ("interval-rn", "native/discovery_math.hpp", None, None,
     "one_third_false_zero", 0),
    ("rounded-root-upper", "native/discovery_math.hpp",
     "!witness_upper(original_net,p,q,.5,.5,target,answer.fields[7])",
     "!(answer.fields[7]=answer.fields[5],true)",
     "one_third_false_zero", 0),
    ("dop-wrong-norm", "native/discovery_math.hpp",
     "down_div(down_mul(gap,gap),n<6?2:3)",
     "down_div(down_mul(gap,gap),1)", "diagonal_norm3", 1),
    ("dop-target-sign", "native/discovery_math.hpp",
     "out.dots[n]=projection(values,n);", "out.dots[n]=negate(projection(values,n));",
     "degree_0_0_constant", 1),
    ("bernstein-negative-c0", "native/discovery_cpu.cpp",
     "intersect(coefficient[c],c==4?1:-limit,limit)",
     "intersect(coefficient[c],c==4?1:(c==0?0:-limit),limit)", "negative_C0", 2),
    ("bernstein-padding", "native/discovery_math.hpp",
     "const unsigned count=(2*p+1)*(2*q+1);", "const unsigned count=49;",
     "negative_C0", 2),
    ("cap-partial-lower", "native/discovery_math.hpp",
     "answer.fields[6]=capped?root_lower:minimum(answer.fields[7],least);",
     "answer.fields[6]=minimum(answer.fields[7],least);", "cap_wide", 0),
)

# This probe consumes the existing independent exact geometry oracle. Return10
# means an actual geometric/cache/contract failure,20 is infrastructure failure.
PYTHON_PROBE = r'''
import argparse
import sys
from brepgpu import Engine,Patch,HierarchyOptions,DiscoveryOptions,BgError,BG_INTERNAL_ERROR
from brepgpu.discovery_fixtures import discovery_cases,qualify_hierarchy,qualify_row
p=argparse.ArgumentParser();p.add_argument("case");p.add_argument("mode",type=int);p.add_argument("library")
a=p.parse_args();case=next(c for c in discovery_cases() if c.name==a.case);f=case.fixture
try:
    with Engine(backend="cpu",library_path=a.library) as engine:
        with engine.create_patch_set([Patch(f.degree_u,f.degree_v,f.controls)]) as patches:
            with engine.create_hierarchy(patches,HierarchyOptions(case.depth,mode=a.mode)) as hierarchy:
                failures,oracle=qualify_hierarchy((f,),hierarchy.download(),case.depth,a.mode)
                if failures or oracle is None:
                    print("GEOMETRY_RED cache: "+"; ".join(failures));raise SystemExit(10)
                with engine.create_discovery_query_set(hierarchy,[0],case.target) as query:
                    with engine.discover(query,DiscoveryOptions(case.max_visits,case.max_refinements,case.tolerance_squared)) as result:
                        batch=result.download()
                if batch.count!=1 or len(batch.data)!=8:
                    print("GEOMETRY_RED output shape");raise SystemExit(10)
                failures,_=qualify_row(case,tuple(batch.data),int(batch.statuses[0]),int(batch.flags[0]),
                    int(batch.visits[0]),int(batch.pruned[0]),int(batch.refinements[0]),int(batch.evaluation_counts[0]),oracle,0)
                if failures:
                    print("GEOMETRY_RED row: "+"; ".join(failures));raise SystemExit(10)
    print("GEOMETRY_GREEN "+case.name+" mode="+str(a.mode))
except BgError as e:
    if e.code==BG_INTERNAL_ERROR:
        print("GEOMETRY_RED valid-input numerical construction: "+str(e));raise SystemExit(10)
    print("INFRASTRUCTURE_FAILURE "+str(e));raise SystemExit(20)
except Exception as e:
    print("INFRASTRUCTURE_FAILURE "+type(e).__name__+": "+str(e));raise SystemExit(20)
'''

CUDA_PROBE = r'''
#include "discovery_math.hpp"
#include <cuda_runtime.h>
#include <cstdlib>
#include <cstdio>
struct Zero {
    __device__ bool position(double,double,double* out) noexcept {
        for(unsigned i=0;i<3;++i)out[i]=0;return true;
    }
    __device__ bool full(double,double,double* out) noexcept {
        for(unsigned i=0;i<18;++i)out[i]=0;return true;
    }
};
__global__ void production_zero(brepgpu::DiscoveryCandidate* result) {
    const double controls[4]={0,0,0,1},target[3]={0,0,0};
    bg_hierarchy_node node{};node.flags=BG_HIERARCHY_LEAF;
    node.left=node.right=UINT32_MAX;
    const bg_discovery_options options{3,sizeof(bg_discovery_options),1,0,0,0,0};
    Zero evaluator;*result=brepgpu::discover_local(evaluator,controls,0,0,&node,1,0,
        nullptr,BG_BOUND_AABB,target,options);
}
int main(int argc,char**argv){
    const int device=argc==2?std::atoi(argv[1]):0;
    auto check=[](cudaError_t code){if(code!=cudaSuccess){
        std::fprintf(stderr,"INFRASTRUCTURE_FAILURE CUDA %s\n",cudaGetErrorString(code));return false;}return true;};
    if(!check(cudaSetDevice(device)))return 20;
    brepgpu::DiscoveryCandidate* out=nullptr;
    if(!check(cudaMalloc(&out,sizeof(*out))))return 20;
    production_zero<<<1,1>>>(out);bool ok=check(cudaGetLastError())&&check(cudaDeviceSynchronize());
    brepgpu::DiscoveryCandidate host{};
    if(ok)ok=check(cudaMemcpy(&host,out,sizeof host,cudaMemcpyDeviceToHost));
    const bool freed=check(cudaFree(out));if(!ok||!freed)return 20;
    const bool correct=host.status==BG_DISC_GAP_SATISFIED&&host.fields[6]==0&&host.fields[7]==0
        &&(host.flags&BG_DISC_CANDIDATE_VALID)&&(host.flags&BG_DISC_COVERAGE_COMPLETE)
        &&host.visits==1&&host.pruned==0&&host.refinements==0&&host.evaluations==1;
    std::printf("%s status=%u flags=%u L=%.17g U=%.17g visits=%u pruned=%u\n",
        correct?"GEOMETRY_GREEN":"GEOMETRY_RED",host.status,host.flags,host.fields[6],host.fields[7],host.visits,host.pruned);
    return correct?0:10;
}
'''


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def copy_source(destination):
    # Only active owned source, never archives, instructions, baseline binaries
    # or Git metadata. copy2 produces independent files rather than hard links.
    destination.mkdir()
    for name in ("include", "native", "python"):
        shutil.copytree(ROOT/name,destination/name,
                        ignore=shutil.ignore_patterns("__pycache__", "*.pyc", ".DS_Store"))


def edit(source,name,path,old,new):
    file=source/path;before=file.read_text()
    if name=="interval-rn":
        after=before
        for expression in ("a+b", "a-b", "a*b", "a/b"):
            for direction in ("-", ""):
                anchor=f"std::nextafter({expression},{direction}std::numeric_limits<double>::infinity())"
                if after.count(anchor)!=1:raise RuntimeError("source anchor changed: "+anchor)
                after=after.replace(anchor,expression)
    else:
        if before.count(old)!=1:raise RuntimeError("source anchor changed: "+str(old))
        after=before.replace(old,new)
    file.write_text(after)
    return hashlib.sha256(before.encode()).hexdigest(),sha(file)


def command(arguments,log,env=None):
    started=time.monotonic()
    with log.open("w") as stream:
        stream.write("command: "+repr(list(map(str,arguments)))+"\n");stream.flush()
        result=subprocess.run(list(map(str,arguments)),stdout=stream,stderr=subprocess.STDOUT,env=env)
    return result.returncode,time.monotonic()-started


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--run-id",required=True,help="fresh owned context/discovery subdirectory")
    p.add_argument("--cxx",default="g++")
    p.add_argument("--nvcc",help="optional pinned nvcc for the sm_60 equality-pruning check")
    p.add_argument("--device",type=int,default=0)
    args=p.parse_args()
    if platform.system()!="Linux" or platform.machine()!="x86_64":
        p.error("native mutation execution is confined to the qualified Linux x86-64 fleet")
    if not args.run_id or Path(args.run_id).name!=args.run_id or args.run_id in (".",".."):
        p.error("run-id must be one ordinary directory component")
    home=ROOT/"context"/"discovery"/args.run_id
    if home.exists():p.error("run directory already exists; preserved artifacts will not be overwritten")
    home.mkdir(parents=True)
    (home/"probe.py").write_text(PYTHON_PROBE)
    (home/"zero-pruning.cu").write_text(CUDA_PROBE)
    baseline=home/"baseline";copy_source(baseline)
    objects=home/"objects";objects.mkdir()
    rows=[];manifest=[]
    env=dict(os.environ,PYTHONPATH=str(baseline/"python"))
    manifest.extend(("baseline",str(path.relative_to(baseline)),sha(path),sha(path))
                    for path in baseline.rglob("*") if path.is_file())
    def compile_object(source,name,destination,log):
        return command([args.cxx,*STRICT,"-I",source/"include","-I",source/"native",
                        "-c",source/"native"/name,"-o",destination],log)
    def build(source,builddir,common):
        builddir.mkdir()
        obj=builddir/"discovery_cpu.o"
        code,cost=compile_object(source,"discovery_cpu.cpp",obj,builddir/"compile.log")
        if code:return code,cost,None
        library=builddir/"libbrepgpu.so"
        code,elapsed=command([args.cxx,"-shared",*STRICT,*common,obj,"-ldl","-o",library],builddir/"link.log")
        return code,cost+elapsed,library
    common=[]
    for name in ("runtime.cpp","evaluator_cpu.cpp","projection_cpu.cpp"):
        obj=objects/(name+".o");code,_=compile_object(baseline,name,obj,objects/(name+".log"))
        if code:raise RuntimeError("unchanged production object build failed: "+name)
        common.append(obj)
    code,baseline_build_cost,library=build(baseline,home/"baseline-build",common)
    if code:raise RuntimeError("unchanged production discovery build failed")
    for name,path,old,new,case,mode in MUTATIONS:
        green=home/(name+"-baseline.log")
        code,elapsed=command([sys.executable,home/"probe.py",case,mode,library],green,env)
        rows.append((name,"cpu","baseline",code,"green" if code==0 else "infrastructure",elapsed,str(green.relative_to(home))))
        if code:raise RuntimeError("unmutated associated geometry oracle not green: "+name)
        source=home/name;copy_source(source)
        old_sha,new_sha=edit(source,name,path,old,new);manifest.append((name,path,old_sha,new_sha))
        code,cost,mutant=build(source,home/(name+"-build"),common)
        if code:
            rows.append((name,"cpu","mutant-build",code,"build-failure-not-kill",cost,name+"-build/compile.log"));continue
        red=home/(name+"-mutant.log")
        code,elapsed=command([sys.executable,home/"probe.py",case,mode,mutant],red,env)
        outcome="killed" if code==10 else "survived" if code==0 else "infrastructure-not-kill"
        rows.append((name,"cpu","mutant",code,outcome,elapsed,str(red.relative_to(home))))
    if args.nvcc:
        name="prune-equality";source=home/name;copy_source(source)
        old_sha,new_sha=edit(source,name,"native/discovery_math.hpp",
                           "if(lower>answer.fields[7])", "if(lower>=answer.fields[7])")
        manifest.append((name,"native/discovery_math.hpp",old_sha,new_sha))
        for variant,active in (("baseline",baseline),("mutant",source)):
            binary=home/(name+"-"+variant)
            log=home/(name+"-"+variant+"-build.log")
            code,cost=command([args.nvcc,"-std=c++17","-O2","-arch=sm_60","--fmad=false","--ftz=false",
                "--prec-div=true","--prec-sqrt=true","-Xcompiler=-fno-fast-math,-ffp-contract=off",
                "-I",active/"include","-I",active/"native",home/"zero-pruning.cu","-o",binary],log)
            if code:
                rows.append((name,"cuda",variant+"-build",code,"build-failure-not-kill",cost,str(log.relative_to(home))))
                break
            log=home/(name+"-"+variant+".log")
            code,cost=command([binary,args.device],log)
            outcome=("green" if code==0 else "infrastructure") if variant=="baseline" else (
                "killed" if code==10 else "survived" if code==0 else "infrastructure-not-kill")
            rows.append((name,"cuda",variant,code,outcome,cost,str(log.relative_to(home))))
            if variant=="baseline" and code:raise RuntimeError("unmutated CUDA equality geometry oracle not green")
    with (home/"mutations.tsv").open("w",newline="") as stream:
        w=csv.writer(stream,delimiter="\t",lineterminator="\n")
        w.writerow(("mutant","backend","phase","exit","outcome","seconds","evidence"));w.writerows(rows)
    with (home/"source-manifest.tsv").open("w",newline="") as stream:
        w=csv.writer(stream,delimiter="\t",lineterminator="\n")
        w.writerow(("variant","source","original_sha256","variant_sha256"));w.writerows(manifest)
    shutil.copy2(ROOT/"scripts"/"run_discovery_mutations.py",home/"runner-source.py")
    killed=sum(row[4]=="killed" for row in rows)
    expected=len(MUTATIONS)+int(bool(args.nvcc))
    print(f"production discovery mutations: {killed}/{expected} killed; baseline build {baseline_build_cost:.3f}s; {home.relative_to(ROOT)}")
    if not args.nvcc:print("CUDA equality-pruning mutation was not requested; no CUDA mutation qualification claim")
    return 0 if killed==expected else 1


if __name__=="__main__":
    try:raise SystemExit(main())
    except Exception as error:
        print("Mutation qualification infrastructure failure: "+str(error),file=sys.stderr)
        raise SystemExit(2)
