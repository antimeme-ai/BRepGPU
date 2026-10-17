# Native whole-mesher value/numerical/runtime implementation planning

2026-10-16, after accepted whole design `f7aedef`. Wrote
`docs/native-mesher-runtime-plan.md` for parent whole-plan integration/review.
No product/test edits, builds, CAD or performance execution.

The plan selects a strict typed BGSRC001 source mirror/parser preserving its
existing13 sections and degree limits; immutable cross-owner ExactWords with
qualified pool/index references; owner-local Rational arithmetic reusing the
owned integer engine with explicit upstream allocation, division/gcd/rounding
APIs; actual degree-bounded CPU/P100 residual tasks and buffers; yielding
continuations with separately reserved child budgets; frozen contact epochs;
and concrete NativeMesh wire rows/map/profile variants, stored-XYZ display and
atomic whole-result publication.

Read existing source validation/read/write, native source writer units/metadata,
integer/budget APIs and GPU/display contracts. Read primary GMP algorithm manual
discussion of normalized basecase division and Lehmer quotient batching to
specify owned arithmetic; no dependency or reference code adopted. The proposed
math unit remains bounded by actual fleet vectors and implementation review.

Coordinated geometry-owner chart/profile/pole payloads and topology-owner site,
generation, contact-epoch and optimizer seams. Accepted topology review feedback:
ExactRef must identify its sealed table as well as index; child bounds/exact work
must not share parent mutable Budget; complete pending frontiers/contact folds
must be deterministic; backend filter endpoints cannot enter canonical mesh
bytes; source-domain interpolation maps must recover node-role/facet-order
permutations by physical keys, including negative face orientation and pole fans.
All incorporated directly, not deferred behind an unspecified task graph.

Parent clarified original unit metadata: parser preserves positive units_mm and
does not multiply already-mm snapshot geometry again or reject non-1 metadata.
Actual current STEP writer itself rejects non-mm source parameters pending an
explicit source-UV conversion map; inspected that code and recorded the limit
without claiming the missing adapter conversion is delivered.

Initial CDT may use a shared power-of-two scale for dyadic transformed sites.
That is explicitly separated from forbidden general-rational global mesh LCM.
The full-mesh schema carries source/geometry/approximation semantics, not a new
certificate protocol. Independent actual-output research qualification remains
off the product path. Root owns whole-plan review and authorization to begin
the integrated implementation sequence already requested by the operator.

Parent assigned independent review of root/topology/geometry unit plans; this
does not self-approve the runtime unit. Saved provisional independent findings in
`papers/2026-10-16-native-mesher-plan-runtime-review.md`. Identified complete
boundary-request horizon determinism gap (sorted arrived requests alone are not
a deterministic set) and pending-priority reason wording. Reviewed fixed-metric
LEPP, typed direct CDT/append predicates, actual-facet contacts and optimizer
epochs; accepted root's added exact-instance norm/storage-rounding transport
math. Geometry unit and owner closures remain pending before overall acceptance.
Review only, no product/test changes or execution.

Completed independent root/topology/geometry review after geometry unit landed.
Geometry owner resolved analytic/nonrational authoritative-star center maps,
dyadic pole-cut interpolation and source-circle/dyadic angular endpoint mismatch.
Topology closed complete boundary request horizons and pending queue keys.
Accepted the actual revised independently owned scope for implementation planning;
runtime unit remains independently reviewed by other agents. Retained complete
findings/closures in the review paper. Root selected a necessary new native
display feature profile, one-sided source-normal flags/cofactor transport and
v2 capability guard; geometry owner reviewed the formulas. Integrated plan-only
consumer/RED changes and explicit reachable-only exact serialization. Root still
owns authoritative support descriptor storage decision. No product/test edits,
CAD, builds or experiments.

Root selected storage of actual authoritative B/T support maps. Added exact
`DOMMAPS_` and `SUPPORTS` rows and optional face domainmap reference to the runtime
schema, with finite source-point descriptors (mapped pcurve s distinguished from
edge t), rational nodal support P/chosen Q and explicit fan equation; graph-strip
source interval, pre-fan refs, exact coordinate frame, chosen endpoints and
tapered envelopes. Order is all B then T. No proof subdivision/cached decision
booleans are persisted. Requested geometry-owner peer review of actual fields
before root closes the whole plan. No product/test edits or execution.

Geometry owner peer-reviewed the actual DOMMAPS_/SUPPORTS fields and accepted
their source-parameter, composition, frame/taper and ownership meanings. Updated
the independent review paper to close stored-map integration. A newly raised
mixed authoritative/reconstructed face realization policy remains with root/
geometry owner; numeric payloads need no speculative change. No product/test
edits or execution.

Root selected constructive HybridPullback for mixed-authority faces. Added the
finite DOMMAPS reference_policy enum AuthoritativeOriginal/HybridPullback, with
all-reconstructed domainmap0, explicit inverse-B fan equation and source-use
authority consistency. Hybrid reconstructed arcs are B^-1 of selected polygons;
T supports avoid those polygons except common fixed joints. Ordered domain
displacement and original-source discrepancy charge inverse/forward movements.
Geometry owner is updating/reviewing the shared policy wording; no production
code or tests changed.

Reread actual geometry HybridPullback policy and accepted its finite reference/
map/error semantics. Identified one final inverse-coverage detail: physical-plane
reconstructed pullbacks must lie inside Q, even if their selected polygons do;
the owner added affine-piece/convex-halfplane admission before evaluating Psi.
Updated independent review to all findings resolved/ACCEPT for planning in the
root/topology/geometry scope. Runtime source-point schema now explicitly includes
retained use period/lift and already-mapped s, avoiding raw/lifted or t/s aliasing.
No product/test changes or execution.

Root clarified existing quality input meaning from actual imported mesher node
checks: source_residual_mm is a nodal recovery/correspondence cap, not a cap on
whole-curve/polygonal-trace sagitta. Updated QUALITY_ normative name to
requested_NODE_source_residual and selected COMPONTS tags separating node carrier/
endpoint-curve residuals from whole trace/chord positional, domain and actual-cell
terms. Whole-interval approximation uses epsilon_def; node checks retain the
source-only kappa-scaled cap. Verified old endpoint-to-curve check at source
materialization as well as the carrier recovery node check. Geometry-owner peer
review requested. This preserves request semantics and does not relax positional
accuracy or add a new solver/check. No product/test edits or execution.

Closed the quality clarification with an actual-text peer review. Geometry
sections1/7 now explicitly compare original source targets to S at the stored
recovery UV and preserve endpoint-to-curve checks; whole trace/reference/chord
and domain-plus-cell comparisons use positional epsilon. Verified the proposed
rational quarter-circle residual bound 2R/3 independently from its degree3
Bernstein coefficients. Geometry owner independently accepted the matching
runtime schema/paragraph. Scoped geometry/quality ACCEPT recorded in the
independent review; runtime unit still relies on its other reviewers. No
product/test edits or execution.
