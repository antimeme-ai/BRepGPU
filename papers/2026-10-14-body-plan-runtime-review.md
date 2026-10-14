# Body sampling plan runtime review

2026-10-14. Accepted, no blocking findings. Reviewed
[BODY_SAMPLING_PLAN.md](../docs/BODY_SAMPLING_PLAN.md), SHA-256
`e18846ac9f3caa9099df4c43e7a6cec8dc04f2eb25ab78c44b65f4ccd8831b8e`,
against the accepted [runtime design review](2026-10-14-body-design-runtime-review.md)
and actual ABI1/Python resource implementation. No product changes, tests/builds,
native/fleet execution or new dependencies.

The division of work is coherent: immutable analytic body data supplies charts,
classification and source identity; the sampler owns the complete prepared-input/
native-batch/normal/scatter concept; independent analytic/exact oracles verify the
result without calling production geometry as truth. Direct tests precede product
implementation on the fleet. The unchanged native core/plugin needs no unrelated
compatibility or mutation rerun.

## API and binding

SampleRequest owns frozen candidate copies, labels and admitted indices and retains
the exact snapshot. BodySampler borrows the Engine, owns one cached PatchSet and
rejects requests bound to another snapshot before any native work. Chart/face
identities derive from authoritative body tables, not caller labels. Native routing
is snapshot-local; source IDs remain complete body/revision/face/chart lineage.
Explicit empty selection goes through valid empty batch handling.

Two concrete implementation choices clarify the shared API without changing scope:

* Make SampleBatch.rows a tuple of frozen SampleRow values and timings an immutable
  tuple of `(phase_name, milliseconds)` pairs. Callers can use `dict(batch.timings)`
  to obtain a private mutable presentation copy. A frozen dataclass containing a
  mutable dict would violate the output ownership promise.
* Enforce the explicit candidate cap during copying of an arbitrary iterable,
  rather than first allocating an unbounded tuple and only then checking length.
  Grid selection/count validation already occurs before grid buffer allocation.

These were communicated to the coordinator as ordinary implementation details,
not blockers or new review ceremony.

## Lifetime and failures

Constructor admission/preparation should happen before PatchSet creation where
possible; otherwise guard post-creation initialization so failure closes the
created resource. Sampler close releases only its owned patch set and remains
idempotent; it does not close the borrowed Engine. Existing creator/context-manager
guards protect query/result publication and retain the primary error if cleanup
also fails. Successful SampleBatch publication occurs only after child cleanup.

Each call makes a fresh QuerySet and order-1 result, downloads/checks statuses and
source provenance, converts exactly once and computes normals/scatter on CPU.
Native-failed admitted rows produce a call failure with candidate/source/status
details. Normal failure is an ArithmeticError, not a fabricated backend diagnostic.
No hidden fallback or partial successful batch is introduced. Final row tuples
remain readable after sampler/Engine close and do not retain live native handles.

## Timing and qualification

The eight named sampler phases separate gather, query packing/snapshot/upload,
evaluation, Python download/provenance, host conversion, normal/scatter, child
cleanup and actual reused wall. Reused wall retains snapshot/prepared input/
Engine/PatchSet but includes fresh query/result work and cleanup. Classification
preparation is correctly measured separately and retained as an explicit reusable
condition. Source metadata and Python allocations remain part of download;
76 bytes per admitted native result is not total host memory traffic.

Cold, reused, exact qualification, export and actual export-inclusive wall are
separate observations. Independent median components are not added into caller
time. CPU24 changes native evaluation workers, not Python normal/classification
execution. Initial independent qualification plus status/provenance/normal checks
on every timed call is proportionate to the scoped sampler. The five-sample serial
matrix, focused sanitizer checks and selected body-host mutations directly target
new claims; they do not create a new native benchmark or abstract framework.

Plan accepted for coordinator authorization of implementation. Native ABI/math/
toolchain bytes and protected prior baselines remain unchanged.
