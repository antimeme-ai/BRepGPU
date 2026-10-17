# Physical-plane cell bound review

Independent actual-source review of native/mesher/plane_cell_bounds.hpp/.cpp and
the synchronous import/use in numerical_service.cpp. Root/numerical owner own
the direct CPU/device/service and sanitizer executions; no duplicate execution.

The profile borrows same-Budget source origin/basis/Q/height until the batch
finishes. Its precomputed halfplanes use the correct CCW orientation; strict
positive convex Q and closed per-corner containment put the entire positive
XY triangle in the admitted inverse image. Source identity, carrier and
face/generation checks precede service use of the retained plane lease.

For the same XY point, original S(Psi(q)) differs from its affine plane lift by
at most the existing whole-natural-domain height. Affine plane lift minus the
actual emitted facet is the affine interpolation of its three actual corner
residuals. Convexity of the norm therefore bounds it by the largest corner
norm. The exact squared comparison against (tau-height)^2 is equivalent to
that combined test after the explicit tau>=height check; equality, including
zero remainder, is handled correctly. No chosen-UV interpolation or sampled
inverse is substituted for the admitted source map.

ACCEPT within the retained geometric-profile premise. This review neither
re-proves that profile nor approves authoritative-domain B/T displacement or
other mapped cells. No second projection or source proof tree is introduced.
