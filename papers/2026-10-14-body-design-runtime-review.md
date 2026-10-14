# Body sampling design runtime and consumer review

2026-10-14. Reviewed [BODY_SAMPLING_DESIGN.md](../docs/BODY_SAMPLING_DESIGN.md)
SHA-256 `977f4a7d40b54a45896ba6555b8abd32683956667f8aae5106fa710f96585ce9`
against the [runtime study](2026-10-14-body-runtime-study.md), current ABI1/Python
resources, frame conversion and public lifetime tests. Source/design review only;
no implementation, tests/builds, native/fleet work or dependency adoption.

Accepted design with no unresolved blockers. The proposed CPU body placement and
single native order-1 batch are compatible with the current API. Both concrete
clarifications on per-row diagnostics and prepared-request binding are resolved
in the integrated design below. No need for an ABI change, generic normal solver
or larger body framework.

## Runtime/data placement

Frozen records, recursively immutable tuples and privately owned packed binary
tables are a defensible first CPU body model. The design explicitly distinguishes
that Python topology/classification layer from C++/CUDA carrier evaluation. It
does not claim the existing plugin consumes body adjacency or loops. Body/revision,
face/chart identity, face-use orientation and snapshot slots remain separate.
No mutable array/list is retained as snapshot authority and no pointer-derived
identity or JSON persistence is introduced.

One sampler owns one native PatchSet and borrows its bound Engine. Patch data is
uploaded/snapshotted once; a prepared immutable candidate/mask/gather request can
be reused while each call creates a fresh query/result. Constructor failure must
close a successfully created PatchSet if later sampler initialization fails, using
the existing wrapper resource guards. Close remains idempotent, Engine closure
prevents further live native work, and finalized output rows must own values rather
than handles or borrowed JetBatch arrays. This agrees with current native retained
parent semantics without using garbage collection as normal cleanup.

The grid limit/10-chart cap gives 10*257²=660,490 candidate rows. Admitted count
is independently bounded by it; packed slots/UV use that admitted count. Empty
selection should execute the defined actual empty native batch, so handle/profile/
conversion behavior is not inferred from an early Python shortcut. Order 1 gives
exactly the position and two first-jet terms needed for normals. Scatter must use
the owned admitted→candidate map; native output stride is admitted count, even
when candidate count is much larger. Rejected/invalid rows retain explicit
not-evaluated sentinel and NaN geometry, without converting native failures into
trim labels.

## Frame and oriented-normal scope

All local charts remain [0,1]². Cylinder source rectangles [k,k+1]x[0,1] are
metadata for the lifted atlas and have positive widths1, so conversion factors
are common scale for both tangents. The common frame admits only finite origins
bounded by16 and positive power-of-two scales in[1/4,4]. This lies well inside
current conversion support and requires no general affine/rotation ABI.

Normal direction is invariant under that positive common scale. Apply the
face orientation once to converted Su×Sv; loop/coedge traversal remains a
separate topological relationship. Planar charts have orthogonal nonzero tangents;
cylinder tangents are orthogonal, with circular magnitude bounded away from zero
on each quadrant and vertical magnitude1. Frame bounds keep all cross-product
components finite and separated from underflow. Direct cross/hypot followed by
finite/nonzero checks is therefore practical for this restricted analytic family.
A scaled generic-chart routine is unnecessary for the first delivery.

The design does not infer regularity from BG_QUERY_OK. It supplies regular owned
carriers and raises on unusable normals. Per-face seam/corner normals may differ;
no smoothing or unique body normal is claimed. World-position and normal error
targets are explicit finite-corpus qualification targets with ample frame margin,
not universal consequences of the existing local evaluator budgets.

## Findings and required clarification

### BR-R1 — native per-row status does not include a native diagnostic

The sentence saying every failed status raises "with native diagnostics" is
stronger than the API. `JetBatch.require_ok` raises QueryError containing row/status
pairs. A failed batch/native call raises BgError with stage/backend/flags/message,
but an otherwise successful batch containing a failed query does not expose a
separate per-query diagnostic object.

Correction: surface candidate index, face/chart/UV and native query status for
failed admitted rows; include the actual batch diagnostic when a native call
provides one. Do not invent a native diagnostic or conflate application normal
ArithmeticError with a GPU failure. This is a wording/API correction rather than
a product defect.

### BR-R2 — bind prepared requests before deriving patch slots

The design permits explicit reusable/adversarial candidate sequences but should
state the structural admission rule: bind a prepared request to the same body
snapshot/body ID+revision, validate every face/chart belongs to that snapshot,
and derive patch routing from its validated face/chart tables. Reject foreign
prepared requests and inconsistent face/chart/slot combinations even if table
sizes match. UV-invalid rows may retain INVALID; foreign topology routing is a
request error rather than a silently evaluated row.

This prevents a same-sized different body/grid from being evaluated through the
sampler's cached PatchSet with plausible source IDs. It directly extends the
existing same-sized-patch-set binding invariant and needs no new runtime handle.
Make this explicit before the implementation plan; do not adopt an unreviewed
public caller-provided slot override.

## Timing and transfer counts

The design's transfer payloads are correct: slot4+two UV doubles16=20 bytes per
admitted input, and nine jet doubles72+status4=76 bytes per admitted native output.
Those are numeric CUDA payloads, not total Python download memory traffic or
bytes of body representation. `Result.download` additionally allocates arrays and
copies retained host UV/slots and whole patch descriptors. `convert` copies host
input/status arrays, allocates output/status and calls host conversion. The design
names provenance and conversion separately, so it does not mislabel them PCIe work.

Cold wall includes actual snapshot/request preparation, first context/patch setup,
query/evaluate/download/convert/normal/scatter and declared cleanup. Reused wall
keeps Engine/PatchSet/prepared classification/gather inputs and includes new query
creation through child cleanup. It is intentionally a reused prepared body caller,
not a claim that each repetition regenerates/classifies the body. Input/classification
cost remains visible in preparation, with the reusable condition named.

Export and exact qualification are separately timed, and a separate actual wall
through those steps is reported. No sum of independent medians is used as caller
time; no phase claims pure device time. CPU24 configures native evaluator workers,
not Python parallel classification. The small serial five-sample matrix is
proportional to this first concrete consumer; known independent geometry and trim
oracles supply truth instead of CPU/GPU parity. Point PLY/SVG remain distinct from
mesh/chord/embedding or GPU-topology claims.

## Integrated disposition

Re-read the corrected design SHA-256
`0399143f7aaaa48159df1462a3c4a9264ac7fd8da3d639dde5d2894c17f4fa1b`.
BR-R1 resolved: failed admitted query rows surface candidate row, face/chart/UV
and native status; failed native calls retain their available batch diagnostic.
BR-R2 resolved: prepared requests are bound to one exact snapshot, and supplied
routing references must belong to it; face/chart identities come from checked
snapshot tables, and a sampler rejects a request bound to another snapshot.
Those rules prevent same-sized foreign routing without changing native ABI.
Design accepted for subsequent plan review. This disposition is source review
only and does not assert execution or an implemented sampler.
