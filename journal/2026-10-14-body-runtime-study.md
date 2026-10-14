# Body sampler runtime/consumer study

Read the owned carrier sampler, Python ABI1 resource/jet API, native metadata/
conversion implementation, public lifetime/snapshot/conversion tests and native
benchmark scopes. Report: [body runtime study](../papers/2026-10-14-body-runtime-study.md).

Recommend an explicitly CPU body/classification layer using immutable records and
owned packed tables, cached PatchSet, all-row labels, gathered normalized queries
and one order-1 native batch. Identified source-UV versus normalized-UV routing,
positive uniform common-frame limits, per-face normal orientation, admitted SoA
stride/scatter mapping, degenerate normals, frozen-buffer ownership and Python
download/provenance costs as concrete integration risks. Scoped cold, PatchSet
reused and query-reused callers separately. Existing native lifetime/error guards
remain reusable; no ABI/dependency change is required for this narrow consumer.

No product code, tests/builds, native/local/fleet execution, new agent, dependency,
Git mutation or benchmark change. Source entry points and hashes are recorded;
the report proposes consumer contracts rather than asserting an implemented body
graph or GPU topology operation.
