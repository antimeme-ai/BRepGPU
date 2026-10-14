#!/usr/bin/env python3
"""Actual body-chart broad-phase caller; all output is owned TSV/binary/SVG."""
import argparse
import csv
import math
from pathlib import Path
import struct
import time

from brepgpu import Engine
from brepgpu.body import Frame, make_body
from brepgpu.pair_scene import PairDiscovery, prepare_scene
from brepgpu.pair_oracles import qualify_scene, qualify_page


def milliseconds(start):
    return 1000*(time.perf_counter()-start)


def tsv(path, header, rows):
    with path.open("w", newline="") as stream:
        writer = csv.writer(stream, delimiter="\t", lineterminator="\n")
        writer.writerow(header)
        writer.writerows(rows)


def make_inputs(size):
    """Deterministic distinct body identities; no duplicated boxes to pad work."""
    if not 1 <= size <= 128:
        raise ValueError("scene size must be in [1,128]")
    width = min(16, math.ceil(math.sqrt(size)))
    height = math.ceil(size/width)
    shapes = ("box", "cylinder", "plate")
    sides = []
    for side in range(2):
        bodies = []
        for index in range(size):
            x = 1.5*(index%width-(width-1)/2)+side*.375
            y = 1.5*(index//width-(height-1)/2)+side*.25
            z = side*.125
            scale = .5 if index%2 else 1.
            bodies.append(make_body(shapes[(index+side)%3], Frame((x,y,z), scale)))
        sides.append(tuple(bodies))
    return tuple(sides)


def prepare_inputs(size, library_path=None):
    start = time.perf_counter()
    left_bodies, right_bodies = make_inputs(size)
    body_ms = milliseconds(start)
    start = time.perf_counter()
    with Engine(backend="cpu", threads=1, library_path=library_path) as cpu:
        left = prepare_scene(cpu, left_bodies)
        right = prepare_scene(cpu, right_bodies)
    return left, right, (("body_prepare_ms", body_ms),
                         ("cpu_scene_prepare_ms", milliseconds(start)))


def render(page, path):
    """Draw the actual mask; no surface-intersection curves are invented."""
    nleft, nright = len(page.left.sources), len(page.right.sources)
    if nleft > 100 or nright > 100:
        # Keep the drawing bounded; the full mask/shortlist export remains exact.
        shown_left, shown_right = min(100,nleft), min(100,nright)
    else:
        shown_left, shown_right = nleft, nright
    survivors = tuple(page.survivors())
    pitch = min(16., 530/max(1,shown_right), 500/max(1,shown_left))
    x0, y0 = 285., 160.
    parts = ['<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 900 760">',
             '<rect width="900" height="760" fill="#101923"/>',
             '<g font-family="sans-serif">',
             '<text x="28" y="40" fill="#edf4fc" font-size="25">BRepGPU · conservative surface pairs</text>',
             '<text x="28" y="72" fill="#a9bed0" font-size="15">Common-world carrier bounds → resident pair mask → typed chart shortlist</text>',
             f'<text x="28" y="107" fill="#58c9e7" font-size="19">{len(survivors):,} survivors / {page.info.count:,} examined pairs</text>',
             f'<text x="28" y="134" fill="#a9bed0" font-size="14">{nleft} left charts × {nright} right charts · full-domain flag {bool(page.info.flags & 2)}</text>',
             f'<rect x="{x0}" y="{y0}" width="{shown_right*pitch}" height="{shown_left*pitch}" fill="#223344"/>']
    for flat, ls, rs in survivors:
        li, ri = divmod(flat,nright)
        if li < shown_left and ri < shown_right:
            color = ("#58c9e7", "#f4c878", "#b697ed")[(ls.snapshot.body_id-page.left.bodies[0].body_id)%3]
            parts.append(f'<rect x="{x0+ri*pitch:.3f}" y="{y0+li*pitch:.3f}" width="{pitch:.3f}" height="{pitch:.3f}" fill="{color}"><title>pair {flat}: body {ls.snapshot.body_id} face {ls.face_id} chart {ls.chart_id} / body {rs.snapshot.body_id} face {rs.face_id} chart {rs.chart_id}</title></rect>')
    previous = None
    for index, source in enumerate(page.left.sources[:shown_left]):
        key = source.snapshot.body_id
        if key != previous:
            y = y0+index*pitch
            parts.append(f'<path d="M{x0},{y:.3f} H{x0+shown_right*pitch:.3f}" stroke="#101923" stroke-width="1.5"/>')
            parts.append(f'<text x="28" y="{y+13:.3f}" fill="#edf4fc" font-size="13">left body {key} · {source.snapshot.shape}</text>')
            previous = key
    previous = None
    right_groups = []
    for index, source in enumerate(page.right.sources[:shown_right]):
        if source.snapshot.body_id != previous:
            x = x0+index*pitch
            parts.append(f'<path d="M{x:.3f},{y0} V{y0+shown_left*pitch:.3f}" stroke="#101923" stroke-width="1.5"/>')
            right_groups.append(f'{source.snapshot.body_id}:{source.snapshot.shape}')
            previous = source.snapshot.body_id
    parts.extend((f'<text x="28" y="672" fill="#a9bed0" font-size="13">Columns: right bodies {", ".join(right_groups)}</text>',
                  '<text x="28" y="699" fill="#edf4fc" font-size="15">Colored cells survive closed-box culling. Dark cells are excluded carrier pairs.</text>',
                  '<text x="28" y="725" fill="#a9bed0" font-size="14">Touching stays. Hole and cap false positives stay. Survivors do not establish intersections.</text>',
                  '<text x="28" y="747" fill="#a9bed0" font-size="12">CPU bounds/world conversion and shortlist decoding · native CPU or P100 mask · no topology upload</text>',
                  '</g></svg>'))
    path.write_text("\n".join(parts)+"\n")


def export_page(page, output):
    source_rows = []
    for side, scene in (("left",page.left),("right",page.right)):
        for index, (source, local, world) in enumerate(zip(scene.sources,scene.local_boxes,scene.boxes)):
            frame = source.snapshot.frame
            source_rows.append((side,index,source.snapshot.body_id,source.snapshot.revision,
                                source.face_id,source.chart_id,source.patch_slot,source.snapshot.shape,
                                *map(float.hex,frame.origin),frame.scale.hex(),
                                *map(float.hex,local),*map(float.hex,world)))
    tsv(output/"sources.tsv", ("side","index","body_id","revision","face_id","chart_id",
        "patch_slot","shape","origin_x_hex","origin_y_hex","origin_z_hex","scale_hex",
        "local_lx_hex","local_ly_hex","local_lz_hex","local_ux_hex","local_uy_hex","local_uz_hex",
        "world_lx_hex","world_ly_hex","world_lz_hex","world_ux_hex","world_uy_hex","world_uz_hex"), source_rows)
    (output/"mask.u32le").write_bytes(struct.pack(f"<{len(page.words)}I",*page.words))
    fields = ("total_pairs","start","count","next_start","left_count","right_count",
              "word_count","backend","flags","kernel_ms")
    tsv(output/"page.tsv", ("field","value"), ((name,getattr(page.info,name)) for name in fields))
    tsv(output/"survivors.tsv", ("flat_index","left_index","right_index","left_body","left_face",
        "left_chart","right_body","right_face","right_chart"),
        ((flat,*divmod(flat,page.info.right_count),ls.snapshot.body_id,ls.face_id,ls.chart_id,
          rs.snapshot.body_id,rs.face_id,rs.chart_id) for flat,ls,rs in page.survivors()))
    render(page,output/"pairs.svg")


def main(args):
    output = args.output_directory
    output.mkdir(parents=True,exist_ok=False)
    actual_start = time.perf_counter()
    left,right,setup = prepare_inputs(args.size,args.library_path)
    phases = list(setup)
    start = time.perf_counter()
    with Engine(backend=args.backend,device=args.device,threads=args.threads,profile=args.profile,
                library_path=args.library_path,plugin_path=args.plugin_path) as engine:
        phases.append(("cold_engine_ms",milliseconds(start)))
        start = time.perf_counter()
        with PairDiscovery(engine,left,right) as discovery:
            phases.append(("cold_index_upload_ms",milliseconds(start)))
            start = time.perf_counter()
            page = discovery.page()
            phases.append(("cold_page_caller_ms",milliseconds(start)))
            phases.extend(page.timings)
            start = time.perf_counter()
            actual_left,actual_right = discovery.download_boxes()
            phases.append(("qualification_input_download_ms",milliseconds(start)))
            start = time.perf_counter()
            quality = {}
            quality.update(("left_"+key,value) for key,value in qualify_scene(left,actual_left).items())
            quality.update(("right_"+key,value) for key,value in qualify_scene(right,actual_right).items())
            quality.update(qualify_page(page,actual_left,actual_right))
            phases.append(("independent_qualification_ms",milliseconds(start)))
            if not page.info.flags & 2:
                raise RuntimeError("demonstration scene did not fit a complete page")
            start = time.perf_counter()
        phases.append(("index_cleanup_ms",milliseconds(start)))
        start = time.perf_counter()
    phases.append(("engine_cleanup_ms",milliseconds(start)))
    start = time.perf_counter()
    export_page(page,output)
    tsv(output/"quality.tsv",("metric","value"),quality.items())
    tsv(output/"input.tsv",("field","value"), (("bodies_per_side",args.size),
        ("backend",args.backend),("device",args.device),("threads",args.threads),
        ("profile",int(args.profile)),("left_charts",len(left.sources)),("right_charts",len(right.sources)),
        ("cuda_index_upload_bytes",48*(len(left.sources)+len(right.sources)) if args.backend=="cuda" else 0),
        ("cuda_mask_download_bytes",4*len(page.words) if args.backend=="cuda" else 0),
        ("topology_upload_bytes",0),("mask_layout","page-local LSB-first u32 little-endian")))
    phases.append(("export_ms",milliseconds(start)))
    phases.append(("actual_export_caller_ms",milliseconds(actual_start)))
    # Final timing-file publication follows this explicit timer boundary.
    tsv(output/"timings.tsv",("phase","milliseconds"),phases)
    print(f"{args.backend}:{args.device} scene={args.size} "
          f"pairs={page.info.count} survivors={sum(word.bit_count() for word in page.words)}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--backend",choices=("cpu","cuda"),default="cpu")
    parser.add_argument("--device",type=int,default=0)
    parser.add_argument("--threads",type=int,default=1)
    parser.add_argument("--profile",action="store_true")
    parser.add_argument("--size",type=int,default=4)
    parser.add_argument("--library-path",type=Path)
    parser.add_argument("--plugin-path",type=Path)
    parser.add_argument("--output-directory",type=Path,required=True)
    main(parser.parse_args())
