# Intersection consumer design review

Reviewed exact endpoints/one-field request semantics, two-sided dense/source
mapping, existing native order-zero resources, error grades and timing scopes.
Found IR-R1 unspecified candidate/precision/field bounds and IR-R2 ambiguity
between empirical evaluator targets and sample-success grades. Root added
explicit caps/factory parameters and direct exact-curve/pairwise checks with
named arithmetic failure. Point row wording and circle-XY derivative wording
were clarified. Re-read corrections and accepted, no unresolved blocker.

Saved papers/2026-10-14-intersection-design-runtime-review.md. Implementation
plan should name native row order/scatter and field interfaces; this is not a
new design gate. No product/native/performance/fleet execution, dependencies,
new agents or Git mutations.

Accepted the subsequent `span_id` clarification: sample rows/export preserve
span and piece IDs, including multiple source-chart occurrences of one spatial
span. This changes provenance interpretation, not geometry or native row caps.
