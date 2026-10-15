# Watch-inspired operation-chain visual study

2026-10-15. Scope: a concrete, distinctive caller for actual profile → extrusion
→ interior circular through-cut → mesh composition. This study selects visual
organization, not a functioning watch design or an imported historical caliber.

## Primary sources read

1. Archie B. Perkins, “The Different Units of a Watch,” *Horological Times*,
   August 1998, printed pp.22–23 (PDF pp.24–25), American Watchmakers-Clockmakers
   Institute. [Original PDF](https://www.awci.com/wp-content/uploads/ht/1998/1998-08-web.pdf).
   Acquired unchanged at ignored `papers/pdfs/awci-1998-08-watch-units.pdf`.
   Read the unit inventory and the following fusee discussion. The useful
   distinction is the separate power source, transmission, escapement,
   regulation and supporting framework: an attractive pile of wheels alone
   would miss the recognizable organization of a movement.
2. Orient Watch USA, [“How Does a Mechanical Watch Work?”](https://orientwatch.freshdesk.com/support/solutions/articles/9000213531-how-does-a-mechanical-watch-work-),
   modified 2022-03-29, read 2026-10-15. The maker describes the path from winding
   and mainspring through the train, escapement and pallet fork to the balance.
   This supports distinct recognizable barrel, train and balance regions rather
   than arbitrary concentric circles. Its simplified explanation is not a
   dimensional, tooth-form or kinematic design specification.
3. Vacheron Constantin, [Overseas tourbillon skeleton 6000V/210T-B935](https://www.vacheron-constantin.com/jp/en/collections/overseas/6000v-210t-b935.html),
   maker product description and caliber inventory, read 2026-10-15. Caliber
   2160 SQ exposes the movement with a dark structure and precious-metal
   peripheral detail; the maker lists a 31 mm diameter and 5.65 mm thickness.
   We take the open visual hierarchy and material contrast as inspiration,
   not its geometry, dimensions, branding, accuracy or part count.
4. Vacheron Constantin, [historical timeline](https://www.vacheron-constantin.com/ch/en/maison/timeline.html),
   “1994 – Skeletonised mechanisms,” read 2026-10-15. Openworking exposes a
   movement by removing material from structural components. Our pierced
   convex components are a bounded first modeling exercise; they do not
   reproduce the compound freeform bridges shown by the maker.

No source text, image or CAD data is incorporated into the modeled geometry.
The dimensions and layout will be explicitly owned illustrative choices.

## Existing implementation read

`examples/ncsx_display.py` already owns orthographic camera projection, lighting,
barycentric interpolation, opaque depth buffering and PNG encoding without a
render dependency. It currently consumes contiguous sampled patch tiles;
watch display needs actual indexed mesh triangles instead. Its lighting is
intentionally two-sided for surfaces. A solid-mesh display should use oriented
facet normals and depth-tested actual triangle geometry. SVG remains a suitable
owned wrapper for typography, leaders and presentation background.

`examples/mesh_body.py` reads actual rounded shared vertex arrays and triangle
indices, derives geometric facet normals, and exports PLY. Existing cut and
extrusion callers serialize source/result topology and typed ancestry. Reuse
those data conventions; do not reconstruct a prettier disconnected display
solid or fill holes with a decorative replacement surface.

`python/brepgpu/extrusion.py` accepts only strictly convex CCW lattice outlines
with 3–32 vertices, local coordinates in ±3/4 on the 1/256 grid, and centered
heights in [1/4,2]. Frame supplies translation and power-of-two uniform scale
in [1/4,4]. Thus a whole nonconvex toothed wheel or a skeleton bridge cannot be
silently passed off as one supported profile. A rounded 16/24/32-sided outline
with one circular hole is directly useful. A long thin bridge can use a
rectangle or bevelled convex polygon. Minimum thickness and uniform scaling
favor an explicitly exploded, sculptural movement study over claims of a
manufacturable ultra-thin watch.

Quarantine search found an OpenNURBS `v4_Gear.3dm` example and STEP kinematics
gear schema records, but no acquired watch movement construction reference.
Those files neither select a dependency nor provide an implementation oracle
for a functioning watch. No reference code is copied.

## Proposed visual target: openwork movement study 01

Build a schematic hand-wound movement, viewed from above at an oblique angle,
with a slightly separated upper structure revealing the train. Use a dark
blue/anthracite outer rim, warm gold/brass wheel bodies, silver bridge pieces,
and small ruby-red bearing-like parts. Give it an original asymmetric layout:
large barrel at upper left, a descending train toward the right, and a large
open balance assembly at lower left. Keep the central region airy rather than
covering the actual holes with a solid back plate.

Every displayed part is a named actual closed mesh of an owned extrusion or
the new extrusion-through-cut result. A published TSV names its role, input
profile namespace, extrusion namespace, optional cut namespace, mesh size,
placement, material, and history bindings. The assembly retains separate parts;
it is not an assembly Boolean union or interference-qualified mechanism.

The main delivered operation is visible in several different silhouettes:

- A beveled polygonal structural ring, convex before its central circular cut.
- A larger polygonal barrel-like ring and smaller pierced wheel-like bodies.
- One elongated pierced bridge or bearing support with a visibly off-axis hole,
  if that location belongs to the admitted chain contract.
- Several differently scaled rings forming a balance-like assembly.

Optional detail consists of separately modeled convex bars, posts, bolt-like
heads, and radial tooth-like lugs. These are actual extruded source profiles,
not arbitrary triangles invented by the display. If radial lugs are used, name
them schematic separate assembly pieces; do not claim a generated nonconvex
gear, involute/cycloidal tooth geometry, meshing contact, gear ratio or motion.
Avoid a decorative spiral unless it too consists of disclosed modeled pieces;
a freeform spring/sweep would be a different implementation unit.

## Deliverables and direct checks

1. Hero image: large oblique whole movement with depth, holes and material
   contrast; concise caption states CPU geometry/mesh/render and schematic
   nonfunctional assembly. No GPU rendering claim.
2. Companion image: near-top view or separated layers, exposing the actual
   cut walls and geometric relationships. A small inset may identify one
   profile → extrusion → cut → mesh chain from its actual tables.
3. Actual per-part PLY, profile and history tables; aggregate part manifest,
   triangle mapping and camera/material records. Read the saved display image.
4. Measure construction, source/history consumption, mesh and CPU rendering
   separately. No speedup claim from one assembly or a new camera.

The chain owner's geometry/material/mesh tests establish the admitted family.
The scene caller checks its actual part/history bindings and qualifies each
distinct produced mesh through the appropriate direct independent oracle.
Display checks cover valid indices, finite positions/normals, positive triangle
areas, all actual part counts and complete scene triangle inventory. They do
not certify product geometry by pixels. No screenshot golden, renderer
mutation campaign, test-for-tests framework or extra certificate layer.

## Admission details needed from chain owner

Confirm public API, exact cutter-center/radius ranges, retention of actual
ExtrusionResult, qualifier entry point and complete history reader. A wheel
requires a small bore; a rim/balance requires a larger still-interior bore.
If only a narrow radius is admitted, vary surrounding convex outlines and
uniform frame scales within that contract rather than expanding the kernel
for visual convenience. Scene creation waits for reviewed design and plan.
