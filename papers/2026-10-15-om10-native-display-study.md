# OM10 native CPU/P100 geometry and display study

Status: research and proposed design input; no implementation or watch render.
The unit owns native source-carrier consumption, instance placement and opaque
triangle display together. STEP parsing, trim meshing, authored explosion and
labels have other owners. No third-party rendering dependency is proposed.

## Read source and literature

Read the active AGENTS/Blackbird doctrine, OM10_ASSEMBLY_DELIVERY, existing
include/brepgpu.h, native/evaluator.hpp, evaluator_cpu.cpp, cuda_backend.cu,
backend_api.h, CMakeLists and examples/watch_display.py. The existing ABI1
positive rational degree<=3 evaluator already provides batched first jets and
separate CPU/CUDA lifetimes. Its evaluator.hpp recover_channel is reusable.
The old watch display loops triangles/pixels in Python and is unsuitable as
our CPU performance baseline; its original facet and depth semantics are useful.

- Juan Pineda, *A Parallel Algorithm for Polygon Rasterization*, SIGGRAPH1988,
  DOI10.1145/54852.378457. Read the four-page paper at
  https://people.csail.mit.edu/ericchan/bib/pdf/p17-pineda.pdf . Its linear edge
  functions permit independent sample coverage and interpolation. We use owned
  integer edge functions, not that paper's implementation.
- Samuli Laine and Tero Karras, *High-Performance Software Rasterization on
  GPUs*, HPG2011, DOI10.1145/2018323.2018337. Read the pipeline/setup/bin/fine
  rasterization discussion at
  https://research.nvidia.com/sites/default/files/pubs/2011-08_High-Performance-Software-Rasterization/laine2011hpg_paper.pdf .
  The useful architectural lesson is to bin triangles before independent tile
  rasterization and to separate setup from visibility. This delivery needs
  opaque deterministic output, not graphics-API ordering or transparency. We
  therefore select maximum depth and stable primitive identity per sample;
  arbitrary bin scatter order cannot change the winner. No CudaRaster code is
  copied or adopted and no performance from that paper is predicted here.
- NVIDIA, CUDA12.8 Pascal Tuning Guide:
  https://docs.nvidia.com/cuda/archive/12.8.1/pascal-tuning-guide/index.html .
  The target is GP100/sm60; native FP64 arithmetic is useful. The design does
  not require OpenGL, RT cores, tensor cores or post-Pascal atomic features.
- Import lane inspected OCCT ElSLib source to establish original-UV analytic
  carrier conventions, particularly cone V as slant distance and independent
  X/Y/Z axes for indirect frames. These conventions are retained below.

PDF acquisition2026-10-15, intact ignored papers/pdfs originals:

| File | SHA256 | Restore |
|---|---|---|
| laine-karras-2011-software-rasterization.pdf | b41fb611a69d3fc3120c5eff02598c4bf1892bb4f93348b7505188d12f317e22 | curl -L --fail URL-above -o papers/pdfs/laine-karras-2011-software-rasterization.pdf |
| pineda-1988-polygon-rasterization.pdf | da8172cedae9212df5918dfea54501ebbbe099bf592e5ea7fca90d78af5a8b14 | curl -L --fail URL-above -o papers/pdfs/pineda-1988-polygon-rasterization.pdf |

## Native values and immutable source ownership

The mesh source corner keeps definition-local canonical XYZ, original face UV,
carrier slot, face orientation and a physical vertex key. Distinct UV/seam/face
corners can share one physical key. Boundary XYZ comes from the common spatial
edge, so separate face evaluations cannot create cracks. Native source-position
output is retained for direct residual checks against that shared boundary;
it does not overwrite a boundary. Interior XYZ is native evaluated source
position. All normals are native evaluated surface normals and actually feed
lighting after rigid placement. This makes geometry evaluation useful to the
image rather than a timed side job.

The carrier is original plane/cylinder/cone/sphere/torus or an original-domain
rational Bezier span. No analytic-to-NURBS reparameterization silently changes
source UV. With e(u)=X*cos(u)+Y*sin(u):

- plane: O+u*X+v*Y;
- cylinder: O+R*e(u)+v*Z;
- cone: O+(R+v*sin(a))*e(u)+v*cos(a)*Z;
- sphere: O+R*cos(v)*e(u)+R*sin(v)*Z;
- torus: O+(Rmajor+Rminor*cos(v))*e(u)+Rminor*sin(v)*Z.

Derivatives follow those maps. Plane/cylinder/cone/sphere/torus normals use
analytic limiting directions at sphere poles/cone apices where appropriate,
with the stored independent-axis orientation and face sign. Bezier original UV
maps affinely into its span; derivatives divide by the corresponding original
span widths. Homogeneous control order is U-major,V-minor. Reuse the owned
rational recovery helper. A genuinely singular unsupported normal is a surfaced
error with source corner ID, not a silently invented smooth normal. A separately
specified faceted fallback can only be used if an actual source fixture requires
it and review agrees its scope.

## Proposed scene binary v1

All integers little endian; f64 IEEE754. Reader verifies exact total size,
checked products/sums before allocation, every range/index/reserved bit, finite
numbers, positive rational weights, supported tags/degrees, valid original UV,
rigid nonsingular instance transforms and all stable IDs. Failure emits an
error stage/row and produces no successful image. No JSON internal persistence.
Header80 bytes:

- magic8 `BGMESH01`, u32version1,u32flags0;
- seven u64 counts: definitions, instances, vertices, triangles, carriers,
  materials, homogeneous control records;
- u64reserved0.

Sections in that order, except controls last:

| Section | Exact row and bytes |
|---|---|
| Definitions | u64(id,first_vertex,vertex_count,first_triangle,triangle_count),40 |
| Instances | u64source_id,u32definition_slot,u32reserved0,f64assembled_matrix3x4[12],f64explosion_translation[3],136 |
| Vertices | f64canonical_xyz[3],f64original_uv[2],u32carrier_slot,u32flags,u64physical_key,56 |
| Triangles | u32definition_local_indices[3],u32material_slot,u64source_face_id,24 |
| Carriers | u64source_id,u32tag,u32flags0,f64origin[3],f64axisX[3],f64axisY[3],f64axisZ[3],f64r0,f64r1,f64original_uv_bounds[4],u64control_offset,u64control_count,u32degree_u,u32degree_v,u64reserved0,192 |
| Materials | f64linearRGB[3],f64roughness,32 |
| Controls | f64homogeneousXYZW[4],32 |

Vertex flag1 retains canonical boundary XYZ; flag2 reverses face orientation.
Other bits are rejected. Carrier tags: plane1,cylinder2,cone3,sphere4,torus5,
Bezier6. r0/r1 mean radius/unused, radius/semiconeangle, or major/minor as
appropriate; unused fields must be canonical zero. Plane axes need not be
reconstructed. For Bezier, original_uv_bounds=(umin,umax,vmin,vmax), origin/axes
are zero and homogeneous controls already in definition-local coordinates.
Analytic UV bounds are finite source-admitted ranges, with periodic original
UV permitted at seams. Source carrier IDs need not equal source face IDs.

IDs belong to source identity, never triangle order. Source_instance_id is a
root-assigned stable u64 referring to the full retained occurrence path, not a
product definition ID. The outer BOM/label table retains the complete string
path. Physical keys are definition-scoped. Primitive IDs are consecutive u64
across instance order and local triangle order, with an explicit mapping back
to instance ID and source face ID; no hash collision risk.

The same carrier records can be the import package's own native carrier values;
this scene file is an owned consumer snapshot, not a replacement STEP model.
Exact final format acceptance is pending cross-lane review.

## Request and result

Standalone C++ executable reads a scene and a batch of view requests, creates
one explicit CPU or CUDA session, processes frames, closes everything, and
writes TSV stage times plus binary results. CPU-only executable links no CUDA
library; it dlopens a separate optional display CUDA plugin only on explicit
`--backend cuda --device N`. Existing ABI1 evaluator is unchanged.

Request header16: magic8 `BGVIEW01`,u32version1,u32frame_count.
Each request192 bytes: u32width,height,supersample,reserved0; f64explode_alpha;
f64camera_basis9 (right/up/toward rows); f64camera_center_xyz3;
f64pixel_center_xy2; f64pixels_per_unit; f64background_linearRGB3;
u32flags,u32reserved0; f64reserved[2]=0. These sum192 bytes. Native CLI selects
CPU thread count and output prefix. Width/height<=8192, supersample1or2,
width*supersample,height*supersample<=8192, alpha in[0,1]. Camera rows must be
orthonormal and right-handed under a stated tight binary64 check. Pixel center
and scale are in final-image pixels, before supersampling.

Each frame writes tightly packed RGBA8 and, for label anchoring/oracles, a
final-pixel f64 depth plus u64 primitive ID and u64 instance ID. Background
uses depth=-infinity, IDs=UINT64_MAX. With supersampling, ID/depth refer to the
nearest-to-center subpixel (specified deterministic lowest-index tie); these
are not averaged IDs. An optional full-sample diagnostic buffer supports exact
coverage qualification. Geometry first-jet source position/normal output is
available once per session for independent actual-source checks.

Root's identical CPU PNG/SVG labels/BOM compositor consumes RGBA/ID/depth and
its preserved BOM. Its full caller timer includes source import/meshing,
native process, image encoding and labels. Static SVG rasterization, if used,
is named and measured separately rather than silently omitted from a claimed
whole-image wall.

## Projection, visibility and appearance

Rigid instance position is assembled_matrix*localXYZ + alpha*explosion_offset.
At alpha0, branch directly to the original rigid position before evaluating any
explosion arithmetic, preserving assembled values and signed-zero behavior.
Camera projection is orthographic. Projected XY is snapped to1/256 sample pixel
by an explicitly shared round-half-away-from-zero rule. Absolute snapped X/Y
must be<=2^29, making signed64 edge products/subtractions safe. Overflow is an
input failure, not clipping by wraparound. Zero projected-area triangles emit
no samples; offscreen bounding boxes produce no tiles. No perspective near-plane
contract is implied. Both windings render, while source face orientation still
controls lighting. Reorder projected winding to positive area, keep corner
attributes paired, use integer edge signs plus a single top-left rule. Depth
is barycentrically interpolated binary64 camera Z. Largest Z wins; equal Z
selects lowest stable primitive ID. No epsilon changes coverage or depth.

Owned16x16 sample tiles use complete CSR triangle bins. GPU count/scan/fill
never truncates bins; total references are checked before allocation. A simple
owned scan is acceptable until real measurements justify more machinery. GPU
scatter order may vary; visibility selection is order independent. CPU workers
own disjoint tiles and rasterize triangles into local depth/identity buffers,
using source normals only for final visible samples. GPU threads independently
select sample winners from the same tile lists and shade once. CPU can exploit
triangle bounds/cache and GPU can exploit sample parallelism without forcing
either into the other's unfavorable loop order. Retain workers/device storage
between explicitly reused requests. No Python triangle loop enters the baseline.

Normals are smoothly interpolated only within the actual source face and
renormalized; duplicated edge corners retain sharp face boundaries. Lighting
is documented illustrative metallic Lambert/fill/Blinn, source/curated linear
material colors, no fake bevel geometry, no physically based/photometric claim.
Use fixed integer specular powers if needed to avoid unnecessary CPU/GPU pow
variation. Supersampling averages linear RGB before common sRGB conversion.
No shadows are promised in this first matched renderer; dense real geometry,
proper source normals and antialiasing carry the image.

## Correctness oracles and failures

Independent lane owns tiny analytic/fixed-rational coverage and depth expected
values: exact boundary top-left, opposite winding, shared edge, depth order,
coplanar tie ID, moved/rotated instances, offscreen/edge-on, supersample resolve,
invalid files/ranges/transforms and explicit CUDA unavailable/error behavior.
Analytic first-jet expected maps and derivatives, sphere/cone singular limits,
Bezier spans and indirect axes attack geometry independently. Actual whole OM10
source positions/normals, per-face coverage and shared physical boundary
positions must pass the import/geometry oracle before any product render.
CPU/CUDA identity parity is useful but not its own geometry oracle. Allow only
justified bounded RGB/normal differences from transcendental implementations;
exact fixed coverage and selected ID must match independently expected scenes.

No silent CUDA fallback, missing faces, capped candidate lists or repaired
geometry. CUDA allocation/launch/sync/download errors identify the stage and
invalidate the frame. Close frees each owned allocation/stream, records any
cleanup error and restores previous device. Session becomes unusable after a
fatal device error. CPU and GPU retain immutable copies of scene input, so
caller mutation cannot race the active frame. CLI nonzero exit and absence of
success rows distinguish failed work from a successful empty scene.

## Timing contract

Named native rows, in wall milliseconds unless explicit event suffix:

1. file_read_decode_validate;
2. session_create (CPU workers or dlopen/device/stream);
3. scene_allocate_upload (CPU preparation/retention or GPU allocation+H2D);
4. source_evaluate (useful source positions/normal first jets);
5. instance_transform_project;
6. triangle_setup;
7. tile_count_scan_allocate_fill (including total-count host sync/allocation);
8. visibility_shading_resolve;
9. result_download (CPU result preparation or GPU D2H+completion);
10. output_write;
11. session_close (workers, allocations, stream and plugin);
12. complete_native_wall (includes1..11 and all overhead; stage rows may overlap
    only if explicitly labeled, no asynchronous kernel-only speedup headline).

A reused request excludes1..4 only when their outputs remain resident on both
paths; it includes changed placements/camera, new bins, complete display,
download and output. Geometry evaluation is rerun if source geometry changes.
Assembled/exploded camera/quality/material/label policies and source bytes are
identical across CPU1/CPU24 and each P100. Repeated cold processes and repeated
same-session updates report median/range; final graphic numbers come from the
actual full caller stages, not selected kernel events. CUDA event rows, if
collected, supplement host costs and are not added to overlapping wall rows.

## Next reviewed work

Cross-review byte layout and scalar carrier/raster contracts with import and
independent oracle owners; root integrates design then reviewed plan. Only after
that: written lane subplan, independent red tests on fleet, implementation,
green and independent review. No actual watch rendering until every missing
capability and whole-model qualification are complete. End with matched
assembled/exploded CPU/P100 images and honest observed times, not certification
or an unrequested renderer framework.
