# Pair oracle source review

2026-10-14. Read the exact enclosure/mask helper and initial direct red test;
requested the oracle owner's final test checkpoint before judging coverage.
Saved [the review](../papers/2026-10-14-pair-oracle-code-review.md).

No defect found in positive-weight convex-hull enclosure, actual-root/world
conversion checks, resident endpoint identity, exact closed comparisons,
cursor/profile flags, padding or retained-source mapping. Whole requested
face selection needs direct input-to-output tests; a helper cannot infer a
completely omitted face from the result alone.

Source inspection only. No edits to oracle/tests, execution, builds, new
agents, fleet work or Git operations.

Reviewed the owner's stable twelve-test checkpoint. Exact mask, alias/admission,
lifetime and primary-cleanup cases are coherent; independently checked the
analytic frame, tangent and trim/concentric false-positive expectations.
Reported PO1: assert complete default selection against canonical charts rather
than only inferred represented faces. Root accepted; owner is adding all-chart
and selected four-wall-chart source tuples. Requested expected-backend and
bit-identical signed-zero download assertions in existing tests. Native owner
owns separate rounding/DAZ/FTZ and actual CUDA current-device/worker checks.

Re-read the corrected stable thirteen-test checkpoint. PO1 resolved by full
three-body all-chart expected identities and the separately selected repeated
local-face-ID/four-wall-chart tuple. Extreme boxes now compare endpoint bits;
native backend metadata matches the requested engine. The new CUDA-only test
observes actual callbacks on both the caller and host-worker threads while
preserving their other current device. Accepted, no remaining source blocker.
No native execution was performed here.
