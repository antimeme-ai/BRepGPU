# Direct CPU display of the NVIDIA OM10 mesh

Proposed reference-only design,2026-10-15. The operator additionally requires the
actual NVIDIA USD/BRep CPU baseline. The separate reference owner studies that
kernel's import/tessellation APIs. This note studies the display boundary in the
current owned native implementation; no reference run, render, build or source
edit to the active whole-source qualification is performed.

## Existing code and the gap

`native/om10/cpu.cpp` creates an immutable carrier-backed session, calls
`evaluate` for every original-UV source vertex, and then projects and displays
those results. Its `Api` has no direct XYZ+normal entry. Using artificial plane
carriers to smuggle foreign triangles through that entry would hide the actual
source and add an irrelevant evaluator cost. Reaching its anonymous `Session`
implementation from a reference translation unit would couple a public caller
to private ownership details. Neither approach is proposed.

The unchanged `math.hpp` already supplies the actual projection, snapped
triangle setup, top-left coverage/depth, normal shading and color encoding.
`types.hpp` supplies the existing definition/instance/triangle/material and
image records. `scene.hpp` supplies finite little-endian input reads, checked
size arithmetic, proper rigid-matrix checks and the complete BGVIEW01 loader.
These are reusable directly without editing or rebuilding the qualified target.
The raster method and literature are already recorded in the
[native display study](2026-10-15-om10-native-display-study.md).

## Proposed complete reference boundary

Add a standalone `native/om10_reference` executable named
`brepgpu-om10-reference-display`, built into its own build directory, CPU only.
It links no NVIDIA, OCCT, CUDA or new library. The reference owner's CPU NVIDIA
adapter writes genuine tessellation XYZ/normals/indices into an explicit raw
mesh record. This executable never constructs or evaluates surface carriers.

Command:

```sh
brepgpu-om10-reference-display --mesh genuine-nvidia.bgrefmesh \
  --views actual-matched.bgview --threads 24 --output-directory new-directory
```

BGREFM01 is explicit little endian with no alignment padding:

| Section | Bytes | Fields |
| --- | ---: | --- |
| Header |64|magic BGREFM01; u32 version1,flags0; five u64 counts for definitions,instances,vertices,triangles,materials; u64 reserved0|
| Definition |40|existing five u64: ID,first vertex,vertex count,first triangle,triangle count|
| Instance |136|existing u64 ID,u32 definition slot,u32 reserved0,twelve f64 rigid matrix and three f64 authored explosion offsets|
| Raw vertex |56|six f64: genuine NVIDIA XYZ and normal; u64 external vertex key|
| Triangle |24|existing three u32 definition-local vertex indices,u32 material slot,u64 external face ID|
| Material |32|existing three f64 linear RGB and f64 roughness|

Section order is the order above. Every record belongs to a definition, counts
and resulting placed slots remain within the existing u32 limits, and exact
file size must match checked section arithmetic. Define source IDs and export
mapping in companion TSV, not through invented original STEP correspondences.
Definition and occurrence IDs must be unique in their respective tables and may
not equal UINT64_MAX. Genuine external vertex keys and face ordinals may be0;
vertex keys may repeat for distinct normal corners, and face ordinals normally
repeat across multiple triangles from the same imported face. Do not apply
definition/occurrence uniqueness or reserved-ID guards to those external keys.
Reject reserved definition/occurrence IDs, duplicate definition/occurrence IDs, overlapping/unowned
ranges, out-of-range indices/materials, repeated/collapsed triangle vertices,
nonfinite values, zero normals and non-proper/nonrigid instance matrices. The
raw input remains immutable through rendering. No fallback replacement mesh is
allowed on decoder/export/kernel failure.

Prefer actual definition-local NVIDIA geometry plus the original166 occurrence
placements and reviewed explosion. Duplicate only actual vertices as necessary
to preserve NVIDIA per-corner or face-varying normal assignments; retain their
external keys and index correspondence. If NVIDIA returns already placed/world
geometry, establish that explicitly and never apply a second source transform.
The reference owner must settle this representation before implementation.

The reference owner found a preferable native SDK extraction path: the release
includes SMLib/SM_API_USD headers for actual import/tessellation, direct binary64
`SmPolyVertex::GetPoint()` and normal/face records keyed by original imported
faces. The selected contract requires EVERY actual output polygon to have three
corners and no inner loops. Fail on any other polygon; do not add fan or hidden
owned triangulation. The original-face labels are genuine imported NVIDIA face
identities, not automatically original STEP entity IDs.

Normals must be actual NVIDIA tessellation output with explicit per-corner
indexing, definition-local coordinates and face orientation. Normalize their
nonzero magnitudes for display without reversing orientation or reconstructing
them from owned carriers. The reference owner confirmed SDK corner
`PolyEdge::GetAuxData()->m_vNormal` stores the genuine f64 tessellation normal,
adjusted for original face reversal before emission; obtain XYZ from that edge's
start physical vertex. This corner record preserves seam/apex distinctions a
single normal per physical vertex/face can lose. Require each corner to supply
a finite nonzero normal and real physical vertex key; reject absent/ambiguous
correspondence. The public
Python binding's `to_mesh_arrays(face_varying_normals=True)` casts coordinates
AND normals to float32; `get_points()` retains binary64 positions in the same
physical vertex order. That path would retain the already rounded float32
normal values when promoting to f64; promotion cannot recover lost precision.
It is a possible fallback, not the selected native f64 extraction contract.
Do not claim matching geometry quality merely because cameras/resolution match.

Use the existing BGVIEW01 requests byte-for-byte, including both actual source
poses, alpha0 then1, camera, supersampling and background. Stable instance IDs
must match the agreed actual source occurrence mapping, so the existing label
compositor can consume the resulting ID buffer. Native primitive IDs enumerate
the actual external triangles and placed occurrences; they are not interchangeable
with owned-mesh primitive IDs.

## Shared mathematics and independent ownership

The reference session owns the raw arrays, projected records, prepared triangles,
complete tile bins and image buffers. A retained CPU worker pool and the existing
16x16 count/prefix/fill/tile loop reproduce the owned CPU orchestration, with
the same thread count. Projection calls unchanged `project`: an internal
`Jet`-layout adapter contains only the raw XYZ in `.used` and supplied normal in
`.normal`; no carrier, derivative, source jet claim or evaluation is involved.
The fields unused by projection stay zero and no source-jets file is emitted.
Alternatively this small adapter can be replaced by an explicitly raw projection
overload after qualification, but no product refactor is needed for this unit.

`SceneView` points only at retained definition/instance/triangle/material/ref
arrays consumed by projection/setup/shading; carrier/control/source-UV arrays
are empty. The reference decoder does not construct a carrier `Scene` or invoke
its carrier validator. It performs its own raw-record checks above.

Same projection arithmetic, integer snap, both-winding setup, top-left rule,
FP64 depth and stable primitive tie rule, lighting, antialias resolve and sRGB
encoding apply. The reference-only outer scheduling code is small duplication
of the existing CPU scheduling, justified by keeping the running qualification's
product code unchanged. No macro/include access to private CPU internals and no
new general renderer framework are introduced.

Write the existing frame-NNNNNN RGBA/depth/primitive/instance outputs, optional
BGPROJ01/BGSAMP01 diagnostics, and TSV stages. PNG/SVG/source credit use existing
presentation only after native output. Fail with stage and offending row;
allocation, decoding, projection, nonfinite raster or output failures terminate
the caller, leave no successful-result claim, and join/close owned resources.
Reject existing output destinations to prevent stale output reuse.

## Measurement claims

Call the baseline **NVIDIA USD/BRep CPU import+tessellation + shared owned CPU
display**. NVIDIA's viewer is not being represented as a CPU kernel renderer.
The reference owner's outer complete fresh process wall must enclose actual
NVIDIA source loading/import/tessellation, mesh/index export, this CPU display,
both PNG/labeled SVG outputs and exit. Record kernel preparation, export, owned
native display and presentation separately; do not add overlapping rows or
infer a complete wall from their sum. Use the same source revision, placements,
poses, cameras, palette, CPU24 display and output quality settings where those
settings have an actual common meaning. Report NVIDIA tessellation settings,
output counts and errors; a similarly named tolerance is not automatically the
owned mesher's bound. Keep required source/kernel failures explicit.

Keep one actual full two-view baseline observation distinct from repeated
prepared-scene native display medians. Any integration/export work around
NVIDIA belongs in the measured full baseline and stays separately visible.
No owned or OCCT triangles may stand in for failed NVIDIA output.

## Proposed bounded implementation sequence

1. Root accepted this structure conditional on source extraction. Reference
   owner confirms actual XYZ/normal/index/local-frame APIs and source mapping,
   then reviews this subplan and root accepts the concrete extraction contract.
   No edits to qualification-pinned code or builds until permitted.
2. Add only reference CMake/decoder/main/CPU scheduling files. Initial red is an
   independent tiny raw triangle caller failing on the absent executable.
3. Complete input validation, retained multithread CPU display, actual raw outputs
   and scoped timing. Reuse existing math rather than reimplementing its rules.
4. Direct fixtures check exact scalar pixels/IDs/depth; opposite winding/shared
   edge, front/equal-depth, rigid placement/explosion, face-varying normal and
   SS2 cases; malformed counts/nonfinite/zero-normal/index failures. Compare one
   identical plane fixture to the qualified CPU display as a compatibility
   check alongside the independent scalar answer. A focused sanitizer run checks
   this new decoder/buffer ownership; no repeated product geometry campaign.
5. Independent root/reference-owner source review, fix actual findings, then use
   the genuine NVIDIA output and existing matched request bytes after complete
   whole-source PASS and inspected pilot. Stop at the direct requested baseline.

Root design and plan review ACCEPT. The independent reference owner also accepts
the complete design/subplan and confirms native SDK f64 point and per-corner
normal access: supplied normals account for original face reversal. The producer
still must directly check real indexing/orientation before the NVIDIA watch run;
the standalone raw consumer can now proceed on independent tiny inputs. All
qualification-pinned files and binaries stay unchanged. No actual target raster
is authorized by this implementation approval.

Implementation review completed: root and the independent reference owner both
accept the isolated raw decoder, CPU scheduling, shared math, lifetime and output
scope. Six direct methods and the same focused address/undefined sanitizer run
pass on the fleet. This establishes the raw consumer on authored inputs; genuine
NVIDIA geometry admission and whole-watch qualification remain separate work.
