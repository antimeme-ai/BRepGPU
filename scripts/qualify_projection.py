#!/usr/bin/env python3
"""Qualify one explicitly requested projection backend with independent truth."""
import argparse
from array import array
from collections import Counter
import csv
import math
from pathlib import Path
import sys

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"python"))
from brepgpu.projection_fixtures import geometry,projection_cases,qualify_row


def table(path,columns,rows):
    with path.open("w",newline="") as stream:
        writer=csv.writer(stream,delimiter="\t",lineterminator="\n")
        writer.writerow(columns);writer.writerows(rows)


def fraction_hex(value):
    return format(value.numerator,"x")+"/"+format(value.denominator,"x")


def qualify(backend,device,directory,library_path=None,plugin_path=None,threads=1):
    from brepgpu import Engine,Patch,ProjectionOptions
    cases=projection_cases()
    groups={}
    for index,case in enumerate(cases):
        groups.setdefault((case.max_iterations,case.max_backtracks,case.tolerance),[]).append((index,case))
    rows=[];failures=[];witnesses={};maxima={};status_counts=Counter();flag_counts=Counter()
    all_updates=[];all_evaluations=[]
    with Engine(backend=backend,device=device,threads=threads,library_path=library_path,plugin_path=plugin_path) as engine:
        for group,(key,items) in enumerate(groups.items()):
            items=items[::2]+list(reversed(items[1::2]))
            patches=[Patch(c.fixture.degree_u,c.fixture.degree_v,c.fixture.controls) for _,c in items]
            slots=array("I",(0xFFFFFFFF if c.status==1 else i for i,(_,c) in enumerate(items)))
            targets=array("d",(x for _,c in items for x in c.target))
            seeds=array("d",(x for _,c in items for x in c.seed))
            options=ProjectionOptions(*key)
            with engine.create_patch_set(patches) as patch_set:
                with engine.create_projection_query_set(patch_set,slots,targets,seeds) as queries:
                    with engine.project(queries,options) as result:batch=result.download()
            if batch.count!=len(items) or len(batch.data)!=7*len(items):
                raise AssertionError("public result shape differs from requested batch")
            for request,(case_index,case) in enumerate(items):
                values=tuple(batch.data[field*batch.count+request] for field in range(7))
                status=int(batch.statuses[request]);updates=int(batch.iterations[request])
                evaluations=int(batch.evaluation_counts[request]);flags=int(batch.flags[request])
                violations,metrics=qualify_row(case,values,status,updates,evaluations,flags)
                failures.extend((case_index,case.name,message) for message in violations)
                status_counts[status]+=1;flag_counts[flags]+=1
                all_updates.append(updates);all_evaluations.append(evaluations)
                exact=[""]*5
                if status in (0,5,6,7,8) and case.status not in (1,2) and all(math.isfinite(x) for x in values) and all(0<=u<=1 for u in values[:2]):
                    truth=geometry(case.fixture,case.target,values[:2])
                    exact=[fraction_hex(x) for x in (*truth.point,truth.distance_squared,truth.residual)]
                row=(case_index,case.name,group,request,slots[request],case.max_iterations,
                     case.max_backtracks,case.tolerance.hex(),*(float(x).hex() for x in case.target),
                     *(float(x).hex() for x in case.seed),status,updates,evaluations,flags,
                     *(float(x).hex() for x in values),*exact,len(violations))
                rows.append(row)
                for metric,error in metrics.items():
                    if metric not in maxima or error>maxima[metric]:
                        maxima[metric]=error;witnesses[metric]=(case_index,row)
    directory.mkdir(parents=True,exist_ok=True)
    columns=("case_index","case","group","request","patch_slot","max_iterations","max_backtracks","tolerance_hex",
             "target_x_hex","target_y_hex","target_z_hex","seed_s_hex","seed_t_hex",
             "status","accepted_updates","evaluation_count","flags","s_hex","t_hex","x_hex","y_hex","z_hex",
             "distance_squared_hex","residual_hex","exact_x_ratio_hex","exact_y_ratio_hex","exact_z_ratio_hex",
             "exact_distance_ratio_hex","exact_residual_ratio_hex","failures")
    table(directory/"projection-requests.tsv",columns,rows)
    table(directory/"projection-controls.tsv",("case_index","case","degree_s","degree_t","control","Ax_hex","Ay_hex","Az_hex","W_hex"),
          ((index,c.name,c.fixture.degree_u,c.fixture.degree_v,control,
            *(float(x).hex() for x in c.fixture.controls[4*control:4*control+4]))
           for index,c in enumerate(cases) for control in range(c.fixture.control_count)))
    table(directory/"projection-failures.tsv",("case_index","case","violation"),failures)
    table(directory/"projection-summary.tsv",("metric","value","backend","device","threads"),
          [("requests",len(rows),backend,device,threads),("violations",len(failures),backend,device,threads)]
          +[("maximum_error_"+key,repr(float(value)),backend,device,threads) for key,value in maxima.items()]
          +[("status_"+str(key),value,backend,device,threads) for key,value in sorted(status_counts.items())]
          +[("flags_"+str(key),value,backend,device,threads) for key,value in sorted(flag_counts.items())])
    table(directory/"projection-work-distribution.tsv",("quantity","value","requests"),
          [("accepted_updates",value,count) for value,count in sorted(Counter(all_updates).items())]
          +[("evaluations",value,count) for value,count in sorted(Counter(all_evaluations).items())])
    for metric,(_,row) in witnesses.items():table(directory/("worst-"+metric+".tsv"),columns,[row])
    report=("# Seeded projection qualification\n\n"
            f"Requested backend `{backend}`, device `{device}`, CPU thread option `{threads}`. "
            f"{len(rows)} requests; {len(failures)} direct violations.\n\n"
            "The corpus checks supplied-input exact geometry at returned binary64 UV, "
            "distance, box residual, status/counters and known analytic/curvature fixtures. "
            "It includes all16 degree pairs, singular and admitted conditioned charts, "
            "initial saddles/flats, boundary cones, zero update/search budgets and a stable distance-change witness. "
            "Stationarity is a first-order candidate result; it is not a minimum or global nearest guarantee.\n\n"
            "| Reported-field error | Maximum |\n| --- | ---: |\n"
            +"".join(f"| {key} | {float(value):.17g} |\n" for key,value in maxima.items())
            +"\nRequests and controls preserve exact input bits; exact output expectations are hexadecimal "
            "integer ratios. Worst witnesses, every violation and work/status distributions accompany this report. "
            "Allowances follow the accepted plan's finite-corpus evaluator/rounding budgets, without universal error enclosures. "
            "Public snapshot/lifetime/guard/ABI checks run separately in test_projection.py; "
            "native fault, compatibility and memory checks are separate mechanisms.\n")
    (directory/"QUALIFICATION.md").write_text(report)
    return not failures


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--backend",choices=("cpu","cuda"),required=True)
    parser.add_argument("--device",type=int,default=0)
    parser.add_argument("--threads",type=int,default=1)
    parser.add_argument("--output-directory",type=Path,required=True)
    parser.add_argument("--library-path");parser.add_argument("--plugin-path")
    args=parser.parse_args()
    try:passed=qualify(args.backend,args.device,args.output_directory,args.library_path,args.plugin_path,args.threads)
    except Exception as error:
        print(f"Projection qualification failed: {type(error).__name__}: {error}",file=sys.stderr)
        return 1
    print(f"Projection qualification {'passed' if passed else 'FAILED'}: {args.backend} device {args.device}")
    return 0 if passed else 1


if __name__=="__main__":raise SystemExit(main())
