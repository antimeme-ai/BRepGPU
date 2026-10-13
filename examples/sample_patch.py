#!/usr/bin/env python3
"""Sample an untrimmed cylinder or polynomial carrier in one native batch.

Writes points and a projected grid; neither output is a chord-certified mesh.
"""
import argparse
import csv
from array import array
import math
from pathlib import Path
import time

from brepgpu import Engine, Patch


def main():
    parser = argparse.ArgumentParser(__doc__)
    parser.add_argument("--backend", choices=("cpu", "cuda"), default="cpu")
    parser.add_argument("--shape", choices=("cylinder", "polynomial"), default="cylinder")
    parser.add_argument("--device", type=int, default=0)
    parser.add_argument("--resolution", type=int, default=65)
    parser.add_argument("--output-directory", type=Path, required=True)
    args = parser.parse_args()
    if not 2 <= args.resolution <= 2049:
        parser.error("resolution must be between 2 and 2049")
    # Rational quadratic circle (1-s²,2s)/(1+s²), extruded in t.
    controls = array("d")
    if args.shape == "cylinder":
        degrees = (2, 1)
        for x, y, weight in ((1., 0., 1.), (1., 1., 1.), (0., 2., 2.)):
            for t in (0., 1.):
                controls.extend((x, y, t * weight, weight))
    else:
        degrees = (3, 3)
        coordinates = (0., .25, .75, 1.)
        for i in range(4):
            for j in range(4):
                controls.extend((coordinates[i], coordinates[j],
                                 .5 if 0 < i < 3 and 0 < j < 3 else 0., 1.))
    n = args.resolution
    count = n * n
    uv = array("d", (value for i in range(n) for j in range(n)
                     for value in (i / (n - 1), j / (n - 1))))
    start = time.perf_counter()
    with Engine(backend=args.backend, device=args.device) as engine:
        with engine.create_patch_set([Patch(*degrees, controls)]) as patches:
            with engine.create_query_set(patches, patch_slots=array("I", [0]) * count, uv=uv) as queries:
                with engine.evaluate(queries, order=1) as result:
                    jets = result.download()
    jets.require_ok()
    if not all(math.isfinite(value) for value in jets.data):
        raise RuntimeError("nonfinite successful carrier output")
    wall_ms = 1000 * (time.perf_counter() - start)
    points = [tuple(jets.data[axis * count + i] for axis in range(3)) for i in range(count)]
    max_locus_error = max(abs(x*x + y*y - 1.) for x, y, z in points) if args.shape == "cylinder" else 0.
    max_position_error = 0.
    max_derivative_error = 0.
    for k, (x, y, z) in enumerate(points):
        s, t = uv[2*k:2*k+2]
        if args.shape == "cylinder":
            truth = ((1-s*s)/(1+s*s), 2*s/(1+s*s), t)
            d = 1 + s*s
            derivative_truth = ((-4*s/(d*d), 2*(1-s*s)/(d*d), 0.), (0., 0., 1.))
        else:
            truth = (.75*s + .75*s*s - .5*s*s*s, .75*t + .75*t*t - .5*t*t*t,
                     4.5*s*(1-s)*t*(1-t))
            derivative_truth = ((.75+1.5*s-1.5*s*s, 0., 4.5*(1-2*s)*t*(1-t)),
                                (0., .75+1.5*t-1.5*t*t, 4.5*s*(1-s)*(1-2*t)))
        max_position_error = max(max_position_error, *(abs(a-b) for a,b in zip((x,y,z),truth)))
        for term, expected in enumerate(derivative_truth, start=1):
            observed = tuple(jets.data[(term*3+axis)*count+k] for axis in range(3))
            max_derivative_error = max(max_derivative_error, *(abs(a-b) for a,b in zip(observed,expected)))
    if max_derivative_error > math.ldexp(1., -30):
        raise RuntimeError("carrier analytic derivative qualification failed")
    if max_locus_error > math.ldexp(1., -38) or max_position_error > math.ldexp(1., -40):
        raise RuntimeError("carrier analytic qualification failed")
    locus_text = f"{max_locus_error:.17g}" if args.shape == "cylinder" else "not-applicable"
    args.output_directory.mkdir(parents=True, exist_ok=True)
    with (args.output_directory / "queries.tsv").open("w", newline="") as stream:
        writer = csv.writer(stream, delimiter="\t", lineterminator="\n")
        writer.writerow(("query", "patch_slot", "s_hex", "t_hex", "status"))
        writer.writerows((k, 0, uv[2*k].hex(), uv[2*k+1].hex(), jets.statuses[k]) for k in range(count))
    with (args.output_directory / "controls.tsv").open("w", newline="") as stream:
        writer = csv.writer(stream, delimiter="\t", lineterminator="\n")
        writer.writerow(("control", "Ax_hex", "Ay_hex", "Az_hex", "W_hex"))
        writer.writerows((k, *(value.hex() for value in controls[4*k:4*k+4])) for k in range(len(controls)//4))
    (args.output_directory / "RECIPE.md").write_text(
        f"# {args.shape.capitalize()} carrier samples\n\n"
        f"Owned analytic fixture; degree {degrees}, j-fast homogeneous controls. "
        "s,t span [0,1]; output uses the local frame (origin 0, scale 1). "
        "Lineage is absent: this fixture has no imported body or face identity. "
        "queries.tsv preserves patch slot and exact binary64 parameters; "
        "controls.tsv preserves the exact homogeneous input bits.\n\n"
        f"Backend {args.backend}, device {args.device}, resolution {n}, {count} samples. "
        f"Maximum position/locus/first-derivative errors: {max_position_error:.17g}, "
        f"{locus_text}, {max_derivative_error:.17g}. "
        "The outputs are points and a parameter grid, without trimming or a chord tolerance claim.\n")
    with (args.output_directory / f"{args.shape}-points.ply").open("w") as stream:
        stream.write(f"ply\nformat ascii 1.0\ncomment untrimmed carrier point samples\nelement vertex {count}\n")
        stream.write("property double x\nproperty double y\nproperty double z\nend_header\n")
        for point in points:
            stream.write(" ".join(format(value, ".17g") for value in point) + "\n")
    def project(point):
        x, y, z = point
        return 125 + 380*x - 100*y, 565 - 340*z - 110*y - 30*x
    # Show at most 33 parameter grid lines per direction.
    line_count = min(33, n)
    indices = sorted({round(k * (n-1) / (line_count-1)) for k in range(line_count)})
    with (args.output_directory / f"{args.shape}-grid.svg").open("w") as stream:
        stream.write('<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 650 650">\n')
        stream.write('<rect width="650" height="650" fill="#101923"/>\n')
        stream.write('<text x="35" y="45" fill="#edf4fc" font-family="sans-serif" font-size="24">BRepGPU · rational patch sampler</text>\n')
        stream.write('<text x="35" y="75" fill="#a9bed0" font-family="sans-serif" font-size="15">Untrimmed carrier · points and first derivatives</text>\n')
        for axis, color in ((0, "#58c9e7"), (1, "#8de5b7")):
            for i in indices:
                path = [project(points[i*n+j if axis==0 else j*n+i]) for j in range(n)]
                commands = " ".join(("M" if j==0 else "L")+f"{x:.3f},{y:.3f}" for j,(x,y) in enumerate(path))
                stream.write(f'<path d="{commands}" fill="none" stroke="{color}" stroke-width="1" opacity=".8"/>\n')
        stream.write('<text x="35" y="620" fill="#a9bed0" font-family="sans-serif" font-size="14">Sampled parameter grid; no trimming or tessellation tolerance claim.</text>\n</svg>\n')
    print(f"shape={args.shape} backend={args.backend} device={args.device} points={count} host_wall_ms={wall_ms:.6f} "
          f"max_position_error={max_position_error:.17g} max_locus_error={locus_text} "
          f"max_derivative_error={max_derivative_error:.17g}")


if __name__ == "__main__":
    main()
