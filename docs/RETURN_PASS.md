# Integration and performance after the broad pass

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
