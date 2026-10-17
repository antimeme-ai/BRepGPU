# Actual shared-curve geometry and device interface review

2026-10-16, runtime-owner review of the current native curve geometry unit. This
is a scoped source review, not a new numerical execution campaign. The geometry
owner's independent original-source reader and root's complete boundary-output
reader test the actual numerical contracts.

Read CurveSourceCache original Bernstein Cox recurrence, literal Bezier bypass,
prepared interval merging/index lookup, full-multiplicity C0 admission, immutable
original/prepared control leases, common sample/chord/pair kernels, and the
shared CurveCudaAPI ownership/admission paths. The level recurrence computes
adjacent degree levels once, and the final original-control combination has the
expected support indices. Original source endpoint ownership is the lower span.
The anchored p+1 residual compares original N against W times the same actual
chord. Paired midpoint restriction uses one convex pyramid for both children.
The analytic norm uses the original, possibly nonorthogonal frame.

Three concrete findings were accepted and fixed by the geometry owner:

* Fixed source-vertex overrides must retain signed-zero binary64 words. The
  ordinary and precise override branches now preserve them; only constructed
  points normalize zero. The runtime override cache uses exact XYZ bit keys.
* Request slots and finite/increasing domains must be checked before sorting.
  NaNs must not reach an ordered-comparison algorithm.
* A valid positive exact interval may have overlapping binary64 endpoint
  enclosures. The host ordinary node service now returns needs_wider for this
  case, rather than refusing input only on CPU; the exact-t fallback remains
  available and the device has the same finite source admission.

The later seven-line common curve_lerp change was reviewed directly. Exact 0/1
parameters return the appropriate immutable endpoint; otherwise the common
(1-t)*a+t*b interval expression encloses the same source affine combination and
avoids the severe dependency growth of a+t*(b-a). It changes neither the source
geometry nor the accepted error contract. CPU and CUDA must compile the same
helper. Ordinary midpoint restriction was already convex.

The typed device implementation reuses the existing preregistered
allocation/ticket/drain mechanism. Unique destinations, immutable retained
control ranges, disjoint parent reads, resident actual endpoint samples, and
counted dynamic trace staging were checked. The curve trace kernel reads the
flat coefficient slab with small temporaries; it does not instantiate a private
132-interval net per thread. No new correctness blocker was found there.

Scoped verdict: ACCEPT after the three valid fixes. No dependency or additional
geometry mechanism was adopted. Root and the construction owner separately
review the runtime state transitions and occurrence/domain semantics.
