# Sphere tessellation chord-tolerance witnesses

USD BRep / SMLib 1.0.0 returns successful tessellations of a radius-10 sphere
whose emitted triangles contain points farther from the analytic surface than
the requested absolute chord-height tolerance. This finding concerns these
fixtures and settings; it does not identify the underlying implementation cause.

The [v1.0.0 Python binding contract](https://github.com/NVIDIA-Omniverse/usd-brep/blob/v1.0.0/source/SmPyLib/src/SmPyTessellation.cpp)
describes the chord-height parameter as a maximum distance in model units and
allows angular criteria to be disabled. The benchmark uses
`curve_angle_tolerance_deg=0` and `surface_angle_tolerance_deg=0`.
The runtime reports a mesh, no failures and manifold output.

## Reproduce

Install the hash-pinned runtime using the [README](../README.md), then run:

```sh
CUDA_VISIBLE_DEVICES= .venv/bin/python benchmarks/cpu/benchmark.py \
  --cases tess_sphere_0.2,tess_sphere_0.05,tess_sphere_0.01 \
  --output results/local-sphere-witnesses
```

Exit status 1 is expected for the recorded release. The driver preserves sample
records and a witness for each failed case.

| Requested chord height | Witness distance to sphere | Coordinates | Barycentric data |
| ---: | ---: | --- | --- |
| 0.2 | 0.220274972648955 | [triangle and point](../results/cpu-2026-10-13/tess_sphere_0.2/mesh-witness.tsv) | [witness info](../results/cpu-2026-10-13/tess_sphere_0.2/mesh-witness-info.tsv) |
| 0.05 | 0.057393185307909 | [triangle and point](../results/cpu-2026-10-13/tess_sphere_0.05/mesh-witness.tsv) | [witness info](../results/cpu-2026-10-13/tess_sphere_0.05/mesh-witness-info.tsv) |
| 0.01 | 0.014526118389071 | [triangle and point](../results/cpu-2026-10-13/tess_sphere_0.01/mesh-witness.tsv) | [witness info](../results/cpu-2026-10-13/tess_sphere_0.01/mesh-witness-info.tsv) |

## Independent check

For each recorded triangle `(a,b,c)`, the retained point `p` is expressed by
nonnegative barycentric coordinates summing to one:

```text
p = λa a + λb b + λc c
distance(p, radius-10 sphere) = abs(norm(p) - 10)
```

The driver considers the origin's projection onto the triangle plane only when
it lies inside the triangle. It also computes clamped projections onto all three
edges, selecting the closest point on the actual triangle. This avoids mistaking
a point on an infinite plane for a point on the emitted mesh.

The acceptance rule allows `1e-5` model units of numerical slack beyond the
requested tolerance. Every retained witness exceeds that slack. A single point
on an emitted triangle outside the distance bound suffices to disqualify this
maximum-distance contract; these first witnesses do not establish the global
maximum error of any mesh.

The sample failed before later mesh-incidence and volume checks completed.
The successful kernel manifold report therefore does not independently qualify
the mesh. [CPU methods](CPU_BENCHMARK.md) ·
[Exact driver and runtime provenance](../results/cpu-2026-10-13/environment.tsv).

No upstream issue has been filed by this project yet. The reproduction and
witnesses are public for independent confirmation.
