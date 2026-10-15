# Owned convex-profile extrusion

Purpose: establish real profile-driven feature construction before returning to
integration/tuning. The operation builds two trimmed caps and every swept side,
a closed embedded solid, exact readers and complete feature history from the
actual source profile. It does not call a box/Boolean factory or widen existing
canonical/Boolean readers. Revolution, fillet and offset are separate later units.
Read the three extrusion studies/bibliographies; no new dependency/native build.

## Inputs and exact placement

`make_profile(points, frame=Frame()) -> PlanarProfile` accepts an ordered strictly
convex CCW polygon with3..32 distinct two-coordinate vertices. Coordinates accept
int/Fraction/finite binary64, excluding bool, and must be exactly on1/256 grid in
[-3/4,3/4]. Read at most33 vertices and at most3 coordinates per vertex; reject
malformed/off-grid/nonfinite data without rounding, sorting, welding or repair.
For every directed edge, EVERY nonincident original vertex must be strictly to
its left. This global exact support test excludes concavity, collinearity and
self-crossing winding stars that positive adjacent turns/area do not exclude.
No closed repeated endpoint, holes, curves or clockwise normalization is admitted.
Existing immutable bounded Frame applies world XYZ=origin+scale*local.

The separate factory-only profile has shared namespace/revision1, original Frame,
N vertices/line curves/edges/coedges, one loop/face/chart and one native Patch.
Reuse existing immutable body record meanings; vertex positions are local XYZ
with Z0. Source edge i runs vertex i→(i+1)%N; source loop/use word is input order,
all signs+1. Face kind convex_polygon, orientation+1. Fixed carrier
S(u,v)=(-1+2u,-1+2v,0), Patch.uv=[0,1]^2, W1, original frame, face/chart Lineage.
`profile.area` is exact positive local shoelace Fraction.
`profile.classify(u,v)` returns existing trim constants using exact submitted
numeric values: outside carrier/nonfinite INVALID, outside polygon REJECTED,
on one or more polygon supports BOUNDARY, otherwise INTERIOR. Unsupported numeric
types raise TypeError; exact Fraction requests remain exact.

`extrude(profile,height) -> ExtrusionResult` requires the exact factory profile
class and positive exact1/256-grid height in[1/4,2], in PROFILE LOCAL units.
The result spans Z∈[-h/2,h/2], centered on the source plane. Both caps are
translated, so neither is falsely called an unchanged retained source base.
Wrong types raise TypeError; unsupported geometry/grid/family raises named
UnsupportedExtrusion(ValueError). No empty/repaired/fallback result is returned.

## Geometry and complete boundary

The exact material is P×[-h/2,h/2], where P is the intersection of all original
closed CCW edge halfplanes. Strict interior makes all inequalities strict.
Return every supporting output face at side/vertical/cap edges and vertices.
The full boundary is (∂P×interval) union(P×two endpoints). Strict global convexity
and positive h ensure sides only meet on declared neighboring edges; all face
maps/domains establish complete embedding without sampled-volume inference.

Caps preserve original source UV and XY control bits, with Z=±h/2. They use the
same full square carrier and actual polygon trim. Side i has complete[0,1]^2:
S_i(u,v)=(1-u)*a_i+u*b_i+(v-1/2)*h*ez. Its bilinear homogeneous controls combine
original endpoint XY with the two exact Zlevels, weights1. All local controls
lie in[-1,1]^3 and on at least1/512 grid; no fitting/rounding/refinement occurs.
All patches retain the original Frame. Minimum edge length≥1/256 and h≥1/4
avoid vanishing wall crosses. Lower cap material sense−1, upper+1, every side+1;
side du×dv=(dy*h,-dx*h,0) points outward for original CCW traversal.

Output order is lower cap face1, upper cap face2, sides face(i+3), with matching
loop/chart order. Vertices: lower N then upper N. Edges: lower N, upper N,
vertical N. Each lower/upper edge preserves original source-edge direction;
vertical edge i runs lower i→upper i. Lower cap loop reverses original edge word
and every use sign; upper cap keeps it. Side i word is:

1. lower edge i+, UV(t,0);
2. vertical edge next(i)+, UV(1,t);
3. upper edge i−, UV(t,1);
4. vertical edge i−, UV(0,t).

UV maps are intrinsic; occurrence orientation is applied once in traversal.
Cap coedge UV=(original edge XY(t)+1)/2. Reuse Curve/UVMap/Vertex/Edge/Coedge/
Loop/Face/Chart records but evaluate exact Fraction readers without existing
float-coercing evaluate helpers. No coordinate matching determines shared IDs.

The graph has2N vertices,3N edges,6N coedges and N+2 each of loops/faces/charts:
14N+6 output entities over the promised six kinds. Every physical edge has two
opposed uses, one connected shell and a single cyclic link at every vertex;
no seam/hole is manufactured. Euler2/genus0 and exact world volume
profile.area*h*Frame.scale^3 supplement full domain/map/graph checks.

## Owned results and readers

PlanarProfile, ExtrudedSolid and ExtrusionResult are distinct factory-only frozen
records, not BodySnapshot/CutSolid subclasses and without canonical shape/packed
tags. ExtrusionResult retains the exact original profile, height, solid, complete
history and phase timings; solid also retains that profile for exact domains.
Fresh output namespace comes from shared body._namespace, revision1.

Solid fields: profile,body_id,revision,frame,height,vertices,curves,edges,coedges,
loops,faces,charts,patches,volume(Fraction world units). Public methods:
`classify(slot,u,v)`, `classify_point(worldpoint)` returning solid,exact point,
label(interior/boundary/exterior),all boundary_faces; `edge_point(edge_id,t)`
exact world XYZ; `coedge_uv(coedge_id,t)` exact incident UV;
`topology_input()` existing typed TopologyInput with snapshots=(). Keep result
alongside it. IDs1-based, patch slots0-based. Canonical meshing/pack_bodies/FaceRef
and Boolean-only sampler remain scoped to their existing types.

## Feature history and exact maps

Reuse owned generic EntityKey(body_id,revision,kind,entity_id), six kinds
vertex/edge/coedge/loop/face/chart, positive uint64 exclbool. Operation-owned
HistoryRecord(source,status,images,generated) and History(records,origins) have
from_input/origin lookup, unknown keys raise KeyError. All3N+3 input entities have
status modified and exactly2 same-kind cap images. All14N+6 output origins are
present:6N+6 modified cap images and8N generated side entities. No retained or
deleted inputs in this centered operation. Equal geometry in another namespace
never replaces the actual retained profile.

Origin(target,relation,parents,role,local_translation,source_t,source_uv,axial,
orientation) retains distinct typed actual source parents; relation is modified/
generated, role names lower_cap,upper_cap,side,swept_vertex,side_lower,side_upper,
side_start,side_end. Optional exact maps use common output(u,v), with intrinsic
output edge t=u: source_t=(a,b,c) means t_source=a+b*u+c*v; source_uv has two
such rows; axial=(a,b,c) means profile-local Z=a+b*u+c*v. Orientation±1 is separate
from intrinsic maps. local_translation is exact3Fraction when present.

Every input vertex/edge/coedge/loop/face/chart has lower/upper cap-copy origins:
translation(0,0,±h/2), identity source_t for edges/uses, identity source_uv for
face/chart, constant axial for all. Lower use/loop/face/chart parity−1, upper+1;
vertex/edge intrinsic direction parity+1 on both caps. Source loop words/trim
are preserved via those maps, not replaced with an untrimmed carrier square.

Generated records:

- vertical edge i: parent source vertex i; role swept_vertex,
  axial=(-h/2,h,0),orientation+1;
- side face/chart/loop i: parents source edge i and source coedge i; role side,
  source_t=(0,1,0),axial=(-h/2,0,h),orientation+1;
- side horizontal lower/upper uses i: same source edge+coedge parents,
  source_t identity, constant axial±h/2, role side_lower/side_upper,
  orientation+1/−1 respectively;
- side vertical start/end uses i: source vertex i/next(i) plus owning source
  edge i, role side_start/side_end, axial=(-h/2,h,0),orientation−1/+1.

There are8N generated entities: Nverticaledges, Nsidefaces, Nsidecharts,Nsideloops,
4Nsideuses. Side loops' maps name their supporting swept chart; their boundary
word is explicit. No source_range pretends a new height parameter was an old
profile-edge parameter. Every forward image/generated link and reverse parent
must agree. Whole-body/feature roles are retained separately by result/profile.

## Qualification, native consumption and failure

CPU constructs privately, validates exact maps/geometry, complete ownership/loop
words, opposed uses, connected shell/vertex links and entire history, then
publishes one immutable result. A private final validation boundary permits late
exception tests without production fault flags. Failure preserves source profile
and previous results; namespace gaps are acceptable. No native handle is needed
to construct the solid.

A narrow extrusion sampling adapter copies Candidate float UV BEFORE trim
classification, gathers only INTERIOR/BOUNDARY, uses existing Engine first jets,
checks full actual descriptors/querybits/Lineage/backend/order/count/statuses,
converts original per-Patch frames and orients normals. Reuse owned type-agnostic
provenance/row/child-cleanup machinery where its actual contract fits, without
widening existing type guards. Per-call children close on all outcomes, borrowed
Engine remains usable; failure emits no partial sample batch. Rejected/invalid
rows retain order/IDs, status−1 and NaN vectors. Resolution2..257 exclbool, total
candidate cap1,048,576 checked before generation and at cap+1 for iterables.

Independent truth derives from original lattice vertices/height/Frame, not
constructor/validation/trim helpers. Check complete source-domain/carrier/lineUV
polynomial identities and every actual exact reader, full incidence and all
six-kind histories; counts/volume alone cannot certify output. Native position
component error≤2^-36, oriented normal error≤2^-30 at actual copied float UV.
For integer edge deltas DX,DY, use rational bounds on sqrt(M),M=DX²+DY², obtained
from isqrt(M<<128)/2^64 (exact-square branch), then signed component intervals
for(DY,-DX,0)/sqrt(M). Require actual normal within the bound of BOTH endpoints;
no production normalization supplies expected truth. Test real sampled rows and
actual ABI5 mates/neighbors including slot permutations on CPU1/CPU24/bothP100s.

Real caller consumes material/boundary/history, samples and actual topology.
Measure profile preparation, complete extrusion construction/publication and
whole useful call including all setup/transfers/validation/cleanup. Independent
qualification, SVG/exports, reference and process/module overhead have separate
scopes; nested timings/medians are not additive. Use genuinely different triangle,
chamfered and many-sided profiles. Optional already installed reference creates
same lower-world-plane source segments and capped linear_sweep of world distance
h*scale, preserving original reference curves; compare finite full bounds,volume,
manifold and actual boundary witnesses. Record exact world-endpoint conversion
or use dyadic reference recipes. No foreign history/material API or alpha waiver
is invented. Broader features, tuning and Python/NCSX timing work remain deferred.
