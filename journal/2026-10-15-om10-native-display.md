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

After the reviewed two-view measurement plan was accepted, updated only the
benchmark driver/compositor and associated documentation. Two fresh CPU24 then
P1000 callers now each construct the whole model once and emit assembled then
exploded labeled views. One n1 full observation per backend is keyed view=both;
both posters show that explicitly shared total, without divided/inferred
per-pose complete walls. Full snapshot/source-metadata/scene/two-view/source-pin
comparisons precede exact192-byte pose-row copies for the unchanged repeated
native matrix. Actual P1000 imagery/callouts use000-assembled and001-exploded.
Fresh processes do not imply flushed caches or statistical full-workflow speedup.
Both scripts' CLI help checks passed on the fleet after syncing those scripts
only. No benchmark/raster ran, and no qualification-pinned product/helper/native
file or binary changed. Root diff review and complete nonvisual PASS plus pilot
acceptance remain required before actual measurement.

The operator additionally requires the genuine NVIDIA USD/BRep CPU baseline.
Studied the current native display gap and proposed an isolated reference-only
raw indexed-mesh CPU executable in
papers/2026-10-15-om10-reference-display-study.md. Explicit BGREFM01 XYZ/normals
replace carrier input; unchanged projection/setup/coverage/shading math and a
matching multithread tile schedule supply common owned display. No fake carriers,
owned/OCCT triangle substitution, product refactor or viewer-as-kernel claim.
Reference owner confirmed definition-local NVIDIA geometry and actual per-corner
normal availability; position/normal precision and returned polygon triangulation
still need the actual API/extraction contract resolved. Wrote design and bounded
candidate sequence for root review only; no new source, build, test or render.

Root and reference owner accepted the isolated raw-display design/subplan after
confirming genuine f64 NVIDIA corner AuxData normals and explicit triangle-only
output. Authored absent-entry fleet red first. Root review caught two fixture
mistakes before a target build: keys initially identified distinct XYZ as the
same physical vertex, and two hypotenuse-boundary pixels were incorrectly
counted inside. Corrected keys and independently derived the non-top-left
hypotenuse exclusion (only pixel0 inside); reran absent-entry red. Shared raster
math was never changed to fit the fixture.

Implemented native/om10_reference only: explicit raw-record decoder/ownership,
proper source placements, unchanged projection/raster/shading math, retained
CPU worker/tile scheduling and raw image/diagnostic output. Repeated physical
keys require identical XYZ within each definition and retain distinct corner
normals. No carriers/evaluation/CUDA/reference-kernel dependency is introduced.
The new namespace initially made finite ambiguous with libc; qualified the shared
function and built the new target. Six final direct methods pass CPU1(0.075s)
andCPU24(0.111s), covering scalar pixels, shared edges/winding/SS2, front/ties,
placements/normals, corner identities, errors and one byte-identical plane
comparison against the existing CPU path. Added an actual two-request retained
pose check in the same focused method; the complete six-method CPU24 ASan/UBSan
run including it passed in0.492s with leak/error halting enabled. CLI help passed;
dynamic linkage lists no CUDA. Reference owner adversarially reviewed all four
source/build files and accepted without findings. Root review remains pending.
No existing product/helper/native source or binary changed; no watch raster,
NVIDIA kernel run or whole-source qualification campaign was performed here.

Root/reference owner accepted the bounded NVIDIA-outcome poster subplan and
direct reference-outcomes.tsv interface. Authored a synthetic temporary TSV/
one-pixel-image fleet fixture first; it genuinely failed because the compositor
lacked the required reference input. Implemented exact heal0/heal1 rows, source/
two-view/scope/n1 checks, finite actual times, explicit failure/unsupported status
without a comparable headline total, and retained original outcome table. The
owned P100 actual image and per-pose native median panel remain separate.

The positive fixture exposed an arithmetic error in our prior reviewed callers:
BGVIEW01 has16 header bytes plus two192-byte records, totaling400, not408. Our
earlier benchmark/compositor/privatepilot length guards and plan text inherited
the wrong total. Actual encoding and offsets184/376 were already correct. Root
and reference owner reviewed the bounded correction; changed only those guards
and current plan wording, preserving all source/helper/native record formats.
This note corrects the earlier review conclusions rather than claiming they had
already exercised an actual two-view file. Three focused fleet methods then
passed in3.076s, covering actual SVG text, four-row footer placement, correct pose
images, retained input table and rejected invalid outcome records. CLI help also
passed. Root static diff review accepted; reference-owner review pending. All
synthetic timings/images remained in temporary test directories, never published
assets/results. No actual watch raster, reference benchmark or pinned-file edit.

Reference-owner code review found one direct timing consistency gap: a supplied
complete wall could exceed its enclosing attempt wall. Added exact complete≤
elapsed validation and one malformed record to the existing negative fixture;
no new mechanism/campaign. Three existing focused methods passed in2.892s, and
the affected negative method passed in2.886s after completed source/test sync.
Root reports whole V3 stopped for a separate importer policy lifecycle issue;
there is no whole-source PASS and no authorization to reuse V3 for a pilot.
Reference owner then accepted the final poster code after the timing guard;
root's static acceptance and focused checks complete this publication unit.

While the source owner designs explicit2D trim conversion, completed a read-only
adversarial audit of pinned OCCT FixSeam/FixShifted and ProjectCurveOnSurface,
including their storage/status/orientation and singularity callees. The bounded
note is papers/2026-10-15-om10-occt-2d-method-adversarial-review.md. Direct selected
operations avoid3D setters, but same-TShape aliases, spherical nonperiodic shifts,
partial flags, projector cache/order and minimal nudged PCurves require explicit
original-t/chart/source-use admission. Broad fixing entry points retain topology/
range/tolerance hazards and are outside that selection. Sent concrete facts to
root/source owner; no CAD, tests or product edits for this review.

Root completed the raw-display code review and accepted the implementation:
explicit checked binary decoding, per-definition physical-key consistency,
proper rigid placement, unchanged shared projection/raster math, retained CPU
scheduling and measured teardown match the reviewed scope. Zero-valued external
keys are valid; the occurrence sentinel is UINT64_MAX, as in the owned entry.
The independent scalar tests and focused sanitizer run already cover the changed
unit; no additional campaign is warranted without a new finding. Actual whole
watch rasterization remains pending complete nonvisual qualification.

Reviewed the proposed explicit source-UV design/subplan against the selected
OCCT method audit. Deep whole-definition detachment, original-t intervals,
true-period checks and explicit pole connectors fit the native contract. Found
one concrete wrapper failure gap: FixAddPCurve ignores projector failure and
dereferences a possibly null seam curve before final readback can reject it.
Sent the exact call sites and a direct checked-projector/2D-attachment option to
root and source owner. Recorded the bounded review in
papers/2026-10-15-om10-explicit-uv-design-native-review.md. Root's separate natural
domain/original-column recovery finding remains open in the source lane. No CAD
execution, new tests, source reconstruction or target raster occurred here.

Source owner resolved the null seam finding in the proposed design: project and
check output directly once, attach the single checked PCurve, then use a fresh
FixAddPCurve only on a true seam with the same support's non-null PCurve already
read back. Inspected HasPCurve/PCurve and projector construction: their matching
lookup and fresh OK status exclude that null projection path and stale DONE3.
Accepted the bounded resolution; mature torus seam handling remains in use.

Final reread accepted the revised source-UV study/subplan: safe projection and
seam attachment, whole-basis true periods, ordered source-use ownership, raw PCs
for existing owned recoveries and a separate narrowly corrected query copy.
Deterministic source bytes exclude timings; existing qualification/report hashes
bind one optional query export, while raw NVIDIA import remains unchanged.
Sent final ACCEPT to root/source owner and remain available for code review.

Started read-only raw source-UV code review while the owner completes its query
phase. Found missing support-alias preflight, pointer-only edge keys, directed
instead of geometric endpoint tolerances at projection, and missing final
retained-PC/face-state checks. The owner is integrating those findings. Located
edge identity and the storage alias key differ: the latter uses edge TShape,
surface handle and relative support location. Also caught a FaceState member
initialization-order error before execution: Surface(f,location) wrote location
before its constructor because of declaration order. Sent the narrow fix.
Inspected selected FixSeam/FixShifted and direct storage callees for root's
working-WireData concern: they do not replace or reorder original edge slots,
and a fresh null-context Load does not apply a replacement map. No extra wire
guard is needed for those exact calls. No CAD/tests/product edits by this lane;
full code acceptance waits for the complete source/query implementation.

Actual source preparation later stopped at the adapter's complete-use check on
a reversed cylindrical face. Independently read WireExplorer Init/Next: it uses
face-composed seam slots and strict chart-space endpoint connectivity, so a
period-wrong slot can omit uses despite passing original-word/C-S probes. The
selected ShapeAnalysis_Edge/FixShifted helpers ignore face orientation, while
CheckSeam/ReplacePCurve explicitly use FORWARD. Told root/source owner that
SetFace(REVERSED) alone cannot repair that convention mismatch; proposed only a
temporary explicitly mapped local-forward fixer traversal for investigation.
Original source word and complete-use guard stay unchanged. Recorded source
facts in the selected-method review. No actual diagnostic execution by this lane.

Reviewed query pole topology and paired branch replacement. The seam replacement
uses edge-orientation XOR face-orientation to select its stored slot, matching
BRep_Tool's composed lookup and BRep_Builder's explicit paired storage. Found
a concrete reversed-face topology error in pole-wire insertion: code reversed
the child before Add, but TopoDS_Builder::Add already computes relative parent
orientation/location. Sent owner/root the double-reversal finding and confirmed
the need to check the actual final face's original-use cycle modulo only declared
analytic pole connectors; geometry-handle/endpoint guards cannot detect a lost
or reversed face wire. The local-forward temporary fixer view now written in
raw code fits the selected helper convention after the owner's genuine reversed
cylinder red. Review lane made no code edits and ran no CAD.

Final reread confirmed raw located identities/storage alias checks, geometric
endpoint tolerances, immediate/final source guards and safe OCCT projection
calls. Query branch replacement, parent-relative wire insertion and copied-wire
mapping are coherent after fixes. The new actual-face quotient checker retains
the original physical word and proves declared connector geometry, but initially
did not check its actual neighboring uses. Sent the narrow remaining finding:
require each actual connector between its declared incoming/outgoing uses, so a
misplaced connector cannot disappear harmlessly during quotient comparison.
Also noted the adapter had not yet emitted query correction counts/perturbation/
connector diagnostics retained in memory. Numerical out-of-domain metric review
remains assigned to source/oracle owners. No additional test campaign here.

Final native code review ACCEPT after rereading the last changes. The actual
connector's preceding/following wire edges now match its declared original joint
by located identity and orientation before the analytic-pole quotient is allowed.
The adapter emits original source-linked connector rows, correction counts and
separately named raw/query residual and query UV-metric maxima to stdout, keeping
deterministic BGSRC bytes unchanged. Root accepted the scoped numerical amendment
with its other reviewers. No remaining native preservation/topology/selected-call
or adapter finding; existing owner-run focused checks suffice for this review.
Actual whole extraction/query qualification and target rendering remain separate.
