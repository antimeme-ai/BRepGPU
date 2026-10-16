# Whole native mesher plan: independent runtime/numerical review

2026-10-16. Review of `docs/NATIVE_MESHER_PLAN.md` and
`docs/native-mesher-topology-plan.md`, against accepted design `f7aedef` and its
constructive resolutions. The runtime unit's own plan is not self-approved by
this review. Geometry-unit acceptance is pending its complete finite pole/cap
construction. No product/test edits or execution.

## P1 — complete source-boundary request horizons

Blocking determinism seam in topology plan section3 and root shared-boundary
rounds. Sorting arrived split requests does not determine which requests have
arrived. A fast face can request an edge split; applying that partial set can
cancel a slower incident face before its own deterministic request. CPU/P100
completion then changes retained samples even if all individual decisions are
certified and sorted. Contact epochs already avoid this issue with a full fold;
the source-boundary coordinator needs the corresponding finite request horizon.

Resolution sent to owner/root: define a boundary epoch per definition or static
incident dependency component. Every active face from the same snapshot reaches
its deterministic first-boundary-request yield or bound-settled horizon. Only
after all those results arrive does the coordinator apply the complete merged
request set. Unaffected completed faces remain cached; only incident generations
restart. Do not wait for a face to finish against an invalidated schedule, or
form the epoch from whichever jobs happened to report first.

Related small clarification sent to topology owner: the pending-cell priority's
`failure reason` must be a known preassigned schedule cause/task kind, not a
failure category learned from a returned numerical result. Otherwise equal-
diameter pending cells lack a defined order. Diameter plus stable cell key alone
is sufficient if no preassigned cause is needed.

Owner closure: topology section3 now selects a per-definition epoch from one
snapshot. Every active face reaches its canonical bounded first-request yield
or local-bound-settled horizon; the coordinator waits for ALL current-generation
participants, fails on a hard current-generation failure, then merges/applies the
full request set. Only changed incidents restart. Section5 now uses diameter+
cell key only; failure reason cannot change priority. Mirrored these exact rules
in the runtime plan at parent's request. **P1 and the priority clarification are
resolved.** This is review of the topology owner's protocol and seam alignment,
not self-approval of the runtime implementation unit.

## Reviewed native topology/numerical choices

The indexed all-other-loop winding admission is constructive after exact simple
boundary/nonintersection admission. With one CCW outer and disjoint CW holes,
querying a hole vertex against every other loop must give total winding1:
outside gives0 and a containing hole changes it to0 or lower. Nonintersection
and adjacency-overlap checks are indispensable prerequisites. This avoids the
old explicit all-hole-pairs containment loop without inventing a universal
BVH worst-case bound.

Unit-weight initial CDT on exact dyadic transformed sites T0*q is coherent with
the existing constructor. Positive exact determinant preserves orientation.
A shared power-of-two scale is an integer adapter, not a general-rational global
LCM. Subsequent append-only exact dyadic orientation/length predicates are a
distinct mutable owner API. Fixed normalization may filter newly deep dyadics
with outward enclosures and exact fallback; it must not quantize them to the
initial integer lattice. No dynamic incircle API is falsely inferred from the
old immutable initial predicate table.

Fixed generation M0, terminal equal-longest LEPP ties, exact midpoints and sticky
heuristic ancestry flags match the selected transformed-norm proof. Circumcentres
are exact rational proposals rounded once, not geometric acceptance. Requiring
the proposal inside its failed target/unspent shared edge prevents a remote
circumcentre insertion from repeatedly granting spent lineages a new heuristic.
Refinement has no flips; optimization is a separate accepted fixed-site phase.
The finite-state strict potential decrease applies there, not to the whole mixed
boundary-restart/refinement pipeline. Its source regularity/strict-feasibility
and singular-profile qualifications remain explicit.

Actual-facet intersection is specified on stored XYZ exact dyadics, including
plane-zero and coplanar cases. Physical key/declared simplex incidence, rather
than coordinate equality or shared-vertex blanket exemptions, controls allowed
contact. The sole definition contact owner, frozen complete pair epochs and
both-cell marks avoid independent workers approving stale meshes. Stable global
optimizer commits recheck current facets and replacement bounds before mutation.
This construction claims actual output embedding, not discovery of all source
self-intersections hidden below tolerance.

Complete source inventories and explicit unresolved/unsupported output are
retained. Stored XYZ is selected for every display vertex, so approximate
original-UV recovery or physical-plane chart inversion cannot silently move the
accepted mesh. Independent original-value readers remain off the product path;
no additional native certificate system is required.

## Assembly world-error transport: accepted resolution

Root added `Carry positional tolerance through actual assembly transforms` to
the main plan after the algorithm review finding. Compose source usage paths
exactly from source words, store a once-rounded affine matrix and retain its
source path. Let T=(A,b), stored T_hat=(Ahat,bhat), source S and actual local
mesh L. Then

    T(S)-T_hat(L) = A*(S-L) + (A-Ahat)*L + (b-bhat).

The proposed Gram/Gershgorin bound kappa>=max(1,||A||2) and eta over a box
containing actual L therefore imply world error<=kappa*tau+eta. The source/
selected-domain box expanded by requested epsilon contains L when local tau is
capped by epsilon. Canonical dyadic local lower allowances over every instance,
with typed refusal when eta exhausts epsilon, preserve the world positional
claim. Exact source-only comparison transported by A does not acquire the output
matrix eta a second time. INSTS source path and actual stored matrix are enough
for the independent reader to reconstruct this relationship. Scoped math
verdict: ACCEPT; rendering arithmetic remains its separate consumer scope.

## Initial pending items, before owner closure

- Owner closure of P1/full request horizon and pending-priority wording.
- Complete geometry unit, especially original evaluation at arbitrary dyadic
  sites, authoritative strip/star maps, explicit pole cap coverage/subdivision,
  and how special cells enter the same final facet/contact state.
- Main/unit interface reconciliation after those choices. Runtime unit review
  belongs to the other independent reviewers, not this author.

## Geometry unit review and owner resolutions

Read complete `docs/native-mesher-geometry-plan.md` after publication. Original
Cox-on-span Bernstein extraction is constructive for unclamped/nonuniform knots;
zero denominators take the zero branch, and source positional continuity is
checked on complete one-sided limit curves. Exact clipping/subcell L preserves
the actual emitted affine map. The analytic triangle remainder's1/8 terms follow
the barycentric variance bound, with mixed derivatives retained. Bilinear pole
interpolation instead uses successive one-dimensional interpolation, so pure
xi/rho second derivatives plus corner departures suffice; it does not need to
reuse the triangle bound incorrectly.

The host correspondence composition is a separate larger-degree unit: ordinary
GPU residuals stay degree7/33, selected source-to-carrier host comparisons may
reach38, and authoritative pcurve/surface composition plus spatial comparison
can reach224. The plan now names those counts and exact/interval resource limits.
It does not promise a36-coefficient accelerator for arbitrary compositions.
Candidate-span polynomial extensions conservatively cover a continuous knot
crossing, provided every applicable extension's denominator is proved positive;
otherwise subdivision/refusal remains explicit.

### P2 — authoritative analytic nodal maps (resolved)

Initial section7 assumed source star center A was rational. Admitted analytic
circle/ellipse pcurve values at dyadic parameters can be transcendental, as can
projected carrier/pcurve values in a plane chart. Rounding them and calling the
PL map exact would change authoritative geometry. Sent this blocker to root and
geometry owner.

Owner selected finite `OriginalPcurveAtParameter(use,s)` or
`ProjectedPcurveAtParameter(chart,use,s)` source-point descriptors. The fan map
is explicitly `B(x)=x+lambda_A(x)*(Q-A)`, with lambda defined by original point A
and rational support vertices. Exact arithmetic handles rational A; source
enclosures prove positive denominators/orientations for analytic A. A is never
a topology site. The remaining map is finite affine arithmetic, with uncertain
piece domains handled through enclosing all applicable extensions. Actual revised
section7 supplies this procedure. Scoped verdict: resolved.

### P3 — pole sites must be constructed by dyadic fractions (resolved)

An arbitrary dyadic angular u_j does not imply the linear cut g(u_j) is dyadic:
uL=0,uR=3,vA=0,vB=1,u_j=1 gives1/3. Initial fan cuts must use a dyadic fraction
theta_j for BOTH interpolations, with a power-of-two subdivision count and then
theta midpoints; radial cuts reuse theta. Sent to owner, now explicitly selected
in section8. Radial refinement requests new existing master nodes and forms a
dyadic annulus, rather than passing the generally nonrational source rho=1/2
line to exact dyadic topology. Scoped verdict: resolved.

### P4 — cylinder arc endpoint correspondence (resolved)

Section5's CylinderArcOrder initially says an original rational circle covers
the same ordered arc as a selected dyadic angular segment. Source endpoint ray
angles generally are not dyadic, so exact endpoint agreement is unavailable.
The source→selected trace bound must include angular/ray endpoint departures in
both directed correspondence, as well as radial/height tube error. Physical
chord sagitta/canonical endpoint departures are a separate part and do not alone
bound this source-to-carrier trace mismatch. Sent to geometry owner/root; a finite
endpoint-ray/selected-angle descriptor and enclosing ordered-arc correspondence
can resolve it without putting exact atan roots into CDT.

Owner closure in actual section5: source ray/branch descriptors define the
original arc, enclosed rational-angle bisection bounds the true-to-chosen endpoint
departures, and the monotone angle correspondence has discrepancy at most the
maximum endpoint discrepancy. Charge frame_norm*radius*delta_angle plus separate
radial/axial terms in both directed trace bounds. Physical chord sagitta/canonical
endpoint departures remain their separate comparison. Exact identity is claimed
only when rays actually agree. Scoped verdict: resolved.

## Actual display consumer seam raised by root (resolution proposed)

Checked existing `native/om10/scene.hpp` and `math.hpp`: parser requires nearly
orthonormal carrier/instance frames at2e-10, while the source/geometry contract
retains independent axes and source placements admitted near1e-9. Stored-XYZ
alone does not make those scenes consumable. Cone apex normal sign also comes
from a tiny recomputed radius at rounded UV, which can choose the wrong regular
sheet; sphere pole cos-sign has the analogous issue.

Proposed to root: an explicit new known BGMESH01 feature profile, unchanged
record sizes, with validated finite source frames and positive-determinant stored
instance transforms; finite normal-limit/sheet flags derived from admitted source
quotients; ideal sphere limiting normal and cone one-sided derivative-cross
normal; and scaled cofactor instance normal transformation for approximate-rigid
matrices. Legacy strict scenes retain their old profile/behavior. This needs
matching CPU/CUDA/parser/independent consumer tests, not synthetic orthogonal
carriers substituted for originals. No normal-angle guarantee is added. Root
selected the finite profile/flags extension and its matching tests. Integrated
the concrete consumer seam into the runtime plan at parent's request, including
Vertex32→Jet.flags value2→project transport and a v2 capability/factory so an old
plugin cannot silently ignore the new flags. Geometry owner independently
accepted these normal-limit/cofactor formulas, with cone sign(rho) distinguished
from r0. The runtime unit's updated procedure remains for its other reviewers;
this author does not self-approve it. No consumer code is implemented yet.

## Final scoped verdict

**ACCEPT for implementation planning** for the independently reviewed root/
topology/geometry numerical and runtime-interface scope. Reread the actual owner
resolutions, including complete request horizons, stable pending priority,
instance world transport, finite authoritative nodal/fiber maps and singular
cap child recipes. Geometry physical-plane displacement now uses direct plane
lift movement plus one original-height term; it does not require evaluating
the inverse source chart outside Q at intermediate support points.

Root selected storage of authoritative PL-star and tapered-strip maps as actual
geometry semantics. Added `DOMMAPS_`, `SUPPORTS` and face domain-map references
to the runtime schema, with source-point/P/Q/projection/envelope/composition
payloads. Geometry owner independently reviewed the actual fields and accepted
mapped-pcurve-s versus edge-t semantics, pre-B/full-map order, finite frame/taper
and ownership. The stored-map integration item is resolved; this is not a
certificate protocol or self-approval of runtime code.

Geometry owner subsequently raised mixed authoritative/reconstructed coedges.
Root selected the finite HybridPullback policy; reread the actual geometry
section7 and header/schema relation. Authoritative endpoint A owns the reference
joint, reconstructed reference arcs are explicit B^-1(selected polygon), and
T supports avoid reconstructed selected polygons except fixed common Q joints.
The homeomorphism B-then-T maps every reference arc to its selected boundary;
ordered domain displacement and original-edge/reference mismatch include the
inverse/forward movements. DOMMAPS carries AuthoritativeOriginal/HybridPullback;
all-reconstructed faces retain identity reference0. No unknown inverse roots,
private callbacks or free tolerance are introduced.

One final physical-plane clarification was accepted and added: every reconstructed
inverse-B reference arc, not only the final polygon/raw authoritative points,
must lie in Q before source inverse evaluation. Auxiliary P supports may extend
outside Q. Exact/enclosed inverse affine pieces against convex-Q halfplanes
supply this finite check; failure refines chosen sites/support or refuses. A
simple closed reference boundary in convex Q also contains its interior there.
No undefined inverse at an auxiliary support is evaluated. OriginalPcurve source-
point descriptors include retained use branch/lift and already-mapped s.

All findings in this independently owned root/topology/geometry review are now
resolved. **ACCEPT for implementation planning.** The runtime implementation
unit remains subject to its independent reviewers. No production/test execution
or product code changes accompany this review.

## Node recovery versus positional accuracy clarification

Reread geometry sections1/7/10 after the root's input-semantics clarification.
The node cap compares original V or exact Ce(t) to original S at the retained
dyadic recovery UV, with the endpoint V-to-Ce check preserved. Neither a rounded
emitted XYZ nor an implicit plane-chart inverse replaces that stored UV. The
source-only instance norm transports this cap without output-transform eta.
Whole trace/reference, chord and domain-plus-actual-cell approximation retain
the definition-local positional allowance. Hybrid inverse-B movement is charged
to its whole-reference positional comparison, not relabeled as nodal recovery.

The proposed rational quarter-circle example is constructive: for radius R and
t in[0,1], homogeneous actual-chord residual Bernstein coefficients have maximum
Euclidean norm 2R/3 with denominator lower bound1; R=1/256 places the sagitta
above0.001 while this proven bound is below0.005. Exact endpoint recovery has
zero node discrepancy. This distinguishes the input meanings without weakening
either comparison. **ACCEPT** for this revised geometry/quality seam. Geometry
owner independently accepted the matching runtime QUALITY_/COMPONTS wording;
this is not self-approval of the runtime unit. No product edits or execution.
