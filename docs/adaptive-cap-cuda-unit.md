# Resident analytic cap bounds

2026-10-17. Finite implementation plan, before CUDA body.

Use the accepted source-to-actual-facet derivation in
`adaptive-seam-wedge-geometry-plan.md`, implemented by the common HD
`numeric::bound_cap_facet_rn64`. Sources and mathematical assumptions are those
in that plan; this unit adds placement and ownership, not a new bound.
The pinned reference adaptive cache/session study in the parent mesher plan
motivates resident original-root reuse. Our concrete protocol is the owned
`cuda/adaptive_trim_ops.cuh::source_triangles` and existing base poll/drain:
one DeviceSession, original bank, stream, reservation callbacks and ticket.

Add optional `brepgpu_adaptive_cap_cuda_v1`, CapCudaAPI version1 with a single
`facets(session, DeviceCapFacet*, count, ticket, error)` operation. Each POD task
contains its source-derived CapSourceDescriptor and actual CapFacetInput.
The native owner constructs descriptors only through PreparedCapSource; the
low-level device API consumes valid source enclosures as its explicit math
contract, like the existing original RootDescriptor upload. Pole enclosures
may lie outside original R; actual finite connector/cut UV must lie inside R.
No source root, actual physical Q or quality changes.

Version1 also advertises the new actual-node-box analytic regularity and
full-frame cone frame_f semantics. Future quotient owners must require it;
old base-only modules must refuse quotient admission. Base/curve/trim and
assignment API layouts remain unchanged. Existing regular owners remain valid.

Validate healthy idle surface session, nonzero bounded count, root identities,
matching analytic family/kind, finite valid intervals/XYZ, source-domain cut
and connector coordinates, nonnegative finite allowance and arithmetic stage.
Charge overflow-checked conservative finite work before buffer growth/launch.
Reuse existing input/result/pinned staging and output capacity. One kernel
thread per facet calls the SAME common RN64 function with the resident original
root. Finish/poll returns CapFacetBounds; existing counters include all transfers,
and existing destroy drains before releasing original banks or module ownership.
Geometric uncertainty remains needs_wider rather than a fake success.

Execution: first observe absent getter on the existing real module (RED), then
add header/operation/getter and extend the SAME source-backed DeviceFixture with
cap_facet. The geometry owner's finite north/south/cone source fixture checks
the same whole Fsrc minus SAME L through its independent original Taylor/
Bernstein reader, on CPU and both P100s. Do not introduce another geometry
tester. Actual-source review covers validation, work preflight and owner reuse;
one CUDA memcheck covers the new kernel/staging. Reuse scoped host sanitizer
checks from the cap geometry unit. No whole-watch trial before cap construction,
classification and display provenance are connected.
