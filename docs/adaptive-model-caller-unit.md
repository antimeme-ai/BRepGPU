# Complete source assembly to semantic artifacts and two images

2026-10-17. Design and literal finite plan, reviewed ACCEPT before body. Root owns this caller;
singular construction/runtime are separate reviewed dependencies. The purpose
is to run the native pipeline and publish its complete outcome, not to add a
new geometry algorithm or use old NativeMesh/GeometryStore as an intermediate.

## Sources and operation

Read adaptive_component_main.cpp::run/export_query, adaptive_assembly.hpp,
adaptive_scene.cpp, main.cpp::Display/labels and presentation.cpp. The underlying
algorithms and original-source assumptions remain those documented in the
cached-adaptive mesher, component and scene plans; this composition adds no
numerical claim from an upstream reference. The pinned USD/BRep source/reference
comparison remains a separately executed external operation. Reference source
evaluation, source envelopes, quality transport and rendering remain distinct
costs rather than attributing all elapsed work to a GPU.

`brepgpu-adaptive-model` defaults to all DISTINCT PLACED original definitions
from AssemblyStore.instances, with every face/use/edge of each. Optional repeated
`--definition ID` selects an explicit complete-definition subset, retaining only
its actual original placements and declaring selected_components scope. No
invented identity placement or skipped failing face. Definitions with no faces
refuse visibly. Never derive this scope from a filter of reconstructed source.
Decode unchanged original bytes once; prepare full-source envelopes once.

Compile the sorted unique selected carrier closure using exactly the frozen
AdaptiveAssembly carrier profiles. Compile and retain one immutable curve bank
per complete selected definition from its original curve requests. Actual source
read of AdaptiveBoundarySession::open_device shows it uploads its entire supplied
bank, while Component constructs that service per definition. A global union
would repeat all-assembly uploads for every definition. Per-definition banks
avoid that work; genuinely shared curve sources can incur duplicate host
extraction, which remains accounted for. Runtime peer accepted this amendment
before its implementation. Retain one surface bank and one bounded
ComponentMesher across canonical definitions and quality queries. Current
regular-only banks remain legal; quotient banks require the new produced-domain
runtime and cap-module handshake. Factory choice follows geometric admission,
never source identity. Before quotient integration lands, singular inputs must
refuse, not fall back to a legacy producer or remove source faces.

One NativeScheduler handles parallel face work; complete definitions run in
canonical order on its bounded face-owner pool. This avoids nested schedulers
or whole-definition parallelism multiplying resident banks. Later quality
queries import retained original definition envelopes and rerun prepare_assembly
without rescanning source geometry. Use exact local allowances directly, not
rounded/reparsed TSV. A fresh physical forest per component query preserves the
existing canonical coarse/fine policy. The curve service remains cold per
component; an immutable union curve bank is not GPU-resident warm reuse.
Keep all complete result leases required
by the final scene within the aggregate memory cap; exhaustion aborts the whole
transaction rather than reducing scope.

## Shared output and display seams

Extract only the existing component export routine/stream close/exact text
helpers to adaptive_component_io.hpp/.cpp. Existing component CLI calls that
same implementation. Model output is query-N/definition-ID plus a top-level
face index retaining the FULL relative artifact path and exact captured boundary
pair. Emit original bytes once, requested/exact local caps, canonical inventory,
per-definition counts/time/work/transfers and source/backend/scope metadata.
Source names/paths stay in private outputs; tracked reports contain aggregates.
Mapped-facet writer versioning must retain its explicit map/domain/source roles;
the old ordinary format must refuse such facets until that extension exists.

Extract existing DisplayModule/DisplayState/Display to native_display.hpp/.cpp
using `shared_ptr<const SceneBuffers>` instead of NativeScene. Shared-pointer
conversion retains the actual producer/deleter and its complete semantic leases.
Keep SAME preregistered state/module/token, failed-create handling, final failed-
drain self-retention and buffers. The old main forwards to it; do not duplicate
the teardown protocol. Module selection is explicit CPU API2 vs supplied P100
display module, with no fallback. CPU mode loads no CUDA module.

After all requested components finish, build AdaptiveScene, then make_presentation
from its SceneView. Display owns an instance copy carrying presentation offsets;
source matrices/actual geometry stay unchanged. Write scene/views, render both
assembled/exploded images, label exploded occurrences with the existing numbering
and escaped original BOM/path TSV from AssemblyStore. CPU/P100 use SAME geometry
quality/camera/lighting/explosion settings. Selected label i+1 is a separate BOM
column from original occurrence original_instances[i]+1; explicit subsets can
leave gaps in original occurrence IDs. A subset with no actual placement refuses.
Retain mapped cap normal descriptors
through the separate cap display extension before admitting cap scenes.

Captions state backend/threads, source+assembly preparation, complete meshing,
display startup/render and quality. Timings.tsv separately records semantic
export, labels, PNG/annotation, service/display close and complete prepublication
wall time. Internal display stage counters are nested, not summed twice. External
process wall time and independent source-reader cost are reported separately.
Do not put an external NVIDIA number on an image before its matched run exists.
Close all numerical/display owners and output streams before OutputDirectory's
no-replace publication. Failure/cancellation emits stage/error and no final dir.

## Order, direct RED and finite closure

Review this design/plan before body. Capture actual missing model executable RED.
Extract shared export/display and retain existing component/legacy paths. Implement
regular complete-assembly caller first; quotient admission remains visibly refused
until connected. Direct authored complete two-definition/three-occurrence scene
uses distinct source styles and original placement, plus explicit subset scope.
Reuse SAME original component/scene readers for every actual face, exact paired
snapshot, displayed actual XYZ, normals, original placements/materials and full
scope. Add process tests for original bytes, inventory, exact local quality,
both image outputs/BOM references, no-replace sentinel and mid-assembly refusal
with no published directory. Test one cold and repeated-quality native process;
CPU1/parallel and both P100 placements use those same inputs/readers, not a new
geometry oracle. One existing component caller and legacy display consumer cover
the extracted interfaces; scoped host lifetime/static checks cover the new owner
seam. Stop after direct claims answered.

Then connect the produced cap schema/writer/display extension, run a complete
source-backed singular definition using the SAME cap/ordinary original readers,
and proceed to bounded real watch runs. Full watch acceptance requires all
107 original definitions/9,162 faces/166 placements under its actual source scope,
assembled/exploded labels, CPU/P100 outcomes and matched NVIDIA reference outcome.
Unsupported/failure/censor remains non-green and visible. No whole-watch claim
from the smaller assembly fixture or first imported component.
