#!/usr/bin/env python3
"""Discover seed-free cloud witnesses and export exactly qualified bounds/SVG."""
import argparse
from array import array
from collections import Counter
import csv
from fractions import Fraction
from html import escape
import math
from pathlib import Path
import sys
import time

from brepgpu import Engine, Patch, Lineage, HierarchyOptions, DiscoveryOptions
from brepgpu.discovery_fixtures import (
    DiscoveryCase, diagonal_plane, parabolic, rational_cylinder,
    exact_distance, qualify_hierarchy, qualify_row)

MODES={"aabb":0,"dop26":1,"bernstein":2}
SHAPES={"cylinder":"Rational quarter-cylinder","polynomial":"Parabolic cylinder","diagonal":"Diagonal plane"}


def wall_ms(begin):return 1000*(time.perf_counter()-begin)
def fraction_hex(value):return format(value.numerator,"x")+"/"+format(value.denominator,"x")


def table(path,columns,rows):
    with path.open("w",newline="") as stream:
        writer=csv.writer(stream,delimiter="\t",lineterminator="\n")
        writer.writerow(columns);writer.writerows(rows)


def cloud(shape,count,depth,options):
    fixture={"cylinder":rational_cylinder,"polynomial":parabolic,"diagonal":diagonal_plane}[shape]()
    width=math.isqrt(count-1)+1;denominator=1<<(width-1).bit_length()
    cases=[];targets=array("d");answers=[]
    for i in range(count):
        s=(2*(i%width)+1)/(2*denominator)
        t=(2*(i//width)+1)/(2*denominator)
        if shape=="cylinder":
            lam=1+(i%4)/8
            target=(lam*(1-s*s),2*lam*s,t)
            radius=Fraction(lam)*(1+Fraction(s)**2)
            truth=(radius-1)**2;answer=(s,t)
        elif shape=="polynomial":
            a=s-.5;lam=1/16
            target=(a-2*a*lam,t,a*a+lam)
            truth=Fraction(lam)**2*(4*Fraction(a)**2+1);answer=((a+1)/2,t)
        else:
            target=(s+.25,s-.25,t);truth=Fraction(1,8);answer=(s,t)
        cases.append(DiscoveryCase(f"cloud_{i}",fixture,target,truth,depth,
            options.max_visits,options.max_refinements,options.tolerance_squared))
        targets.extend(target);answers.append(answer)
    return fixture,tuple(cases),targets,tuple(answers)


def point(shape,s,t):
    if shape=="cylinder":
        d=1+s*s;return (1-s*s)/d,2*s/d,t
    if shape=="polynomial":a=2*s-1;return a,t,a*a
    return s,s,t


def selected(count,limit):
    n=min(count,limit)
    if n<=1:return [0] if n else []
    return [i*(count-1)//(n-1) for i in range(n)]


def svg(path,args,cache,batch,targets,counts):
    leaf_ids=[i for i,node in enumerate(cache.nodes) if node.flags&1]
    cells=[leaf_ids[i] for i in selected(len(leaf_ids),16)]
    root=int(cache.roots[0]);box_ids=list(dict.fromkeys([root,*cells]))
    grid=[tuple(point(args.shape,k/16 if axis==0 else j/64,j/64 if axis==0 else k/16)
                for j in range(65)) for axis in range(2) for k in range(17)]
    corners={i:tuple((cache.nodes[i].upper[0] if mask&1 else cache.nodes[i].lower[0],
                     cache.nodes[i].upper[1] if mask&2 else cache.nodes[i].lower[1],
                     cache.nodes[i].upper[2] if mask&4 else cache.nodes[i].lower[2])
                    for mask in range(8)) for i in box_ids}
    def raw(p):x,y,z=p;return x-.65*y,z+.2*x+.1*y
    all_points=[raw(p) for curve in grid for p in curve]
    all_points += [raw(targets[3*i:3*i+3]) for i in range(batch.count)]
    all_points += [raw(batch.point(i)) for i in range(batch.count)]
    all_points += [raw(p) for vertices in corners.values() for p in vertices]
    lo=tuple(min(p[k] for p in all_points) for k in range(2));hi=tuple(max(p[k] for p in all_points) for k in range(2))
    scale=min(720/max(hi[0]-lo[0],1e-9),490/max(hi[1]-lo[1],1e-9))
    margins=((720-(hi[0]-lo[0])*scale)/2,(490-(hi[1]-lo[1])*scale)/2)
    def view(p):
        x,y=raw(p);return 55+margins[0]+(x-lo[0])*scale,645-margins[1]-(y-lo[1])*scale
    shown=selected(batch.count,180)
    # Preserve visible representatives of both statuses when a uniform subset
    # misses one; every row is still exported and counted in the panel.
    for status in (0,1):
        representatives=[i for i in range(batch.count) if batch.statuses[i]==status]
        if representatives and not any(batch.statuses[i]==status for i in shown):
            shown[-1 if status==1 else 0]=representatives[0]
    lines=['<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1100 760">',
           '<rect width="1100" height="760" fill="#101923"/>',
           '<g font-family="sans-serif">',
           f'<text x="35" y="45" fill="#edf4fc" font-size="25">BRepGPU · {escape(SHAPES[args.shape])} discovery</text>',
           f'<text x="35" y="76" fill="#a9bed0" font-size="15">Seed-free local witnesses · {escape(args.mode)} bounds · cached depth {args.depth}</text>']
    for node_id in box_ids:
        vertices=corners[node_id]
        for a in range(8):
            for bit in (1,2,4):
                if a&bit:continue
                start,end=view(vertices[a]),view(vertices[a|bit])
                lines.append(f'<path d="M{start[0]:.3f},{start[1]:.3f} L{end[0]:.3f},{end[1]:.3f}" fill="none" stroke="#708297" stroke-width=".65" opacity=".24"/>')
    for curve in grid:
        commands=" ".join(("M" if i==0 else "L")+f"{x:.3f},{y:.3f}" for i,(x,y) in enumerate(map(view,curve)))
        lines.append(f'<path d="{commands}" fill="none" stroke="#386079" stroke-width=".75" opacity=".7"/>')
    for i in shown:
        start,end=view(targets[3*i:3*i+3]),view(batch.point(i))
        color="#8de5b7" if batch.statuses[i]==0 else "#e9bf62"
        lines.append(f'<path d="M{start[0]:.3f},{start[1]:.3f} L{end[0]:.3f},{end[1]:.3f}" stroke="{color}" stroke-width=".9" opacity=".48"/>')
        lines.append(f'<circle cx="{start[0]:.3f}" cy="{start[1]:.3f}" r="2.2" fill="#ffb17a"/>')
        lines.append(f'<circle cx="{end[0]:.3f}" cy="{end[1]:.3f}" r="2.1" fill="{color}"/>')
    lines.append('<rect x="815" y="118" width="250" height="486" rx="8" fill="#182533"/>')
    panel=[("Requests",str(batch.count)),("Squared-gap precision",str(counts["gap"])),
           ("Unresolved brackets",str(counts["unresolved"])),("Traversal complete",str(counts["coverage"])),
           ("Visit capped",str(counts["visit_limit"])),("Refinement capped",str(counts["refinement_limit"])),
           ("Squared tolerance",f"{args.tolerance_squared:.3g}"),
           ("Cached nodes",str(len(cache.nodes))),("Drawn leaf boxes",f"{len(cells)} / {len(leaf_ids)}")]
    for i,(label,value) in enumerate(panel):
        y=151+43*i
        lines.append(f'<text x="832" y="{y}" fill="#a9bed0" font-size="13">{escape(label)}</text>')
        lines.append(f'<text x="832" y="{y+20}" fill="#edf4fc" font-size="18">{escape(value)}</text>')
    lines += ['<text x="35" y="685" fill="#ffb17a" font-size="14">● target</text>',
              '<text x="155" y="685" fill="#8de5b7" font-size="14">● squared-gap witness</text>',
              '<text x="385" y="685" fill="#e9bf62" font-size="14">● unresolved witness</text>',
              f'<text x="35" y="711" fill="#a9bed0" font-size="13">Showing {len(shown)} requests; all {batch.count} rows passed exact original-UV and analytic bracket checks.</text>',
              '<text x="35" y="735" fill="#a9bed0" font-size="12">Gray boxes are cached AABBs, not visited/pruned trace. Untrimmed local carrier; no unique-UV or trim claim.</text>',
              '</g></svg>']
    path.write_text("\n".join(lines)+"\n")


def note(error,text):
    try:
        if hasattr(error,"add_note"):error.add_note(text)
    except BaseException:pass


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--backend",choices=("cpu","cuda"),default="cpu")
    parser.add_argument("--device",type=int,default=0);parser.add_argument("--threads",type=int,default=1)
    parser.add_argument("--shape",choices=tuple(SHAPES),default="cylinder")
    parser.add_argument("--mode",choices=tuple(MODES),default="aabb")
    parser.add_argument("--count",type=int,default=1024);parser.add_argument("--depth",type=int,default=8)
    parser.add_argument("--max-visits",type=int,default=8191);parser.add_argument("--max-refinements",type=int,default=8)
    parser.add_argument("--tolerance-squared",type=float,default=2.**-24)
    parser.add_argument("--output-directory",type=Path,required=True)
    parser.add_argument("--library",default=None);parser.add_argument("--plugin",default=None)
    args=parser.parse_args()
    if not 1<=args.count<=65536:parser.error("count must be1..65536")
    if not 0<=args.depth<=12:parser.error("depth must be0..12")
    if not 1<=args.max_visits<=1048576 or not 0<=args.max_refinements<=64:parser.error("invalid visit/refinement budget")
    if not math.isfinite(args.tolerance_squared) or not 0<=args.tolerance_squared<=1:parser.error("squared tolerance must be finite in[0,1]")
    if args.output_directory.exists() or args.output_directory.is_symlink():
        parser.error("output directory must be a fresh path")
    phases=[];caller_begin=time.perf_counter();begin=caller_begin
    options=DiscoveryOptions(args.max_visits,args.max_refinements,args.tolerance_squared)
    hop=HierarchyOptions(args.depth,mode=MODES[args.mode])
    fixture,cases,targets,answers=cloud(args.shape,args.count,args.depth,options)
    lineage=Lineage(1,0,1,1,1+tuple(SHAPES).index(args.shape))
    patch=Patch(fixture.degree_u,fixture.degree_v,fixture.controls,lineage=lineage)
    slots=array("I",[0])*args.count;phases.append(("input_preparation",wall_ms(begin)))
    engine=patches=hierarchy=queries=result=None
    try:
        begin=time.perf_counter();engine=Engine(backend=args.backend,device=args.device,threads=args.threads,
                                             library_path=args.library,plugin_path=args.plugin)
        phases.append(("context_first_use",wall_ms(begin)))
        begin=time.perf_counter();patches=engine.create_patch_set([patch]);phases.append(("patch_pack_snapshot_upload",wall_ms(begin)))
        begin=time.perf_counter();hierarchy=engine.create_hierarchy(patches,hop);phases.append(("hierarchy_cpu_build_upload",wall_ms(begin)))
        begin=time.perf_counter();cache=hierarchy.download();phases.append(("actual_hierarchy_download",wall_ms(begin)))
        begin=time.perf_counter();failures,oracle=qualify_hierarchy((fixture,),cache,args.depth,MODES[args.mode])
        if failures or oracle is None:raise RuntimeError("hierarchy failed exact qualification: "+"; ".join(failures[:20]))
        phases.append(("independent_exact_cache_qualification",wall_ms(begin)))
        begin=time.perf_counter();queries=engine.create_discovery_query_set(hierarchy,slots,targets)
        phases.append(("query_pack_snapshot_upload",wall_ms(begin)))
        begin=time.perf_counter();result=engine.discover(queries,options);phases.append(("resident_discover",wall_ms(begin)))
        begin=time.perf_counter();batch=result.download();phases.append(("download_allocate_host_provenance",wall_ms(begin)))
    finally:
        begin=time.perf_counter();primary=sys.exc_info()[1];cleanup=[]
        for resource in (result,queries,hierarchy,patches,engine):
            if resource is not None:
                try:resource.close()
                except BaseException as error:cleanup.append(error)
        phases.append(("native_cleanup",wall_ms(begin)))
        if cleanup:
            if primary is not None:
                for error in cleanup:note(primary,f"Resource cleanup also failed: {error}")
            else:
                for error in cleanup[1:]:note(cleanup[0],f"Further cleanup also failed: {error}")
                raise cleanup[0]
    begin=time.perf_counter();records=[];maxima={};counts=Counter()
    if batch.count!=args.count or len(batch.data)!=8*args.count:raise RuntimeError("downloaded request shape differs")
    if tuple(batch.targets)!=tuple(targets) or batch.options!=options or batch.hierarchy_options!=hop:
        raise RuntimeError("retained request/option snapshot differs")
    downloaded_lineage=batch.lineage
    source=(lineage.body_id,lineage.entity_kind,lineage.entity_id,lineage.revision,lineage.chart_id)
    for i,case in enumerate(cases):
        values=tuple(batch.data[k*args.count+i] for k in range(8))
        integers=tuple(int(x[i]) for x in (batch.statuses,batch.flags,batch.visits,batch.pruned,batch.refinements,batch.evaluation_counts))
        # The exact analytic global minimum directly qualifies L for this cloud.
        # The standalone corpus separately checks the flat/root recipe bound.
        failures,metrics=qualify_row(case,values,*integers,oracle,0,check_flat_lower=False)
        if downloaded_lineage[i]!=lineage:failures.append("synthetic source lineage differs")
        if failures:raise RuntimeError(f"cloud row {i} failed exact qualification: "+"; ".join(failures))
        exact_point,distance=exact_distance(fixture,case.target,values[:2])
        for name,value in metrics.items():maxima[name]=max(maxima.get(name,Fraction(0)),value)
        counts["gap"]+=integers[0]==0;counts["unresolved"]+=integers[0]==1
        for name,flag in (("coverage",4),("visit_limit",8),("refinement_limit",16)):counts[name]+=bool(integers[1]&flag)
        records.append((i,0,*source,*(float(x).hex() for x in case.target),
                        *(float(x).hex() for x in answers[i]),*integers,*(float(x).hex() for x in values),
                        *(fraction_hex(x) for x in exact_point),fraction_hex(distance),fraction_hex(case.distance_answer),0))
    phases.append(("independent_exact_result_qualification",wall_ms(begin)))
    directory=args.output_directory;directory.mkdir(parents=True,exist_ok=False);begin=time.perf_counter()
    table(directory/"records.tsv",("query","patch_slot","body_id","entity_kind","entity_id","revision","chart_id",
          "target_x_hex","target_y_hex","target_z_hex","analytic_nearest_s_hex","analytic_nearest_t_hex",
          "status","flags","visits","pruned","refinements","evaluations",
          "candidate_s_hex","candidate_t_hex","candidate_x_hex","candidate_y_hex","candidate_z_hex","approximate_distance_squared_hex","lower_squared_hex","upper_squared_hex",
          "exact_x_ratio_hex","exact_y_ratio_hex","exact_z_ratio_hex","exact_witness_distance_ratio_hex","exact_global_distance_ratio_hex","violations"),records)
    table(directory/"controls.tsv",("control","Ax_hex","Ay_hex","Az_hex","W_hex"),
          ((i,*(float(x).hex() for x in fixture.controls[4*i:4*i+4])) for i in range(fixture.control_count)))
    table(directory/"hierarchy.tsv",("node","patch_slot","u_level","u_index","v_level","v_index","left","right","flags",
          "lower_x_hex","lower_y_hex","lower_z_hex","upper_x_hex","upper_y_hex","upper_z_hex"),
          ((i,n.patch_slot,n.u_level,n.u_index,n.v_level,n.v_index,n.left,n.right,n.flags,*(float(x).hex() for x in (*n.lower,*n.upper)))
           for i,n in enumerate(cache.nodes)))
    table(directory/"bound-cache.tsv",("offset","value_hex"),((i,float(x).hex()) for i,x in enumerate(cache.bounds)))
    table(directory/"summary.tsv",("metric","value"),[("requests",args.count),("nodes",len(cache.nodes)),("bound_cache_doubles",len(cache.bounds))]
          +[(name,value) for name,value in counts.items()]+[("maximum_descriptive_error_"+name,float(value)) for name,value in maxima.items()])
    phases.append(("record_export",wall_ms(begin)));begin=time.perf_counter()
    svg(directory/(args.shape+"-discovery.svg"),args,cache,batch,targets,counts)
    phases.append(("owned_geometry_svg_export",wall_ms(begin)))
    phases.append(("caller_total_through_svg",wall_ms(caller_begin)))
    table(directory/"timings.tsv",("backend","device","threads","count","shape","mode","depth","phase","ms"),
          ((args.backend,args.device,args.threads,args.count,args.shape,args.mode,args.depth,phase,ms) for phase,ms in phases))
    (directory/"RECIPE.md").write_text(
        f"# {SHAPES[args.shape]} seed-free discovery\n\n"
        f"Explicit backend `{args.backend}`, device `{args.device}`, `{args.threads}` requested CPU workers; {args.count} targets, "
        f"mode `{args.mode}`, complete hierarchy depth {args.depth}, {len(cache.nodes)} actual downloaded nodes. "
        "One owned untrimmed patch in its local unit frame. Lineage labels a synthetic fixture, not an imported CAD body/face.\n\n"
        "The deterministic grid constructs unique cloud targets with known analytic global distances; known UV answers are used only "
        "for qualification/export and never supplied to the seed-free native operation. Exact power restriction qualifies every actual "
        "node/control hull/support/base coefficient. Each returned UV is independently evaluated on original binary64 controls with Fraction, "
        "and its distance is bounded by U without allowance. L and U contain the analytic global minimum; reported GAP rows meet the exact "
        "squared-width and squared-objective tolerance. The standalone corpus additionally checks the flat/root lower recipe.\n\n"
        f"Rows: gap precision {counts['gap']}, unresolved {counts['unresolved']}, complete traversal {counts['coverage']}, "
        f"visit limited {counts['visit_limit']}, refinement limited {counts['refinement_limit']}. All rows passed; unresolved witnesses remain "
        "in records and the graphic. Gap precision and traversal coverage are independent, and neither promises isolated/unique nearest UV.\n\n"
        f"Options: visits {options.max_visits}, refinements {options.max_refinements}, squared tolerance {options.tolerance_squared.hex()}. "
        "records.tsv preserves original target/returned/source bits and exact witness/global ratios; controls.tsv preserves homogeneous input. "
        "hierarchy.tsv and bound-cache.tsv contain actual cached cells/arrays. The SVG shows a uniform subset of up to180 requests with both "
        "outcomes represented when present, root and up to16 cached leaf AABBs, and an analytic orientation wire grid. Gray boxes are not a "
        "query traversal trace. DOP/ Bernstein extra caches are qualified/exported but the graphic draws their common AABBs.\n\n"
        "timings.tsv names preparation, first use, patch/hierarchy build/upload, hierarchy download/exact cache qualification, request ingestion, "
        "resident discover, allocating compact download/provenance, cleanup, exact result qualification and exports. These are one caller's "
        "samples, not native benchmark medians; cache inspection/qualification/export are visible separate costs. "
        "caller_total_through_svg is a directly measured wall interval from just before input preparation through SVG export, including "
        "setup, native calls, exact cache/result qualification and record/SVG exports. It excludes process imports, argument parsing and "
        "the final timings.tsv/RECIPE.md writes; named phases must not be summed to infer a full wall interval. "
        "No trims, imported model adaptation, cross-patch metric selection, topology or implicit backend fallback.\n")
    print(f"shape={args.shape} mode={args.mode} backend={args.backend} device={args.device} count={args.count} "
          f"gap={counts['gap']} unresolved={counts['unresolved']} discover_ms={dict(phases)['resident_discover']:.6f}")


if __name__=="__main__":main()
