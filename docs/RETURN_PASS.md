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
Finish classification, planar arrangements, trim-aware meshing, narrow model
interchange and the other uncovered themes before this return pass.

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
* The existing off-grid separated-bound proposal remains a numerical return
  experiment; it does not displace an uncovered theme without a blocking need.

Application packaging, friendly session management and other integration polish
follow the same scheduling rule. Studies record source-grounded options; they
are not selected dependencies, approved implementations, or an active tuning lane.

Source notes: [GPU lifetime](../papers/2026-10-14-gpu-lifetime-reuse-study.md)
and [scene residency](../papers/2026-10-14-scene-residency-reuse-study.md).
