# Body sampling beachhead implementation plan

2026-10-14. The corrected [design](BODY_SAMPLING_DESIGN.md) passed independent
topology, oracle and runtime review. This plan implements one conceptual body
consumer, with independent ownership of body data, execution and truth checks.
No native source, C ABI, dependency or CUDA toolchain change is required.

## 1. Direct tests before production implementation

Write `tests/test_body.py` and independent `python/brepgpu/body_oracles.py`.
Expect the public API below and derive truth from canonical analytic formulas,
exact binary64 ratios and explicit fixture topology, not production classifiers,
curve evaluation or CPU parity. Run focused tests on the fleet before product
files exist; the missing operation is the recorded initial failure, not a
fabricated mutation of unchanged native code.

Cover the three expected body graphs, opposed edge uses, ordered UV/spatial
loop closure, circle/seam identity, hole-sensitive Euler and owned packed
tables. Analytic identities establish full curve-to-UV correspondence; finite
native checks cover float evaluation/mapping error. Cover integer-predicate
truth at endpoints/nextafter/cardinal/rounded arc UVs, chart joins, mixed
rejected/invalid/admitted scatter, frame transforms, selected/empty requests,
foreign snapshot rejection, closed handles, result ownership and normals.

One independent `qualify_batch(batch)` routine supports the tests, actual
caller and measured-case initial qualification. It verifies all supplied rows
using exact analytic positions/trim truth and analytic oriented normals,
returns count/error statistics and raises on violation. It never calls the
production trim/curve/surface evaluator as its expected-value mechanism.

## 2. Immutable body records and canonical factories

Implement `python/brepgpu/body.py`. Public creation is `make_body(shape,
frame=Frame())`; arbitrary assembled `BodySnapshot` input is unsupported.
Snapshots and nested records are immutable owned tuples/bytes, not retained
mutable buffers. ID namespaces and graph semantics follow the design.

Interface shared with the sampler/caller:

* `Frame(origin=(0.,0.,0.), scale=1.)` validates/freezes the admitted frame.
* `BodySnapshot`: `shape`, `body_id`, `revision`, `frame`, `vertices`, `curves`,
  `edges`, `coedges`, `loops`, `faces`, `charts`, `patches`, `packed_tables`.
  Tables are `(name, bytes)` pairs with explicit layouts documented in-module.
* `Face`: `id`, `name`, `kind`, `orientation`, `loop_slots`, `chart_slots`.
  Names are top/bottom/front/back/right/left for box/plate, top/bottom/wall for
  cylinder; plate appends wall. Chart records expose `id`, `face_slot`,
  `quadrant` (or -1 for planes), `patch`.
* Vertex/edge/coedge/loop records expose IDs and typed slot relationships.
  Vertices use `position`. Chart slot equals native patch slot.
  Edges use `start`, `end`, `curve_slot`; coedges use `edge_slot`, `face_slot`,
  `loop_slot`, `orientation`, `uv_map`; loops use `face_slot`, `hole`,
  ordered `coedge_slots`. Curves/UV maps expose owned kind/data and
  `evaluate(t)`; their numeric output is approximate to the exact expression.
* `snapshot.classify(patch_slot,u,v)` implements exact submitted-UV membership.
  Constants `REJECTED=0`, `INTERIOR=1`, `BOUNDARY=2`, `INVALID=3`.
* `snapshot.edge_point(edge_slot,t)` returns body-space curve evaluation.
  World conversion is explicit via the snapshot frame.

Curve/map data schemas are flat immutable float tuples: line curve
`(x0,y0,z0,x1,y1,z1)`, circle curve `(radius,z)` centered on the z axis;
UV-map kinds `line` `(u0,v0,u1,v1)`, `circle_plane` `(radius,half_width)`,
`circle_wall` `(v,)`, `seam_wall` `(a,)` with v=t. Their `evaluate(t)` methods
use canonical spatial-edge t; coedge orientation reverses traversal externally.

Use one shared cylinder face with four native charts, closed ring edges and a
two-use seam. Plane and circle control data are copied into existing `Patch`
records with complete `Lineage`. Packed table export describes this factory's
read-only data; no general loader or cross-session identity is promised.

## 3. Bound requests and body-aware sampling

Implement `python/brepgpu/body_sampling.py`, borrowing an existing `Engine`:

* Frozen `Candidate(patch_slot,u,v)`.
* `SampleRequest(snapshot,candidates)` copies candidates into immutable records,
  validates slot membership and row cap, derives `labels` and
  `admitted_indices` once, and retains its exact source snapshot.
* `prepare_grid(snapshot,resolution,face_ids=None)` validates integer range,
  duplicates/unknown faces and count before allocation. Face/chart order is
  canonical snapshot order. Empty selection is supported.
* `BodySampler(engine,snapshot)` owns a single native PatchSet, supports a
  context manager/idempotent close, and rejects foreign prepared snapshots.
* `sampler.sample(request)` creates a fresh native query and order1 result,
  downloads, checks statuses/provenance, converts once, then computes oriented
  normals/scatters. Scope children with existing guarded context managers.
* Frozen `SampleBatch(snapshot,rows,timings)` owns row values and exposes
  `count`, `evaluated_count`. Row fields: `index`, `patch_slot`, `face_id`,
  `chart_id`, `u`, `v`, `trim`, `native_status`, `position`, `normal`.
  Rejected/invalid rows have NaN vectors and native_status=-1.
  `rows` is a tuple of frozen row records; `timings` is a tuple of
  `(phase_name, milliseconds)` pairs, usable as `dict(batch.timings)`.

Canonical plane parameter maps use a=3/4 (box/plate) or 1/4 (cylinder caps),
h=1/2, x(u)=-a+2au, y(u)=-a+2au, z(v)=-h+2hv:

| Face | Position | Cross-product orientation multiplier |
| --- | --- | --- |
| top | (x(u),y(v),h) | +1 |
| bottom | (x(u),y(v),-h) | -1 |
| front | (x(u),-a,z(v)) | +1 |
| back | (x(u),a,z(v)) | -1 |
| right | (a,y(u),z(v)) | +1 |
| left | (-a,y(u),z(v)) | -1 |

Cylinder cap charts use their own a=1/4; wall quarters retain the design's
explicit circle map and ±1 orientation. The independent oracle uses these
maps, not a map reverse-engineered from production control nets. Existing
`Patch.__post_init__` freezes controls through `_flat` into a tuple; retaining
those frozen Patch values introduces no mutable control buffer.

When copying a user-supplied candidate iterable, enforce the row cap during
iteration before allocating beyond it, rather than first creating an unbounded
tuple. Derived labels/indices are computed once after structural admission.

Record host phases `gather_ms`, `query_ms`, `evaluate_ms`, `download_ms`,
`convert_ms`, `normal_scatter_ms`, `child_cleanup_ms`, `reused_wall_ms`.
Reused wall includes gather through child cleanup, retaining only the body,
prepared input, borrowed engine and uploaded patch set. The result is published
only after successful cleanup. Normal/native failures return no successful
batch; preserve the primary failure if cleanup also fails. CPU body data and
normal work must not be described as GPU execution.

## 4. Runnable caller and small measurements

Implement `examples/sample_body.py`: explicit backend/device/CPU workers,
shape/resolution/frame/face selection and a fresh output directory. Export
all-row TSV (hex UV plus XYZ/normals/labels/lineage), admitted point-cloud PLY,
packed snapshot tables, source recipe and an owned SVG with actual samples,
normal directions and boundaries. Do not draw connectors across rejected holes
or call this a mesh. Measure cold sampling, independent qualification, export
and actual export-inclusive wall; scope final timing-file writes explicitly.

Implement `benchmarks/body/run.py` using subprocesses for fresh-case first use.
Run 24 cases (three shapes, resolutions17/65, CPU1/CPU24/P1000/P1001) serially.
One exact-qualified initial/cold result per case, one excluded warmup, five
reused calls. All reused calls retain sampler status/provenance/normal checks;
the first exact check is outside reused timers. Record raw phases and counts,
median/min/max for repeated phases and single cold/preparation observations.
Export is absent from these performance calls and measured by the actual caller.
Do not construct wall latency by adding independent phase medians.

Retain stdout/stderr in ignored context, public numeric artifacts in results.
No fleet copying, builds, qualification or other owned performance jobs may
overlap the measured matrix. Record current native source/binary hashes; their
bytes must remain unchanged. Toolchains and protected prior baselines are not
overwritten. Use nondeleting rsync into the owned checkout, with private/source
archive/build exclusions. Local execution is limited to static text/AST checks;
product tests and actual samples run on the fleet.

## 5. Review, qualification and delivery

Review this plan independently before implementing product code. After direct
tests and implementation, run focused CPU tests, then both P100s under Compute
Sanitizer; the native implementation is unchanged, so do not repeat unrelated
native compatibility/mutation campaigns. Review body topology, exact oracles
and runtime/caller code separately; integrate every valid finding.

Use a small fleet-only set of production host-code mutants attacking exact
hole membership, wall normal orientation, seam labeling and scatter mapping.
Each mutation must be caught by the relevant independent direct test, with its
unmutated baseline passing. Keep mutant files and operational logs private.
Then run actual caller qualification/output and the serial measurement matrix.
Publish a concise report with costs/limitations and README graphics; update
topology/roadmap/API status and issue15. Keep coherent commits on
`feat/body-sampling`, push milestones and merge the qualified beachhead with
commits preserved. Move to another theme rather than optimizing this caller.
