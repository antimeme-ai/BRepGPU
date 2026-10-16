# Native mesher design: adversarial geometry/source review

Reviewed `docs/NATIVE_MESHER_DESIGN.md`, proposed whole-system design, against
the geometric-contract research and current source/oracle semantics. This is
a design review only; no product/tests or geometry execution. Initial decision:
**REQUEST CHANGES** for the concrete items below. The remaining construction
and error architecture is coherent enough to review as one operation.

**Rereview after root's whole-design revision: ACCEPT.** G1–G3 below retain the
actual initial findings and counterexamples; the resolution and remaining plan
obligations are recorded at the end. This accepts the design, not unimplemented
algorithms or a completed mesh result.

## Blocking G1: common Cbar endpoints do not close per-face chart traces

The reconstructed-trace procedure makes each auxiliary spatial curve end at
the same original vertex V. It then permits a different regular projection
chart per use/interval. This does not make the selected UV endpoints equal.

Exact counterexample: `S(u,v)=(u,v,0)`, `V=(0,0,delta)`, `delta>0`. Two incident
uses choose `Pi1(x,y,z)=(x,y)` and `Pi2(x,y,z)=(x+z,y)`. Both projected carrier
Jacobians are the identity and are globally one-to-one. Both corrected curves
end at the same V. Yet their selected endpoints are respectively `(0,0)` and
`(delta,0)`. There is no closed selected UV loop. Interval isolation and a source
residual bound do not repair the logical gap.

Required resolution: establish a common selected per-face/per-source-vertex UV
realization and branch (modulo explicit seam/pole occurrences), before edge
continuation. One valid route uses the same projection chart in the complete
protected vertex star; each incident continuation must begin/end at the same
selected UV, and chart changes must agree through an explicit transition.
Another may solve a common vertex chart then define a bounded endpoint-matched
trace correction. The global design must select one and preserve its relation
to authoritative original domains. State the actual face-joint equality
invariant; matching canonical XYZ alone is insufficient.

The auxiliary Cbar is useful and bounded: its difference from the original C is
an affine endpoint correction and is at most the maximum endpoint departure.
This finding is about the downstream chart selection, not a request to replace
canonical interior C samples or alter the source record.

## Blocking G2: the selected projection and analytic inverse are different maps

Step2 defines gamma by a projected equation; step3 says analytic angle/height
inverses implement that selection. They do not implement a fixed linear
projection for off-carrier source points.

On a unit cylinder `S(u,v)=(cos(u),sin(u),v)` near u=0, choose the regular linear
projection `Pi(x,y,z)=(y,z)`. For off-carrier `X=(1,delta,0)`, the projected
equation selects `u=asin(delta)` on the local branch. The cached analytic
angle/height inverse selects `u=atan(delta)`. Both are continuous, bounded and
potentially useful; they are not the same trace and cannot share an undefined
reference-domain meaning. The difference is mathematical, not libm rounding.

Required resolution: either restrict Pi in that procedure to the intended
proved nonlinear analytic map (angle/height with its explicit excluded axis,
range, branch and derivative bounds), or state that analytic inverses and linear
projection-chart inverses are separate trace selections satisfying common
existence/endpoint/continuation/residual obligations. Use the actually selected
map consistently in stars, strips, domain reference and full-mesh records.
This must be resolved together with G1; a projection name is not enough metadata.

### Concrete lower-complexity resolution for reconstructed traces

Root requested whether a deterministic selected piecewise UV trace can replace
implicit inverse truth. Yes, under the already declared reconstructed-use
policy, with these actual obligations:

1. Establish one exact dyadic UV value per regular face/source-vertex branch;
   seam/pole occurrences have their explicit equivalence/side records. Validate
   its original-carrier/original-vertex discrepancy and use it as the shared
   endpoint of every incident selected face trace. Candidate projection charts
   near a vertex must use this choice rather than choose independent endpoints.
2. For exact dyadic edge parameter intervals, use local analytic/Newton/chart
   calculations only to propose exact dyadic UV sites in the admitted branch.
   Store those sites and define gamma itself as their continuous piecewise
   affine interpolation in source t. No generally algebraic inverse root is
   passed to the existing rational-site CDT.
3. Bound the entire original source-curve/carrier-trace discrepancy, not just
   site residuals. For rational `C=Nc/Wc`, `S(gamma)=Ns(gamma)/Ws(gamma)`, bound
   `Ns(gamma)*Wc-Nc*Ws(gamma)` over the interval with positive denominator lower
   bounds. Gamma is affine there, so this is a finite univariate polynomial
   residual after common-basis products. Use enclosing analytic alternatives
   for trig carriers/curves. Endpoint source-vertex departures remain explicit;
   optional Cbar is not the physical generator.
4. Apply the actual cell/edge error inequalities and exact selected-domain
   topology checks, plus the branch/star/embedding obligations. Root isolation
   is needed only where the chosen branch/admission or a certified proposed
   chart truly requires it; it is not needed to define gamma once its exact
   piecewise sites are chosen.
5. Trace updates are explicit BoundarySnapshot generations. They invalidate
   affected face coverage/maps and contacts. The final artifact names the final
   selected toleranced realization; it does not claim that all refinements
   discretized one pre-existing implicit exact inverse curve.

This makes the reference construction algorithmic and finite without a hidden
algebraic-coordinate backend. It does not authorize relabeling an authoritative
original pcurve domain as the output polygon: those uses still need the actual
curved-domain strip/star relation and its charged displacement. The geometry
policy must distinguish these cases in the result.

For reconstructed selected polygon traces, no curved-original-domain isotopy is
promised. Jointly chosen common endpoint sites, exact simple-loop/hole ownership
checks and uniform original-generator discrepancies supply the finite selected
domain contract. Vertex protection can propose sites, but a vague collar theorem
need not define truth. Later changes recheck that exact domain and all affected
physical contacts before publication. For authoritative curved original domains,
the harder curve/strip/star construction is real additional work and cannot be
waived: isolate regular monotone arcs, enclose arc/chord strips, protect them
against nonincident features, and jointly handle their endpoint connectors in a
source-matched star. If tangent-sector ordering is not established by the chosen
bounded procedure, return unresolved. Do not claim universal success by naming
leading jets without implementing the actual separation/order decisions.

## Blocking G3: output embedding does not prove source embedding

The design states that an intersecting source cannot become a successfully
embedded result, but the described BVH/contact mechanism checks actual emitted
facets and is activated when those facets have forbidden contact. A self-crossing
source sheet can have an embedded approximation within a large enough epsilon.
Concrete bounded-degree example: a thin box has bottom `z=0` and top
`z=delta-2*delta*f(x,y)` for `0<=x,y<=1`, with
`f=16*x*(1-x)*y*(1-y)`. The top is a regular degree(2,2) polynomial graph, has
z=delta on all four boundary edges and reaches z=-delta inside, so it intersects
the bottom. Emitting two flat top triangles at z=delta with the original
unchanged boundary gives an embedded box. The full pointwise top error is at
most `2*delta<epsilon`. All source corner generators, boundary curves and face
inventories can be retained. Thus an empty
actual-facet contact queue alone does not establish the stronger source claim.

Required resolution: distinguish the claims explicitly. Either original source
embedding is a declared admission precondition with a named means of establishing
it on supported inputs, or the product promises an embedded realization with
bounded per-entity approximation without claiming that every input geometric
self-intersection was detected. If invalid source intersections must be rejected,
the source-geometry candidate/separation work must be part of admission or the
same integrated contact mechanism even when the first output has no contact.
Do not add a second certification framework: choose the actual required claim
and the one direct construction mechanism that supports it.

In either case, publication needs all actual facet-pair decisions current for
the final node/boundary generations. Pending contacts or pairs invalidated by
late split/flip/rounding changes cannot be treated as accepted. State this final
queue/generation invariant explicitly.

## Numerical obligations that should remain explicit in the plan

* The ordinary rational residual is correct with positive Bernstein denominator
  coefficients and exact/enclosed source restriction, common-basis products and
  elevation. Actual emitted XYZ in L already includes nodal/XYZ rounding; do not
  add the same allowance again. Anchoring must be an algebraic identity, not
  rounded subtraction presented as exact cancellation.
* A projected physical-plane chart with a uniform positive symmetric Jacobian
  lower bound on a convex source domain has a valid strong-monotonicity route
  to injectivity. A nonsingular Jacobian alone does not. Its positional bound
  must cover the full inverse-image cell/height enclosure, not only normals or
  boundary heights. A height bubble with zero boundary values exposes a
  boundary-only test. Store an actual map/domain description, not a certified
  boolean which an independent reader cannot interpret.
* Positive weights and knot-array validity do not imply a continuous source
  surface: internal multiplicity degree+1 may permit a positional jump. State
  whether C-1 source discontinuities are rejected, treated as distinct components
  with source topology, or supported by an explicit construction. C0 cuts are
  not a solution to an unacknowledged positional discontinuity.
* The monotone graph-strip and protected-star domain argument must include
  original/recovered endpoint connectors where their UVs differ; otherwise the
  symmetric-difference containment has a gap at the vertices. Tangent common
  endpoints require the stated leading-order/order mechanism, not disjoint-set
  positive clearance. Source discrepancy can prevent collars from separating;
  that is bounded unresolved/infeasible work, not a reason to relax epsilon.
* Source Cbar, selected gamma, canonical emitted C samples and original
  authoritative pcurves remain separate objects. Their bounds reference the
  declared source policy once. A large foreign-pcurve residual alone does not
  prove all valid recovered branches impossible.
* Pole rectangle-to-fan map is sound as a full coverage-cell map. The output
  format/oracle must preserve that map and its association with actual nonzero
  facets; three corner UVs cannot stand in for the full rectangular source domain.

## Direct oracleability and scope

Use independent authored source values and actual emitted full-mesh records.
The two elementary projection counterexamples above are literal analytic
families, not watch-dependent regressions. Domain/strip topology, source mapping,
actual physical contact and local positional bounds attack different faults.
They do not require a second native checker or a Python product path. Existing
Fraction/source readers are appropriate development answers after their missing
whole-cell/strip/contact mechanisms are addressed for the stronger new claims.

I did not review derived-cut ownership or optimizer/progress details in duplicate;
the algorithm owner has those findings. Runtime field/publication ownership has
its own reviewer. The initial review required concrete resolution of G1–G3 and
integration of the stated numerical assumptions into the implementation plan.

## Rereview: resolved design and implementation-plan obligations

Root revised the actual design and requested a second geometry/source review.
No geometry runs or product/test edits were used for this review.

* G1 is resolved: one exact dyadic face/source-vertex joint is established before
  incident selected traces; seam/pole occurrences keep their own declared sides
  and quotient. Gamma is the explicit piecewise-affine stored trace, not a
  presumed common implicit inverse curve.
* G2 is resolved: analytic/projected inverse calculations are distinct candidate
  locators; neither is called the exact value of another. Whole original-curve/
  selected-carrier-trace bounds and actual cell/edge bounds decide acceptance.
  Exact dyadic topology no longer receives unrepresented algebraic roots.
* G3 is resolved: the claim is an embedded bounded realization, explicitly not
  discovery of every source self-intersection below epsilon. The final actual
  contact decisions must be complete/current for final generations. The
  thin-box source counterexample therefore refutes no remaining claimed theorem.
* Crossing C0 source spans now uses exact rational clipping and common emitted L
  in the error calculation; it does not promise an unconstructed exact internal
  crease mesh. C-1 positional discontinuities are explicitly rejected unless
  matching one-sided source limits are established.
* Physical-plane charts now bound full inverse-image cell height, not boundary
  samples, and their projected curved traces receive a domain correspondence.
  General NURBS atlases/periodicity/singularities are not silently inferred.

Bounded unsupported/unresolved authoritative tangent sectors are an acceptable
admission boundary. This design does not promise universal completion on all
singular/toleranced CAD. It still requires the actual complete watch and named
non-watch families for the selected delivery. The implementation plan must:

1. Specify the constructive authoritative-curve strip and endpoint-star cases,
   exact accepted tests, and unresolved exits. A radius or leading-jet theorem
   name without the actual procedure is insufficient. Reconstructed selected
   polygons have the distinct finite realization contract; do not apply an
   invented curved-original-domain claim to them.
2. State the requested per-entity error inequalities and which computed source
   discrepancies are reported versus acceptance constraints. Include original
   endpoint departures, actual dyadic sites, UV/native rounding and knot/span
   extraction exactly once. A finite residual with no relevant quality limit
   must not become an implicit source-equivalence claim.
3. For physical-plane charts, bound actual in-plane output/inverse-site rounding
   as well as full inverse-image height. A pure height bound is sufficient only
   when the actual cell map has the exact claimed projected coordinates; source
   UV recovery remains a represented approximate site/map with charged error.
4. Define branch/lift records, degenerate constant/quotient paths, C-1 source
   admission and coverage-cell/physical-facet associations concretely in the
   native schema. Current finite source charts cannot acquire hidden NURBS
   periods or general algebraic-coordinate predicates during implementation.
5. Exercise the independent original-value development reader on actual output
   for these distinct faults, including implicit-site rounding, the height
   bubble and pole rectangular coverage. Native construction remains one direct
   acceptance mechanism; no second native geometry-certification layer is added.

No remaining geometry/source architectural blocker in the revised design.

## Final parameterization clarification: ACCEPT

Root identified a remaining performance-semantic regression: forcing a
same-source-t spatial chord or UV-affine trace comparison can oversample a
geometric straight line solely because its parameterization is nonlinear.
Likewise choosing a reconstructed UV polygon first can introduce unnecessary
projected curved-edge strip work before using a physical-plane chart.

At root's request edited only the main design's source-realization procedure
and physical-plane profile. The working chart is now selected first. Gamma is
the finite polygon in that chart; checked monotone correspondence profiles may
reparameterize it without inventing implicit topology sites. Actual maps/profile,
whole-interval error and endpoint coverage are retained. Parameter-matched
residuals and geometric-image distance bounds are distinct; line-to-line image
equality does not falsely assert equality at the same original t.

Selected root's finite sufficient physical-plane profile: convex original UV
rectangle, uniform preconditioned symmetric-Jacobian lower bound, four natural
projected boundary curves monotonically covering the straight sides of a
nondegenerate convex Q, and working cells inside Q. The continuous injective
source projection and boundary correspondence establish onto inverse coverage.
Its inverse Psi is a source/chart descriptor, not an algebraic coordinate store.
Whole-original-rectangle height bounds plus actual output projection/off-plane
and rounding errors bound cells without solving arbitrary inverse-cell images.
Reconstructed domains are selected directly in Q. Authoritative domains still
need their true projected-trace/strip correspondence. Unproved profiles fall
back to original-UV construction; no universal planar two-triangle promise.

This is coherent and preserves the accepted error/source claims while avoiding
parameter-speed work on the admitted nonlinear planar-square family. The plan
still must define actual monotonicity/coverage, geometric line/coupled-trace
profile and enclosure procedures with explicit unresolved/fallback outcomes.
No product/test edits or geometry runs. DESIGN ACCEPT remains in force.
