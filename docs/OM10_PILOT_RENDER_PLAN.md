# OM10 pilot images from the qualified scene

Root-reviewed and accepted plan. **Do not execute until root explicitly reports
the complete nonvisual source/mesh/assembly/CPU/P100 qualification PASS.** During
the active qualification run, documentation/journal and the private pilot utility
may be prepared. No actual watch rasterization, product/helper/binary edits or
rebuilds.

The pilot inspects real geometry and labels before two fresh complete
STEP-to-both-views observations. It reuses the existing private qualified display
inputs; it adds no product API, mesh persistence, cache or qualification mechanism.

## Inputs and exact permitted change

Use the completed qualification directory's actual `scene.bgmesh`,
`source.bgsource`, `views.bgview`, source/BOM/explosion/primitive tables,
`sources.tsv`, `binaries.tsv` and reported quality settings. Keep original files
unchanged. Require current production/helper/binary hashes to match that completed
run. Record the private utility's own source hash and input/output hashes.

The original BGVIEW01 is16 header bytes plus two192-byte requests,400 bytes total.
Each request stores flags at row offset168. Copy its bytes, confirm original
flags3, and change only these fields:

| Pose | File offset | Change |
|---|---|---|
|assembled|184|u32 little-endian3 →0|
|exploded|376|u32 little-endian3 →0|

All camera, scale, dimensions, supersampling, material, alpha and geometry values
stay identical. Flags3 means PROJECT_ONLY+DUMP_PROJECTED; flags0 permits the
existing visibility/shading/image path. Do not repack floating-point fields or
recompute the camera/explosion. BGMESH01 stays byte-identical.

## Bounded pilot recipe after PASS

A small private fleet utility/common recipe will:

1. Load the retained source snapshot and use existing `assemble` only for source
   names, group membership and label identity. Reuse existing BOM/explosion/
   primitive tables; do not call `mesh_imported`, source translation or a second
   geometry constructor. Decode the two retained view rows for presentation.
2. Invoke the existing native executable once with CPU24 and once with P1000,
   each with the same original scene and patched two-view request file. Use
   distinct fresh output directories. Explicit CUDA plugin selection and normal
   failure handling remain unchanged. Native output is actual RGBA/depth/stable
   primitive/instance IDs for both poses.
3. Produce PNG and labeled SVG through existing `write_png`, `visible_anchors`,
   `render_om10.callouts` and `labeled_svg`. Keep source attribution, actual
   counts and authored palette/explosion from the qualified inputs. Validate
   anchors against the166 source occurrence IDs. Retain CPU and P100 images.
4. Record completed native-process walls and PNG/SVG composition separately.
   Caption these as **prepared qualified scene; native process for both poses;
   one observation per backend**. Per-frame retained-geometry rows may be shown
   only with their existing narrower raw-output scope. No full STEP-to-image
   time, median, speedup headline or division of a two-pose total into invented
   per-pose complete times.
5. Check unchanged production/helper/binary and original input hashes afterward.
   End immediately on a failed process or inconsistent source identity; do not
   accept stale files from an earlier attempt.

## Inspection and next step

Root inspects full-resolution assembled/exploded geometry, framing, source-linked
labels, leader readability and source attribution. Native lane can diagnose
actual CPU/P100 fidelity differences using RGBA/depth/IDs and the same poses'
existing qualified projected/source-jet diagnostics. Exact tiny renderer oracles already
establish raster rules; do not add a generic pixel-certification framework or
repeat qualification because the picture is attractive.

If outputs agree, report that directly. If they differ, identify actual affected
pixels/instances/faces and distinguish bounded source/projection/shading effects
from unexplained visibility or missing geometry. Do not silently bless unexplained
differences. Presentation changes require a named new view/layout revision and
focused affected checks; preserve the original qualified pilot inputs.

After root accepts the pilot, run the separately reviewed two fresh full callers
(CPU24 then P1000, each producing both labeled views) and matched per-pose native
timing matrix. See the [two-view measurement plan](OM10_TWO_VIEW_BENCHMARK_PLAN.md).
The pilot never substitutes reused source preparation for those directly
enclosed full-workflow observations.
