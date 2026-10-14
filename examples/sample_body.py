#!/usr/bin/env python3
"""Sample owned analytic faces with exact trim labels and oriented normals."""
import argparse
import csv
import hashlib
import html
import math
from pathlib import Path
import time

from brepgpu import Engine
from brepgpu.body import Frame, make_body, INTERIOR, BOUNDARY
from brepgpu.body_sampling import BodySampler, prepare_grid
from brepgpu.body_oracles import qualify_batch


def write_tsv(path, header, rows):
    with path.open("w", newline="") as stream:
        writer = csv.writer(stream, delimiter="\t", lineterminator="\n")
        writer.writerow(header)
        writer.writerows(rows)


def render_samples(batch, path, label):
    """Render actual admitted samples and the owned reference boundaries."""
    body = batch.snapshot
    origin, scale = body.frame.origin, body.frame.scale

    def project(point):
        x, y, z = point
        return 400 + 225*x - 140*y, 315 + 80*x + 100*y - 220*z

    def local(point):
        return tuple((point[k]-origin[k])/scale for k in range(3))

    admitted = tuple(row for row in batch.rows if row.trim in (INTERIOR, BOUNDARY))
    stride = max(1, math.ceil(len(admitted)/5000))
    arrow_stride = max(1, math.ceil(len(admitted)/90))
    fragments = [
        '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 800 640">',
        '<rect width="800" height="640" fill="#101923"/>',
        '<defs><marker id="normal" markerWidth="4" markerHeight="4" refX="3" refY="2" orient="auto"><path d="M0,0 L4,2 L0,4" fill="#f4c878"/></marker></defs>',
        '<text x="28" y="40" fill="#edf4fc" font-family="sans-serif" font-size="24">BRepGPU · body samples and normals</text>',
        f'<text x="28" y="68" fill="#a9bed0" font-family="sans-serif" font-size="15">{html.escape(label)} · {body.shape} · {len(body.faces)} faces / {len(body.charts)} charts</text>',
    ]
    for i, row in enumerate(admitted):
        if i % stride:
            continue
        x, y = project(local(row.position))
        face = body.faces[body.charts[row.patch_slot].face_slot]
        color = "#8de5b7" if face.name == "wall" else "#58c9e7"
        fragments.append(f'<circle cx="{x:.3f}" cy="{y:.3f}" r="1.15" fill="{color}" opacity=".65"/>')
    for edge_slot, edge in enumerate(body.edges):
        steps = 64 if body.curves[edge.curve_slot].kind == "circle" else 1
        points = [project(body.edge_point(edge_slot, i/steps)) for i in range(steps+1)]
        commands = " ".join(("M" if i == 0 else "L") + f"{x:.3f},{y:.3f}"
                            for i, (x, y) in enumerate(points))
        fragments.append(f'<path d="{commands}" fill="none" stroke="#a9bed0" stroke-width="1.2" opacity=".75"/>')
    for i, row in enumerate(admitted):
        if i % arrow_stride:
            continue
        point = local(row.position)
        x0, y0 = project(point)
        x1, y1 = project(tuple(point[k]+.085*row.normal[k] for k in range(3)))
        fragments.append(f'<path d="M{x0:.3f},{y0:.3f} L{x1:.3f},{y1:.3f}" stroke="#f4c878" stroke-width="1" marker-end="url(#normal)"/>')
    rejected = batch.count-len(admitted)
    fragments.extend((
        f'<text x="28" y="562" fill="#edf4fc" font-family="sans-serif" font-size="16">{len(admitted):,} evaluated / {batch.count:,} candidates · {rejected:,} excluded or invalid</text>',
        '<text x="28" y="590" fill="#a9bed0" font-family="sans-serif" font-size="14">Cyan: planar samples · green: cylindrical wall · gold: oriented normals</text>',
        '<text x="28" y="614" fill="#a9bed0" font-family="sans-serif" font-size="14">Point output, thinned for display. Gray: CPU-owned boundaries. No mesh/chord claim.</text>',
        '</svg>',
    ))
    path.write_text("\n".join(fragments)+"\n")


def export_samples(batch, directory, label):
    body = batch.snapshot
    directory.mkdir(exist_ok=False)
    write_tsv(directory/"samples.tsv",
              ("candidate", "body_id", "revision", "entity_kind", "face_id", "chart_id", "patch_slot",
               "u_hex", "v_hex", "trim", "native_status", "x", "y", "z", "nx", "ny", "nz"),
              ((row.index, body.body_id, body.revision, 5, row.face_id, row.chart_id, row.patch_slot,
                row.u.hex(), row.v.hex(), row.trim, row.native_status,
                *(format(x, ".17g") for x in row.position+row.normal)) for row in batch.rows))
    tables = directory/"tables"
    tables.mkdir()
    manifest = []
    for name, data in body.packed_tables:
        if not name or Path(name).name != name:
            raise ValueError("unsafe body table name")
        (tables/(name+".bin")).write_bytes(data)
        manifest.append((name, len(data), hashlib.sha256(data).hexdigest()))
    write_tsv(directory/"tables.tsv", ("table", "bytes", "sha256"), manifest)
    write_tsv(directory/"patches.tsv",
              ("patch_slot", "face_id", "chart_id", "degree_u", "degree_v", "u0_hex", "u1_hex",
               "v0_hex", "v1_hex", "origin_x_hex", "origin_y_hex", "origin_z_hex", "scale_hex", "controls"),
              ((i, body.faces[chart.face_slot].id, chart.id, chart.patch.degree_u, chart.patch.degree_v,
                *(x.hex() for x in chart.patch.uv+chart.patch.origin+(chart.patch.scale,)),
                len(chart.patch.controls)//4) for i, chart in enumerate(body.charts)))
    write_tsv(directory/"controls.tsv", ("patch_slot", "control", "Ax_hex", "Ay_hex", "Az_hex", "W_hex"),
              ((i, k, *(x.hex() for x in patch.controls[4*k:4*k+4]))
               for i, patch in enumerate(body.patches) for k in range(len(patch.controls)//4)))
    admitted = tuple(row for row in batch.rows if row.trim in (INTERIOR, BOUNDARY))
    with (directory/"points.ply").open("w") as stream:
        stream.write("ply\nformat ascii 1.0\ncomment owned body face samples, not a mesh\n")
        stream.write(f"element vertex {len(admitted)}\n")
        for name in ("x", "y", "z", "nx", "ny", "nz"):
            stream.write(f"property double {name}\n")
        stream.write("property uint face_id\nproperty uint chart_id\nend_header\n")
        for row in admitted:
            stream.write(" ".join(format(x, ".17g") for x in row.position+row.normal)
                         + f" {row.face_id} {row.chart_id}\n")
    render_samples(batch, directory/"body-samples.svg", label)


def main():
    parser = argparse.ArgumentParser(__doc__)
    parser.add_argument("--shape", choices=("box", "cylinder", "plate"), default="plate")
    parser.add_argument("--backend", choices=("cpu", "cuda"), default="cpu")
    parser.add_argument("--device", type=int, default=0)
    parser.add_argument("--threads", type=int, default=1)
    parser.add_argument("--resolution", type=int, default=33)
    parser.add_argument("--origin", nargs=3, type=float, default=(0.,0.,0.))
    parser.add_argument("--scale", type=float, default=1.)
    parser.add_argument("--face-ids", nargs="*", type=int)
    parser.add_argument("--library-path", type=Path)
    parser.add_argument("--plugin-path", type=Path)
    parser.add_argument("--output-directory", type=Path, required=True)
    args = parser.parse_args()
    if args.output_directory.exists():
        parser.error("output directory must be fresh")
    args.output_directory.parent.mkdir(parents=True, exist_ok=True)
    wall_start = time.perf_counter()
    start = time.perf_counter()
    body = make_body(args.shape, frame=Frame(tuple(args.origin), args.scale))
    body_ms = 1000*(time.perf_counter()-start)
    start = time.perf_counter()
    request = prepare_grid(body, args.resolution, face_ids=args.face_ids)
    request_ms = 1000*(time.perf_counter()-start)
    start = time.perf_counter()
    with Engine(backend=args.backend, device=args.device, threads=args.threads,
                library_path=args.library_path, plugin_path=args.plugin_path) as engine:
        engine_ms = 1000*(time.perf_counter()-start)
        start = time.perf_counter()
        with BodySampler(engine, body) as sampler:
            patchset_ms = 1000*(time.perf_counter()-start)
            batch = sampler.sample(request)
            close_start = time.perf_counter()
        # Both sampler and engine close belong to the cold caller.
    cleanup_ms = 1000*(time.perf_counter()-close_start)
    cold_ms = 1000*(time.perf_counter()-wall_start)
    start = time.perf_counter()
    metrics = qualify_batch(batch)
    qualification_ms = 1000*(time.perf_counter()-start)
    label = f"{args.backend.upper()} device {args.device}" if args.backend == "cuda" else f"CPU {args.threads} requested workers"
    start = time.perf_counter()
    export_samples(batch, args.output_directory, label)
    write_tsv(args.output_directory/"qualification.tsv", ("metric", "value"), metrics.items())
    write_tsv(args.output_directory/"input.tsv", ("field", "value"), (
        ("shape", body.shape), ("backend", args.backend), ("device", args.device),
        ("requested_cpu_workers", args.threads), ("resolution", args.resolution),
        ("origin_hex", " ".join(x.hex() for x in body.frame.origin)),
        ("scale_hex", body.frame.scale.hex()), ("body_id", body.body_id), ("revision", body.revision),
        ("selected_face_ids", "all" if args.face_ids is None else " ".join(str(x) for x in args.face_ids)),
        ("candidate_rows", batch.count), ("evaluated_rows", batch.evaluated_count),
        ("query_upload_bytes_cuda", 20*batch.evaluated_count if args.backend == "cuda" else 0),
        ("jet_status_download_bytes_cuda", 76*batch.evaluated_count if args.backend == "cuda" else 0),
        ("body_tables_upload_bytes", 0),
    ))
    (args.output_directory/"RECIPE.md").write_text(
        f"# {body.shape.capitalize()} face samples\n\n"
        f"{label}, resolution {args.resolution}. Canonical owned analytic fixture; "
        "no imported body or Boolean operation. CPU body/trim/normal work consumes "
        "one existing native first-jet batch. Cylinder quarters are charts of one "
        "face; the seam has two distinct uses of one spatial edge.\n\n"
        "samples.tsv retains every candidate and exact submitted UV bits. Labels "
        "0 rejected, 1 interior, 2 exact boundary, 3 invalid; native status -1 "
        "means not evaluated. The plate's circular hole has no exact binary64 UV "
        "boundary witnesses; rounded edge UV has its actual exact trim label. "
        "Points/normals are per face/chart, with duplicates at chart joins. "
        "Gray SVG curves show owned CPU boundaries. No meshing or chord guarantee.\n\n"
        "tables/ contains immutable read-only factory record dumps; layouts are "
        "documented in python/brepgpu/body.py. IDs are process-local namespaces, "
        "not a cross-session import contract. Patches/control bits and frame are "
        "retained separately. Independent analytic/exact-input qualification "
        "checks all candidate labels, accepted XYZ/normals and provenance.\n\n"
        "Timing begins after imports/argument parsing and fresh-directory admission, "
        "before body/request creation. Cold sampling includes native setup/cleanup. "
        "Export-inclusive wall adds independent qualification and all artifacts "
        "through this recipe/SVG; the final timings.tsv write is excluded. "
        "Download includes Python arrays and host source metadata; CUDA payload "
        "counts are not total host-memory traffic or network transfers.\n")
    export_ms = 1000*(time.perf_counter()-start)
    complete_ms = 1000*(time.perf_counter()-wall_start)
    write_tsv(args.output_directory/"timings.tsv", ("phase", "milliseconds"), (
        ("body_prepare_ms", body_ms), ("request_prepare_ms", request_ms),
        ("engine_first_use_ms", engine_ms), ("patchset_ms", patchset_ms),
        *batch.timings, ("cold_cleanup_ms", cleanup_ms), ("cold_sampling_wall_ms", cold_ms),
        ("independent_qualification_ms", qualification_ms), ("export_ms", export_ms),
        ("export_inclusive_wall_ms", complete_ms),
    ))
    print(f"shape={body.shape} backend={args.backend} device={args.device} candidates={batch.count} "
          f"evaluated={batch.evaluated_count} cold_sampling_wall_ms={cold_ms:.6f} export_inclusive_wall_ms={complete_ms:.6f}")


if __name__ == "__main__":
    main()
