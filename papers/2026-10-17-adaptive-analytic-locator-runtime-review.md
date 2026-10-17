# Analytic locator recovery: runtime/source review

Read docs/adaptive-analytic-locator-unit.md and the actual FaceLocator,
target_point, prepare_trace_sites and complete_trace_sites paths in
native/mesher/adaptive_traces.cpp. This is source review only; no new execution
or separate source checker was added.

The cofactor rows cross(Y,Z), cross(Z,X), cross(X,Y), divided by
cross(X,Y) dot Z, invert the literal frame without orthonormal assumptions.
Their exact construction occurs once per face; ordinary finite conversion and
action only propose coordinates. Original endpoint V and interior original Ce
enclosure midpoint remain the targets. Constructed Q does not enter the locator
or final original source-node guard.

The cylinder signed-radius division, cone two-sheet physical generator
projection, signed sphere reflected chart and separated torus sign(R) seed
agree with the original source formulas. Proposal value/D1 use those same
literal frames/radii. The normalized physical Gram correction has the correct
negative-residual signs; coordinate directions handle rank ambiguity without
declaring invalid source. Nonfinite scores/directions and finite trial limits
stop proposal work and retain the guide. The fixed bank is only an eligibility
test; there is no analytic clamp or bank expansion.

The representative search is finite, lifted-guide relative, and does not alter
periodic flags or original mapped parameters. Wedge source incidence is
identified before iteration and locks the two endpoint sectors independently.
Existing joint merging and recovered regular-endpoint meridian U selection
remain followed by actual source sampling and all original V/Ce/S/alias guards.
The two extra common RN samples require a winner residual upper strictly below
a guide residual lower before replacing an in-bank guide. That comparison only
selects a proposal; it cannot accept a node or full trace.

One narrow finding was sent to the owner and parent: feasible step scaling is
rounded arithmetic. The candidate must be explicitly checked against the
represented guide-relative angular window after multiply/add, in addition to
the fixed bank check, before score/commit. Otherwise a one-ulp window overshoot
can pass the bank check. Seed eligibility should use the same window predicate.
The owner closed this finding in the actual source: improve() constructs the
represented feasible rectangle once, uses it for shortening and explicitly
checks every rounded candidate before scoring/commit. representative() uses
the same represented guide-relative inequalities. Locked U is also copied from
the original Rational guide when sealing the candidate, preserving sector
identity instead of round-tripping it through the proposal double.

Final scoped actual-source verdict: ACCEPT. This covers source/frame semantics,
finite proposal/branch guards, fixed-bank behavior, original authority and staged
guard separation. It does not establish universal inversion/convergence, a new
numerical acceptance theorem or a whole-watch performance result. No additional
test execution was requested by this review.
