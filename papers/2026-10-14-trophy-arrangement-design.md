# Trophy candidate: suspended perforated lattice

Design and implementation plan for root review before code. This is artwork
rendered from existing exact arrangement results, with declared display offsets;
no new geometry-kernel behavior, dependencies or fused-solid representation.

## Image and actual geometry

Create a large oblique view of a 16-by-16 rectangular lattice, pierced by 128
exact circle regions. A deterministic weave of original cell regions lifts to
several display levels, leaving negative space. The corresponding source disk
regions hover higher as a loose constellation. Warm disk colors contrast with
cool perforated cells, with thin pale original curve edges and selected twin
connections. The result should read as one intricate suspended sculpture,
with enough scale and empty space to see the holes and separate layers.

One real target top face is an owned box with frame scale4 and origin(0,0,-2):
its exact source square is[-3,3]² at z=0. Actual arrange_face cuts are i/16 for
both axes, i=1..15. Select128 cells by(i+j)%2==0 on integer cell coordinates. Cell rendering
heights repeat(0,0,0,1/3,2/3,1,2/3,1/3) indexed by(i+2j)%8; disk
heights are2+((3i+j)%5)/8 world display units. Each selected cell gets one independently created
cylindrical wall and an actual target/wall intersection: radius1/16 or1/8 (wall Frame scale1/4 or1/2) and
center at the source cell center with a small alternating binary-rational offset
of at most1/32 world unit. The minimum remaining source-cell margin is positive;
all circles are strictly interior, disjoint and within current API limits.
No clipped, tangent or overlapping curves are requested.

Expected complete arrangement: 417 vertices, 672 edges, 1,344 opposed uses,
513 loops, 385 regions including exterior, 128 closed circle edges preserving
512 original pieces and source events. These counts must be derived from the
actual declared cut/circle recipe and independently qualified, not used as a
substitute for geometry truth.

All256 cell regions and128 disk regions appear. Each region is assigned one
rendering height by a deterministic integer rule. Its entire outer loop and
holes receive the same offset; every hole retains the actual clockwise use word.
The source domain and its area remain unchanged. No extrusion sidewalls, solid
thickness or invented surface joins are drawn. Fine vertical leader lines may
show the display displacement from original source witnesses. A small, clear
caption states that heights are rendering offsets and that this is a planar
arrangement sculpture, not a fused solid or certified trimmed mesh.

## Existing API and meaningful qualification

Own `examples/trophy_arrangement.py`, importing existing Frame/make_body,
FaceRef/arrange_face and independent qualify_arrangement/qualify_locations.
Reuse `examples/arrange_face.py` source_tables/export helpers rather than invent
another serialization of original geometry. Construction and full qualification
run only in the owned Linux fleet worktree after root coordinates CPU execution.

Bind result.face and every retained section to the actual submitted objects and
cuts. Qualify the complete result and meaningful closed lookup queries: every
region witness, every graph vertex, all circle cardinal points, selected original
boundary/grid crossings and exterior points. Export the actual submitted cuts,
source records, all graph tables and exact query/results using the existing
serializer. Do not replace source controls with idealized decorative geometry.

Render loop paths by following actual next/previous/use orientation and calling
edge_uv/edge_point for every segment. Lines use endpoints; circles use exact
rational quarter-reader samples at a declared display resolution. Cross-check
sampled world positions against original target controls. Topological detail uses
actual twin ownership and source events; both UV maps remain in the source export.
Visibility/painter order and color are display choices. Geometry is never inferred
from a mesh, SVG pixels, Euler totals or CPU/GPU parity.

## Implementation and review sequence

1. After design approval, write the standalone owned caller with deterministic
   recipe, exact source binding, whole-result qualification, original exports,
   rendering-height table and SVG renderer. Guard product execution to Linux;
   source text can be edited and AST/whitespace checked locally.
2. Root reviews source and coordinates one CPU fleet construction/qualification
   and render. Save full output under
   `results/trophies-2026-10-14/arrangement/`, including input/height metadata,
   original source tables, counts, qualified geometry queries and source pins.
3. Retrieve artifacts. Independently review serialized source/region/loop geometry
   and SVG paths against actual offsets and projection. Inspect an actual browser
   render/PNG via root; revise only display choices if required for legibility.
4. Save final `artwork.svg` and a concise actual-result `REPORT.md` in that result
   directory and copy the reviewed SVG to `assets/trophies/arrangement.svg`.
   Root owns raster conversion, Git operations and publication.

Root reviewed the design and corrected the original invalid scale12 proposal:
Frame admits power-of-two scales1/4..4. The corrected source recipe stays within
that unchanged contract. Implementation is now authorized; root source review
precedes fleet execution. No kernel-admission change is needed.
