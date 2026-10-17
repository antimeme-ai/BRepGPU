# Component design: runtime review

Read docs/adaptive-component-unit.md against actual FaceTrimSession,
SurfaceCacheSession/Core, exact-table keys, DeviceSession validity arrays and
DeviceOwner teardown. The bounded-pool choice is accepted with the concrete
interfaces in docs/adaptive-face-assignment-unit.md. No product changes or
executions were made for this design review.

The resolved requirements are: one fixed SAME source bank; explicit owner epoch
distinct from quality/boundary; no same-quality ordinal reset; epoch captured by
domain/construction/view work; complete preflight; drained native/device page
retirement; old immutable host outputs retain copied values and source/resource
leases. Device retirement uses the existing owner/stream/reservation mechanism,
clears validity and preserves immutable controls. A failed drain keeps ownership
alive and cannot report a healthy reusable owner. Native cancellation remains
terminal rather than silently resetting a stop_source for another assignment.

Retaining all immutable component output intentionally consumes output memory;
that is separate from a pool retaining mutable caches for every source face.
The first direct fixture must use distinct carrier identities so an unchanged
same-carrier point cache cannot falsely demonstrate bounded retirement. Surface
root/point slots are never semantic identity without the retained bank and epoch.

No remaining architecture blocker after root selected the fixed source bank and
versioned device retirement seam. Implementation GO still follows the finite
subplan review. Contact/component/assembly/display construction stays outside
this first runtime unit.

The concrete coordinator subplan was subsequently read in
docs/adaptive-component-coordinator-unit.md. Scoped runtime ACCEPT: each owner
uses one CPU step under the outer scheduler, avoiding nested queued waits;
complete face arrivals precede a physical forest mutation; requests are copied
before retirement; every retained completed face is matched to its actual
per-face boundary generation while preserving its exact older snapshot lease.
Final source/resource bank binding, terminal failed owners and output-after-contact
closure are explicit. Contact policy mathematics and new assembly envelope
arithmetic remain their owners' review scopes.
