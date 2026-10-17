# Adaptive occurrence-trace numerical source review

Reviewed actual `native/mesher/adaptive_traces.cpp`, from IntervalState through
prepare/complete_trace_intervals, against the accepted shared-curve unit. Scope
is numerical source/domain/image correspondence, not runtime scheduler lifetime.
No duplicate geometry execution was requested or performed for this review.

The indexed original carrier support check distinguishes positive-area boxes,
1D union coverage and point support. The exact original positive-weight pcurve
hull is used only after RN support coverage is ambiguous; there is no unproved
clipping to the natural rectangle. Convex raw/selected UV support plus original
first derivative upper bounds gives the stated deltaUV*(du+dv) transport. The
physical original curve-to-Q chord and selected carrier trace-plus-authoritative
transport each use positional tau; original node delta remains a separate source
relationship. A generic retained source-to-selected-trace upper may be 2*tau,
which is not silently substituted as a nodal source cap. Same-Q virtual trace
restrictions and explicit finite plane image map fields preserve correspondence.

One concrete source-truth finding was fixed: circle-kind BGSRC expressions use
both stored radii literally, just as the original source evaluator does. The
cylinder harmonic identity and analytic raw pcurve support initially substituted
rx for the Y radius on circle-kind rows. Current source instead compares
`curve.radii[a]` and bounds the original Y coefficient with `root.radii[1]`.
Reread these actual fixes: ACCEPT. A normal adapter's equal circle radii do not
justify silently rewriting other admitted original coefficient rows.

Scoped numerical/source-domain ACCEPT after that fix. This does not claim a
complete face-domain homeomorphism or implementation of a general analytic
inverse/period identity outside the explicit finite profiles.
