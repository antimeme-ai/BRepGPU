# Intersection consumer: source, graph and lifetime review

2026-10-14. **Accepted; no consumer source blocker found.** Reviewed the whole
[intersection sampler](../python/brepgpu/intersection_sampling.py) against the
agreed exact construction and consumer handoff. Source inspection only.
Reviewed source SHA-256:
`8691091fa8ad411713aa180bf1ad7000edc9301e4ec57cbb06c2092de87de316`.

Request preparation binds the exact factory result, copies recursively
immutable Scalar/vector values, retains component/piece/span IDs and caches
rounded native UVs. Resolution and total-candidate admission precede candidate
allocation. Curve interiors are rational; irrational-endpoint windows use exact
comparisons inside neighboring-float outer brackets with a shared 4,096-step
cap. Failure remains SamplingPrecisionError without altering the complete
intersection. Point occurrences contribute one candidate each. Representation
duplicates are counted explicitly rather than mistaken for new graph edges.

Source packing maps `(side, original_chart_slot)` to dense native slots, so
equal slot numbers on different bodies cannot alias. One order-zero batch
interleaves left/right rows at 2i/2i+1. Shape/status checks precede conversion.
Actual returned full descriptor bytes, slot bytes and UV bytes must equal the
bound inputs; exported provenance is copied from these returned buffers,
not manufactured from expected metadata. Both converted source positions and
their pairwise difference receive exact Fraction/Scalar world-model grades.

Nested child scopes catch BaseException and guard secondary cleanup attachment;
both result and query closes are attempted while the exact primary propagates.
Native patch ownership is adopted under a constructor guard. Normal cleanup
failure prevents publication; consumed handles stay closed while allocation
failure may leave an owned handle retryable. Borrowed engines are not closed
by the sampler. Empty results issue no query rows. Cross-result requests and
closed engines fail before native work, and detached immutable batches retain
their result/source facts after native cleanup.

The nine internal timers cover gather, query, evaluation, actual download and
provenance checking, conversion, scatter, model grades, child cleanup and the
internal wall. Exact maps/rounding belong to request preparation; immutable
batch publication belongs to the external caller wall. This is two-sided
numerical consumption of an already exact CPU result, not GPU graph construction.

Root reported the first actual consumer direct test green; that report does
not substitute for full fixture/cleanup qualification. No product execution,
tests, builds or edits to the reviewed unit were performed here.
