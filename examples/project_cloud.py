#!/usr/bin/env python3
"""Refine an off-surface cloud in one native batch and export checked candidates."""
import argparse
from array import array
import csv
from fractions import Fraction
import math
from pathlib import Path
import sys
import time

from brepgpu import Engine, Patch, ProjectionOptions
from brepgpu.fixtures import rational_cylinder
from brepgpu.projection_fixtures import ProjectionCase, parabolic, qualify_row


def wall_ms(begin):
    return 1000 * (time.perf_counter() - begin)


def point(shape, s, t):
    if shape == "cylinder":
        d = 1 + s*s
        return ((1-s*s)/d, 2*s/d, t)
    a = 2*s-1
    return (a, t, a*a)


def main():
    parser = argparse.ArgumentParser(__doc__)
    parser.add_argument("--backend", choices=("cpu", "cuda"), default="cpu")
    parser.add_argument("--device", type=int, default=0)
    parser.add_argument("--threads", type=int, default=1)
    parser.add_argument("--shape", choices=("cylinder", "polynomial"), default="cylinder")
    parser.add_argument("--count", type=int, default=1024)
    parser.add_argument("--output-directory", type=Path, required=True)
    args = parser.parse_args()
    if not 1 <= args.count <= 1048576:
        parser.error("count must be between 1 and 1048576")
    phases = []
    begin = time.perf_counter()
    fixture = rational_cylinder() if args.shape == "cylinder" else parabolic()
    cases, targets, seeds = [], array("d"), array("d")
    slots = array("I", [0]) * args.count
    for i in range(args.count):
        t = (1 + (i*7) % 15) / 16
        if args.shape == "cylinder":
            s, lam = (1 + i % 15) / 16, 1 + (i % 4) / 8
            target = (lam*(1-s*s), 2*lam*s, t)
            radius = Fraction(lam) * (1 + Fraction(s)**2)
            distance = (radius-1)**2
        else:
            a, lam = (-.5, -.25, .25, .5)[i % 4], 1/16
            s, target = (a+1)/2, (a-2*a*lam, t, a*a+lam)
            distance = Fraction(lam)**2 * (4*Fraction(a)**2+1)
        seed = (1.0 if i % 2 else 0.0, 0.0 if i % 3 else 1.0)
        cases.append(ProjectionCase(f"cloud_{i}", fixture, target, seed,
                                   answer=(s, t), distance_answer=distance))
        targets.extend(target)
        seeds.extend(seed)
    patch = Patch(fixture.degree_u, fixture.degree_v, fixture.controls)
    options = ProjectionOptions()
    phases.append(("input_preparation", wall_ms(begin)))
    begin = time.perf_counter()
    engine = Engine(backend=args.backend, device=args.device, threads=args.threads)
    phases.append(("context_first_use", wall_ms(begin)))
    patches = queries = result = None
    try:
        begin = time.perf_counter()
        patches = engine.create_patch_set([patch])
        phases.append(("patch_pack_snapshot_upload", wall_ms(begin)))
        begin = time.perf_counter()
        queries = engine.create_projection_query_set(patches, patch_slots=slots, targets=targets, seeds=seeds)
        phases.append(("query_pack_snapshot_upload", wall_ms(begin)))
        begin = time.perf_counter()
        result = engine.project(queries, options=options)
        phases.append(("resident_project", wall_ms(begin)))
        begin = time.perf_counter()
        batch = result.download()
        phases.append(("download_allocate_and_host_provenance", wall_ms(begin)))
    finally:
        begin = time.perf_counter()
        primary = sys.exc_info()[1]
        cleanup_errors = []
        for resource in (result, queries, patches, engine):
            if resource is not None:
                try:
                    resource.close()
                except BaseException as error:
                    cleanup_errors.append(error)
        phases.append(("native_cleanup", wall_ms(begin)))
        if cleanup_errors:
            if primary is not None:
                for error in cleanup_errors:
                    primary.add_note(f"Resource cleanup also failed: {error}")
            else:
                for error in cleanup_errors[1:]:
                    cleanup_errors[0].add_note(f"Further cleanup also failed: {error}")
                raise cleanup_errors[0]
    begin = time.perf_counter()
    batch.require_stationary()
    if not all(math.isfinite(x) for x in batch.data):
        raise RuntimeError("nonfinite stationary output")
    maxima = {}
    for i, case in enumerate(cases):
        values = tuple(batch.data[k*args.count+i] for k in range(7))
        failures, metrics = qualify_row(case, values, batch.statuses[i], batch.iterations[i],
                                        batch.evaluation_counts[i], batch.flags[i])
        if failures:
            raise RuntimeError(f"cloud query {i} failed exact qualification: {failures}")
        for name, value in metrics.items():
            maxima[name] = max(maxima.get(name, 0.), float(value))
    phases.append(("independent_exact_qualification", wall_ms(begin)))
    args.output_directory.mkdir(parents=True, exist_ok=True)
    begin = time.perf_counter()
    with (args.output_directory / "records.tsv").open("w", newline="") as stream:
        writer = csv.writer(stream, delimiter="\t", lineterminator="\n")
        writer.writerow(("query", "patch_slot", "target_x_hex", "target_y_hex", "target_z_hex",
                         "seed_s_hex", "seed_t_hex", "candidate_s_hex", "candidate_t_hex",
                         "candidate_x_hex", "candidate_y_hex", "candidate_z_hex", "distance_squared_hex",
                         "rho_hex", "status", "accepted_updates", "evaluations", "flags"))
        for i in range(args.count):
            writer.writerow((i, slots[i], *(x.hex() for x in targets[3*i:3*i+3]),
                             *(x.hex() for x in seeds[2*i:2*i+2]),
                             *(batch.data[k*args.count+i].hex() for k in range(7)),
                             batch.statuses[i], batch.iterations[i], batch.evaluation_counts[i], batch.flags[i]))
    with (args.output_directory / "controls.tsv").open("w", newline="") as stream:
        writer = csv.writer(stream, delimiter="\t", lineterminator="\n")
        writer.writerow(("control", "Ax_hex", "Ay_hex", "Az_hex", "W_hex"))
        writer.writerows((i, *(x.hex() for x in fixture.controls[4*i:4*i+4]))
                         for i in range(len(fixture.controls)//4))
    phases.append(("record_export", wall_ms(begin)))
    begin = time.perf_counter()
    def view(p):
        x, y, z = p
        return (420+155*x-90*y, 510-270*z-60*y-35*x)
    with (args.output_directory / f"{args.shape}-projection.svg").open("w") as stream:
        stream.write('<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 900 650">\n')
        stream.write('<rect width="900" height="650" fill="#101923"/>\n')
        stream.write(f'<text x="35" y="46" fill="#edf4fc" font-family="sans-serif" font-size="25">BRepGPU · {args.shape} point projection</text>\n')
        stream.write('<text x="35" y="76" fill="#a9bed0" font-family="sans-serif" font-size="15">Off-surface targets → seeded stationary candidates · FP64</text>\n')
        for axis in (0, 1):
            for k in range(17):
                grid = [view(point(args.shape, k/16 if axis == 0 else j/64, j/64 if axis == 0 else k/16)) for j in range(65)]
                commands = " ".join(("M" if j == 0 else "L")+f"{x:.3f},{y:.3f}" for j, (x, y) in enumerate(grid))
                stream.write(f'<path d="{commands}" fill="none" stroke="#386079" stroke-width=".7" opacity=".65"/>\n')
        for i in range(min(args.count, 180)):
            start = view(tuple(targets[3*i:3*i+3]))
            end = view(tuple(batch.data[(axis+2)*args.count+i] for axis in range(3)))
            stream.write(f'<path d="M{start[0]:.3f},{start[1]:.3f} L{end[0]:.3f},{end[1]:.3f}" stroke="#58c9e7" stroke-width="1" opacity=".55"/>\n')
            stream.write(f'<circle cx="{start[0]:.3f}" cy="{start[1]:.3f}" r="2.3" fill="#ffb17a"/>\n')
            stream.write(f'<circle cx="{end[0]:.3f}" cy="{end[1]:.3f}" r="2" fill="#8de5b7"/>\n')
        stream.write('<text x="35" y="597" fill="#ffb17a" font-family="sans-serif" font-size="15">● input target</text>\n')
        stream.write('<text x="210" y="597" fill="#8de5b7" font-family="sans-serif" font-size="15">● qualified candidate</text>\n')
        stream.write('<text x="35" y="626" fill="#a9bed0" font-family="sans-serif" font-size="13">Synthetic untrimmed carrier; no general nearest-point or topology guarantee.</text>\n</svg>\n')
    phases.append(("visual_export", wall_ms(begin)))
    with (args.output_directory / "timings.tsv").open("w", newline="") as stream:
        writer = csv.writer(stream, delimiter="\t", lineterminator="\n")
        writer.writerow(("backend", "device", "threads", "count", "shape", "phase", "ms"))
        writer.writerows((args.backend, args.device, args.threads, args.count, args.shape, phase, ms) for phase, ms in phases)
    (args.output_directory / "RECIPE.md").write_text(
        f"# {args.shape.capitalize()} seeded projection\n\n"
        f"Backend {args.backend}, device {args.device}, {args.threads} requested CPU workers, {args.count} requests. "
        f"Owned degree ({fixture.degree_u},{fixture.degree_v}) carrier with local origin 0, scale 1, unit-square parameters. "
        "No imported body/face lineage. records.tsv preserves exact binary64 targets, seeds and returned candidate fields; "
        "controls.tsv preserves homogeneous controls. The SVG shows at most 180 target-to-candidate segments over an analytic carrier grid.\n\n"
        f"All rows met stationary status and independent exact geometry/known-solution checks. Maximum observed errors: {maxima}. "
        "The named target family has independently known nearest solutions; the operation itself promises seeded first-order stationarity.\n\n"
        f"Options: updates {options.max_iterations}, backtracks {options.max_backtracks}, tolerance {options.gradient_tolerance.hex()}. "
        "timings.tsv separates preparation, first use, native snapshots/uploads, resident solve, allocating download with host provenance, "
        "cleanup, exact qualification and exports. Device result payload is 72 bytes/request; retained-source materialization adds "
        "44 host bytes/request plus descriptors/options and Python wrappers. These single caller samples are separate from native medians. "
        "Source CAD export/import, global search, trims/topology and automatic CPU fallback are absent.\n")
    print(f"shape={args.shape} backend={args.backend} device={args.device} count={args.count} stationary={args.count} "
          f"project_ms={dict(phases)['resident_project']:.6f} download_provenance_ms={dict(phases)['download_allocate_and_host_provenance']:.6f}")


if __name__ == "__main__":
    main()
