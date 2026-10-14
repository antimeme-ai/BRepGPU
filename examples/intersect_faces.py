#!/usr/bin/env python3
"""Complete supported face sections and a two-sided CPU/CUDA sample consumer."""
import argparse
import csv
import hashlib
from pathlib import Path
import struct
import time

from brepgpu import Engine
from brepgpu.body import Frame, make_body
from brepgpu.pair_scene import PairDiscovery, prepare_scene
from brepgpu.pair_oracles import qualify_scene, qualify_page
from brepgpu.intersection_sampling import IntersectionSampler, prepare_samples
from brepgpu.intersection_oracles import recipe_counts, qualify_batch
from discover_pairs import engine_scope, milliseconds, tsv


def make_inputs(recipe):
    if recipe not in ("circle", "clipped"):
        raise ValueError("recipe must be circle or clipped")
    scale = 1. if recipe == "circle" else .25
    return (make_body("box", Frame((0., 0., -.5*scale), scale)),
            make_body("cylinder"))


def scalar(value):
    return str(value.a), str(value.b), str(value.d)


def manifest_values(core,plugin):
    root = Path(__file__).resolve().parents[1]
    sources = ("python/brepgpu/__init__.py","python/brepgpu/body.py","python/brepgpu/body_oracles.py",
        "python/brepgpu/intersections.py","python/brepgpu/intersection_sampling.py",
        "python/brepgpu/intersection_oracles.py","python/brepgpu/pair_scene.py",
        "python/brepgpu/pair_oracles.py","examples/discover_pairs.py","examples/intersect_faces.py")
    source_rows = tuple((name,hashlib.sha256((root/name).read_bytes()).hexdigest()) for name in sources)
    plugin = plugin or core.parent/"libbrepgpu_cuda.so"
    binary_rows = tuple((name,hashlib.sha256(path.read_bytes()).hexdigest())
                        for name,path in (("core",core),("cuda_plugin",plugin)))
    return source_rows,binary_rows


def export_result(result, output):
    tsv(output/"components.tsv", ("component_id", "kind", "closed", "tangent", "piece_ids"),
        ((c.id,c.kind,int(c.closed),int(c.tangent),",".join(map(str,c.piece_ids)))
         for c in result.components))
    tsv(output/"pieces.tsv", ("piece_id","component_id","span_id","kind","left_slot","right_slot",
        "lo_a","lo_b","lo_d","hi_a","hi_b","hi_d","boundary_constraints"),
        ((p.id,p.component_id,p.span_id,p.kind,p.left_slot,p.right_slot,*scalar(p.lo),*scalar(p.hi),
          ",".join(p.boundary_constraints)) for p in result.pieces))
    coefficients = []
    for p in result.pieces:
        for name,mapping in (("spatial",p.spatial_map),("left_uv",p.left_map),("right_uv",p.right_map)):
            for axis,poly in enumerate(mapping.numerators):
                coefficients.extend((p.id,name,str(axis),power,*scalar(coefficient))
                                    for power,coefficient in enumerate(poly))
            coefficients.extend((p.id,name,"denominator",power,*scalar(coefficient))
                                for power,coefficient in enumerate(mapping.denominator))
    tsv(output/"maps.tsv",("piece_id","map","coordinate","power","a","b","d"),coefficients)
    tsv(output/"events.tsv", ("event_id","constraints","joins","incidences"),
        ((e.id,",".join(e.constraints),",".join(e.joins),
          ",".join(f"{p}:{end}" for p,end in e.incidences)) for e in result.events))
    sources,controls = [],[]
    for side,ref in (("left",result.left),("right",result.right)):
        body = ref.snapshot
        face = next(f for f in body.faces if f.id == ref.face_id)
        for slot in face.chart_slots:
            chart,patch = body.charts[slot],body.patches[slot]
            sources.append((side,slot,body.body_id,body.revision,face.id,chart.id,chart.quadrant,
                body.shape,face.kind,face.orientation,*map(float.hex,body.frame.origin),
                body.frame.scale.hex(),patch.degree_u,patch.degree_v,*map(float.hex,patch.uv)))
            controls.extend((side,slot,i//4,*map(float.hex,patch.controls[i:i+4]))
                            for i in range(0,len(patch.controls),4))
    tsv(output/"sources.tsv",("side","original_slot","body_id","revision","face_id","chart_id",
        "quadrant","shape","face_kind","orientation","origin_x_hex","origin_y_hex","origin_z_hex",
        "scale_hex","degree_u","degree_v","u0_hex","u1_hex","v0_hex","v1_hex"),sources)
    tsv(output/"controls.tsv",("side","original_slot","control_slot","ax_hex","ay_hex","az_hex","w_hex"),controls)


def export_batch(batch, request, output):
    rows = []
    for i,r in enumerate(batch.rows):
        refs = tuple(v for coordinate in r.reference_position for v in scalar(coordinate))
        rows.append((i,r.piece_id,r.component_id,r.span_id,r.location,*scalar(r.parameter),
            r.left_slot,r.right_slot,*map(float.hex,r.left_uv),*map(float.hex,r.right_uv),
            r.left_status,r.right_status,*map(float.hex,r.left_position),*map(float.hex,r.right_position),*refs))
    tsv(output/"samples.tsv", ("index","piece_id","component_id","span_id","location",
        "parameter_a","parameter_b","parameter_d","left_slot","right_slot",
        "left_u_hex","left_v_hex","right_u_hex","right_v_hex","left_status","right_status",
        "left_x_hex","left_y_hex","left_z_hex","right_x_hex","right_y_hex","right_z_hex",
        "reference_x_a","reference_x_b","reference_x_d","reference_y_a","reference_y_b","reference_y_d",
        "reference_z_a","reference_z_b","reference_z_d"),rows)
    for field,filename in (("source_descriptors","native-descriptors.bin"),
                            ("native_slots","native-slots.u32le"),("native_uv","native-uv.f64le")):
        (output/filename).write_bytes(getattr(batch,field))
    tsv(output/"native-sources.tsv",("dense_slot","side","original_slot"),
        ((i,*source) for i,source in enumerate(batch.dense_sources)))
    tsv(output/"request.tsv",("index","piece_id","component_id","span_id","location",
        "parameter_a","parameter_b","parameter_d",
        "left_u_a","left_u_b","left_u_d","left_v_a","left_v_b","left_v_d",
        "right_u_a","right_u_b","right_u_d","right_v_a","right_v_b","right_v_d",
        "reference_x_a","reference_x_b","reference_x_d","reference_y_a","reference_y_b","reference_y_d",
        "reference_z_a","reference_z_b","reference_z_d"),
        ((i,c.piece_id,c.component_id,c.span_id,c.location,*scalar(c.parameter),
          *(v for coordinate in (*c.left_uv,*c.right_uv,*c.reference_position) for v in scalar(coordinate)))
         for i,c in enumerate(request.candidates)))


def render(batch, output, recipe, backend, device):
    colors = ("#5cd8ed","#ad9ef8","#f3a9cb","#f4c878")
    result = batch.result
    parts = ['<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1200 560">',
        '<rect width="1200" height="560" fill="#101923"/>','<g font-family="sans-serif">',
        '<text x="28" y="42" fill="#edf4fc" font-size="26">BRepGPU · complete analytic face sections</text>',
        f'<text x="28" y="72" fill="#a9bed0" font-size="15">{recipe}: {len(result.components)} components · '
        f'{len(set(p.span_id for p in result.pieces))} spans · {len(result.pieces)} chart occurrences · '
        f'{batch.count} two-sided samples · {backend}{" "+str(device) if backend=="cuda" else ""}</text>']
    panels = (("World XY section",28),("Planar face UV",422),("Wall atlas UV",816))
    y0,size = 132,330
    for name,x0 in panels:
        parts.extend((f'<text x="{x0}" y="113" fill="#edf4fc" font-size="18">{name}</text>',
            f'<rect x="{x0}" y="{y0}" width="{size}" height="{size}" fill="#182733" stroke="#476071"/>'))
    plane_scale = result.left.snapshot.frame.scale
    width = .75*plane_scale
    x0 = panels[0][1]
    project = lambda x,y: (x0+size*(x+.3)/.6,y0+size*(.3-y)/.6)
    x,y = project(-width,width)
    rectsize = size*2*width/.6
    # Clip the outer rectangle to this view; the full circle's large face lies
    # outside it, so state the rectangle size instead of drawing a false trim.
    if rectsize <= size:
        parts.append(f'<rect x="{x:.3f}" y="{y:.3f}" width="{rectsize:.3f}" height="{rectsize:.3f}" fill="none" stroke="#91a8b7" stroke-dasharray="5 5"/>')
    parts.append(f'<circle cx="{x0+size/2}" cy="{y0+size/2}" r="{size*.25/.6}" fill="none" stroke="#476071"/>')
    grouped = {}
    for row in batch.rows:
        grouped.setdefault(row.piece_id,[]).append(row)
    pieces = {p.id:p for p in result.pieces}
    for pid,rows in grouped.items():
        q = result.right.snapshot.charts[pieces[pid].right_slot].quadrant
        positions = [project(r.right_position[0],r.right_position[1]) for r in rows]
        planar = [(422+size*r.left_uv[0],y0+size*(1-r.left_uv[1])) for r in rows]
        atlas = [(816+size*(q+r.right_uv[0])/4,y0+size*(1-r.right_uv[1])) for r in rows]
        for points in (positions,planar,atlas):
            textpoints = " ".join(f"{x:.3f},{y:.3f}" for x,y in points)
            parts.append(f'<polyline points="{textpoints}" fill="none" stroke="{colors[q]}" stroke-width="3"/>')
            for x,y in (points[0],points[-1]):
                parts.append(f'<circle cx="{x:.3f}" cy="{y:.3f}" r="3.5" fill="#edf4fc"/>')
    note = "Wall a=0/4 retains two seam lifts." if recipe=="circle" else "All four arcs end on the planar trim."
    parts.extend((f'<text x="28" y="491" fill="#a9bed0" font-size="14">Rectangle half-width {width:g}; wall radius 0.25. White dots are exact endpoint occurrences drawn approximately.</text>',
        f'<text x="28" y="517" fill="#edf4fc" font-size="14">One common parameter → spatial curve + both UV maps. {note}</text>',
        '<text x="28" y="543" fill="#a9bed0" font-size="13">Exact CPU component construction · CPU/CUDA evaluates supplied curves · no general SSI or Boolean claim</text>',
        '</g></svg>'))
    (output/"section.svg").write_text("\n".join(parts)+"\n")


def main(args):
    output = args.output_directory
    output.mkdir(parents=True,exist_ok=False)
    whole = time.perf_counter()
    phases = []
    start = time.perf_counter()
    identities = manifest_values(args.library_path,args.plugin_path)
    phases.append(("identity_read_ms",milliseconds(start)))
    start = time.perf_counter()
    plane,wall = make_inputs(args.recipe)
    phases.append(("body_preparation_ms",milliseconds(start)))
    start = time.perf_counter()
    with engine_scope(Engine(backend="cpu",threads=1,library_path=args.library_path)) as cpu:
        left = prepare_scene(cpu,(plane,),face_ids=((plane.body_id,1),))
        right = prepare_scene(cpu,(wall,),face_ids=((wall.body_id,3),))
    phases.append(("cpu_pair_scene_ms",milliseconds(start)))
    start = time.perf_counter()
    with engine_scope(Engine(backend=args.backend,device=args.device,threads=args.threads,
                            library_path=args.library_path,plugin_path=args.plugin_path)) as engine:
        phases.append(("cold_engine_ms",milliseconds(start)))
        start = time.perf_counter()
        with PairDiscovery(engine,left,right) as pairs:
            page = pairs.page()
            actual_left,actual_right = pairs.download_boxes()
            quality = {}
            quality.update(("left_"+k,v) for k,v in qualify_scene(left,actual_left).items())
            quality.update(("right_"+k,v) for k,v in qualify_scene(right,actual_right).items())
            quality.update(("pair_"+k,v) for k,v in qualify_page(page,actual_left,actual_right).items())
            if not page.info.flags & 2 or page.info.count != 4:
                raise RuntimeError("caller did not consume the complete four chart pairs")
            face_pairs = {(ls.snapshot.body_id,ls.face_id,rs.snapshot.body_id,rs.face_id)
                          for _,ls,rs in page.survivors()}
            if face_pairs != {(plane.body_id,1,wall.body_id,3)}:
                raise AssertionError("complete known section was lost by the broad phase")
        phases.append(("pair_discovery_qualification_cleanup_ms",milliseconds(start)))
        start = time.perf_counter()
        result = plane.intersect_faces(1,wall,3)
        phases.append(("exact_construction_ms",milliseconds(start)))
        start = time.perf_counter()
        quality.update(recipe_counts(result,args.recipe))
        phases.append(("literal_recipe_qualification_ms",milliseconds(start)))
        start = time.perf_counter()
        request = prepare_samples(result,args.resolution)
        phases.append(("request_preparation_ms",milliseconds(start)))
        start = time.perf_counter()
        with IntersectionSampler(engine,result) as sampler:
            phases.append(("geometry_setup_ms",milliseconds(start)))
            start = time.perf_counter()
            batch = sampler.sample(request)
            phases.append(("actual_sample_caller_ms",milliseconds(start)))
            phases.extend(batch.timings)
            start = time.perf_counter()
            sample_quality = qualify_batch(batch,request)
            quality.update(("sample_"+k,sample_quality[k]) for k in
                           ("count","evaluated_count","max_position_error","max_pair_error"))
            phases.append(("sample_qualification_ms",milliseconds(start)))
            cleanup = time.perf_counter()
        phases.append(("sampler_cleanup_ms",milliseconds(cleanup)))
        cleanup = time.perf_counter()
    phases.append(("engine_cleanup_ms",milliseconds(cleanup)))
    start = time.perf_counter()
    export_result(result,output)
    export_batch(batch,request,output)
    (output/"pair-mask.u32le").write_bytes(struct.pack(f"<{len(page.words)}I",*page.words))
    tsv(output/"pair-page.tsv",("field","value"),
        ((name,getattr(page.info,name)) for name in
         ("total_pairs","start","count","next_start","left_count","right_count","flags")))
    tsv(output/"pair-resident-boxes.tsv",("side","index","lx_hex","ly_hex","lz_hex","ux_hex","uy_hex","uz_hex"),
        ((side,i,*map(float.hex,box)) for side,boxes in (("left",actual_left),("right",actual_right))
         for i,box in enumerate(boxes)))
    render(batch,output,args.recipe,args.backend,args.device)
    tsv(output/"quality.tsv",("metric","value"),quality.items())
    tsv(output/"input.tsv",("field","value"),(("recipe",args.recipe),("resolution",args.resolution),
        ("backend",args.backend),("device",args.device),("threads",args.threads),
        ("components",len(result.components)),("pieces",len(result.pieces)),("candidate_samples",batch.count),
        ("native_rows",2*batch.count),("pair_total",4),("unique_face_pairs",len(face_pairs)),
        ("cuda_geometry_upload_bytes",976 if args.backend=="cuda" else 0),
        ("cuda_query_upload_bytes",40*batch.count if args.backend=="cuda" else 0),
        ("cuda_position_status_download_bytes",56*batch.count if args.backend=="cuda" else 0),
        ("cuda_pair_index_upload_bytes",240 if args.backend=="cuda" else 0),
        ("cuda_pair_mask_download_bytes",4 if args.backend=="cuda" else 0),
        ("cuda_pair_bounds_download_bytes",240 if args.backend=="cuda" else 0),
        ("topology_upload_bytes",0)))
    if identities != manifest_values(args.library_path,args.plugin_path):
        raise RuntimeError("caller sources or binaries changed during the operation")
    tsv(output/"code.tsv",("source","sha256"),identities[0])
    tsv(output/"binaries.tsv",("binary","sha256"),identities[1])
    phases.append(("export_ms",milliseconds(start)))
    # Final timing-file write/console publication are outside this measured span.
    phases.append(("actual_whole_caller_ms",milliseconds(whole)))
    tsv(output/"timings.tsv",("phase","milliseconds"),phases)
    print(f"{args.recipe} {args.backend}:{args.device}: {len(result.components)} components, "
          f"{len(result.pieces)} occurrences, {batch.count} samples, {2*batch.count} native rows")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--recipe",choices=("circle","clipped"),default="clipped")
    parser.add_argument("--resolution",type=int,default=65)
    parser.add_argument("--backend",choices=("cpu","cuda"),default="cpu")
    parser.add_argument("--device",type=int,default=0)
    parser.add_argument("--threads",type=int,default=1)
    parser.add_argument("--library-path",type=Path,required=True)
    parser.add_argument("--plugin-path",type=Path)
    parser.add_argument("--output-directory",type=Path,required=True)
    main(parser.parse_args())
