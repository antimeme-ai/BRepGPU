# Body design runtime review

Reviewed the body design against current ABI1 resource/snapshot/download/frame
conversion and the runtime consumer study. Report:
[runtime design review](../papers/2026-10-14-body-design-runtime-review.md).

CPU immutable body records, cached PatchSet, one order-1 batch, explicit frame
conversion and per-face normals fit the existing API. Numeric transfer counts
20/76 bytes per admitted query are correct, with host provenance/conversion costs
separate. Exact empty batch and prepared-input-reused caller scope are explicit.
Sent two concrete clarification findings: per-row native statuses do not include
native diagnostic objects, and prepared requests must validate snapshot/face/chart
binding before deriving slots. No product code, tests/builds, native/fleet work,
new dependency or Git mutation.

Re-read integrated diagnostic and exact-snapshot binding clauses; both BR-R1 and
BR-R2 are resolved. Appended corrected design identity and accepted disposition.
No unresolved runtime/consumer design blocker; implementation-plan review is next.
