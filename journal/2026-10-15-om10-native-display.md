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
