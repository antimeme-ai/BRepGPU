# Full revolution: independent geometry, history and consumer truth

Research only, following the delivered extrusion unit. Read current doctrine,
AGENTS, feature roadmap, actual extrusion/body/oracle/sampling code, pinned OCCT
revolution source/tests, pinned USD BRep bindings/tests and rational-conic notes.
No implementation, product/reference execution, new dependency or native build.
Source hashes and immutable upstream paths are in
`revolution-oracles-sources.tsv`; bibliography in `revolution-oracles.bib`.

## Selected candidate and original source placement

Reuse the actual existing factory `PlanarProfile`, whose original points lie in
local XY with Z=0. Interpret its original coordinates as radial r=X and axial
 a=Y; revolve about **profile-local +Y**, through its original Frame origin.
Do not silently move XY into XZ. At zero revolution the output meridian is the
actual original source embedding. Require every original r>=1/4; retain the
existing upper r<=3/4, |a|<=3/4, 1/256 lattice, 3..32 vertices, strict global CCW
support, immutable source/frame and supported positive power-of-two scales.
Full turn only; no poles, axis contact/crossings, negative radii, partial turns,
arbitrary axes, holes, concavity or repair. A distinct result/solid type preserves
existing canonical/extrusion type guards. This gives annular planar, cylindrical
and genuinely slanted conical bands in the same complete source-driven family.

For edge endpoints p=(r0,a0), p+d=(r1,a1), define meridian t in[0,1],
r(t)=r0+t*dr and a(t)=a0+t*da. The exact output map is

    S(U,t)=(r(t)*c(U), a(t), -r(t)*s(U)), U in[0,4].

Here U is the existing four-quarter rational atlas parameter, **not radians or
uniform angle**. For U=q+w, w in[0,1], start with
c=(1-w²)/(1+w²), s=2w/(1+w²) and rotate (c,s) by q right-angle steps. Therefore
c²+s²=1, angular direction is positive, each quadrant is covered exactly once,
and U0/U4 share one physical point with distinct source occurrences. The angular
speed is 2/(1+w²), so adjacent rational quarters need not have equal raw U
derivatives at their common endpoint. Bind a jet oracle to the actual submitted
chart occurrence; compare common positions/unit normals without inventing global
parameter C1 continuity or treating U as angle.

Four degree(2,1) native patches per source edge suffice. Base homogeneous
angular controls are (cx,sy,W)=(1,0,1),(1,1,1),(0,2,2); combine each with both
source meridian endpoints to emit (r*cx,a*W,-r*sy,W), with quadrant rotation.
Every product is exact dyadic; no sqrt weights, fitting, sampled reconstruction,
control quantization or carrier clipping is necessary. Native admission is
|Aaxis|<=W, weights1..256, so r<=.75 and |a|<=.75 fit even when a homogeneous
coordinate exceeds1. Keep the original per-Patch Frame and atlas domains
[q,q+1]×[0,1]. Native queries are normalized (u,v) in[0,1]²: the actual retained
chart occurrence supplies q and raw U=q+u. Existing source profile controls are
evidence, not output carriers.

The rational map is independently checkable by denominator-cleared polynomial
identities. Sauer's rational-conic discussion supplies the homogeneous framework;
the asymmetric dyadic quarter parameterization above is directly derived here,
not a claim that its parameter equals the symmetric sqrt-weight construction.
[Sauer, Geometric Modeling, §§6.3–6.4](https://www.fim.uni-passau.de/fileadmin/dokumente/fakultaeten/fim/lehrstuhl/sauer/geyer/Vorleseskripteab2012/GeometricModelling-SS2015.pdf).

## Exact material and complete boundary oracle

Convert an actual submitted world point to local (X,Y,Z) using exact Fraction
values of the original binary64 Frame. Set Q=X²+Z² and rho=sqrt(Q). For every
original directed edge i, its profile support inequality is

    dr*(Y-ai) - da*(rho-ri) = A+B*sqrt(Q) >= 0,
    A=dr*(Y-ai)+da*ri, B=-da.

Determine signs using rational arithmetic: Q0 gives A; zero coefficients and
same-sign terms are immediate; opposite-sign terms compare A² with B²Q,
retaining the original sign branch. For opposite nonzero signs the result is
sign(A)*sign(A²-B²Q). Squaring without checking signs admits false material and
false boundary. All strict positive supports mean interior; any negative means
exterior; otherwise boundary with **every original supporting output face ID**.
The axis is exterior because all source r>=1/4. No floating sqrt, epsilon,
angular inversion, CPU/GPU parity or product material helper supplies truth.

This is a complete model rather than a sample argument. The revolution map is a
homeomorphism from P×S¹ onto the solid because r is strictly positive; its inverse
recovers (rho,Y) and angular direction. Thus the boundary is exactly ∂P×S¹.
Require one whole swept domain for each actual source edge, original endpoint
bijections, complete four-chart rational coverage, explicit periodic endpoint
identification and no extra faces. Strict support makes source edges a simple
convex boundary; distinct bands only meet on the declared shared vertex rings.
This proves embedded complete coverage independently of counts/volume. There
are **no artificial start/end caps** after a full turn.

A supporting scalar check is exact volume pi coefficient:

    volume = pi * scale³/3 * sum((ri+rj)*(ri*aj-rj*ai)).

It follows from volume=2*pi*integral_P r dr da and the polygon first moment.
Store the rational coefficient, never approximate pi as exact. Full world bounds
are origin+(scale*[-rmax,+rmax], scale*[amin,amax],
scale*[-rmax,+rmax]). These are useful independent foreign comparisons but cannot
replace the domain proof.

## Topology, seam occurrences and six-kind history

One physical anchor per source vertex at U0; one closed ring per source vertex;
one seam edge per source edge at U0/U4. Face i sweeps source edge i. Its one
rectangular atlas loop uses: ring i+, seam i at U4+, ring next(i)-, seam i at U0-.
Intrinsic maps are applied before occurrence orientation. Closed rings have
start=end anchor but two endpoint ports; seam edges have two opposite uses on
**the same face**. Never deduplicate these occurrences by XYZ or neighbor face.
Internal quadrant joins are chart boundaries, not extra physical seam edges.

Counts: N vertices,2N edges,4N coedges,N loops,N faces,4N charts =13N six-kind
entities. Euler0/genus1 is a supporting check. Directly check all curve/UV
identities, oriented closed words, two opposed uses per edge, one shell and one
cyclic link at each anchor. The link has four distinct ports: both ends of its
closed ring plus its two incident source-edge seams. Native ABI5 must return
N ordinary paired ring edges and N same-face paired seam edges, under full
edge/face/coedge slot permutations and actual typed mate/neighbor readers.

The agreed provisional history convention distinguishes consumed source
interiors from deletion: source V/E/C are modified, source loop/face/chart are
**consumed** (no same-kind images, contextual descendants allowed). No retained
input is inferred from geometric equality at angle0.

- Source vertex has one modified anchor image and generates its ring.
- Source edge has one modified seam-edge image and generates its swept face.
- Source coedge has two modified seam-use images, at atlas0/4 with opposed
  traversal, while both retain original intrinsic source-edge t.
- New loops, ring uses, faces and charts have explicit actual source parents;
  consumed loop/face/chart remain contextual provenance where specified.

There are4N modified origins (N anchors+N seams+2N seam uses),9N generated
origins (N rings+2N ring uses+N loops+N faces+4N charts), and all3N+3 inputs are
dispositioned. Root design must choose the exact ordered parent lists and affine
fields before code. Source t for a face is its meridian V, not angular U;
original source face/chart UV is ((r(V)+1)/2,(a(V)+1)/2). Ring sweep parameter is
new; it must never masquerade as an original profile-edge parameter. Record
angular atlas interval/occurrence separately from source t and orientation.
Check every forward image/generated link and reverse origin against independently
reconstructed roles, source namespace/revision and full maps. Equal geometry in
another profile namespace is not ancestry.

## Actual native point and normal contract

At every actual copied binary64 native query (u,v), form exact Fractions, retain
its actual chart quadrant/endpoint occurrence, map raw U=q+u, and evaluate the
rational c,s and original
linear r,a, then apply the original Frame. Test direct world position component
error<=2^-36. Do not evaluate truth at an intended pre-rounding Fraction UV.

For angular-U/meridian-V order, the outward unit normal is

    (da*c, -dr, -da*s) / sqrt(dr²+da²).

The omitted angular speed and radius factors are strictly positive. CCW source
orientation therefore fixes every face's sense without per-face guesses. This
also tests inner inward-radial walls and horizontal annular faces. With original
integer grid deltas DR,DA, set M=DR²+DA². Perfect-square M gives rational truth;
otherwise K=isqrt(M<<128) gives K/2^64<sqrt(M)<(K+1)/2^64. Signed rational
component intervals enclose each exact component. Compare actual component error
against **both** interval endpoints, conservatively requiring<=2^-30. Numerator
magnitudes<=384 give interval width<384*2^-64<2^-55. This independently extends
the delivered extrusion mechanism; no product normalization supplies expected
truth. The component budget is an observed published-output contract, not a
consequence automatically inferred from the evaluator partial grades.

Check complete request/result ownership, copied UV bits, chart/face Lineage,
native descriptors/status/count/order/backend and scatter order, all rejected/
invalid rows, per-call child cleanup and healthy borrowed-Engine retries. Exact
CPU construction and GPU sampling/adjacency are distinct placements; do not call
the constructor GPU accelerated.

## Direct witnesses and fault sensitivity

Use an asymmetric triangular meridian, a chamfered meridian with horizontal and
vertical edges, and a verified many-sided convex lattice meridian. Include the
maximum32/128-chart case, minimum grid edges/radius, every Frame scale and
translations. Cyclic source order preserves material/volume but relabels exact
source history. Keep independent same-geometry namespaces distinct.

Material probes must include radial/axial extremes, axis/central hole, each face,
every shared ring, seam anchors, all quarter charts, exact rational off-cardinal
circle points, tiny Fraction/nextafter offsets and non-square Q. Include literal
A+B*sqrt(Q) witnesses for both opposite-sign branches, equality, sign-squaring
traps and irrational interior/exterior; a rational squared-radius-only predicate
must not pass. At shared rings return both incident faces; seam/chart duplication
must not add false material supports. Exact seam0/4 trim boundary differs from
internal quarter joins; V0/1 remain physical ring boundaries.

Negative inputs cover axis contact/crossing, under-minimum positive radius,
clockwise/concave/collinear/winding-star/duplicate/off-grid/nonfinite/oversized
source profiles, malformed requests and unsupported angle/axis APIs if exposed.
Test bounded/failed iterators via the existing profile admission contract.
Inject wrong handedness, swapped angular/meridian controls, weight corruption,
missing chart/face/ring, collapsed closed-ring endpoint ports, merged seam uses,
wrong same-face mates, sign-branch material error, inward-normal flips, wrong
source t/history parent/consumed outcome and actual-query bit mismatches. Direct
falsified outputs plus an actual late-construction exception address named fault
classes; do not label them a broad mutation score. Root may select actual
production mutants if direct mechanism needs stronger evidence.

Whole caller costs should consume exact material/history/boundary readers, full
per-chart sampled rows and actual ABI5 topology. Separate profile preparation,
complete revolution publication, full useful wall and complete input-to-consumed
wall from qualification, SVG/exports, foreign reference and process startup.
Use CPU1/CPU24/both P100s with existing binaries and serial GPU lanes after the
reviewed design/plan and actual missing-operation red. No tuning lane is selected.

## Primary references and feasible isolated experiment

OCCT documents meridian/axis coplanarity, angular U and meridian V, and warns
that the general surface constructor does not itself establish source validity.
Its full revolution builder lifts edges to faces and faces to solids, with
explicit generated/degenerated-edge APIs. These motivate explicit axis clearance
and complete owned ancestry, not adopting its internal history labels.
[SurfaceOfRevolution](https://dev.opencascade.org/doc/refman/html/class_geom___surface_of_revolution.html),
[MakeRevol](https://dev.opencascade.org/doc/refman/html/class_b_rep_prim_a_p_i___make_revol.html).

Read pinned OCCT `bug30346_1` (literal full/partial history counts), `bug31460`
(actual checkshape+bopcheck on full/partial curved sweep), and the revolution
GeomGridEval tests (scalar-evaluator parity plus a literal cylinder-radius check).
They address real regressions but do not prove complete polygon toroidal material,
source maps or seam multiplicity. Axis/curved cases outside this unit are future
scope, not evidence that our operation handles them.

Existing USD BRep exposes pure `rotational_sweep` around a supplied world axis;
its documented curves remain owned/unchanged and the result is newly owned.
The pinned wrapper and input-preservation test match that scope. Its basic sweep
test uses an open line and only checks Brep type/nonempty faces, so it cannot
certify this closed solid. [SmApiCreateRotationalSweep](https://nvidia-omniverse.github.io/usd-brep/latest/api/function_SmApiBrep_8h_1abc10938350f69803e70ebe4afd19b893.html).

A feasible optional fleet experiment creates actual source world segments from
our dyadic profile, axis=(0,1,0) through Frame.origin, then pure full360 sweep.
Check unchanged original curves, finite full bounds, manifold result, analytic
pi-volume and actual surface closest-point witnesses on every swept edge at
rational-circle points. Do not compare native quarter U with foreign angle or
unknown NURBS parameters. If a public reference API is absent, report it; do not
invent foreign material/history semantics. No experiment was executed here.

The external alpha policy remains required and incomplete. Pin applicable
community revolution cases against advertised scope before observing outcomes;
missing data/adapters, unsupported required cases and unresolved disagreements
remain non-green. Reference-library success, parity and selected regression
examples do not qualify BRepGPU or waive the operator's community-suite gate.
