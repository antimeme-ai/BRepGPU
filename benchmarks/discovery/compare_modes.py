#!/usr/bin/env python3
"""Compare preserved dynamic/specialized CUDA plugins using matched native cases."""
import argparse
from dataclasses import dataclass
from datetime import datetime, timezone
import csv
import hashlib
import math
import os
from pathlib import Path
import subprocess
import sys


KEYS=("backend","device","threads","count","corpus","mode","depth",
      "max_visits","max_refinements","tolerance_squared","phase")


@dataclass(frozen=True)
class Case:
    count: int
    corpus: str
    mode: int
    depth: int
    refinements: int=8


def cases():
    result=[Case(n,"cylinder",m,8) for n in (1,256,4096,65536) for m in range(3)]
    result += [Case(4096,c,m,8) for c in ("cylinder_offgrid","axis_offgrid") for m in range(3)]
    result += [Case(4096,"diagonal",1,d) for d in (0,8)]
    result += [Case(4096,"axis_midpoint",2,d) for d in (2,8)]
    result += [Case(4096,"capped",m,8,r) for m in range(3) for r in (0,8)]
    return tuple(result)


def sha256(path):
    digest=hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda:stream.read(1<<20),b""):digest.update(chunk)
    return digest.hexdigest()


def write(path,rows):
    if not rows:return
    with path.open("w",newline="") as stream:
        writer=csv.DictWriter(stream,list(rows[0]),delimiter="\t",lineterminator="\n")
        writer.writeheader();writer.writerows(rows)


def read(path):
    with path.open(newline="") as stream:return list(csv.DictReader(stream,delimiter="\t"))


def stage(binary):
    # The retained native executable uses $ORIGIN; the core locates its plugin
    # beside itself. Resolve only the directory so an executable symlink cannot
    # silently select another origin than the admitted staged siblings.
    if binary.is_symlink():raise ValueError("staged benchmark must not be a symlink")
    binary=binary.absolute();directory=binary.parent.resolve()
    paths={"benchmark":directory/binary.name,"core":directory/"libbrepgpu.so",
           "plugin":directory/"libbrepgpu_cuda.so"}
    for role,path in paths.items():
        if not path.is_file():raise ValueError(f"missing staged {role}: {path.name}")
    if not os.access(paths["benchmark"],os.X_OK):raise ValueError("staged benchmark is not executable")
    return paths,{role:sha256(path) for role,path in paths.items()}


def check_loader_environment():
    # Do not claim sibling identity if inherited loader overrides select a
    # different core. CUDA-library-only search paths remain allowed.
    if os.environ.get("LD_PRELOAD"):raise ValueError("unset LD_PRELOAD for preserved-binary comparison")
    search=os.environ.get("LD_LIBRARY_PATH")
    if search is not None:
        for entry in search.split(":"):
            if (Path(entry or ".")/"libbrepgpu.so").exists():
                raise ValueError("LD_LIBRARY_PATH contains a core override; use staged $ORIGIN resolution")


def verify_stage(paths,identities):
    for role,path in paths.items():
        if sha256(path)!=identities[role]:raise RuntimeError(f"staged {role} changed after admission")


def matched_rows(path,key_fields):
    result={}
    for row in read(path):
        key=tuple(row[field] for field in key_fields)
        if key in result:raise RuntimeError(f"duplicate row key in {path.name}")
        result[key]=row
    if not result:raise RuntimeError(f"empty artifact: {path.name}")
    return result


def compare_artifacts(dynamic,specialized,device,case_id,case):
    a=matched_rows(dynamic/"summary.tsv",KEYS);b=matched_rows(specialized/"summary.tsv",KEYS)
    if a.keys()!=b.keys():raise RuntimeError("variant summary condition/phase keys differ")
    sample_keys=KEYS+("sample",)
    ar=matched_rows(dynamic/"samples.tsv",sample_keys);br=matched_rows(specialized/"samples.tsv",sample_keys)
    if ar.keys()!=br.keys():raise RuntimeError("variant raw sample keys differ")
    work_keys=("backend","device","threads","count","corpus","mode","depth","sample","phase","quantity","value")
    aw=list((dynamic/"work").glob("*.tsv"));bw=list((specialized/"work").glob("*.tsv"))
    if len(aw)!=1 or len(bw)!=1:raise RuntimeError("one-case invocation did not emit exactly one work artifact")
    ah=matched_rows(aw[0],work_keys);bh=matched_rows(bw[0],work_keys)
    work_equal=ah==bh
    quality_fields=[field for field in next(iter(ar.values())) if field not in sample_keys+("ms",)]
    quality_equal=all(all(ar[key][field]==br[key][field] for field in quality_fields) for key in ar)
    result=[]
    for key in sorted(a):
        x,y=a[key],b[key]
        expected={"backend":"cuda","device":str(device),"threads":"1","count":str(case.count),
                  "corpus":case.corpus,"mode":str(case.mode),"depth":str(case.depth),
                  "max_visits":"1" if case.corpus=="capped" else "8191",
                  "max_refinements":str(case.refinements)}
        if any(x[field]!=value for field,value in expected.items()):raise RuntimeError("native condition differs from requested case")
        if float(x["tolerance_squared"])!=2**-24:raise RuntimeError("native tolerance differs from reviewed comparison")
        expected_samples="1" if key[-1] in ("input_generation_output_allocation","context_first_use",
            "initial_patch_snapshot_upload","initial_hierarchy_build_upload","context_and_retained_cleanup") else "7"
        if x["samples"]!=expected_samples or y["samples"]!=expected_samples:
            raise RuntimeError("native phase sample count differs from reviewed comparison")
        if x.keys()!=y.keys():raise RuntimeError("variant summary columns differ")
        median_a,median_b=float(x["median_ms"]),float(y["median_ms"])
        if not all(math.isfinite(float(row[field])) and float(row[field])>=0
                   for row in (x,y) for field in ("median_ms","min_ms","max_ms")):
            raise RuntimeError("invalid native phase time")
        row={"case_id":case_id,**dict(zip(KEYS,key)),"quality_equal":int(quality_equal),"work_equal":int(work_equal)}
        row.update({"dynamic_"+field:value for field,value in x.items() if field not in KEYS})
        row.update({"specialized_"+field:value for field,value in y.items() if field not in KEYS})
        row["median_improvement_percent"]=100*(median_a-median_b)/median_a if median_a else ""
        result.append(row)
    return result,quality_equal and work_equal


def integers(value):
    result=[int(part) for part in value.split(",")]
    if not result or len(set(result))!=len(result):raise ValueError("selection must contain distinct integers")
    return result


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dynamic-binary",type=Path,required=True)
    parser.add_argument("--specialized-binary",type=Path,required=True)
    parser.add_argument("--output-directory",type=Path,required=True)
    parser.add_argument("--cuda-devices",default="0,1")
    parser.add_argument("--cases",default=None,help="comma-separated IDs0..27; same selection runs on each card")
    parser.add_argument("--reverse-order",action="store_true",help="invert variant order for every selected case")
    args=parser.parse_args();matrix=cases()
    try:
        devices=integers(args.cuda_devices);ids=list(range(len(matrix))) if args.cases is None else integers(args.cases)
        if any(d not in (0,1) for d in devices):raise ValueError("CUDA devices must select0 and/or1")
        if any(i not in range(len(matrix)) for i in ids):raise ValueError("case IDs must be0..27")
        if args.output_directory.exists() or args.output_directory.is_symlink():raise ValueError("output directory must be fresh")
        check_loader_environment()
        variants={};identities={}
        for name,binary in (("dynamic",args.dynamic_binary),("specialized",args.specialized_binary)):
            variants[name],identities[name]=stage(binary)
        for role in ("benchmark","core"):
            if identities["dynamic"][role]!=identities["specialized"][role]:raise ValueError(f"staged {role} SHA differs")
        if variants["dynamic"]["benchmark"].parent==variants["specialized"]["benchmark"].parent:
            raise ValueError("variants need isolated sibling-artifact directories")
    except (ValueError,OSError) as error:parser.error(str(error))
    output=args.output_directory;output.mkdir(parents=True,exist_ok=False)
    runner=Path(__file__).resolve().with_name("run.py")
    provenance=[{"variant":name,"role":role,"basename":path.name,"sha256":identities[name][role],"bytes":path.stat().st_size}
                for name,paths in variants.items() for role,path in paths.items()]
    provenance.append({"variant":"shared","role":"runner","basename":runner.name,"sha256":sha256(runner),"bytes":runner.stat().st_size})
    write(output/"provenance.tsv",provenance)
    write(output/"cases.tsv",[{"case_id":i,"count":c.count,"corpus":c.corpus,"mode":c.mode,"depth":c.depth,
                              "max_visits":1 if c.corpus=="capped" else 8191,"max_refinements":c.refinements,
                              "tolerance_squared":float(2**-24).hex(),"repetitions":7} for i,c in enumerate(matrix)])
    order=[];comparisons=[]
    for device in devices:
        for case_id in ids:
            case=matrix[case_id];names=("dynamic","specialized")
            if bool(case_id%2)^args.reverse_order:names=names[::-1]
            directories={}
            for name in names:
                verify_stage(variants[name],identities[name])
                directory=output/f"card{device}"/f"case{case_id:02d}"/name
                directories[name]=directory
                command=[sys.executable,str(runner),"--binary",str(variants[name]["benchmark"]),
                         "--output-directory",str(directory),"--counts",str(case.count),"--corpora",case.corpus,
                         "--modes",str(case.mode),"--depths",str(case.depth),"--cpu-threads","",
                         "--cuda-devices",str(device),"--repetitions","7","--max-refinements",str(case.refinements)]
                receipt={"sequence":len(order),"device":device,"case_id":case_id,"variant":name,
                         "reverse_order":int(args.reverse_order),"started_utc":datetime.now(timezone.utc).isoformat(),
                         "returncode":"","artifacts":directory.relative_to(output).as_posix()}
                order.append(receipt);write(output/"order.tsv",order)
                print(f"card{device} case{case_id:02d} {name}",flush=True)
                result=subprocess.run(command,text=True,capture_output=True)
                receipt["returncode"]=result.returncode;write(output/"order.tsv",order)
                directory.mkdir(parents=True,exist_ok=True)
                (directory/"runner-stdout.txt").write_text(result.stdout)
                (directory/"runner-stderr.txt").write_text(result.stderr)
                if result.returncode:raise RuntimeError(f"case{case_id} card{device} {name} failed; retained runner logs")
                verify_stage(variants[name],identities[name])
            rows,equal=compare_artifacts(directories["dynamic"],directories["specialized"],device,case_id,case)
            comparisons.extend(rows);write(output/"comparison.tsv",comparisons)
            if not equal:raise RuntimeError(f"case{case_id} card{device} quality/work changed; inspect preserved artifacts")
    (output/"SCOPE.md").write_text(
        "# Matched CUDA mode comparison\n\n"
        f"{len(ids)} selected reviewed conditions per card, cards {','.join(map(str,devices))}, seven repetitions per variant. "
        f"Reverse variant order: {args.reverse_order}. Case IDs use the full28-condition cases.tsv.\n\n"
        "Each card runs sequentially; variant order alternates by stable case ID. No CPU job is launched. "
        "The staged core/native benchmark bytes match; plugin identities are recorded and may differ. "
        "Original per-invocation samples.tsv, summary.tsv and work files are retained. comparison.tsv retains full "
        "condition keys, both timing medians/min/max and all reported quality/work extrema. Quality columns and "
        "raw work histograms must match; no numerical certificate is inferred from timing.\n\n"
        "Resident, stream-event bracket and cached-hierarchy host-to-host phases are distinct. Event duration may "
        "include host submission delay. Host-to-host includes the native benchmark's analytic consume checks. "
        "Setup/first-use/cleanup phases retain their existing scopes. Independent phase medians cannot be added. "
        "Positive median_improvement_percent means specialized is faster; a single run does not decide retention. "
        "Confirm selected same-case benefits/regressions in a fresh reverse-order run on both cards. "
        "No concurrent owned build/copy/qualification/performance job is permitted by the reviewed experiment.\n")


if __name__=="__main__":main()
