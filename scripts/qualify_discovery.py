#!/usr/bin/env python3
"""Qualify an explicitly requested discovery backend with exact original inputs."""
import argparse
from array import array
from collections import Counter
import csv
import math
from pathlib import Path
import sys

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"python"))
from brepgpu.discovery_fixtures import discovery_cases,qualify_hierarchy,qualify_row,exact_distance


def table(path,columns,rows):
    with path.open("w",newline="") as stream:
        writer=csv.writer(stream,delimiter="\t",lineterminator="\n")
        writer.writerow(columns);writer.writerows(rows)


def fraction_hex(value):return format(value.numerator,"x")+"/"+format(value.denominator,"x")


def qualify(backend,device,directory,modes=(0,1,2),library_path=None,plugin_path=None,threads=1):
    from brepgpu import Engine,Patch,Lineage,HierarchyOptions,DiscoveryOptions
    cases=discovery_cases();groups={}
    for index,case in enumerate(cases):
        groups.setdefault((case.depth,case.max_visits,case.max_refinements,case.tolerance_squared),[]).append((index,case))
    directory.mkdir(parents=True,exist_ok=True)
    rows=[];violations=[];nodes=[];build_rows=[];maxima={};histograms={name:Counter() for name in ("status","flags","visits","pruned","refinements","evaluations")}
    with Engine(backend=backend,device=device,threads=threads,library_path=library_path,plugin_path=plugin_path) as engine:
        for mode in modes:
            for group,(key,items) in enumerate(groups.items()):
                items=items[::2]+list(reversed(items[1::2]));fixtures=tuple(c.fixture for _,c in items)
                patches=[Patch(f.degree_u,f.degree_v,f.controls,lineage=Lineage(1,5,index+1000,1,1))
                         for (index,_),f in zip(items,fixtures)]
                with engine.create_patch_set(patches) as patch_set:
                    hop=HierarchyOptions(key[0],mode=mode)
                    with engine.create_hierarchy(patch_set,hop) as hierarchy:
                        cache=hierarchy.download();failure,oracle=qualify_hierarchy(fixtures,cache,key[0],mode)
                        violations.extend((mode,group,-1,"hierarchy",message) for message in failure)
                        if oracle is None:raise AssertionError("invalid hierarchy prevents direct query qualification")
                        active=sum((2*f.degree_u+1)*(2*f.degree_v+1) for f in fixtures)
                        build_rows.append((mode,group,key[0],len(fixtures),len(cache.nodes),len(cache.bounds),len(failure)))
                        for node_index,node in enumerate(cache.nodes):
                            nodes.append((mode,group,node_index,int(node.patch_slot),int(node.u_level),int(node.u_index),int(node.v_level),int(node.v_index),
                                          int(node.left),int(node.right),int(node.flags),*(float(v).hex() for v in (*node.lower,*node.upper))))
                        targets=array("d",(v for _,case in items for v in case.target))
                        options=DiscoveryOptions(key[1],key[2],key[3])
                        with engine.create_discovery_query_set(hierarchy,range(len(items)),targets) as queries:
                            with engine.discover(queries,options) as result:batch=result.download()
                        if batch.count!=len(items) or len(batch.data)!=8*len(items):raise AssertionError("public output shape differs")
                        if batch.options!=options or batch.hierarchy_options!=hop:raise AssertionError("result option snapshot differs")
                        if tuple(batch.targets)!=tuple(targets):raise AssertionError("result target snapshot differs")
                        for row,(case_index,case) in enumerate(items):
                            values=tuple(batch.data[k*batch.count+row] for k in range(8))
                            integers=tuple(int(s[row]) for s in (batch.statuses,batch.flags,batch.visits,batch.pruned,batch.refinements,batch.evaluation_counts))
                            failure,metrics=qualify_row(case,values,*integers,oracle,row)
                            if batch.lineage[row]!=Lineage(1,5,case_index+1000,1,1):failure.append("result source lineage differs")
                            violations.extend((mode,group,case_index,case.name,message) for message in failure)
                            for name,value in zip(histograms,integers):histograms[name][value]+=1
                            exact=[""]*5
                            if all(math.isfinite(x) for x in values) and all(0<=u<=1 for u in values[:2]):
                                point,distance=exact_distance(case.fixture,case.target,values[:2])
                                exact=[fraction_hex(x) for x in (*point,distance,oracle.lower_limit(row,case.target,bool(integers[1]&8)))]
                            rows.append((mode,group,case_index,case.name,row,key[0],key[1],key[2],float(key[3]).hex(),
                                         *(float(x).hex() for x in case.target),*integers,*(float(x).hex() for x in values),*exact,len(failure)))
                            for name,value in metrics.items():maxima[name]=max(maxima.get(name,0),value)
    table(directory/"discovery-requests.tsv",("mode","group","case_index","case","patch_slot","depth","max_visits","max_refinements","tolerance_squared_hex",
          "target_x_hex","target_y_hex","target_z_hex","status","flags","visits","pruned","refinements","evaluations",
          "s_hex","t_hex","x_hex","y_hex","z_hex","approximate_distance_squared_hex","lower_squared_hex","upper_squared_hex",
          "exact_x_ratio_hex","exact_y_ratio_hex","exact_z_ratio_hex","exact_witness_distance_ratio_hex","exact_lower_limit_ratio_hex","violations"),rows)
    table(directory/"discovery-hierarchy.tsv",("mode","group","node","patch_slot","u_level","u_index","v_level","v_index","left","right","flags",
          "lower_x_hex","lower_y_hex","lower_z_hex","upper_x_hex","upper_y_hex","upper_z_hex"),nodes)
    table(directory/"discovery-cache-summary.tsv",("mode","group","depth","patches","nodes","cache_doubles","violations"),build_rows)
    table(directory/"discovery-controls.tsv",("case_index","case","degree_u","degree_v","control","Ax_hex","Ay_hex","Az_hex","W_hex"),
          ((i,c.name,c.fixture.degree_u,c.fixture.degree_v,j,*(float(x).hex() for x in c.fixture.controls[4*j:4*j+4]))
           for i,c in enumerate(cases) for j in range(c.fixture.control_count)))
    table(directory/"discovery-failures.tsv",("mode","group","case_index","case","violation"),violations)
    table(directory/"discovery-work-distribution.tsv",("quantity","value","requests"),
          ((name,value,count) for name,histogram in histograms.items() for value,count in sorted(histogram.items())))
    table(directory/"discovery-summary.tsv",("metric","value","backend","device","threads"),
          [("requests",len(rows),backend,device,threads),("hierarchy_nodes_checked",len(nodes),backend,device,threads),
           ("violations",len(violations),backend,device,threads)]
          +[("maximum_descriptive_error_"+name,repr(float(value)),backend,device,threads) for name,value in maxima.items()])
    (directory/"QUALIFICATION.md").write_text(
        "# Conservative discovery qualification\n\n"
        f"Explicit backend `{backend}`, device `{device}`, CPU thread option `{threads}`; modes `{','.join(map(str,modes))}`. "
        f"{len(cases)} direct cases, {len(rows)} result rows, {len(nodes)} actual downloaded nodes; {len(violations)} violations.\n\n"
        "Exact binary-input power-polynomial restriction qualifies complete dyadic cell identity, Cartesian child control hulls, "
        "DOP support intervals and all active Bernstein base coefficients/padding. Exact original-patch geometry at returned UV "
        "qualifies the upper certificate; flat method-specific leaf/root bounds qualify lower output without mirroring traversal. "
        "Analytic global distances and independent gap/coverage/visit/refinement flags cover competing, continuum, degenerate, boundary "
        "and capped cases. Every certificate inequality uses Fraction comparisons without epsilon; allowances apply only to descriptive XYZ/distance.\n\n"
        "Requests, original controls and downloaded node cells/boxes preserve bits. Cache modes and actual cache extents, all violations "
        "and work distributions accompany the report. Public lifetime/raw ABI/environment/device probes run in the corresponding owned "
        "conformance/native fault units. These finite direct cases are evidence for targeted faults, not a theorem over every floating execution.\n")
    return not violations


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--backend",choices=("cpu","cuda"),required=True)
    parser.add_argument("--device",type=int,default=0);parser.add_argument("--threads",type=int,default=1)
    parser.add_argument("--mode",choices=("all","aabb","dop26","bernstein"),default="all")
    parser.add_argument("--output",type=Path,required=True)
    parser.add_argument("--library",default=None);parser.add_argument("--plugin",default=None)
    args=parser.parse_args();modes=(0,1,2) if args.mode=="all" else (("aabb","dop26","bernstein").index(args.mode),)
    try:ok=qualify(args.backend,args.device,args.output,modes,args.library,args.plugin,args.threads)
    except Exception as error:
        print(f"Discovery qualification failed: {type(error).__name__}: {error}",file=sys.stderr);return 1
    print(f"Discovery qualification {'passed' if ok else 'failed'}: {args.output}");return 0 if ok else 1


if __name__=="__main__":raise SystemExit(main())
