# Body sampler execution unit implementation

Implemented only [body_sampling.py](../python/brepgpu/body_sampling.py) after
accepted design/plan and integration's direct fleet red: healthy existing Engine,
then missing new body module. No native/CUDA/ABI or dependency change.

SampleRequest incrementally copies/caps immutable candidates, checks snapshot
slots and derives exact CPU classification/gather indices once. prepare_grid
validates selection/count first and preserves canonical face/chart order.
BodySampler borrows its Engine and owns one cached PatchSet, rejects foreign
snapshot requests, creates guarded query/result children, evaluates order1,
downloads/checks actual query bytes and lineage, converts once, and constructs
oriented normals/all-row scatter on CPU. Rejected/invalid rows keep NaNs and the
not-evaluated sentinel; failed admitted status/normal prevents publication.

Rows/timing pairs are immutable owned tuples. Actual phases separate gather,
query/evaluate/download+provenance/conversion/normal-scatter/child cleanup and
reused wall. Publication follows successful child cleanup; sampler close is
idempotent and preserves primary failures when cleanup also fails. Snapshot/chart
handoff uses the accepted shared body interfaces, with native patch slot==chart
slot and common positive frame.

At coordinator review, simplified timing publication to ordinary frozen-batch
construction. Internal reused_wall_ms ends after child cleanup; the caller's
separate actual_reused_caller_ms includes final timing metadata and SampleBatch
construction/validation. No private post-construction mutation of the frozen
output remains. Both scopes must be labeled explicitly in results.

Local checks are source/AST only; no product import/run, native execution,
tests/builds, fleet work, Git mutation or new agent. Independent tests and actual
CPU/CUDA integration/qualification remain with their owners.
