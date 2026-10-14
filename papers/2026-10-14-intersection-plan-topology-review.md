# Intersection plan: construction and topology review

2026-10-14. **Accepted; no implementation blocker.** Reviewed
[INTERSECTION_PLAN.md](../docs/INTERSECTION_PLAN.md) against the accepted
[design](../docs/INTERSECTION_DESIGN.md). No repeated research, product code
or native execution.

The construction owner keeps Scalar comparison/rounding, exact maps, complete
closed-trim decomposition, graph events, physical spans and immutable source
binding in one unit. The thin lazy `BodySnapshot.intersect_faces` adapter
delegates through FaceRef to that same operation; it does not introduce a
second clipping path. The missing body-method red must follow a healthy actual
body/evaluator check before implementation starts.

The oracle uses independent literal component/end-point truth and original
control/frame identities, rather than deriving expected answers from production
Scalar or construction. Whole-piece correspondence and physical span grouping
are checked separately from sampled output. Mutants attack omitted components,
holes, seam/span identity and UV coupling with real assertions. Unsupported
input and failure remain complete-operation errors; unchanged native binaries
do not require another unrelated native campaign.

The two literal caller recipes are geometrically correct. The full-size box
top at z=0 contains the complete radius 1/4 section circle. The scaled .25 box
has half-width 3/16 and clips it into four disconnected quadrant arcs. In
quadrant 0 the exact parameter endpoints are `1/sqrt(7)` and
`(4-sqrt(7))/3`; both lie strictly inside that chart. All four wall-chart box
pairs survive, but construction still acts once on the complete face pair.
The plan explicitly rejects incomplete prefix coverage and keeps unsupported
scene pairs visible.

**IP1 — low wording clarification:** the SVG instruction says “one physical
component versus its chart occurrences.” That is true for the full circle but
not the four-arc recipe. Label the actual component count and each component's
representation occurrences. Sent to root; the independent literal counts
already prevent a wrong geometric result, so this is not an implementation
blocker.

Construction/request cost, cold setup, reused sampling and final cleanup are
separate. The retained request/result/PatchSet isolate the measured consumer
without pretending that CUDA constructs the graph. Five serial reused samples
and separate unprofiled/profiled scopes preserve existing measurement rules.
Source/control/map/scalar/event exports make the outputs usable for later
arrangements while avoiding Boolean or persistent-edge claims. The breadth-first
return-pass boundary remains intact.

Proceed only after root accepts the plan and observes the direct red. The
construction owner then implements the reviewed whole invariant and lazy body
adapter; no work was started during this review.

Final re-read: IP1 is resolved by labeling each physical component and actual
counts. The appended consumer handoff names the immutable request/batch, original
source slots and native remapping, exact parameter/map access and source outputs.
Those agree with the construction records and whole-unit ownership. Final plan
accepted, no remaining blocker; implementation still awaits root's observed red.
