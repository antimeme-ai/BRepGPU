# OM10 native geometry/display lane — 2026-10-15

Read the shared Blackbird doctrine and active repository instructions. The task
is the complete source-derived assembled/exploded watch with useful native CPU
and P100 work, not an extension of the old Python triangle loop baseline.

Studied existing evaluator ABI/source, CUDA lifetime handling, CMake targets and
watch display. Read and acquired intact Pineda1988 and Laine/Karras2011 raster
papers, plus the pinned CUDA12.8 Pascal guide. Sources/hashes/restoration and
concrete design input are in papers/2026-10-15-om10-native-display-study.md.

Coordinated original-UV analytic frames with import owner: keep all independent
axes; cone V is slant distance. Proposed native scene/vertex/carrier records,
original UV span maps, shared boundary XYZ, physical keys, stable occurrence IDs,
source-normal consumption, native tile display and explicit cold/reused timing.
Request size was checked and corrected to192bytes with two reserved f64 fields
before design acceptance. Alpha0 must branch before explosion arithmetic.

Sent the layout to root/import/oracle owners for review. Called out CPU/CUDA
transcendental last-bit differences at display snap thresholds: exact integer
coverage does not imply universal whole-image identity when source positions
are independently evaluated. Tiny exact-answer fixtures and whole-source
bounded position checks attack the actual faults; do not invent a promise of
byte-identical source-derived images.

No product code, geometry, test, render, native build or Git mutation occurred
in this research stage. Await integrated reviewed design and plan.

Integrated design and implementation plan accepted after native review. Added
explicit finite-result errors, original assembled alpha0 branch, and deterministic
supersample ID/depth policy. Oracle owner caught cameraY convention ambiguity;
we resolved it as screenY=centerY-scale*cameraUp before implementation green.
Initial two native fleet tests genuinely failed because the executable did not
exist; source/assembly reds were separate. Later expanded cases are not claimed
as initial red.

Implemented the isolated native/om10 C++17 executable and optional CUDA12.8/sm60
plugin. Own checked binary parsing, original-UV analytic/Bezier first jets,
limiting normals, copied source values, retained CPU workers/device geometry,
rigid instances/explosion, fixed-subpixel top-left coverage, complete CSR tile
bins, two-level512-entry GPU scans, visibility, source-normal shading, linear
supersampling/sRGB and explicit geometry/projection/identity diagnostics. CPU
path never loads CUDA. No existing ABI/build changed. Root source review caught
valid cone reference radius0 and proper rotation consistency; fixed both.
Static self-review fixed partial-upload cleanup pointer ownership and CUDA
arithmetic source-row errors before the first GPU qualification.

Fleet Release CPU+CUDA build succeeded, with no host warnings after cleanup;
nvcc reports the expected pre-sm75 architecture deprecation. Six direct methods
passed on CPU1, CPU24, P1000 and P1001: exact tiny pixel/depth/source IDs,
original plane/reversed normal jets, shared edge/opposite winding/offscreen,
front/coplanar tie/instances/explosion, projected-only rotated normals/mapping,
and supersample resolve identity. Expanded analytic/failure/multiblock cases and
independent source review remain. No actual OM10 rasterization has run.

Expanded independent native suite now has nine direct methods, all green on
CPU1, CPU24 and both P100s. Added original-domain analytic carriers with indirect
axes, independent rational spline first jets, sphere/cone limiting normals,
and>512tiles with retained request-size changes. Eight focused process/error
methods pass Release; applicable CPU methods also passed ASan/UBSan. Root's
adversarial source review accepted the native geometry/raster ownership and
caught reused-frame host-buffer destruction outside its wall; moved destruction
inside that wall. Metadata explicitly separates native close wall from outer
process exit and host source/request destruction.

Further static self-review fixed secondary cleanup reporting during CLI error
unwinding/failed CUDA create, and consumes the plugin handle before reporting a
close error. No success geometry changed. GPU memcheck uses an ignored wrapper;
its first invocation selected the wrapper directory's nonexistent default plugin,
so it did not test GPU work. Corrected by passing the actual plugin explicitly
and reran. Retain the distinction from product failures; no claimed memory result
comes from the misconfigured attempt.

At root's request independently read the source adapter and relevant OCCT
BRep_Tool/StepToTopoDS source. Sent geometry owner three findings: silent nearest-
endpoint parameter swapping; unproven affine continuation brackets when
SameParameter=false; and missing raw-to-translated actual wire/edge-use inventory
reconciliation. Owner accepted endpoint diagnostic/removal and noted actual
source has zero non-SameParameter edges. They will explicitly reject unproven
maps rather than claim branch recovery, and actual imported-wire reconciliation
was already added in their newer adapter revision. Geometry owner retains fixes;
this lane changed no adapter/mesh code. OCCT CurveOnSurface actually reverses
local edge selection for a reversed face and chooses PCurve2 for a reversed
closed-surface edge; raw occurrence orientation therefore needs that exact
source convention, not treating two seam uses as one UV curve.

Final focused native memory result: all nine independent direct methods pass on
CPU1/CPU24/P1000/P1001. ASan/UBSan runs17 methods:16 applicable CPU methods pass;
the CUDA-only ownership method is intentionally excluded there and separately
passes Release and compute-sanitizer. GPU memcheck's14 instrumented processes
all report zero leaked bytes;13 report zero errors, while the deliberate invalid-
device fixture reports the expected CUDA API error. That is not presented as an
all-zero-error run. Removed redundant invalid-device cleanup selection before
stream creation; its focused rerun now reports exactly one intentional API error,
zero leaks. The source-arithmetic failure rerun reports zero memory errors/leaks.
No success-path changes or additional whole-corpus campaign followed.

Independent oracle owner read all native files and accepted geometry/ownership.
Their one valid stage finding was CPU projected diagnostic copy outside any
named copy stage (whole wall already included it). Assigned that copy to
result_download, symmetric with GPU diagnostics; affected project-only source/
normal mapping case passed CPU24 after rebuild. Native direct review is complete.

Reviewed root's complete render caller, presentation and benchmark driver without
editing their files. Confirmed source_tables per-definition triangle order matches
the actual scene writer and native occurrence/primitive flattening; camera view
packing, source-instance ID decoding and common PNG/SVG path match native bytes.
Sent three concrete reporting/reproducibility findings: CPU metadata should not
claim GPU work; resident packed view rows should match corresponding cold view
bytes, not just scene/software hashes; and the benchmark driver should pin its
own executed source alongside the render caller. Root owns those changes.

Final static review of caller/benchmark/presentation updates: original-source and
actual native qualifier wiring matches their public APIs; source/primitive maps
remain in actual writer order. Read the poster compositor's192-byte view decode,
fixed firstGPU0 image choice, actual repeat medians, copied CPU/P100 comparison
pixels, footer scopes and source-linked callout coordinates. No geometry/raster
execution occurred in this review. Sent one valid reporting finding: prepare/
evaluate currently inherit image-producing caller scope/timer names and CUDA
raster-stage descriptions despite producing no images. Root should make these
mode-specific. Recommended a short CPU-preparation/CUDA-stage legend in the final
poster; actual long-name/leader readability remains planned post-qualification
image inspection. Parent owns all presentation edits; no new tests/mechanisms.

Reviewed the appended nonvisual whole-source backend runner plan before code.
Accepted retaining one immutable freshly constructed mesh and using the existing
raw/source/mesh/assembly/native qualifiers plus identical actual caller scene and
PROJECT_ONLY views for CPU24/P1000/P1001. This avoids duplicate expensive mesh
construction and adds no qualification mechanism or persistence format. Asked
root to include the runner's own source in before/after pins and retain actual
quality/view settings and scene/view hashes in its existing TSV output. No
product geometry/render/test execution occurred in this review.

Read the implemented scripts/qualify_om10.py against actual MeshQuality,
mesh_imported, qualifier and native APIs. Accepted without blockers: one source/
mesh retained, exact caller helpers, same scene/views, flags3, source jets,
per-backend immediate qualification and failures, own-source/product/binary pins,
no raster path. Suggested recording the two existing refinement/allocation CLI
budgets beside quality/view fields. No new mechanism, test or actual execution.

Targeted actual-source cone apex diagnosis requested by root: read carriers1215/
1431 and faces166149/171148, actual apex vertices and seam pcurve endpoints from
the retained snapshot. Wrote a private48-row/36-distinct-UV tiny actual-carrier
scene, zero views, and evaluated jets only on CPU24/P1000/P1001. Included original
recovered Vmin, CPU and independently high-precision nearest apex constructions,
nextafter interior and two small interior offsets. All complete output buffers
are byte-identical. Canonical apex values cancel to rho0/limiting flag1; original
foreign Vmin overshoots the source pole and consistently flips normals on all
backends. No native production change is justified. Saved public source values,
scope and consequences in papers/2026-10-15-om10-native-cone-apex-study.md. Root
and geometry owner informed; source owner retains verified-pole UV canonicalization
and perturbation-budget fixes. No actual watch rasterization occurred.

Reviewed a bounded benchmark scope change after actual full mesh cost became
clear. Accepted four fresh complete callers (CPU24/P1000 × assembled/exploded),
one directly enclosed observation per task, instead of24fresh full callers plus
four resident rebuilds. Correctness still comes from existing actual whole-source
and native qualifiers. Prepared native repeats can reuse the existing identical
BGMESH01 and exact perpose BGVIEW01 outputs: CPU1/CPU24/bothP100 fresh-process
median/range, plus explicitly resident native batches. These are separate named
scopes, never sums or extrapolated complete walls. Poster should separate the
four native median rows from the two full-task n1 rows and fit both panels in the
footer. No new mesh persistence, framework, qualification or actual rendering
was performed for this review. Parent owns plan/driver/poster edits.

Reviewed implemented cost-aware driver/poster amendment and optional second timing
panel. Accepted without blockers: exactly four fresh full callers/n1, identical
scene/software checks across poses/backends,24fresh prepared-native observations
at default3repeats, four resident batches with exact two-pose request rows. All
walls remain directly enclosed and named. Poster checks sample counts, uses fixed
full P1000 imagery and separates native medians from full n1 observations. The
default2400px layout separates counts, full task and native panels without six-row
footer stacking. Command/build paths and actual primitive/source mapping remain
consistent. Static review only; no watch target execution or new test mechanism.

While the full nonvisual V2 run remains active, prepared only a pilot-render plan
in docs/OM10_PILOT_RENDER_PLAN.md. Exact BGVIEW flags offsets184/376 change3→0;
all qualified scene/camera/pose/source values stay unchanged. Proposed two fresh
native CPU24/P1000 processes for both poses and existing PNG/SVG/anchor helpers,
with no remesh/new API/cache. Actual rasterization waits for root's explicit
complete V2 PASS and plan approval. Product/helper/binary files were not changed,
and no product/geometry/render/test command was executed for this planning turn.

Root accepted the written pilot plan and authorized preparing its private utility
while V2 runs. Prepared context/om10-native/pilot_qualified_om10.py for source
review: exact two flags-only changes, unchanged qualified inputs and production
pins, existing source/BOM and presentation helpers, two fresh prepared native
processes, and explicit input/output hash and scoped timing manifests. Local AST
inspection passed; the utility was not executed or synced to the fleet. Product,
helper and binary files remain unchanged. Actual raster still waits for root's
explicit complete V2 PASS and review of this utility.

Root accepted the private pilot utility after static review. While nonvisual V2
continues, reviewed README, previous watch/NCSX reports, gallery, artwork license
and current OM10 compositor. Recommended assembled/exploded PNG+SVG artwork in
assets/om10 with the existing source-derived license, a compact result report
retaining all observed times, BOM/explosion/source mappings, geometry summaries
and source/binary pins, and a README OM10 section preceding the schematic watch.
NCSX remains the continuing goal. Original CAD, operational native snapshots,
large raw image/jet/ID buffers and process logs remain private.

Static presentation risks: at mobile feed scale, current 16px callouts,20px times
and15px credit on2400px posters become too small to read. Full-resolution gallery
artwork and brief accompanying text should carry actual counts, native median
versus full-task n1 scopes, and source credit; an actual-pixel detail crop can
supplement the two full poses. Existing source-linked labels choose at most12
visible root groups, while all166 parts remain in the BOM, so captions must not
imply166 individually labeled components. Long source names and leader crossings
still require the already planned real-image inspection. The old SHARE_ON_X
draft incorrectly says the GPU evaluator is merely proposed and must be replaced
before sharing. No actual target execution or product changes for this review.
