#!/usr/bin/env python3
"""Consume actual packed topology adjacency; run native work on the Linux fleet."""
import argparse
import csv
import hashlib
from html import escape
from pathlib import Path
import random
import struct
import sys
import time

from brepgpu.body import make_body
from brepgpu.adjacency import AdjacencyLibrary, pack_bodies
from brepgpu.adjacency_oracles import qualify_result

ROOT = Path(__file__).resolve().parents[1]
SOURCE_NAMES = (
    "CMakeLists.txt", "include/brepgpu.h", "include/brepgpu_topology.h", "native/topology_internal.hpp",
    "native/topology_test_hooks.h", "python/brepgpu/__init__.py",
    "native/topology.cpp", "native/topology_cuda.cu",
    "python/brepgpu/adjacency.py", "python/brepgpu/adjacency_oracles.py",
    "python/brepgpu/body.py", "python/brepgpu/body_oracles.py",
    "examples/build_adjacency.py", "benchmarks/adjacency/run.py")


def milliseconds(start):
    return 1000*(time.perf_counter()-start)


def tsv(path, header, rows):
    with path.open("w", newline="") as stream:
        writer = csv.writer(stream, delimiter="\t", lineterminator="\n")
        writer.writerow(header)
        writer.writerows(rows)


def manifest_values(library_path):
    sources = tuple((name, hashlib.sha256((ROOT/name).read_bytes()).hexdigest())
                    for name in SOURCE_NAMES)
    return sources, (("topology_library", hashlib.sha256(library_path.read_bytes()).hexdigest()),)


def prepare_case(recipe, replicas=1, order="ordered"):
    """Retain actual immutable bodies; repeats are separate batch occurrences."""
    if recipe not in ("mixed", "box", "cylinder", "plate") or replicas < 1:
        raise ValueError("unknown recipe or nonpositive replica count")
    if order not in ("ordered", "permuted"):
        raise ValueError("unknown storage order")
    phases = []
    start = time.perf_counter()
    originals = tuple(make_body(shape) for shape in
                      (("box", "cylinder", "plate") if recipe == "mixed" else (recipe,)))
    bodies = originals*replicas
    phases.append(("body_preparation_ms", milliseconds(start)))
    start = time.perf_counter()
    prepared = pack_bodies(bodies)
    phases.append(("input_packing_ms", milliseconds(start)))
    start = time.perf_counter()
    if order == "permuted":
        rng = random.Random(20261014)
        orders = []
        for sources in (prepared.edge_sources, prepared.face_sources, prepared.coedge_sources):
            permutation = list(range(len(sources)))
            rng.shuffle(permutation)
            orders.append(permutation)
        prepared = prepared.permuted(edge_order=orders[0], face_order=orders[1],
                                     coedge_order=orders[2])
    phases.append(("input_permutation_ms", milliseconds(start)))
    return prepared, bodies, phases


def consume(result):
    """Read every actual row and emitted mate/neighbor identity; no truth reconstruction.

    The sum records source consumption, not a correctness certificate. Independent
    qualify_result checks complete output membership and relationships afterward.
    """
    members = mates = neighbors = source_sum = 0
    kinds = [0]*6
    for edge_slot in range(len(result.input.edge_sources)):
        kinds[result.edge_kind(edge_slot)] += 1
        for source in result.edge_uses(edge_slot):
            members += 1
            source_sum += source.instance+source.body_id+source.revision+source.entity_id
    for slot in range(len(result.input.coedge_sources)):
        mate, neighbor = result.mate(slot), result.neighbor(slot)
        if mate is not None:
            mates += 1
            source_sum += mate.instance+mate.body_id+mate.revision+mate.entity_id
        if neighbor is not None:
            neighbors += 1
            source_sum += neighbor.instance+neighbor.body_id+neighbor.revision+neighbor.entity_id
    return {"read_edges": len(result.input.edge_sources), "read_members": members,
            "read_mates": mates, "read_neighbors": neighbors,
            "consumed_source_sum": source_sum,
            **{f"read_kind_{kind}": count for kind, count in enumerate(kinds)}}


def build_and_consume(library, prepared, *, backend, device, threads):
    start = time.perf_counter()
    result = library.build(prepared, backend=backend, device=device, threads=threads)
    build_ms = milliseconds(start)
    start = time.perf_counter()
    consumed = consume(result)
    reader_ms = milliseconds(start)
    return result, consumed, build_ms, reader_ms


def source_fields(source):
    return source.instance, source.body_id, source.revision, source.entity_id


def source_label(source, kind):
    return "none" if source is None else f"i{source.instance} · {kind}{source.entity_id}"


def binary_values(payload, code="I"):
    return tuple(row[0] for row in struct.iter_unpack("<"+code, payload))


def export(result, bodies, output):
    prepared = result.input
    for name in ("coedge_edges", "coedge_faces", "orientations"):
        (output/(name+(".i32le" if name == "orientations" else ".u32le"))).write_bytes(
            getattr(prepared, name))
    for name in ("offsets", "members", "kinds", "mates", "neighbors"):
        (output/(name+".u32le")).write_bytes(getattr(result, name))
    tsv(output/"sources.tsv", ("kind", "slot", "instance", "body_id", "revision", "entity_id"),
        ((kind, slot, *source_fields(source))
         for kind, sources in (("edge", prepared.edge_sources), ("face", prepared.face_sources),
                               ("coedge", prepared.coedge_sources))
         for slot, source in enumerate(sources)))
    edges, faces = binary_values(prepared.coedge_edges), binary_values(prepared.coedge_faces)
    signs, mates, neighbors = (binary_values(prepared.orientations, "i"),
                               binary_values(result.mates), binary_values(result.neighbors))
    original = {(instance, coedge.id): coedge for instance, body in enumerate(bodies)
                for coedge in body.coedges}
    tsv(output/"coedges.tsv", ("slot", "instance", "body_id", "revision", "coedge_id",
        "edge_slot", "face_slot", "orientation", "loop_id", "hole", "uv_kind", "uv_data",
        "mate_slot", "neighbor_slot", "mate_instance", "mate_body", "mate_revision", "mate_id",
        "neighbor_instance", "neighbor_body", "neighbor_revision", "neighbor_id"),
        ((slot, *source_fields(source), edges[slot], faces[slot], signs[slot],
          bodies[source.instance].loops[original[(source.instance, source.entity_id)].loop_slot].id,
          int(bodies[source.instance].loops[original[(source.instance, source.entity_id)].loop_slot].hole),
          original[(source.instance, source.entity_id)].uv_map.kind,
          ",".join(float(x).hex() for x in original[(source.instance, source.entity_id)].uv_map.data),
          mates[slot], neighbors[slot],
          *(source_fields(result.mate(slot)) if result.mate(slot) is not None else ("",)*4),
          *(source_fields(result.neighbor(slot)) if result.neighbor(slot) is not None else ("",)*4))
         for slot, source in enumerate(prepared.coedge_sources)))
    offsets, members, kinds = (binary_values(result.offsets), binary_values(result.members),
                               binary_values(result.kinds))
    tsv(output/"edges.tsv", ("slot", "instance", "body_id", "revision", "edge_id", "degree",
        "kind", "member_slots"),
        ((slot, *source_fields(source), offsets[slot+1]-offsets[slot], kinds[slot],
          ",".join(map(str, members[offsets[slot]:offsets[slot+1]])))
         for slot, source in enumerate(prepared.edge_sources)))
    tsv(output/"loops.tsv", ("instance", "body_id", "revision", "loop_id", "face_id", "hole",
        "ordered_original_coedge_ids"),
        ((instance, body.body_id, body.revision, loop.id, body.faces[loop.face_slot].id,
          int(loop.hole), ",".join(str(body.coedges[slot].id) for slot in loop.coedge_slots))
         for instance, body in enumerate(bodies) for loop in body.loops))
    tsv(output/"original-edges.tsv", ("instance", "body_id", "revision", "edge_id", "start_vertex_id",
        "end_vertex_id", "curve_kind", "curve_data"),
        ((instance, body.body_id, body.revision, edge.id, body.vertices[edge.start].id,
          body.vertices[edge.end].id, body.curves[edge.curve_slot].kind,
          ",".join(float(value).hex() for value in body.curves[edge.curve_slot].data))
         for instance, body in enumerate(bodies) for edge in body.edges))
    tsv(output/"faces.tsv", ("instance", "body_id", "revision", "face_id", "shape", "name", "kind",
        "orientation", "loop_ids", "chart_ids"),
        ((instance, body.body_id, body.revision, face.id, body.shape, face.name, face.kind,
          face.orientation, ",".join(str(body.loops[slot].id) for slot in face.loop_slots),
          ",".join(str(body.charts[slot].id) for slot in face.chart_slots))
         for instance, body in enumerate(bodies) for face in body.faces))


def render(result, bodies, output, backend, device):
    """Draw actual CSR rows and reader outputs; placement is diagrammatic."""
    offsets, members, kinds = (binary_values(result.offsets), binary_values(result.members),
                               binary_values(result.kinds))
    count = len(result.input.edge_sources)
    height = 158+84*count
    parts = [f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1120 {height}">',
        f'<rect width="1120" height="{height}" fill="#101923"/>',
        '<g font-family="sans-serif">',
        '<text x="28" y="36" fill="#edf4fc" font-size="24">BRepGPU · constructed edge-use adjacency</text>',
        f'<text x="28" y="62" fill="#a9bed0" font-size="14">{count} edge rows · '
        f'{len(result.input.coedge_sources)} distinct uses · {escape(backend)} {device if backend == "cuda" else ""}</text>',
        '<text x="28" y="87" fill="#a9bed0" font-size="13">Edge → actual CSR occurrences → emitted mate/face. Cyan: two uses on the same face.</text>']
    for slot, edge in enumerate(result.input.edge_sources):
        y = 132+84*slot
        color = "#58c9e7" if kinds[slot] == 3 else "#f4c878"
        parts.append(f'<text x="28" y="{y+5}" fill="{color}" font-size="14">{escape(source_label(edge,"e"))}</text>')
        row = members[offsets[slot]:offsets[slot+1]]
        for index, cslot in enumerate(row):
            x, use_y = 240, y-20+40*index
            coedge = result.input.coedge_sources[cslot]
            neighbor, mate = result.neighbor(cslot), result.mate(cslot)
            label = f'{source_label(coedge,"c")} → {source_label(mate,"c")} / {source_label(neighbor,"f")}'
            parts.extend((f'<path d="M180,{y} H210 V{use_y} H{x}" stroke="{color}" fill="none"/>',
                f'<rect x="{x}" y="{use_y-16}" width="840" height="32" rx="5" fill="#182733" stroke="{color}"/>',
                f'<text x="{x+10}" y="{use_y+5}" fill="#edf4fc" font-size="13">{escape(label)}</text>'))
    parts.extend((f'<text x="28" y="{height-21}" fill="#a9bed0" font-size="13">Immutable body sources · original loop/UV occurrences retained · incidence diagram, no geometric embedding claim</text>',
                  '</g></svg>'))
    (output/"adjacency.svg").write_text("\n".join(parts)+"\n")


def metadata(prepared, recipe, replicas, order, backend, device, threads):
    E, F, C = map(len, (prepared.edge_sources, prepared.face_sources, prepared.coedge_sources))
    return {"recipe": recipe, "replicas": replicas, "order": order, "backend": backend,
        "device": device, "threads": threads, "edges": E, "faces": F, "coedges": C,
        "instances": replicas*(3 if recipe == "mixed" else 1),
        "host_slot_sign_input_bytes": 12*C,
        "host_csr_reader_output_bytes": 8*E+12*C+4,
        "cuda_slot_sign_upload_bytes": 12*C if backend == "cuda" else 0,
        "cuda_csr_reader_download_bytes": 8*E+12*C+4 if backend == "cuda" else 0,
        "cuda_source_identity_upload_bytes": 0}


def main(args):
    output = args.output_directory
    output.mkdir(parents=True, exist_ok=False)
    whole = time.perf_counter()
    identities = manifest_values(args.library_path)
    prepared, bodies, phases = prepare_case(args.recipe, 1, args.order)
    start = time.perf_counter()
    library = AdjacencyLibrary(args.library_path)
    phases.append(("library_load_ms", milliseconds(start)))
    start = time.perf_counter()
    result, consumed, build_ms, reader_ms = build_and_consume(library, prepared,
        backend=args.backend, device=args.device, threads=args.threads)
    phases.extend((*(result.timings), ("external_build_ms", build_ms),
                   ("python_reader_ms", reader_ms), ("actual_build_reader_ms", milliseconds(start))))
    start = time.perf_counter()
    quality = qualify_result(result, literal=True)
    phases.append(("independent_qualification_ms", milliseconds(start)))
    start = time.perf_counter()
    export(result, bodies, output)
    render(result, bodies, output, args.backend, args.device)
    tsv(output/"input.tsv", ("field", "value"), metadata(prepared, args.recipe, 1, args.order,
        args.backend, args.device, args.threads).items())
    tsv(output/"quality.tsv", ("metric", "value"), quality.items())
    tsv(output/"consumed.tsv", ("metric", "value"), consumed.items())
    if identities != manifest_values(args.library_path):
        raise RuntimeError("caller source or binary changed during execution")
    tsv(output/"code.tsv", ("source", "sha256"), identities[0])
    tsv(output/"binaries.tsv", ("binary", "sha256"), identities[1])
    phases.append(("export_ms", milliseconds(start)))
    # Final timing TSV and console publication are outside this measured wall.
    phases.append(("actual_whole_caller_ms", milliseconds(whole)))
    tsv(output/"timings.tsv", ("phase", "milliseconds"), phases)
    print(f"{args.recipe} {args.order} {args.backend}:{args.device}: "
          f"{len(prepared.edge_sources)} edges, {len(prepared.coedge_sources)} uses; "
          f"{quality.get('same_face_pair', 0)} same-face pairs", flush=True)


if __name__ == "__main__":
    if sys.platform != "linux":
        raise RuntimeError("native callers run on the fleet")
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--recipe", choices=("mixed", "box", "cylinder", "plate"), default="mixed")
    parser.add_argument("--order", choices=("ordered", "permuted"), default="ordered")
    parser.add_argument("--backend", choices=("cpu", "cuda"), default="cpu")
    parser.add_argument("--device", type=int, default=0)
    parser.add_argument("--threads", type=int, default=1)
    parser.add_argument("--library-path", type=Path, required=True)
    parser.add_argument("--output-directory", type=Path, required=True)
    main(parser.parse_args())
