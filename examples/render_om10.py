#!/usr/bin/env python3
"""Complete supplied OM10 source-to-image caller; product execution is fleet-only.

Prepare and evaluate modes never rasterize the movement. Run actual render mode
only after the reviewed implementation and complete source qualification pass.
"""
import argparse
import csv
import hashlib
from pathlib import Path
import subprocess
import sys
import time

from brepgpu.imported_assembly import assemble
from om10_scene import (PALETTE, authored_layout, authored_material, camera_view,
                        labeled_svg, pack_views, placed_corners, visible_anchors,
                        write_png)


ROOT = Path(__file__).resolve().parents[1]
OM10_SHA256 = 'c60675abe3314dca55a31da0406deef851e128e2ee8608536525f926f56d61e3'


def tsv(path, header, rows):
    with Path(path).open('w', newline='', encoding='utf-8') as output:
        writer = csv.writer(output, delimiter='\t', lineterminator='\n')
        writer.writerow(header)
        writer.writerows(rows)


def pin_files():
    files = tuple((ROOT/'python/brepgpu').glob('imported_*.py'))
    files += tuple((ROOT/'native/om10').glob('*'))+tuple((ROOT/'native/step_source').glob('*'))
    files += (ROOT/'examples/om10_scene.py', ROOT/'examples/render_om10.py')
    files += tuple(ROOT/p for p in ('native/evaluator.hpp', 'native/backend_api.h', 'include/brepgpu.h'))
    return tuple((str(p.relative_to(ROOT)), hashlib.sha256(p.read_bytes()).hexdigest())
                 for p in sorted(files) if p.is_file())


def binary_pins(args):
    binaries = [('step_source_adapter', args.adapter)] if args.step else []
    if args.mode != 'prepare':
        binaries.append(('native_executable', args.native))
        if args.backend == 'cuda':
            binaries.append(('cuda_plugin', args.plugin))
    return tuple((role, hashlib.sha256(path.read_bytes()).hexdigest()) for role, path in binaries)


def require(condition, message):
    if not condition:
        raise ValueError(message)


def source_tables(output, source, assembly, layout_records, mesh):
    tsv(output/'bom.tsv', ('occurrence_id', 'source_usage_path', 'product_id', 'solid_id',
        'original_reference', 'original_name', *('matrix_'+str(i) for i in range(12))),
        ((o.id, '/'.join(map(str, o.path)), o.product_id, o.definition_id,
          o.reference, o.name, *o.transform) for o in assembly.occurrences))
    tsv(output/'groups.tsv', ('usage_path', 'product_id', 'reference', 'name', 'occurrence_ids'),
        (('/'.join(map(str, g.path)), g.product_id, g.reference, g.name,
          ','.join(map(str, g.occurrence_ids))) for g in assembly.groups))
    tsv(output/'explosion.tsv', ('occurrence_id', 'group_usage_path', 'authored_tier',
        'world_offset_x_mm', 'world_offset_y_mm', 'world_offset_z_mm', 'authored_reason'),
        ((r.occurrence_id, '/'.join(map(str, r.group_path)), r.tier, *r.offset, r.reason)
         for r in layout_records))
    tsv(output/'source-products.tsv', ('product_id', 'solid_id', 'name', 'reference', 'source_style_id'),
        ((p.id, p.definition_id, p.name, p.reference, p.style_id) for p in source.products))
    tsv(output/'source-styles.tsv', ('style_id', 'source_ids', 'has_color', 'linear_R', 'linear_G',
        'linear_B', 'alpha', 'material_name', 'density_unit', 'density', 'has_density'),
        ((s.id, ','.join(map(str, s.source_ids)), s.has_color, *s.rgba,
          s.material_name, s.density_unit, s.density, s.has_density) for s in source.styles))
    tsv(output/'source-diagnostics.tsv', ('severity', 'stage', 'source_ids', 'message'),
        ((d.severity, d.stage, ','.join(map(str, d.source_ids)), d.message) for d in source.diagnostics))
    tsv(output/'authored-palette.tsv', ('material_slot', 'display_name', 'linear_R', 'linear_G',
        'linear_B', 'roughness', 'status'),
        ((i, name, *rgb, roughness, 'authored illustrative palette; source styles retained separately')
         for i, (name, rgb, roughness) in enumerate(PALETTE)))
    faces_by_definition = {d.id: [] for d in source.definitions}
    face_definition = {f.id: f.definition_id for f in source.faces}
    for triangle in mesh.triangles:
        if not getattr(triangle, 'collapsed', False):
            faces_by_definition[face_definition[triangle.face_id]].append(triangle.face_id)
    counts = {id: len(faces) for id, faces in faces_by_definition.items()}
    start = 0
    ranges = []
    for occurrence in assembly.occurrences:
        count = counts[occurrence.definition_id]
        ranges.append((occurrence.id, occurrence.definition_id, start, count))
        start += count
    tsv(output/'primitive-instance-ranges.tsv', ('occurrence_id', 'solid_id', 'first_primitive_id',
        'triangle_count'), ranges)
    tsv(output/'definition-triangle-faces.tsv', ('solid_id', 'local_triangle_id', 'source_face_id'),
        ((definition.id, i, face_id)
         for definition in source.definitions
         for i, face_id in enumerate(faces_by_definition[definition.id])))
    return start


def callouts(assembly, anchors, width, height):
    # One source-linked group label per substantial root subassembly, plus visible
    # direct components. Complete individual identity remains in the BOM.
    group_for_path = {g.path: g for g in assembly.groups}
    candidates = {}
    for occurrence in assembly.occurrences:
        if occurrence.id not in anchors:
            continue
        ax, ay, pixels = anchors[occurrence.id]
        path = occurrence.path[:1]
        group = group_for_path.get(path)
        label = (group.reference+' '+group.name) if group is not None else occurrence.reference+' '+occurrence.name
        if path not in candidates or pixels > candidates[path][0]:
            candidates[path] = (pixels, label, ax, ay, occurrence.id,
                                group.occurrence_ids if group is not None else (occurrence.id,))
    # Pick visible mechanical groups rather than label every small repeated screw.
    chosen = sorted(candidates.values(), reverse=True)[:12]
    left = sorted((c for c in chosen if c[2] < width/2), key=lambda c: c[3])
    right = sorted((c for c in chosen if c[2] >= width/2), key=lambda c: c[3])
    labels, records = [], []
    for items, x, anchor in ((left, 65., 'start'), (right, width-65., 'end')):
        for i, (_, label, ax, ay, id, members) in enumerate(items):
            y = 245.+i*(height-500.)/max(1, len(items)-1)
            labels.append((label, x, y, ax, ay, anchor))
            records.append((label, id, ','.join(map(str, members)), ax, ay, x, y))
    return tuple(labels), tuple(records)


def main(args):
    require(sys.platform == 'linux', 'source/model/native/render callers execute on the Linux fleet')
    begin = time.perf_counter()
    pins = pin_files()
    executable_pins = binary_pins(args)
    from brepgpu.imported_source import load_source
    from brepgpu.imported_meshing import (MeshQuality, SceneInstance, SceneMaterial, mesh_imported)
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    times = []
    step = args.step.resolve() if args.step else None
    if step is not None:
        require(hashlib.sha256(step.read_bytes()).hexdigest() == OM10_SHA256, 'supplied OM10 source hash differs')
        binary_source = output/'source.bgsource'
        start = time.perf_counter()
        with (output/'adapter.tsv').open('w') as log:
            subprocess.run((str(args.adapter.resolve()), str(step), str(binary_source)), stdout=log, check=True)
        times.append(('foreign_cpu_step_translation', time.perf_counter()-start))
    else:
        binary_source = args.source.resolve()
    start = time.perf_counter()
    source = load_source(binary_source)
    times.append(('owned_source_decode', time.perf_counter()-start))
    require(source.source_sha256 == OM10_SHA256, 'source snapshot is not the supplied OM10 revision')
    require((len(source.definitions), len(source.faces), len(source.products), len(source.usages)) ==
            (107, 9162, 138, 196), 'complete original OM10 source inventory differs')
    start = time.perf_counter()
    quality = MeshQuality(tolerance_mm=args.tolerance, source_residual_mm=args.source_residual,
                          max_refinement=args.max_refinement, max_edge_samples=args.max_edge_samples)
    mesh = mesh_imported(source, quality)
    times.append(('owned_trimmed_mesh', time.perf_counter()-start))
    start = time.perf_counter()
    assembly = assemble(source)
    require(len(assembly.occurrences) == 166 and len({o.definition_id for o in assembly.occurrences}) == 107,
            'complete OM10 occurrence/definition expansion differs')
    definition_bounds = mesh.definition_bounds
    layout, records, axes = authored_layout(assembly, definition_bounds)
    times.append(('owned_assembly_explosion_layout', time.perf_counter()-start))
    if args.qualify:
        start = time.perf_counter()
        from brepgpu.imported_oracles import qualify_source, qualify_mesh
        source_checks = qualify_source(source, step)
        mesh_checks = qualify_mesh(mesh)
        tsv(output/'qualification.tsv', ('field', 'value'),
            (*source_checks.items(), *mesh_checks.items()))
        times.append(('independent_source_mesh_qualification', time.perf_counter()-start))
    start = time.perf_counter()
    instances = tuple(SceneInstance(o.id, o.definition_id, o.transform, layout.offsets[o.id-1])
                      for o in assembly.occurrences)
    materials = tuple(SceneMaterial(rgb, roughness) for _, rgb, roughness in PALETTE)
    material_map = tuple((d.id, authored_material(source.product(d.product_id).name)) for d in source.definitions)
    mesh.write_scene(output/'scene.bgmesh', instances, materials, definition_materials=material_map)
    placed_triangles = source_tables(output, source, assembly, records, mesh)
    times.append(('scene_and_source_tables_write', time.perf_counter()-start))
    names = ('assembled', 'exploded') if args.view == 'both' else (args.view,)
    views = tuple(camera_view(placed_corners(assembly, definition_bounds, layout, float(name == 'exploded')),
                    width=args.width, height=args.height, azimuth=args.azimuth, elevation=args.elevation,
                    alpha=float(name == 'exploded'), supersample=args.supersample,
                    padding=(360, 360, 210, 240), axes=axes,
                    flags=3 if args.mode == 'evaluate' else 0) for name in names)
    # Repeated frames run in one actual retained native scene. They are useful
    # view/update measurements, distinct from fresh STEP-to-image callers.
    requested_views = views*args.repeat_frames
    (output/'views.bgview').write_bytes(pack_views(requested_views))
    tsv(output/'views.tsv', ('frame', 'name', 'width', 'height', 'supersample', 'alpha',
        *('basis_'+str(i) for i in range(9)), 'center_x', 'center_y', 'center_z',
        'pixel_center_x', 'pixel_center_y', 'pixels_per_unit', 'flags'),
        ((i, names[i % len(names)], v.width, v.height, v.supersample, v.alpha,
          *v.basis, *v.center, *v.pixel_center, v.pixels_per_unit, v.flags)
         for i, v in enumerate(requested_views)))
    if args.mode != 'prepare':
        command = [str(args.native.resolve()), '--scene', str(output/'scene.bgmesh'),
                   '--views', str(output/'views.bgview'), '--output-directory', str(output/'native'),
                   '--backend', args.backend, '--threads', str(args.threads)]
        if args.backend == 'cuda':
            command += ['--device', str(args.device), '--plugin', str(args.plugin.resolve())]
        if args.mode == 'evaluate':
            command += ['--dump-source-jets']
        start = time.perf_counter()
        with (output/'native-process.tsv').open('w') as log:
            subprocess.run(command, stdout=log, check=True)
        process_seconds = time.perf_counter()-start
        times.append(('complete_native_process_including_exit', process_seconds))
        if args.mode == 'render':
            for i, view in enumerate(requested_views):
                start = time.perf_counter()
                name = f'{i:03d}-{names[i % len(names)]}'
                prefix = output/'native'/f'frame-{i:06d}'
                png = output/(name+'.png')
                write_png(png, view.width, view.height, prefix.with_suffix('.rgba').read_bytes())
                anchors = visible_anchors(prefix.with_suffix('.instance'), view.width, view.height)
                labels, label_rows = callouts(assembly, anchors, view.width, view.height)
                tsv(output/(name+'-callouts.tsv'), ('label', 'visible_anchor_occurrence_id',
                    'source_group_occurrence_ids', 'anchor_x', 'anchor_y', 'label_x', 'label_y'), label_rows)
                backend = f'CPU / {args.threads} threads' if args.backend == 'cpu' else f'P100 / device {args.device}'
                labeled_svg(output/(name+'.svg'), png, view,
                    title='OM10 / '+names[i % len(names)].upper(),
                    subtitle='Complete source movement · owned trimmed mesh · '+backend,
                    counts=(('source solids', 107), ('source faces', '9,162'), ('placed parts', 166)),
                    timings=(('native process, all frames', f'{process_seconds:.3f} s'),
                             ('placed triangles', f'{placed_triangles:,}'),
                             ('mesh chord target', f'{args.tolerance:g} mm')),
                    callouts=labels)
                times.append((f'frame_{i}_png_svg_labels_write', time.perf_counter()-start))
    require(pin_files() == pins, 'product/caller source changed during operation')
    require(binary_pins(args) == executable_pins, 'selected product binaries changed during operation')
    tsv(output/'sources.tsv', ('path', 'sha256'), pins)
    tsv(output/'binaries.tsv', ('role', 'sha256'), executable_pins)
    tsv(output/'metadata.tsv', ('field', 'value'), (
        ('source_sha256', source.source_sha256), ('source_revision', 'OM10 MAN VA01.3'),
        ('owned_source_snapshot_sha256', hashlib.sha256(binary_source.read_bytes()).hexdigest()),
        ('mode', args.mode), ('backend', args.backend), ('threads', args.threads),
        ('device', args.device if args.backend == 'cuda' else -1),
        ('source_definitions', 107), ('source_faces', 9162), ('leaf_occurrences', 166),
        ('unique_mesh_vertices', len(mesh.vertices)), ('unique_uv_cells', len(mesh.triangles)),
        ('placed_rendered_triangles', placed_triangles), ('frames', len(requested_views)),
        ('tolerance_mm', args.tolerance), ('source_residual_limit_mm', args.source_residual),
        ('foreign_preparation', 'CPU STEP/XDE; original pcurves absent and reconstructed'),
        ('owned_mesh', 'CPU shared-edge/trim-domain construction'),
        ('native_gpu_work', 'original-UV first jets; rigid placement; tile bins; visibility; shading; resolve'
         if args.backend == 'cuda' else 'none; CPU-only native consumer'),
        ('explosion', 'authored world offsets over actual source hierarchy; illustrative'),
        ('palette', 'authored illustrative; original styles retained separately'),
        ('independent_qualification_in_wall', args.qualify),
        ('caller_scope', ('fresh STEP' if step else 'owned source snapshot')+
         ' through PNG and labeled SVG; includes source checks, process exit and file writes; excludes static SVG rasterization'),
        ('derived_assets', 'Source OpenMovement; CC BY-SA 3.0 technical documentation; preserve attribution/terms')))
    # The caller row directly encloses all useful output above. Its own small
    # timing-file write and interpreter shutdown are measured by the outer driver.
    times.append(('caller_through_complete_images_and_metadata', time.perf_counter()-begin))
    tsv(output/'timings.tsv', ('stage', 'seconds', 'scope'),
        ((stage, seconds, 'completed host wall; nested stages not additive') for stage, seconds in times))
    print(f'{args.mode}: 107 solids / 9,162 faces / 166 occurrences; {placed_triangles:,} placed triangles', flush=True)
    return output


def parser():
    p = argparse.ArgumentParser(description=__doc__)
    source = p.add_mutually_exclusive_group(required=True)
    source.add_argument('--step', type=Path)
    source.add_argument('--source', type=Path)
    p.add_argument('--adapter', type=Path, default=ROOT/'build/om10-source/brepgpu-step-source')
    p.add_argument('--native', type=Path, default=ROOT/'build/om10-release/brepgpu-om10-native')
    p.add_argument('--plugin', type=Path, default=ROOT/'build/om10-release/libbrepgpu_om10_cuda.so')
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--mode', choices=('prepare', 'evaluate', 'render'), default='prepare')
    p.add_argument('--backend', choices=('cpu', 'cuda'), default='cpu')
    p.add_argument('--threads', type=int, default=24)
    p.add_argument('--device', type=int, choices=(0, 1), default=0)
    p.add_argument('--view', choices=('assembled', 'exploded', 'both'), default='both')
    p.add_argument('--width', type=int, default=2400)
    p.add_argument('--height', type=int, default=2000)
    p.add_argument('--supersample', type=int, choices=(1, 2), default=2)
    p.add_argument('--azimuth', type=float, default=-58.)
    p.add_argument('--elevation', type=float, default=52.)
    p.add_argument('--tolerance', type=float, default=.005)
    p.add_argument('--source-residual', type=float, default=.001)
    p.add_argument('--max-refinement', type=int, default=20)
    p.add_argument('--max-edge-samples', type=int, default=16384)
    p.add_argument('--repeat-frames', type=int, default=1)
    p.add_argument('--qualify', action='store_true')
    return p


if __name__ == '__main__':
    args = parser().parse_args()
    require(args.repeat_frames > 0, 'repeat-frames must be positive')
    main(args)
