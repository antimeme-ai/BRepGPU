# Body sampler runtime and consumer study

2026-10-14. Source study and proposed consumer contract only; no product code,
tests, native execution, target work or dependency adoption. A first body sampler
can reuse ABI1 unchanged: **CPU body records/classification/packing plus one native
first-jet batch, followed by CPU frame/normal/scatter/export work**. It is a body
consumer of the existing evaluator, not GPU topology or a body-kernel ABI.

## Actual reusable API

Read [sample_patch.py](../examples/sample_patch.py),
[Python adapter](../python/brepgpu/__init__.py), [public ABI](../include/brepgpu.h),
[runtime](../native/runtime.cpp), [public runtime tests](../tests/test_runtime.py)
and [native benchmark](../benchmarks/iteration1/benchmark.cpp). Checkpoint hashes:

| Source | SHA-256 |
|---|---|
| `examples/sample_patch.py` | `f39f8872e8d5a7edf9c221b306116dbc24e2adaec35e938cdd7cf33515879df1` |
| `python/brepgpu/__init__.py` | `f4e657f4608feb52721238d4a6051d6750fb86c012803a6df195e09e722f570e` |
| `include/brepgpu.h` | `0fe02dddab7f397526caedf094ca577c948117ace1fd5e518d9262e3c04736e3` |
| `tests/test_runtime.py` | `11893df2fcccf5774458fca85fcaabaef227af9d0e88a56c297ba04bb9a0408f` |

`Engine.create_patch_set` packs and snapshots all descriptors/homogeneous controls;
the same live patch set supports repeated immutable query sets.
`create_query_set(patches, slots, uv)` accepts mixed patch slots and interleaved
normalized binary64 UV. `evaluate(queries, order=1)` emits S,Ss,St in a fresh
resident result. Specify order 1: the default is 2 and unnecessarily computes/
downloads all six jets for position and normal sampling.

`Result.download` allocates owned host arrays, downloads jets/statuses, then reads
retained host descriptors/slots/UV and returns `JetBatch`. `require_ok()` only
checks native query status. `vector(term,i)` uses component SoA stride equal to
the **admitted** query count; it is not an all-body-candidate stride. `lineage`
returns the source tuple per evaluated query. Results and queries retain closed
public parent storage; closing Engine prevents subsequent live result operations.
Downloaded host arrays remain independent, with conversion using a host-only
library function rather than a live engine handle.

The sample example times context creation through download/close, but stops
before analytic checks, point tuple construction and export. It samples one
untrimmed carrier in the default local frame. It is a useful native batch pattern;
its time is not an existing whole-body sampling measurement.

## Small owned CPU body layer

Use immutable logical records and owned packed numeric/index tables. A body
snapshot names body ID/revision; each face use names face/chart ID, patch slot,
explicit orientation ±1 and loop ranges; loops/coedges/vertices retain the chosen
scope from the body geometry design. The [topology document](../docs/TOPOLOGY.md)
remains a proposed complete architecture, not an already implemented graph.
This first sampler should admit a concrete bounded body recipe, not claim the
entire future topology model merely because records share its names.

Frozen Python dataclasses alone do not freeze referenced `array` buffers. Copy
input geometry into immutable tuples or private packed ownership; publish a
read-only view backed by storage callers cannot independently mutate. A public
read-only memoryview over a caller-retained mutable array is not an immutable
snapshot. Keep typed native records and packed arrays; no JSON model or persisted
pointer/handle addresses as body identity.

The native Patch value already freezes flattened controls as a tuple. Native
patch-set creation additionally snapshots bytes. Cache the exact PatchSet for the
same body revision/carrier/frame recipe and Engine. Engine identity is a valid
process-local cache binding, never a persisted entity identity. A new revision
must produce a new sampler/cache; never reuse a same-sized old patch set by count.
No source pointer or coordinate-proximity deduplication should merge seam/edge uses.

## One batch with a retained classification mask

Generate every candidate row in a deterministic order, retaining candidate index,
face-use/entity/chart identity and actual normalized (s,t) bits. CPU classification
writes a label/mask for every row, including interior, boundary/edge/vertex, outside
and any unresolved case admitted by the classifier contract. It does not obtain
trim membership from native status: ABI1 evaluates an untrimmed carrier.

Gather admitted rows into packed `array('I')` patch slots and `array('d')` interleaved
UV, plus an owned admitted-index→candidate-index table. Evaluate once across all
faces and download once. Scatter with that explicit mapping; do not assume face
chunks, dense labels or candidate count equal admitted count. Preserve repeated
boundary samples belonging to different face uses; averaging normals or welding
by position changes the operation and is outside this proposal.

Store output XYZ/normals/labels in candidate order. Outside/unresolved-classification
rows remain present with NaN geometry and their explicit classifier label. They
were not native evaluation failures; represent absent native status explicitly
with a separate applicability mask/sentinel, never status OK with fabricated XYZ.
Each evaluated row also retains actual native status and complete lineage
`(body_id, entity_kind, entity_id, revision, chart_id)` plus face-use/orientation
identity when that use is distinct from the face entity. Native patch slots are
snapshot-local routing, not face identity.

All-outside/empty bodies should produce the specified empty/admission-masked
result without pretending work ran on the GPU. Either skip the native query or
use the API's valid empty batch consistently and disclose its cost. Allocate and
validate shape/count products before gathering large rows; route slots through
the existing uint32 query contract and count limits chosen for this consumer.

## UV, common frame and oriented normals

Native inputs are normalized s,t in [0,1], irrespective of Patch.uv metadata.
That metadata is the original source UV rectangle. If trim loops live in source
parameters, classify there using an explicit parameter map and send normalized
queries; feeding source U/V directly to native evaluation is wrong. A simpler
first owned body defines charts and trims in the same normalized UV domain.
Keep generated normalized bits as the authority and avoid repeated remapping at
endpoints. Reversed source rectangles cannot encode face orientation: conversion
requires positive source widths.

The existing frame is only `X=origin+scale*S`, not a rotation/general affine map.
The simplest first body places every carrier's local XYZ controls in one common
body frame, with the same positive uniform origin/scale adapter. A rigid face
placement must be baked into those admitted controls or supplied by an explicit
reviewed CPU transform; metadata origin/scale cannot express it. Controls must
still satisfy the local cube/weight envelope.

`JetBatch.convert()` explicitly converts position to world coordinates and
derivatives to **source UV** units. Its factors are scale, scale/du and scale/dv;
it copies input/output arrays and runs host-only conversion. Supported scale/du/dv
are [2^-20,2^20], finite origin components are at most 2^40, and local jet magnitude
must satisfy the conversion envelope. Patch creation only requires finite metadata
and can succeed when later conversion returns UNSUPPORTED. Validate the sampler's
common frame/source widths at admission or surface conversion status honestly.
Huge origins also make world-position error depend on their binary64 ulp; local
evaluator error targets cannot silently become a universal world-space tolerance.

For a common positive uniform scale and increasing source UV rectangles,
`converted_Su × converted_Sv = scale²/(du*dv) * (local_Ss × local_St)`.
The normal direction is unchanged. Compute an oriented unit normal from local
first jets and multiply once by the face-use orientation ±1; translation does not
act on derivatives. Trim-loop winding is a separate boundary-use relationship,
not another automatic sign reversal of the surface normal. If future transforms
include rotation/reflection/shear, their derivative/normal rule needs a distinct
contract; do not reuse the uniform-scale argument.

Use a scaled cross product to avoid overflow/underflow in normal construction:
divide each tangent by its maximum absolute component, cross the scaled tangents,
and normalize with `math.hypot`. Require finite positive tangent scales and a
finite nonzero cross norm. A successful native evaluation can still be a singular
or nearly parallel chart. Give such rows a separate normal-unresolved reason
with NaN normals; XYZ can remain valid if that publication contract permits it.
Never normalize zero or substitute an arbitrary axis. A declared angular/rank
floor is a consumer acceptance rule, not a theorem that the empirical absolute
first-derivative allowance guarantees a universal normal error. For the first
body use independently known regular carriers and qualify oriented normals
against analytic directions. Seam/corner normals are per face use, not a unique
smoothed body normal.

## Failure and ownership behavior

A sampler should clearly borrow an Engine or own one. Borrowed Engine close is
not a sampler responsibility; sampler close releases its cached PatchSet exactly
once. Query and resident result handles remain scoped per call, with download
before Engine close. Reused immutable queries are optional for an explicitly
unchanged sampling mask/grid; they must not survive a revised body recipe by count.

Existing wrapper creators protect native publication through Python constructor/
interrupt failures. Context managers preserve a primary exception and attach
cleanup information. Reuse those guards; do not invent unguarded ctypes handle
ownership for the sampler. On batch/backend/download error publish no new body
sample artifact, release temporaries, preserve the previous immutable body/cache
where healthy, and surface the diagnostic. A failed backend download can partially
write destinations; the wrapper raises before returning those arrays. Do not
recover from it by treating zero-filled arrays as valid samples.

For a first known-valid body, nonzero native status in an admitted row should fail
the call with candidate-row/face/UV diagnostics. A later explicitly partial result
may retain a native-failed row with NaN fields and a reason, but must not silently
drop it. Normal-unresolved and classification-unresolved are separate geometric
labels, not backend success/failure codes. Fatal CUDA failures poison CUDA work
process-wide; no automatic CPU substitute inside the same reported GPU call.
An explicitly selected CPU attempt can be a separate operation with its costs.

## Honest cold/reused caller scopes

Measure actual whole-call wall time and retain phase timestamps; do not construct
caller time by adding independent medians. Publish candidate/admitted/normal-valid
counts alongside backend, CPU worker count, actual body recipe and classification
scope. Use the same candidate order/mask/inputs on CPU and CUDA, with independent
analytic/exact checks for the owned body. Attractive geometry or CPU/GPU parity
alone is not a trim, orientation, closure or normal oracle.

| Phase | Concrete measured work |
|---|---|
| Input/body preparation | Record validation, immutable copies, common-frame carrier packing and lineage |
| Candidate generation | All face UV rows, labels/indices and initial output allocation |
| Classification | CPU trim/boundary decisions over every candidate |
| Gather | Admitted slots/UV and explicit candidate-index mapping |
| Engine/patch setup | First-use context plus Python packing, host validation/snapshot and CPU storage or CUDA upload |
| Query snapshot/upload | Wrapper copies, native snapshot/validation and device transfer where applicable |
| Native first-jet evaluate | Fresh result allocation, CPU/CUDA jets, completion/restoration and handle publication |
| Python download/provenance | Owned array allocation, jet/status download, metadata/source UV/slot copying and JetBatch construction |
| Frame/normal/scatter | Explicit host conversion if used, oriented normal checks and all-row output assembly |
| Cleanup | Query/result and owned sampler/context destruction as named |
| Export | CSV/PLY/SVG or other requested external files, separate from no-export caller time |

The high-level `download()` call is not a pure PCIe timer; its host allocation and
provenance calls are inseparable in that existing public wrapper. The evaluate
timer also includes a Python wrapper/handle boundary and native synchronization.
Call them what they measure rather than labeling them a private kernel/transfer
component. CPU24 changes native evaluator workers, not the serial Python body
classification/normal loops.

**Cold caller** creates/validates body/Engine/PatchSet, generates and classifies,
evaluates/downloads/consumes/scatters, and closes owned resources in its declared
wall scope. **PatchSet-reused caller** keeps Engine/PatchSet but regenerates,
classifies and uploads its new admitted query set. **Query-reused evaluator** also
retains a fixed grid/mask/query and measures a narrower repeated resident operation;
do not rename it general body sampling. Export-inclusive time is an additional
actual whole-call observation, not a sum of the export median and another median.

The initial delivery can emit all-row classification TSV plus admitted XYZ/normals
and lineage-aware point output and face-grid visualization. Masked paths must stop
at exclusions rather than draw connectors through holes. Those points/grid lines
do not establish a chord-certified mesh, watertight triangulation, point-in-solid
classification, body nearest point or GPU topology operation. The useful beachhead
is a concrete body-aware caller with honest CPU/GPU boundaries and independently
checked identities/orientation that can grow without changing the native jet ABI.
