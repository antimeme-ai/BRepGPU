# Integration and performance after the broad pass

2026-10-15 status: the planned breadth sweep is complete, and the matched NCSX
comparison below is delivered. Runtime/language work is now eligible for
selection. Read the [stocktake and proposed tranche](POST_BREADTH_ROADMAP.md)
before selecting a unit. Earlier dated notes below preserve the original
deferral and measured opportunities; they do not reopen completed themes or
select every recorded optimization.

## Required after the watch: NVIDIA CPU references throughout the README

The operator requires NVIDIA USD/BRep CPU reference times for **every scenario
presented in the README**, after the complete OM10 watch pass concludes. The
watch itself already includes that external baseline alongside owned CPU/P100.
This follow-on tranche takes priority as an explicit obligation; it does not
start while the watch delivery is active. Tracked in
[issue63](https://github.com/antimeme-ai/BRepGPU/issues/63).

Enumerate all advertised scenarios against the README revision and run their
actual NVIDIA counterparts with comparable original sources, units, placements,
quality, queries/operations and output scopes. Record version/hardware, input
translation, complete caller and meaningful stage times, repeats and ranges.
Identify shared display machinery. Keep missing adapters, unsupported operations,
source/quality disagreement, failures and timeouts visible; they do not supply
successful reference ratios. Do not quietly shrink the inventory to supported
examples. Deliver a scenario-to-reference-results map and scoped timings beside
the corresponding README claims. New README scenarios retain this obligation.
This benchmark work is separate from the community-suite alpha gate.

## Earlier sequencing notes

2026-10-14. The operator explicitly placed setup/teardown, residency reuse and
making the application pleasant **after the big rocks of the breadth-first
pass**. Record opportunities now; do not start their implementation or a new
profiling campaign before those themes are delivered. Correctness fixes and a
demonstrated prerequisite blocking the next theme retain the roadmap exception.

The operator also explicitly requests an investigation of **whether Python
should be part of the delivered system at all**, after the breadth sweep.
This is an architecture decision, not a commitment to optimize Python or to
remove it. Compare an owned native application/core, an optional thin Python
binding, and Python orchestration against actual modeling consumers. Include
whole-operation latency, cold preparation, memory and ownership, exact arithmetic,
failure handling, interoperation, scripting needs and development cost. Python
may remain useful for research and independent qualification even if it does not
belong in the shipping execution path. Select a role and a concrete migration
sequence only after that study; current beachheads keep their existing APIs.
See the [investigation brief](../papers/2026-10-14-python-role-return-pass.md).

Tracked in [issue19](https://github.com/antimeme-ai/BRepGPU/issues/19).
Complete analytic intersection and spatial/bothUV representation, plus the
GPU edge-use adjacency/mate-reader beachhead, are delivered.
Classification, bounded planar arrangements and complete canonical meshes are now delivered. Finish
narrow model interchange and the other uncovered themes before this return pass.

## GPU lifetime and reusable inputs

The [pair results](../results/pairs-2026-10-14/REPORT.md) expose a useful cached
operation: 879,840 pairs take 6.92–7.78 ms on P100 versus 9.63 ms CPU24,
including host mask conversion and decoding. Cold-to-first-page observations
are 640–840 ms on P100 versus 345–393 ms CPU. CPU scene/root/world preparation
alone costs 189–202 ms; body construction is separate. Warm comparisons retain
engine, scenes and indexes. They already demonstrate reuse without measuring
how much each initialization/teardown component contributes.

Actual caller engine cleanup took about 39/80 ms in two differently sized
GPU cases. That span includes native ownership teardown and potentially the
last plugin `dlclose`; it is not an isolated stream-destruction or module-unload
measurement. The native Context owns a stream and checked resource domain,
not a freshly created CUDA primary context. Existing destruction does not call
`cudaDeviceReset`. Attribute costs by direct measurement before selecting work.

Candidate return-pass units, each with separate ownership and measurements:

1. Explicit application-lifetime process and per-device Engine ownership,
   retaining streams and resident inputs across modeling operations. Establish
   explicit shutdown and diagnostics; preserve CPU-independent loading and
   requested-backend failures. Current APIs already support deliberate retention.
2. Value-only canonical carrier/control and local-root reuse, with fresh typed
   lineage for each body. Reuse a world scene/index only for its exact snapshot,
   revision, frame, selection and ordering. Pose/selection changes require fresh
   outward world bounds; shared coordinates or pointer addresses are not keys.
   Never share a GPU handle across context/device ownership domains.
3. Backend/module retention, separately from Engine/stream retention. Keep the
   exact v1 table base identity and optional-extension semantics; do not mistake
   a loaded plugin for healthy CUDA after a fatal diagnostic. Test explicit
   unload/reload and partial-construction failure when this work is scheduled.
4. Reusable output storage or an owned bounded pool. A live result exclusively
   owns its bytes until all work/downloads are complete; a later request cannot
   overwrite them. Preserve fresh immutable host output, close semantics,
   publication failure and primary/secondary cleanup diagnostics. Pool budgets
   and poisoned resources must be explicit. Query P100 allocator capabilities
   before selecting a CUDA allocator feature.

A future focused measurement should distinguish library load/factory probing,
first device/runtime initialization, stream creation, index preparation/upload,
page allocation, kernel completion, download, buffer release, stream destruction
and final library release. Compare fresh process, repeated Engine, retained
Engine/new indexes and fully retained indexes, with matched inputs. Include
shutdown and recovery outcomes rather than hiding costs at process exit.

## Host and other measured return-pass opportunities

* Pair mask metadata/copy/immutable conversion takes roughly 4.3–5.0 ms for
  27,495 words; survivor decoding another 2.2–2.7 ms. Preserve ownership and
  source identity while reducing Python boxing or providing a native consumer.
* The tiny pair case favors CPU1. CPU24 currently creates workers per call;
  persistent workers or explicit routing need whole-caller comparisons, not
  claims based on one isolated kernel timer.
* Body sampling spends most reused time in CPU normals and row construction;
  retain the [body measurements](../results/body-2026-10-14/REPORT.md) for the
  later placement/representation pass.
* Sparse-scene BVH/sweep/grid pruning, tighter carrier bounds and tiling remain
  pair return-pass candidates. Keep complete examined-domain accounting and
  closed touching semantics when comparing them.
* [Intersection results](../results/intersections-2026-10-14/REPORT.md) show
  exact face construction around13–20ms, but preparing16,388 exact candidates
  costs27–32s. Reused world-row/reference construction takes about1.5–1.63s and
  exact grades another0.74–0.82s. Native P100 evaluation is roughly0.11–0.13ms;
  the complete consumer stays2.3–2.5s on all backends. An owned bulk C++/CUDA
  parameter/UV/row consumer is a later candidate, preserving exact endpoints,
  request identity, whole-map/source truth and cleanup. Do not delete grades
  or infer topology from rounded samples to make a timer attractive.
* [GPU adjacency results](../results/adjacency-2026-10-14/REPORT.md) put large
  construction/reader stages around0.32ms ordered/1.4ms shuffled on P100,
  but transfers/allocation/cleanup bring native totals to15–22ms. CPU1 wins
  ordered storage at8.2ms; shuffled CPU24 is19.4ms. Full typed traversal stays
  4–5s, and preparing/permuting sources takes8–18s. An actual downstream native
  consumer and retained storage are later candidates, preserving complete
  occurrence identity and recoverable cleanup. Transfer differences alone do
  not select a NUMA or PCIe optimization. [Source notes](../papers/2026-10-14-adjacency-round2-notes.md).
* [Exact classification](../results/classification-2026-10-14/REPORT.md) now
  delivers CPU material/rectangular-face semantics. Later placement may use
  certified sign filters and exact fallback, or native region consumers. Its
  complete large point medians are365–490ms and512 face requests147ms; independent
  checking is a separate900–1,723ms cost. These are exact construction workloads,
  not GPU/Boolean speedups. Broader algebraic witnesses and curved contacts are
  [separately scoped](../papers/2026-10-14-classification-round2-notes.md).
* [Planar arrangement results](../results/arrangements-2026-10-14/REPORT.md)
  establish exact CPU regions/holes/shared uses. The32×32/64-circle case builds
  in471ms and takes2.40s with actual graph/source-curve/location readers; exact
  geometry reading alone is1.87s. Source preparation is922ms and independent
  checking10.57s, both separate. Record bulk native consumers and certified
  placement for the later language/integration decision; no GPU construction
  or Boolean speedup is established. [Broader geometry notes](../papers/2026-10-14-arrangement-round2-notes.md)
  retain clipped/tangent/cross-face work without starting a refinement lane.
* The existing off-grid separated-bound proposal remains a numerical return
  experiment; it does not displace an uncovered theme without a blocking need.

Application packaging, friendly session management and other integration polish
follow the same scheduling rule. Studies record source-grounded options; they
are not selected dependencies, approved implementations, or an active tuning lane.

Source notes: [GPU lifetime](../papers/2026-10-14-gpu-lifetime-reuse-study.md)
and [scene residency](../papers/2026-10-14-scene-residency-reuse-study.md).

## Meshing after the first complete family

The [canonical mesh unit](MESHING.md) constructs full exact CPU shells and checks
actual rounded output. Native bulk/GPU placement, general trimmed rational faces,
arrangement-region triangulation, adaptive quality and wider seam/pole cases are
future conceptual units. Current corner fans intentionally provide no minimum
angle/aspect-ratio guarantee. Preserve shared physical vertices, distinct corner/
coedge occurrences and both exact/chordal domain meanings in every extension.

Fine plate construction plus complete indexed consumption is757.459ms median
for2,064triangles, with722.285msconstruction and38.534msreader medians; independent
qualification and export are separate. This records a later placement opportunity,
not an active Python rewrite/session optimization or demonstrated GPU speedup.
The continuing NCSX trophy requires actual source geometry beyond the acquired
engineering PDF/photo references; keep geometry revisions and rendering settings
constant across comparison images.

## NCSX scene comparison

The operator deferred this comparison while continuing the breadth sweep.
Study00 did not record rendering/evaluation timing and did not run a matched CPU
geometry pass. Later compare the identical scene on native CPU and P100, separating
patch preparation, context/setup, uploads, native evaluation, download, derivative
normal construction and CPU rasterization/encoding. Time independent correctness
checking separately from the useful caller. Do not describe the current CPU
rasterizer as GPU rendering or infer a performance result from study00.

## Carrier interchange after the breadth pass

The narrow strict ASCII adapter and actual foreign source/native comparisons are
delivered. General multispan extraction, larger models, additional formats, scene
composition, imported trims/solid topology and cross-session naming remain. Keep
original face/use/status and parameter-map guarantees when broadening. Native
SDK extraction would need matching header/wheel ABI qualification; the owned
ASCII subset avoids installing another parser or hiding reference pointers.

Small cold mouse calls measured34.259ms CPU1 versus319.586/710.947ms P1000/1
medians. GPU context setup252.560/547.433ms and cleanup33.920/108.768ms dominated
those actual calls; do not add per-phase medians as a complete median. Study
reuse only after the remaining themes; these are no resident/large-batch results.
See results/interchange-2026-10-15/REPORT.md. No NCSX render timing was undertaken.

## Owned Boolean/history after the strict through-bore

The first owned CPU Boolean/history and its CPU/P100 native consumers are
[delivered](../results/boolean-2026-10-15/REPORT.md). Complete fresh-process
CPU1 medians are174.576–180.908ms across three scaled/translated recipes, versus
710.412–838.043ms on P1000 and1256.376–1278.801ms on P1001. Each recipe has the
same2,890candidate/2,712admitted grid and fixed topology; this is no large-model
or GPU Boolean construction result.

Unit-case Engine setup medians272.909/561.780ms, separate synchronous adjacency
build246.308/460.278ms and Engine cleanup41.840/84.197ms dominate GPU full calls.
The adjacency API owns its own per-call device context/setup rather than sharing
sampling's Engine. These named spans overlap their internal phases, so do not
add phase medians or attribute the entire span to one CUDA primitive. CPU exact
request preparation is45–53ms in that unit recipe, native sample normal/scatter
about46–95ms. Retained native consumers, certified bulk exact decisions and
compact row/history readers are later candidates for the Python-role study,
not active optimizations. Preserve exact query bits, source carrier frames,
all-kind history, source events and recoverable ownership when revisiting.

General/off-axis/partial/contact Booleans, result meshing, persistent naming and
history composition are distinct later units. See the
[Boolean round-two notes](../papers/2026-10-15-boolean-round2-notes.md). Complete
the remaining feature units before starting integration polish or tuning.

## Convex-profile feature construction

Owned convex-profile extrusion is now [qualified and measured](../results/extrusion-2026-10-15/REPORT.md).
Actual 3/6/16-vertex profiles yield 1,445/2,312/5,202 candidates. Best CPU complete
profile→consumed medians are 85.106 ms (CPU1), 190.853 ms (CPU24), 558.940 ms (CPU24),
versus 677.379/822.236/1033.111 ms P1000 and 1157.871/1396.118/1477.176 ms P1001.
These shared-fleet unpinned small cells establish no CPU thread/clock cause and
no GPU feature construction/win. Contexts/cleanup, exact host decisions/history
and row work remain later candidates; prepared-profile and input-points scopes,
profile preparation and independent checking stay separately visible.
Preserve full affine ancestry and exact generating IDs through any native/GPU/
resident migration. [Broader-family notes](../papers/2026-10-15-extrusion-round2-notes.md)
retain holes/concavity/curves/directions and meshing. Finish revolution/fillet/
offset breadth units before beginning those integrations or the Python-role study.

## Full off-axis profile revolution

The [revolution feature](REVOLUTION.md) is qualified on actual source geometry,
periodic topology, history and CPU/both-P100 consumers. [48 complete callers](../results/revolution-2026-10-15/REPORT.md)
measure 972/1944/5184 native candidates. Best CPU medians are 53.963/161.197/
361.477 ms, versus P1000 658.223/746.626/1008.083 ms and P1001 1172.783/
1270.603/1754.468 ms. All complete GPU ranges lose the best corresponding CPU
range; unpinned CPU variability supplies no causal thread-scaling result.

Keep exact source-map evaluation inside actual operation costs. Any resident/
native migration must preserve two seam occurrences on one face, both closed-ring
endpoint ports, precise contextual ancestry and nonuniform rational quarter maps.
Partial angles/poles/axes, curved or holed profiles and result meshing are distinct
families. [Round-two notes](../papers/2026-10-15-revolution-round2-notes.md) retain
these constraints and the non-green optional foreign triangle volume comparison.
Fillet and offset remain ahead of integration, Python-role and NCSX timing work.


## Complete source-edge fillet

The [fillet beachhead](FILLET.md) retains six original plane carriers and exact
changed trim domains, rounded cap arcs, a rational band and two tangent contacts.
[48 complete callers](../results/fillet-2026-10-15/REPORT.md) have fixed seven-chart
geometry:2023 candidates and1987/1871/1791 admitted for three radius/edge/Frame
recipes. Best CPU prepared-source medians are286.904/299.542/399.260ms, versus
P1000 974.977/817.285/836.671ms and P1001 1442.017/1531.336/1337.354ms.
Every P100 complete-call range loses its corresponding best-CPU range. These
are small fresh-process calls, no large-model or GPU constructor claim.

Mid-case Engine setup medians279.024/579.819ms, separate adjacency build
229.018/506.416ms and Engine cleanup38.810/85.079ms remain material GPU costs.
Exact host request work is154–247ms in the four mid placements. Unpinned timing
variation gives no causal thread/clock claim; phase medians nest and cannot be
added. Preserve actual binary64 trim decisions, precise source-map owners,
spine-vs-coincident semantics and both contact maps in future native/resident
migration. [Round-two notes](../papers/2026-10-15-fillet-return-pass-runtime-notes.md)
retain multi-edge/endpoint/general rolling-ball and ownership constraints.

All three selected same-world foreign fillet comparisons pass unchanged criteria;
the earlier revolution volume issue stays open and external alpha suites remain
non-green. Analytic offset is next before integration, tuning, Python-role or
matched NCSX rendering timing work.


## Signed analytic offset and completion of the feature sequence

The [sharp-cylinder offset](OFFSET.md) closes the planned feature breadth sequence,
with exact changed carriers and actual original-carrier normal feet. [48 complete
calls](../results/offset-2026-10-15/REPORT.md) have the same23-entity/six-chart graph,
1734candidates and1550admitted rows. CPU1 medians90.472/86.931/84.916ms beat P1000
784.377/586.370/638.930ms and P1001 1149.549/1359.455/1180.092ms. Every P100
complete range loses CPU1. Three signed-distance/Frame recipes are not larger CAD
models; observed CPU differences supply no controlled thread/clock explanation.

For outset, P1000/1 Engine setup258.204/637.740ms, separate adjacency build
210.926/521.023ms and Engine cleanup43.278/85.475ms dominate. Exact request work
is28.717–31.679ms across placements, while actual sample/normal/scatter consumed
wall is32.448–39.872ms. Phase medians nest and cannot be summed. Preserve actual
querybits, source-foot effective trim versus carrier extension, rawface versus
normalizedchart domains, source revisions and closed-ring/seam occurrences when
considering retained/native ownership. [Round-two notes](../papers/2026-10-15-offset-return-pass-runtime-notes.md)
keep general joins/offset singularities and future timing/language questions.

Two foreign offsets fail unchanged volume/boundary criteria; the third is blocked
before offset by original source-volume preflight. Issue48 and the source study
retain query tolerance/integration/carrier-path uncertainty; issue43 remains
separate. No required community alpha corpus is green. Revisit the theme map,
external oracle gaps and actual full-caller costs before selecting return-pass
priorities. This delivery starts no integration/tuning/rewrite or NCSX timing.


## NCSX matched rerun after the breadth pass

The user requested an actual NCSX rerun. [Matched current CPU1/P1000 runs](../results/ncsx-rerun-2026-10-15/REPORT.md)
reproduce original P100 controls,jets,scene and image byte-for-byte; CPU geometry
pixels match too. The new feature constructors do not participate in this fitted
carrier display. No aesthetic/source/grid change was made to imply improvement.

Evaluation wrapper measured16.310ms CPU1/1.430ms P1000, while cold native
setup-through-cleanup is119.862/570.840ms. P100 Engine/native-pin setup279.585ms,
patch setup91.623ms and query setup112.050ms dominate; each includes existing
wrapper validation/packing and actual native operations. These are unprofiled
walls, not isolated CUDA primitive measurements. Reuse is a future scoped workload.

Actual CPU projection/shading/depth raster/encoding/writes take9.479/9.125s;
independent exact Fraction checks take28.755/28.062s. Fullqualifiedcaller is
41.024/40.379s, not an established whole-render GPU speedup. Verification cost is
separate from production display work and cannot be silently dropped from a
reported wall. One observation per placement supplies no variability estimate;
original study00 was untimed. Native residency, host packing, CPU display and
source-to-owned CAD integration remain candidates for separately designed work.
This requested rerun adds measurement and comparison, without starting tuning
or changing the Python-role decision.


## First result-meshing connection

Actual strict through-bore CutResult meshes are now [delivered](CUT_MESHING.md),
with exact/rounded complete domains, shared physical boundaries and retained
original history. [Four CPU caller cases](../results/cut-mesh-2026-10-15/REPORT.md)
measure complete prepared-result mesh+indexed+typed-history medians37.965,
220.970,433.318 and234.100ms for32/528/1040/528triangles. Boolean construction
(~54–57ms), first independent qualification and export are separate. These vary
resolution/frames, not Boolean complexity; no GPU or old/new speedup is shown.

This closes only the first result family. One feature-result mesh, an actual
admitted operation chain, external adapters and runtime/Python study remain.
Native bulk consumption, general triangulation and output quality retain their
own future contracts. No tuning or language migration was started by this unit.


## First feature-result mesh

The [actual convex extrusion mesher](EXTRUSION_MESHING.md) is delivered: exact
planar faces, original result/profile/history and shared boundaries, with actual
rounded-prism/error checks. [Three CPU callers](../results/extrusion-mesh-2026-10-15/REPORT.md)
measure mesh+indexed+typed-history medians35.363/28.525/107.844ms for8/20/60triangles.
Only three repeats per case, shared uncontrolled host, no scaling or speedup claim.
The largest history reader median61.942ms exceeds meshing41.470ms; retain this
actual consumer cost for the runtime/language study rather than tuning it now.
Feature preparation, independent checking and export are separate. Direct tests,
one affected compatibility run and actual caller review completed; no additional
campaign is scheduled. Further feature families and a real operation chain remain.

## Actual modeling chain and openwork watch study

The first [actual extrusion/bore/mesh chain](FEATURE_BORE.md) is delivered with
original source Frames, real cap-hole domains and ordered immediate/extrusion
Origin paths. The [watch study](../results/watch-2026-10-15/REPORT.md) constructs158
separate solids,23 pierced, with3,152 vertices and5,764 triangles. No template
reuse or GPU call participates. It supplements the fixed NCSX comparison scene.

One final CPU caller measured construction8.719s, mesh4.868s and indexed/actual
history consumption6.581s (enclosing useful wall20.168s). Independent checks29.501s,
part export6.600s and CPU oblique/top display10.017/10.292s are separate;
fullcaller76.674s includes them. Timings are a single shared-host observation,
with explicit overlaps/exclusions in the report; no scaling/speedup estimate.

Source/history reads already exceed meshing in this scene. This is concrete
input for the requested Python/native-role and runtime-ownership study: retain
map/domain/occurrence identity while comparing bulk typed reads and actual
complete fresh/reused workflows. No bulk-reader rewrite, GPU setup tuning,
native migration or cache was selected here. Further modeling work may need
multiple holes, nonconvex profiles, interacting cuts and compound bridges;
functional tooth forms, clearances, springs and kinematics need their own actual
contracts.17 direct methods,58 affected CPU compatibility methods and all158
actual mesh qualifiers passed, followed by source/output/image review. Stop
without an additional campaign; external alpha qualification remains incomplete.

## Whole OM10 implementation observations

The supplied real movement is now exercising owned imported-solid meshing;
complete-source qualification and trophy rendering are still pending. Early
single-face diagnostics produce80k–399k UV cells on several large carriers.
The first complete attempt spent about42s sampling original shared edges and
reached100 of9,162 faces at253s. These are development observations under
concurrent diagnostics, not quiet benchmark results or an end-to-end estimate.

After complete coverage and actual images, study refinement that uses local
curvature and triangle shape to reduce unnecessary cells while preserving shared
boundaries, holes, original UV ownership and the same approximation components.
Compare it on the actual expensive faces; do not judge by a lower triangle count
alone. Record the exact-UV versus rounded-native-UV distinction in any rewrite.

CPU STEP translation and owned trim meshing remain common preparation for both
display backends. A retained owned mesh could remove this work from subsequent
view/explosion updates, while a fresh source-to-image caller must still report
it. Native view batches already retain actual geometry symmetrically on CPU and
GPU. Further mesh persistence, preparation acceleration and language changes
remain deferred decisions supported by the completed target's measurements.

Later complete-face diagnostics made initial connectivity a correctness blocker:
ordinary rings and extruded spirals exhausted the finite construction budget.
The implementation therefore adds constrained metric Lawson flips before
refinement, preserving all initial samples and unchanged error acceptance.
Face170999 now produces2,048 cells instead of exhausting the million-interior-
vertex allowance. This is a development geometry result, not a benchmark.
Further density/shape tuning still belongs to the return pass.

Investigate tighter original-curve/inverse angular derivative bounds after
delivery. A circular source edge on a cylinder can have nearly affine angular
parameters, while separate norm bounds lose cancellations in atan2 derivatives
and create many shared boundary samples. Derive bounds from the actual retained
coefficients and dual frame; do not assume exact orthogonality, snap angles or
skip source knots. Compare complete preparation/render cost and unchanged
source residual/mesh quality, rather than triangle count alone.

The [connectivity reading](../papers/2026-10-15-om10-return-pass-connectivity-reading.md)
records direct constrained triangulation and compact triangle/neighbor storage
as measured candidates after delivery. Current fixed-vertex Lawson correction
is required for completion; this subsequent preparation work remains deferred.
