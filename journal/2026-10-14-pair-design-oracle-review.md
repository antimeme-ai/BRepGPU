# Pair design oracle review

Reviewed whole enclosure/closed-bit/page/source-truth design. Raised P-OR1:
multi-body face selection must use namespaced keys or explicit per-body mapping,
since local face IDs repeat across snapshots. Remaining independent truth and
narrow broad-phase scope accepted, including exact-Fraction outward world
construction. Suggested bounded 2^32 total-pair boundary case for plan. Saved
papers/2026-10-14-pair-design-oracle-review.md. No code/native execution.

Root integrated full `(body_id,face_id)` selection keys; re-read corrected
clause. P-OR1 resolved. Design accepted with no remaining oracle blocker.
