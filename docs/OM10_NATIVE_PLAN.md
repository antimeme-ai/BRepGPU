# OM10 native geometry/display implementation subplan

Candidate for plan review,2026-10-15. Implements design section3 as a standalone
owned CPU/CUDA consumer, not an extension of the public evaluator registry. No
production code before integrated design/plan acceptance and initial fleet red.

## API and owned files

Isolated native/om10 CMake project builds `brepgpu-om10-native` plus optional
`libbrepgpu_om10_cuda.so`, targeting C++17 and CUDA12.8/sm60. Existing CMake/ABIs
remain unchanged. Planned files in native/om10:

- CMakeLists.txt: strong warnings, no fast math/FMA contraction; CUDA optional;
- scene.hpp: immutable owned records and checked explicit byte decoder;
- math.hpp: shared scalar analytic/Bezier first jets, transforms and fixed
  raster predicates/shading; rational recovery reuses ../evaluator.hpp;
- backend.hpp: private POD plugin view/output/error/timing API, no STL values
  crossing the dynamic-library boundary;
- cpu.cpp: retained native workers, source evaluation, instance geometry,
  tile bins, cache-friendly disjoint tile rasterization, image resolve;
- cuda.cu: explicit device/stream ownership, retained scene/jet buffers,
  geometry/setup/count/scan/fill/sample/resolve kernels and completed downloads;
- main.cpp: CLI, owned source/view copies, CPU path or explicit lazy plugin,
  output binary/TSV writes and complete wall/cleanup handling.

Initial native fixtures are independently written under tests by the oracle
owner, using raw format writers and specification answers. This lane owns the
complete implementation, integration and fixes across all these files.

```
brepgpu-om10-native --scene SCENE.bgmesh --views VIEWS.bgview \
  --output-directory DIR --backend cpu --threads 24 [--dump-source-jets]
brepgpu-om10-native --scene SCENE.bgmesh --views VIEWS.bgview \
  --output-directory DIR --backend cuda --device 0 \
  --plugin LIBRARY [--dump-source-jets]
```

Default oracle binary selection: BREPGPU_OM10_NATIVE or
build/om10-release/brepgpu-om10-native. CPU mode never opens/links CUDA; explicitly
requested CUDA errors cannot silently become CPU work. Request frame_count0 is
source-evaluate-only. It creates the selected session, evaluates all actual
source corners, writes requested jets and closes, without triangle/image work.

Input exact bytes are the80-byte BGMESH01 header and explicit records in the
native study. BGVIEW01 header16 and each192-byte request preserve original
source geometry, view and explosion. Request flags: bit0 PROJECT_ONLY (compute
placed geometry without visibility/image), bit1 DUMP_PROJECTED, bit2
DUMP_SAMPLES. Reject all unknown bits. Useful normal image requests have flags0.

## Outputs for caller and direct oracles

All scalar binary outputs use explicit little-endian writes. Native copies
immutable scene input; no file mapping/caller mutation lifetime is retained.

- `source-jets.bin`, only with --dump-source-jets: magic8 BGJETS01, u64vertex_count,
  then128-byte rows: f64sourceXYZ[3],du[3],dv[3],normal[3],usedXYZ[3],u32status,
  u32flags. Status0 is success. Flag1 identifies an analytic limiting normal.
  Boundary usedXYZ is exact canonical input; interior usedXYZ is evaluated
  sourceXYZ. Derivatives are original-UV derivatives, before face reversal;
  normal includes face sign. This enables actual CPU/P100 geometry qualification
  before any product render.
- `frame-NNNNNN.rgba`: width*height*4 tightly packed RGBA8 bytes.
- `frame-NNNNNN.depth`: width*height explicit f64 camera Z.
- `frame-NNNNNN.primitive`, `.instance`: width*height explicit u64 stable IDs.
  Background Z=-infinity, IDs=UINT64_MAX. At2x sampling the final ID/depth come
  from the upper-left subpixel (nearest-center tie broken by lowest row-major
  subpixel index); color averages all samples in linear space.
- `frame-NNNNNN.projected`, only with DUMP_PROJECTED: magic8 BGPROJ01,
  u64placed_vertex_count, then96-byte rows: u64instance_id,u64definition_id,
  u64source_vertex_slot; i64snappedXY[2]; f64cameraZ; f64worldXYZ[3],worldNormal[3].
  PROJECT_ONLY can emit this without rendering. Source vertex slot is global
  scene vertex slot, permitting independent lookup of carrier/physical key.
- `frame-NNNNNN.samples`, only with DUMP_SAMPLES: magic8 BGSAMP01,
  u64sample_width,u64sample_height; then f64depth,u64primitive_id,u64instance_id
  per row-major supersample. No average of IDs or depths.
- `timings.tsv`: scope/frame/stage/milliseconds/count/notes, all named native
  wall scopes and actual counts. `metadata.tsv` names backend/device/thread
  count, dimensions, quality rules, source counts, native build identifier and
  diagnostic flags. No private machine paths are included in publishable data.

Header/row arithmetic is checked in tests and independent readers. A source
failure emits stderr with stage and source/request row, returns nonzero, and
writes no success row for that operation. Output files from failed attempts are
not accepted by callers. CLI writes into a fresh output directory or requires
it to be empty, preventing accidental stale successful images after an error.

## Scalar contract before implementation

Carrier tags are plane1,cylinder2,cone3,sphere4,torus5,Bezier6. Original O/X/Y/Z,
cone slant V and independent axes are retained. Source kind6 B-spline is not the
same object as a display-kind6 extracted Bezier span; provenance maps remain
explicit. Source face orientation is vertex flag2. Formula normals at a sphere
pole retain radial direction with axis handedness. Cone apex limiting normal
is U-dependent; duplicated UV corners may not merge/average these normals.
Genuinely singular Bezier normal fails pending an actual-source reviewed policy.

CPU and CUDA use source-evaluation formulas in the shared math header, but this
is code reuse, not the independent geometry oracle. Libm trig differences are
bounded by actual source checks. Normalization and every intermediate/final
geometry/projection/depth/shading value must be finite; overflow identifies the
source or request row and fails rather than skipping geometry. Original rational
weights must be positive. Size/range products use checked arithmetic before
allocation. Source canonical 3D triangles must be noncollapsed; a valid world
triangle becoming edge-on in projection correctly contributes no samples.

Matrix/vector and depth operations have explicit evaluation order, with CPU
-fno-fast-math -ffp-contract=off and CUDA --fmad=false --ftz=false
--prec-div=true --prec-sqrt=true. dot3=((a0*b0+a1*b1)+a2*b2).
Rigid position is dot3(rotation_row,XYZ)+translation; alpha0 branches before
explosion multiplication/addition. Camera dot3 uses world-center. Projection
uses pixel_center+pixels_per_unit*cameraXY, then multiplies by supersample.
Snap uses floor(256*x+0.5) for x>=0 and ceil(256*x-0.5) for x<0; bound absolute
snapped coordinates<=2^29 before integer conversion/edge products.

After winding normalization, integer edge coverage is positive or zero on a
single top-left owning edge. Interpolation uses unbiased integer edge values,
not edge-ownership bias. With area>0, wa=Ea/area,wb=Eb/area,wc=Ec/area;
z=((wa*za+wb*zb)+wc*zc). Largest Z wins and equal Z selects smallest stable
primitive ID. Normals use the same three weights on source normals, then unit
normalization. Specular implementation uses fixed integer powers and explicit
lighting coefficients; document coefficients in metadata. Linear supersample
resolve precedes common documented sRGB conversion. Labels remain root-owned
identical CPU composition, timed in the full caller.

Actual CPU/CUDA evaluated interiors can cross a snap threshold or change a
nearly equal depth winner. Projected diagnostics retain the per-backend inputs
needed to explain such differences against independent source-error bounds.
No unexplained pixel/ID disagreement is silently accepted. Tiny exact-answer
fixtures have exact stable projected inputs and exact expected coverage/IDs.

## Ordered implementation and qualification

1. Independent owner writes one plane-backed scene, original-UV first-jet and
   tiny pixel-answer fixture against the reviewed CLI. Root runs it on fleet
   while executable is absent; retain genuine missing-executable red. Later
   expanded cases are reported separately, not retroactively claimed red.
2. Implement checked decoding, analytic/Bezier scalar first jets, CPU session,
   explicit output and evaluate-only. Run direct analytic/Bezier tests; include
   indirect axes, original span scaling, face reversal and singular limits.
3. Implement CPU retained workers/instance geometry/projection/setup/complete
   bins and visibility/shading/resolve. Green independent exact pixel model,
   CPU1/multicore, transformed/offscreen/tie/shared-edge/invalid inputs and
   fresh-output behavior. Investigate genuine failures before CUDA work.
4. Implement optional CUDA plugin for the same immutable values and scalar
   contract. Allocate/upload/evaluate once; transform/setup/count/scan/fill per
   frame; no truncated bins. Keep CPU and CUDA geometry retention symmetric.
   Green direct source/pixel fixtures on each P100 and explicit unavailable
   device/plugin/error cases. Run relevant native host sanitizer/device memory
   checks on the new unit only, not an unrelated full historical campaign.
5. Independent lane code review inspects numeric ownership, corruption/failure
   handling, finite output, ID/normal/source mapping and timing completion.
   Resolve valid findings and rerun affected direct cases. Root reviews API/
   caller integration. No second certificate, mutation campaign or pixel-golden
   test substitutes for these direct specification oracles.
6. Complete source snapshot/owned mesh ready: evaluate-only CPU/P100 on all
   source corners, and project-only diagnostics where needed. Independent
   original-carrier/mesh/assembly checks must pass before trophy visibility.
7. Only after all whole-model capability qualification: actual same-quality
   assembled/exploded images, source-linked labels/BOM, fixed matched repeats
   CPU1/useful multicore/both P100s. Fix actual presentation defects, publish
   observed timings and inspect artifacts. No opportunistic generic renderer
   expansion or performance gates after direct obligations pass.

## Timing and lifetime

Retained CPU worker pool and CUDA context/stream/device allocations own all
scene/jet data. CPU-only has no CUDA symbols or plugin load. Plugin calls are
synchronous at completion boundaries; teardown consumes allocations once and
records errors before returning. Prior CUDA device is restored. Fatal device
errors poison the session; later frames fail and close still attempts cleanup.

Native stages: decode; session_create; scene_allocate_upload; source_evaluate;
source_jet_write (only requested diagnostics); instance_transform_project;
triangle_setup; tile_count_scan_allocate_fill; visibility_shading_resolve;
result_download; output_write; session_close; complete_native_wall. Host walls
include allocations, synchronization and error/finite checks at their actual
stages. Timings emitted after close include close. Root measures external
process/complete STEP-to-image wall including import/mesh, Python dispatch,
identical labels/PNG encoding and shutdown; separate static SVG composition is
named. Reused frame includes current pose/camera/bins/render/download/outputs,
retaining source geometry equally. Diagnostic dumps and independent qualification
are separate from useful work but included in any claimed qualified complete
wall. CPU work is native and multicore, never the old Python triangle loop.
