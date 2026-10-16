# Native mesher geometry implementation plan — 2026-10-16

The accepted whole design f7aedef now has a concrete proposed geometry plan in
`docs/native-mesher-geometry-plan.md`. Read the current whole design, runtime
plan and topology plan, and the original source/map evaluator statically. No
product/test edits, native builds, geometry evaluation or benchmarks occurred.

Selected exact Cox-on-span Bernstein extraction from original knot/control
arrays, finite tensor-to-triangular convolution and actual-output residuals;
analytic interval formulas; explicit host trace composition including degrees
beyond the small GPU tasks; monotone scalar/circle image profiles; finite
strongly monotone convex-Q plane chart; authoritative displaced nodal stars and
graph strips; and the meridian pole/apex cap with shared outside cut chain,
actual nonzero facets and owner-coordinated children. Source natural knot roots
remain distinct from adapter face-envelope hints. Parameter endpoint order is
used once. No new library selected.

Coordinated shared namespace/ExactWords/JobContext, finite NativeMesh map/profile
tags and pole symbolic constants with the runtime owner; chart sites/physical
node keys/master intervals/quotient refinement with the topology owner. The
proposal explicitly retains bounded unresolved cases instead of promising a
general polar atlas, arbitrary continuation map or tangent-star solver.

Requesting root/peer adversarial plan review before any implementation. Focus
on source-interval composition coverage, displaced authoritative nodes and
strip/star gates, pole cap/ordinary cut ownership and legal radial children,
and the distinction between consumer UV locators and actual inverse meaning.

The numerical reviewer found a valid gap in the first proposal: authoritative
analytic pcurve samples can be transcendental, so a rational original nodal-star
center is insufficient. Replaced that assumption by the finite source-point
descriptors OriginalPcurveAtParameter/ProjectedPcurveAtParameter and the explicit
fan formula B(x)=x+orient(Pi,Pj,x)/orient(Pi,Pj,A)*(Q-A). Source intervals prove
the fixed denominator/orientation and enclose each affine sector; A is never
rounded into an exact topology site. The rational path remains exact. Also
specified canonical source-point rounding and the special actual plane lift.
No execution was added. Review remains pending.

Integrated review fixes: pole cut sites use the same dyadic angular fractions
for u and v; radial contraction is in original v distance to the analytic pole,
with an explicit source discrepancy floor. Authoritative degree192 composition
can become degree224 after spatial-curve cross multiplication. Replaced the
unconstructed strip gates by root's whole-arc tapered graph/fiber map after
the nodal PL maps. Angular image profiles now charge finite source-ray versus
chosen dyadic-angle endpoint discrepancy in their source/trace bounds as well
as the separate actual chord bound. Still no product/test edits or execution.

Root selected storing actual authoritative source-to-polygon B/T domain maps in
NativeMesh. Updated the geometry seam to reference runtime DOMMAPS_/SUPPORTS:
finite source-point/rational P/Q nodal fan and original-arc/coordinate/taper
graph strip, applied B then T. These define the chosen domain correspondence;
no proof subdivision or accepted flags become wire authority. Waiting to review
the runtime owner's actual row fields before claiming that seam reconciled.

Read the actual runtime DOMMAPS_/SUPPORTS rows: accepted finite B/T field meaning,
map order and explicit mapped-pcurve-s versus original-edge-t distinction. Raised
the per-coedge mixed-authority reference closure question to root; a mixed
joint A versus chosen Q needs an explicit named reference or finite refusal,
not an assumed face-wide reconstructed Boolean. No code/test execution.

Root selected the constructive HybridPullback mixed policy. Reference A remains
the original authoritative endpoint at any incident authoritative joint; two
authoritative endpoints must agree. Reconstructed reference curves are the
explicit inverse-nodal-fan image B^-1 of their chosen polygons, so applying B
closes every branch at common Q. T acts only on authoritative arcs and must
exclude reconstructed polygons except common endpoints. Geometry section7 and
the wire seam now specify that inverse formula, B/T displacement on every
affected cell, source-reference discrepancy charging and finite policy tag.
Requested peer review of the actual integrated text. No product/tests or runs.

Independent runtime/numerical peer returned scoped ACCEPT for HybridPullback,
with a valid plane-chart admission clarification: every reconstructed inverse-B
reference arc must be inside Q, not merely the selected polygon. Added finite
convex half-plane/enclosure checks on all applicable inverse fan pieces, with
refine/refusal on failure; auxiliary support polygons may still extend outside
Q without inverse evaluation there. Sourcepoint chart branch/lift semantics are
recoverable from its use/face/chart. No execution occurred.

Root corrected an inherited quality-name ambiguity before plan close. Inspected
old imported_meshing node materialization/recovery checks statically: legacy
source_residual_mm is a node cap, not a whole curve/polytrace tolerance. Updated
geometry section1 and HybridPullback charging: source V/exactCe versus original
carrier at stored chosen recovery UV and V/Ce endpoints use the node cap;
whole trace/chord and domain+actual cell use positional epsilon. Added the
hand-derived R=1/256 rational quarter-circle counterfamily distinguishing those
budgets. Requested numerical peer review; no implementation/test execution.

Algorithm peer found a valid natural-boundary support issue: a nodal PL polygon
strictly surrounding source A on u=0 cannot fit inside natural R. Revised the
procedure to allow ambient auxiliary supports outside R, while checking actual
reference/final points and recovery UV in R. Bound total B/T UV movement using
source derivatives on convex endpoint hulls inside R; no intermediate source
evaluation or extrapolation is required. Root notified to align prepared-box
wording. The plane-Q direct decomposition remains separate. Review pending.

Numerical peer reread the actual node/positional quality sections and returned
scoped ACCEPT, independently deriving the rational quarter-circle residual
coefficients (R/3,R/3) and (0,2R/3), Wmin1 and maximum norm2R/3. Independently
reread/accepted runtime's explicit original-target/stored-recovery-UV quality
paragraph and component meanings. Final ambient-support main-plan alignment
and algorithm-peer rereview remain the only notified pending geometry seam.
