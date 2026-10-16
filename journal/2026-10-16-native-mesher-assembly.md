# Exact source placements and effective definition allowances

This is the accepted whole-mesher transform-transport unit, not a new assembly
design. Geometry supplies one complete original source envelope per definition.
The assembly builder indexes usage children once, expands canonical original
usage paths, composes each affine matrix exactly from original binary64 words,
and rounds the twelve stored entries once. Reused and unplaced definitions remain
in the complete source inventory.

For each actual composed A, the outward dyadic square root of the maximum absolute
row sum of A^T*A bounds its operator norm, capped below by one. Exact differences
between composed and stored matrices bound output-transform error over the source
box expanded by requested world epsilon. Their vector norm is bounded by the same
owned outward square-root mechanism. Definition positional allowance is a canonical
dyadic lower bound on min_instances((epsilon-eta)/kappa); node-source allowance
uses min_instances(delta/kappa), without charging output-matrix eta twice.
Nonpositive remaining allowance is unresolved, not a dropped instance.

The immutable output owns original path IDs, source product/definition identities,
stored matrix words and sealed exact allowance values. No exact matrix cache or
private worker pointer becomes the source of truth. The original paths recover
exact compositions for an independent reader.

Direct RED uses a nested reused-product graph, original approximate-rigid matrices
and literal/integer-rational expectations. It also checks unplaced definitions,
invalid envelopes, expansion limits and rounding consuming the requested allowance.
No renderer or watch timing is claimed by this unit.

Actual missing-header RED preceded implementation. The nested reused graph then
passed strict Release and ASan/UBSan/leak checks, including independently stated
exact composed entries and stored-world positions on all source-box corners.
Missing/duplicate/reversed envelopes, source instance expansion limits and
precision refusal are direct input cases. Actual-source lint and named formatting
pass. The numerical owner independently accepted the actual transform, norm,
storage-error, source/owner, graph and publication code; its scoped report lives
beside the geometry research. Complete B0 preparation remains geometry-owned.
