# Full meridian-profile revolution

Purpose: construct a complete curved feature from an actual existing owned
profile, retaining source history, physical seams and exact material readers.
Continue the feature breadth sweep; fillet and offset remain separate next units.
No new library, native build, generalized mesher or integration tuning is selected.
This design follows the independent [geometry/history study](../papers/2026-10-15-revolution-semantics-study.md),
[oracle study](../papers/2026-10-15-revolution-oracles-study.md) and
[runtime study](../papers/2026-10-15-revolution-runtime-study.md); their reviews
precede the implementation plan.

## Actual input, axis and supported family

`revolve(profile) -> RevolutionResult` requires the exact factory `PlanarProfile`
from extrusion. Preserve its actual local XY embedding, full original records,
Frame, namespace and revision. Revolve about profile-local positive Y through the
Frame origin: a source point (r,a,0) becomes (r cos(theta),a,-r sin(theta)).
No implicit XY-to-XZ reinterpretation or displaced rotation axis is permitted.
The existing profile contract remains: 3–32 strictly convex CCW original lattice
vertices, exact 1/256 grid and coordinates in [-3/4,3/4]. Additionally require
all original radial X coordinates >=1/4. The whole convex polygon is then away
from the axis. Reject unsupported geometry without clipping, moving, repairing,
rounding or consuming the source. No angle argument: the operation is one full
positive turn. Existing profile creation is the sole input path, without widening
canonical/Boolean readers. Source validation must detect malformed immutable
records before output construction. Equal geometry in another namespace is a
separate input; exact type guards exclude forged subclasses.

The material is the image of the entire source polygon P times the circular
rotation parameter. Because radius is positive and P is embedded, the boundary
is (boundary P) times S1, an embedded connected genus-one shell. There are no
revolution end caps. Axis contacts/poles, concavity, holes, curved sources, partial
turns and arbitrary axes are deferred families.

## Complete rational atlas and geometric readers

Use tau in [0,4], with quadrant q and s=tau-q in [0,1]. In quadrant zero,
C=(1-s*s)/(1+s*s), S=2*s/(1+s*s). Rotate (C,S) by q right angles. For source
edge i, (r(v),a(v))=(1-v)*p_i+v*p_(i+1), the full swept surface is

    F_i(tau,v) = (r(v)*C(tau), a(v), -r(v)*S(tau)).

Every face has the entire raw rectangle [0,4]x[0,1], with the two tau boundaries
identified physically but retained as distinct occurrences. Four charts cover
[q,q+1]x[0,1], q=0..3. Native query parameters are normalized (u,v) in [0,1]^2,
so raw tau=q+u. Exact rational quarter formulas include every endpoint; quarter
charts are not new physical faces or feature edges. Adjacent chart endpoint
positions and oriented normals agree; their parameter speeds can differ, so raw
first derivatives need not agree across a quarter join.

Each Patch has angular degree2 and meridian degree1, U-major/V-fast controls.
For each angular homogeneous triple (A,B,W) in (1,0,1),(1,1,1),(0,2,2), rotate
(A,B) by q right angles, then for each source endpoint (r,a), emit
(r*A,a*W,-r*B,W). All coefficients are exact dyadics. Weights1/1/2 are within
native admission and abs(Axis)<=W; weighted coordinates up to1.5 do not violate
that relative envelope. No sqrt(2), fitting or control rounding occurs. Each
Patch retains the source Frame, raw UV(q,q+1,0,1), fresh output face/chart Lineage.
This zero coefficient-rounding statement does not claim zero native jet error.

Use explicit result-owned `circle_y` curves for XZ rings at fixed local Y;
existing Curve('circle') remains an XY circle at fixed Z. Store exact line
curves for source meridian seam copies. Exact `edge_point(edge_id,t)` accepts
int/Fraction/finite binary64 excluding bool, t in[0,1], returns exact world
Fractions. Ring parameter t maps to tau=4*t, with t=1 evaluating the terminal
endpoint of quadrant3. Seam parameter t is the original source edge parameter.
`coedge_uv(coedge_id,t)` returns exact raw (tau,v) for that physical edge's
intrinsic parameter, independently of traversal sign. IDs are one-based; slots
are zero-based. Reuse existing immutable primitive record types only where
field and kind semantics remain explicit; do not extend canonical packed tags.

`classify(chart_slot,u,v)` uses exact submitted values in normalized chart
coordinates. Invalid/nonfinite/out-of-square queries return INVALID. Admitted
rows are BOUNDARY at v=0/1 or the physical seam (q0,u0 or q3,u1); an internal
quarter join is INTERIOR for 0<v<1. Full square is admitted, no rejected interior
trim. Reject bool and malformed coordinates consistently with the owned profile.

## Physical topology and orientation

For N source vertices, output has N anchor vertices at the original theta0
positions; N ring edges (each starts/ends at its original anchor); N meridian
seam edges with original endpoint IDs. Edge slots0..N-1 are rings, N..2N-1 are
seams. Face i comes from source edge i and has exactly one loop, four charts,
positive orientation and four ordered uses. Face/loop slots are i; chart slots
are4*i+q and coedge slots4*i+k for the following ordered k=0..3 word:

1. ring i forward: raw UV=(4*t,0);
2. seam i forward at tau4: raw UV=(4,t);
3. ring i+1 backward: intrinsic raw UV=(4*t,1);
4. seam i backward at tau0: raw UV=(0,t).

The seam's two uses belong to the same face and have distinct UV maps. A ring's
uses belong to the two adjacent source-edge faces, even when the two source
vertices share an axial or radial coordinate. Share using original source IDs,
never by coordinate welding. Every edge has two opposed uses; every face loop
closes by vertex identity. The anchor link has four ports: two ends of its closed
ring and incoming/outgoing seam ends, forming one cycle. Establish connected
face adjacency and V-E+F=0. Complete support/domain and edge-map checks establish
embedding; Euler/incidence alone are insufficient.

The natural angular-U/meridian-V normal points outward, proportional to
(da*C,-dr,-da*S). Radial positivity and nonzero original source edges guarantee
nonzero Jacobians throughout all chart squares. Horizontal meridian segments
produce annular planar faces; vertical ones cylinders; sloped ones cones. All
remain the same declared swept-face family rather than relying on canonization.
Six-kind output counts are N vertices,2N edges,4N coedges,N loops,N faces,4N
charts =13N. No result packed_tables export or generalized result mesher exists.
`topology_input()` uses existing typed ABI5 with snapshots=(), preserving all
same-face seam occurrences and fresh output namespace/revision1.

## Exact material and measures

Invert the retained Frame with Fractions. For local query (X,Y,Z), put Q=X*X+Z*Z.
For every source directed edge a->b with dr=b.r-a.r, da=b.a-a.a, the closed
support is A+B*sqrt(Q)>=0, where A=dr*(Y-a.a)+da*a.r and B=-da. Determine its sign
by exact rational sign cases and sign-aware comparison of A*A and B*B*Q. Do
not unconditionally square or compute a floating radius. Return exterior if any
support is negative; otherwise return boundary with EVERY zero supporting face,
or interior if all are positive. The axis is outside. Exact originals, including
subnormal and nextafter floats, are interpreted before arithmetic.

Volume is `Measure(rational=0,pi=K)` using the existing exact coefficient value
type, where K=scale^3/3*sum((r_i+r_j)*(r_i*a_j-r_j*a_i)). This is twice the source
polygon's radial first moment; positive for the admitted profile. Tight local
bounds are X,Z in[-max(r_i),max(r_i)] and Y in[min(a_i),max(a_i)]. Do not substitute
source area times an arbitrary radius or sample-derived bounding box.

## Complete six-kind source history

Retain the original profile on both solid and result. The result records that
its entire source face generates the material solid; that semantic operation
relation is separate from the six boundary entity tables. Each original source
entity has exactly one forward record; each of13N output entities one reverse
Origin. Keys use actual source/output namespace,revision,kind,ID. No equal-geometry
substitution or output-as-source parent is allowed.

Source vertex/edge/coedge records are modified with respectively1/1/2 same-kind
images: anchor vertex, seam edge, and its two distinct seam uses. Thus3N modified
source records and4N modified output origins. Incidence/role changes justify
modified even though theta0 geometry is unchanged. Source loop/face/chart records
are consumed: no modified images, but generated descendants remain explicit.
Consumed means absent as a boundary image, not absence of ancestry. There are
9N generated origins: N rings,2N ring uses,N loops,N faces,4N charts.

An operation-owned frozen Origin contains target,relation(modified/generated),
parents,role,source_t(optional affine triple),source_uv(optional two affine rows),
periodic(optional affine triple),orientation(+1/-1). Maps use rational coefficients.
For edges/coedges the map arguments are(u=t,v=0); faces/loops use raw(tau,v);
charts use normalized native(u,v); vertices use(0,0). `periodic` emits tau, a
piecewise rational circle coordinate, never radians or angle divided by2pi.
`source_uv` emits the original profile's actual raw((r+1)/2,(a+1)/2).
The original source t and traversal orientation remain distinct.

| Origin role | Relation and original parents | Exact maps / orientation |
| --- | --- | --- |
| anchor | modified; own vertex | constant source UV,periodic0,+1 |
| seam | modified; own edge | source_t=u,source UV along original edge,periodic0,+1 |
| seam_end | modified; own coedge | source_t=u,source UV along original edge,periodic4,+1 |
| seam_start | modified; own coedge | source_t=u,source UV along original edge,periodic0,-1 |
| ring | generated; own vertex | constant source UV,periodic4*u,+1 |
| ring_start | generated; source start vertex + own edge/coedge | source_t=0,constant source UV,periodic4*u,+1 |
| ring_end | generated; source end vertex + own edge/coedge | source_t=1,constant source UV,periodic4*u,-1 |
| swept_loop | generated; own edge/coedge + original loop | source_t=v,source UV along original edge,periodic=u,+1 |
| swept_face | generated; own edge/coedge + original face | source_t=v,source UV along original edge,periodic=u,+1 |
| swept_chart | generated; own edge/coedge + original chart | source_t=v,source UV along original edge,periodic=q+u,+1 |

Null maps are explicitly inapplicable. Whole loop words supplement affine loop
maps. Forward images equal all modified origins naming that source; generated
links equal all generated origins naming it. Reverse parents, role, exact
source IDs and maps must match the declared generating entity and occurrence;
reciprocal but consistently wrong ancestry is invalid. Lookup rejects foreign
namespaces/types. Immutable history is published only with a valid entire solid.

## Construction, native consumers and independent qualification

Construct private records and origins, validate complete source binding, native
controls/descriptors, full domains, loop words, exact whole edge/UV/carrier
identities, opposed uses, vertex links, connected shell and every precise history
map, then publish once. A private final validator supports late exception tests
without public failure flags. Failure preserves source and prior result; retry
works and namespace gaps are allowed. No native handle is needed for construction.

A separate revolution sampler copies Candidate float UV before trim decisions,
bounds complete candidates to1,048,576 and grid resolution2..257 excluding bool,
with chart-count budget checked before generation. Reuse existing owned generic
metadata/row/child-lifetime helpers where actual contracts fit. Do not widen
existing operation guards. Borrow Engine and close patch/query/result children
on all outcomes. Bind exact descriptors, querybits, ownership, Lineage, backend,
order/count/statuses. Convert per-Patch source Frame, orient normals, preserve
row order and IDs. INVALID rows keep status-1/NaN vectors; no fallback or partial
batch publication on failure. A closed Engine raises the existing error.

Independent geometry truth derives only from original source vertices and Frame.
Reconstruct complete source/output/carrier/domain/graph/history requirements,
check exact rational whole-boundary identities with every public reader, and
exercise actual native CSR mates/neighbors including slot permutations and
same-face seams. Position component error<=2^-36 and oriented normal component
error<=2^-30 at actual copied binary64 UV. Expected unit normals use integer
edge deltas DR,DA: M=DR^2+DA^2 and rational sqrt intervals from isqrt(M<<128)/2^64,
with an exact-square branch. Multiply component intervals by exact rational
circle components; grade against both endpoints. No production normalization or
CPU/GPU parity supplies truth.

Include real3/6/16/32-vertex off-axis profiles, asymmetric shapes, horizontal/
vertical/sloped edges, minimum lattice edges, all Frame scales, source cyclic
rotations, equal-geometry distinct namespaces, source mutation and malformed
history/control/UV/topology/sample faults, axis exclusion and radial boundary,
all quarter joins and physical seam endpoints, sign-aware exact material
near-boundaries and late constructor/sampler failure with retry. Existing
extrusion/Boolean/body/adjacency/runtime guards remain regression targets.

The actual caller consumes material, exact boundary/maps, forward/reverse
history, native first jets/oriented rows and native CSR mates/neighbors. Measure
prepared-profile and points-to-consumed walls including construction/publication,
all reads, request work, engine/topology setup, transfers/checks/cleanup. Separate
profile preparation, independent qualification, artifact writes, reference and
process/module overhead; nested phases/medians are nonadditive. Use genuinely
different3/6/16-vertex recipes and repeated fresh CPU1/CPU24/P1000/P1001 cells.
Actual geometry/sample SVG is a wireframe/point display, not result meshing.
Optional already-installed reference must revolve identical world source curves
about the world Frame-origin Y axis with pure full-turn semantics, compare finite
bounds,manifold,volume and actual boundary witnesses, and probe source retention.
No optional reference/history shortcut or local tests waive required external
community alpha qualification, which remains incomplete. Return-pass integration,
Python role and NCSX matched timing stay deferred until the breadth units finish.
