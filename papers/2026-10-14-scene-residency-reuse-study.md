# Scene residency and reuse: deferred return-pass notes

2026-10-14. **Defer this work until after the breadth-first pass over the major
themes.** These are source-design notes, not an implementation proposal for
the current increment. Reviewed the delivered
[pair report](../results/pairs-2026-10-14/REPORT.md),
[body factory](../python/brepgpu/body.py),
[scene consumer](../python/brepgpu/pair_scene.py) and
[identity rules](../docs/TOPOLOGY.md). No concrete report-record mistake found.
Caller totals, chart exclusions, measured ratios and timing/transfer scopes
agree with retained artifacts.

The measured 128-body-per-side recipe spends roughly 145–163 ms constructing
both sets of canonical snapshots and 189–202 ms preparing both CPU scenes.
The latter includes the temporary CPU context, actual depth-zero roots,
download/cleanup and outward world conversion; it does not isolate any one
component. CUDA cold context creation takes about 275/480 ms in those cases.
Reused native page **host spans**, including allocation/synchronization and
publication, are .144/.125 ms; whole shortlist calls are 7.776/6.920 ms.
Mask download/immutable conversion and decoding are separate remaining host
costs. Five shared-host samples suggest targets; they do not establish the
benefit of a particular cache or teardown change.

Existing reuse is already useful: retain the exact `PairScene` objects,
`PairDiscovery` and borrowed Engine across queries. The consumer uploads each
side once and does not rebuild snapshots or bounds per page. Different poses,
selections or fresh contexts currently require new scenes/indexes. Reusing a
page for changed inputs is not an optimization of that contract.

| Reusable value/resource | Required key and binding | What invalidates reuse |
| --- | --- | --- |
| Canonical geometry/topology template | Owned recipe/version and exact parameters; geometry/control bits separate from body namespace | Changed dimensions, control order/weights, chart convention or topology recipe |
| Full-carrier local bounds | Degrees and exact homogeneous control bits, normalized full domain, bound-builder contract; retained actual enclosure | Geometry or bounding contract change; an unsupported setup environment must not silently become admitted through a cache hit |
| Common-world bounds | Qualified local endpoints plus exact origin/scale bits | Frame change, local-bound change; later rotation/affine support needs its own transform contract |
| Scene source/order map | Typed body/revision/face/chart lineage and ordered selected carriers, bound to exact snapshots | Revision, selection or order change; equal counts are insufficient |
| Uploaded native index | Exact ordered world-box bytes and the same live context/backend/device, with explicit ownership | Context close, device/backend change, CUDA poison, failed upload or index close |

Equal geometry does not make two bodies the same entity. A future owned
template may share immutable controls, local roots and body-independent packed
tables across box/cylinder/plate instances. Each instance still needs its own
body namespace, frame, lineage-bearing Patch descriptors and source records.
Box and plate cap carriers can even share identical geometry while retaining
different trim semantics. Shared local bounds never permit shared trim labels,
face identity or editable topology. Curve/chart maps and sampled normals retain
their own occurrence/orientation bindings.

For a later explicit residency owner, prefer a bounded session that retains
scenes and leases indexes over a hidden global cache. Numeric source keys are
typed and revisioned; no address or pointer serves as persistent identity.
Control/world-box bit keys preserve signed zeros and compare complete values
rather than treating a digest as proof of equality. A geometry cache may omit
lineage only because its output is geometry, then rebind that output to the
new exact snapshot. Identical boxes in another scene cannot silently reuse the
old scene's source map or Cartesian flat indices.

Publish cache entries only after complete construction, enclosure checks and
successful upload/cleanup. Partial failures leave no usable entry. Keep old
immutable scenes/results valid while separately creating new revisions;
unsupported/transient/fatal failures do not become geometry-cache hits.
Context close invalidates native leases, fatal CUDA state prevents further
CUDA calls, and explicit shutdown attempts all owned closes while preserving
the primary failure. Define bounded retention and eviction before sharing a
native index: the present consumer owns and closes its indexes, so two
consumers cannot simply borrow the same public handle without a lease owner.

After the broad pass, judge a return pass by full repeated scene-to-task wall
cost for real downstream consumers, separating cold creation, geometry-template
reuse, changed pose/selection, index reuse and output consumption. Persistent
engines, template/local-bound reuse and less host boxing are distinct changes;
no setup savings or cold/reused break-even is claimed without those measurements.
None is a prerequisite for the next narrow intersection beachhead.
