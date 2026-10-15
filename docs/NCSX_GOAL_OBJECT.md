# NCSX: our goal object

The National Compact Stellarator Experiment at Princeton Plasma Physics
Laboratory is BRepGPU's continuing modeling and rendering target. Its nested
nonplanar coils, shaped vessel, ports, winding forms and assembly connections
exercise the geometry/topology problems this kernel is being built to solve.
We will revisit one recorded scene as operations improve, keeping changes in
source geometry and presentation visible.

![NCSX goal object, study00](../assets/ncsx/goal-object.png)

## Study00: what this image contains

Eighteen modular-coil envelopes follow actual NCSX Fourier centerlines from a
pinned scientific source. Three original coil shapes expand through the source's
three-period and stellarator symmetry. Each envelope uses a declared illustrative
rectangular section and periodic cubic fitted rails. A separately sourced NCSX
**plasma boundary** supplies the cool inner surface. It is not the vacuum vessel.
The two scientific inputs are not claimed to describe a validated paired
magnetic equilibrium or a specific as-built engineering revision.

The existing native C++/CUDA patch evaluator supplies displayed positions and
first derivatives on P100. Returned derivatives supply the lighting normals.
An owned CPU display depth buffer handles the actual sampled triangles. This
is a fitted surface assembly; it does not establish general CAD import, solid
sweep/Boolean construction, complete NCSX topology or controlled-error meshing.
Native arithmetic checks and sampled Fourier fitting diagnostics have separate
meanings, retained in the actual result records. All 90,720 native first jets
passed the exact supplied-control arithmetic checks; sampled patch seams have
zero positional discrepancy. Source-fit residuals are sampled diagnostics,
not bounds on the original Fourier surfaces.

[Actual results and saved scene](../results/ncsx-2026-10-15/REPORT.md) ·
[Rendering plan](NCSX_RENDER_PLAN.md) · [Scientific inputs and original license](../data/ncsx/README.md) ·
[Source acquisition study](../papers/2026-10-15-ncsx-source-geometry-study.md) ·
[PPPL documentary reference pack](NCSX_TROPHY_BRIEF.md).

## Comparison baseline

The initial scene records source hashes/revision, all coefficient modes, coil
instance transforms, cosmetic section sizes, interpolating controls, native
query layout, camera matrix, projection, dimensions, light, materials and
component visibility. Camera settings are 35° azimuth / 25° elevation, Z-up,
with orthographic projection at 240 pixels per original coordinate unit and
screen center (900, 650) on an 1800×1200 image. The image is a source-based engineering study;
no calibrated dimension legend is inferred from files without unit declarations.

Compare later iterations against these settings. When a source dataset, geometry
fit, framing, material or display sampling changes, record it as such. A new
light/camera alone does not demonstrate a geometry-kernel improvement. Preserve
previous scene/data snapshots so differences can be inspected directly.

## Work toward the complete goal

Acquire and preserve a chosen engineering revision for the actual vessel,
windings/forms, ports and supports. Establish reference-to-owned surface/curve
conversion and trim provenance. Grow owned intersections, solid/feature
construction, shared topology and controlled-error meshes into that assembly.
Measure complete supported operations and compare outputs against independent
geometry/reference systems. Required community qualification still gates alpha.

This target gives the broad-first kernel work a consistent complex object to
return to. It does not turn missing operations into implemented capabilities.
The earlier [synthetic geometry studies](TROPHY_GALLERY.md) remain available as
historical demonstrations.

## Reproduce the first source-based scene

From the owned fleet checkout, select an existing qualified core/CUDA plugin:

```sh
PYTHONPATH=python BREPGPU_LIBRARY=build/pair-release/libbrepgpu.so \
  python3 examples/render_ncsx.py --backend cuda --device 0 \
  --output-directory results/local-ncsx/study-00
```

The caller writes original binary64 control bits, patch/instance/query-layout
records, actual raw native first jets, separate arithmetic/seam/source-fit
checks, native/source hashes, the scene TSV, transparent geometry raster and
labeled SVG. Rasterize that SVG with the existing standalone renderer:

```sh
rsvg-convert -w 1800 -h 1200 -o ncsx.png results/local-ncsx/study-00/ncsx.svg
```

No SIMSOPT installation or new runtime/rendering dependency is needed. The two
unchanged coefficient files are redistributed with upstream attribution/license;
only data and inspected conventions inform our owned caller.
