# Complete sharp-joined analytic cylinder offset

Purpose: deliver the final separately scoped feature breadth unit, with genuine
signed offset carriers, complete closure, original-source correspondence and
CPU/both-P100 consumers. This follows independent geometry/history, oracle and
runtime studies. No broader family, dependency, native build, mesher, integration
tuning or shipping-language decision is selected.

## Actual source, distance and join semantics

`offset_solid(source,distance) -> OffsetResult` requires the exact factory
BodySnapshot shape='cylinder' and its bounded original Frame. Validate complete
typed original incidence, geometry/native controls/Lineage/Frames and packed
exports without minting a replacement source or importing test-oracle helpers.
The actual canonical local source has radius r=1/4 and half-height h=1/2.
Distance d accepts exact int/Fraction/finite binary64 excluding bool, is nonzero
on the1/256 grid in[-1/8,+1/8], and is interpreted before float conversion.
Zero, other solids/results/subclasses, off-grid and collapse distances fail
explicitly. World signed distance is d*source.frame.scale. Preserve source object
and Frame identity, and publish a fresh output namespace/revision1.

The join is explicitly `intersection`: offset all three underlying analytic
surfaces and intersect their continuations, retaining sharp cap/wall rims.
New radius R=r+d and half-height H=h+d are positive throughout the admitted
family. Material is x²+y²<=R² AND -H<=z<=H. This is a complete embedded closed
cylinder with exact tight bounds and genus0. For d<0 it equals Euclidean erosion
by a ball of radius -d. For d>0 it is NOT the rounded Euclidean dilation: a rim
point moves by(d radial,d axial), with norm sqrt(2)*d. Never infer uniform
Euclidean distance to the original bounded body from normal-offset carriers.
General curved surfaces can develop local/global singularities and non-rational
offsets; positive R/H prove their absence only for this selected family.

## Complete carriers, topology and exact readers

The output retains the source graph's deterministic IDs/slots and ownership:
2vertices/3edges/6coedges/3loops/3faces/6charts=23 typed entities. Vertices are
(R,0,-H),(R,0,H). Edges are lower closed ring,upper closed ring,upward seam.
Cap faces/loops are top1,bottom2 with original senses+1,-1. Wall face3 has one
loop containing lower ring+,seam at rawtau4+,upper ring-,seam at rawtau0-.
Keep the seam's two DISTINCT occurrences even though both have one owner face.
Every closed ring has distinct start/end ports at its shared anchor vertex.
The two ordinary rim pairs and one same-face seam pair form a connected shell;
each anchor has one three-port cyclic vertex link. Euler2 alone is insufficient.

Construct actual enlarged/shrunk cap charts, not metadata on unchanged source
patches. Their local carrier is(R*(2u-1),R*(2v-1),±H), degree(1,1), raw UV unit
square. Disk trim is(2u-1)²+(2v-1)²<=1. The four wall degree(2,1) rational quarters
retain q=0..3, native u/v∈[0,1], raw domain(q,q+1,0,1). For homogeneous angular
triples(A,B,W)=(1,0,1),(1,1,1),(0,2,2), rotate(A,B) by q right angles and emit
(R*A,R*B,z*W,W) at z=-H,+H, angular-major/Z-fast. Every coefficient is an exact
dyadic and fits the original native carrier envelope. No fitting, sqrt weights,
source replacement or rounding is needed. Preserve original rawUV and Frame
binary64 bits, with fresh complete Lineage naming actual output face/chart.

New vertex/Curve/UVMap numeric geometry is exact Fraction data; quadrant/index
metadata is strict integer. circle curves store(R,z), seam line stores endpoints.
Cap UVMap circle_plane stores(R,R); wall circle_wall and seam_wall store exact
intrinsic maps with the existing meaning. Reuse primitive records only where
semantics fit; do not widen canonical packed tags, factory guards or mesher APIs.

`classify(chart_slot,u,v)` uses zero-based native chart slots and exact submitted
numeric values, returning REJECTED/INTERIOR/BOUNDARY/INVALID. Nonfinite and
out-of-square inputs are INVALID. Cap disk truth is exact. Wall labels physical
rim v0/1 and the actual seam(q0u0,q3u1) BOUNDARY; internal chart joins are not
physical edges. Native adapters copy actual binary64 UV BEFORE classification;
no query snapping. `classify_point(worldpoint)` inverts Frame exactly and returns
interior/boundary/exterior and every actual supporting face: cap+wall at rims,
wall only on its seam. Seam occurrence is topology, not a second material face.

`edge_point(one_based_edge_id,t)` returns exact world XYZ for intrinsic t∈[0,1].
Full circles use four rational quarters in t; t0/1 coincide spatially but retain
separate endpoint ports. `coedge_uv(one_based_coedge_id,t)` returns exact original
raw output UV at intrinsic edge t; traversal orientation is applied only when
traversing the loop. `topology_input()` emits actual fresh output typed sources
and incidence through ABI5 with snapshots=(). World volume is shared Measure
with rational0 and pi coefficient2*H*R²*scale³. Tight world bounds are Frame
origin+(±R,±R,±H)*scale. No general result mesher or GPU constructor is claimed.

## Source history and carrier extensions

Every original entity has one modified same-kind boundary image:23source records,
23output origins, no generated/retained/deleted entities. Identical incidence or
UV descriptor does not imply unchanged geometric ownership. Forward records and
reverse origins remain complete, ordered by vertex/edge/coedge/loop/face/chart.
`from_input`/`origin` require actual strict typed EntityKeys and reject foreign
namespace/revision/kind/ID and numeric aliases. Every origin's sole causal parent
is its actual same-kind source, not every incident support.

Operation-owned immutable Origin fields:
`target,relation,parents,role,source_t_owner,source_t_relation,source_t,
source_uv_owner,source_uv,source_uv_relation,orientation`.
Relation is modified, parents=(actual own source,), role='source_'+kind.
Orientation is absolute: V/E+1,CO owning traversal sign,F/L/CH owning face sense.
It is never a second sign on intrinsic parameter maps. HistoryRecord fields are
source,status,images,generated; status modified, one same-kind image, no births.

E/CO source_t triple is(0,1,0), owned by corresponding original E/CO,
relation='boundary_parameter'. This is corresponding intrinsic parameter only,
NOT spatial coincidence or one face-normal displacement of the sharp rim.
V has no parameter/UV map. Each CO has an original incident FACE source_uv_owner;
F owns its original F,CH owns its original CH,L names its original owning F.
All UV maps have source_uv_relation='normal_foot': they land on the original
underlying analytic carrier, explicitly allowing extension beyond its bounded
face/chart. A contextual UVowner is not a causal parent. Owner/map/relation
pairs are all present or all absent, with actual original incidence validated.

UVMap2(kind,data,domain) stores an exact target argument rectangle
(u0,u1,v0,v1). `.evaluate(u,v)` takes exact finite numeric values excluding bool
in that rectangle and returns exact original RAW UV; outputs are not clamped.
'affine' data are six Fraction coefficients for two affine rows.
'circle' data=(ratio,) maps a full intrinsic ring parameter u∈[0,1] to
((1+ratio*C(4u))/2,(1+ratio*S(4u))/2), v ignored. C/S use the same stated four
rational-circle pieces, independently derived in qualification. Read fixed-arity
iterables with a width+1 bound; unknown descriptors reject before consumption.

| Target | Exact source UV normal-foot map | Target argument domain |
| --- | --- | --- |
| Cap F/L/CH | (-2d+(1+4d)u,-2d+(1+4d)v) | unit square |
| Wall F/L | (u,-d+(1+2d)v) | [0,4]×[0,1], u is raw tau |
| Wall CHq | (q+u,-d+(1+2d)v) | unit square, output raw source UV |
| Cap CO | circle ratio1+4d | unit square, (u=t,v=0) |
| Wall lower/upper ring CO | (4u,-d or1+d) | unit square |
| Wall seam CO | (4 or0,-d+(1+2d)u) | unit square |

These maps apply to carrier extensions. Positive cap foot can be inside the
original carrier square but outside its disk trim; positive wall foot can be
outside original V[0,1]. Original bounded-domain admission must not be invented,
clamped, snapped or inferred from cap square membership alone. No original
native patch is evaluated beyond its supported unit query domain.

`source_foot(chart_slot,u,v)` requires an admitted exact output-face query and
returns immutable factory Footpoint fields:
`solid,chart_id,u,v,source_face,source_chart,source_uv,source_point,point,normal,
distance,in_source_square,source_trim`.
Source keys refer to actual original F/CH, chart_id is output one-based chart ID.
source_uv is original raw UV; source_point is its exact WORLD analytic carrier
point, possibly on an extension; point is exact WORLD output point. normal is
exact outward unit vector, distance=d*scale. Return all query/ownership fields
unchanged. Every carrier obeys point=source_point+distance*normal with equal unit
normals. Cap normals are±Z; wall normal is the exact rational quarter direction.

in_source_square grades the original owning chart RAW rectangle exactly.
source_trim is original effective face-domain truth, independently of square
membership: INVALID outside that square; cap REJECTED outside disk,BOUNDARY on
rim,INTERIOR inside; wall BOUNDARY only on original rims/actual seam and INTERIOR
else. Original body.classify coerces floats, so it cannot grade these exact feet.
Evaluate analytic original carriers with exact arithmetic rather than submitting
unsupported native UV. Reject nonadmitted target cap points instead of pretending
they belong to the result face. History descriptors may evaluate the entire
carrier square; the Footpoint reader is explicitly bounded to actual output face.

## Publication, native consumer and independent truth

Build candidate solid/history privately. Before immutable publication validate
strict actual original canonical bits/packed tables, every actual changed carrier,
complete curve/both-UV identities, closure, distinct seam uses, ring endpoint ports,
connected three-port links, full history/map owners/relations and exact normal-foot
carrier identities. Prove whole rational identities by coefficient arithmetic or
closed analytic substitution, not selected sampled fits. Failure leaves source
and prior outputs unchanged and publishes no partial result/history.

Separate operation-specific OffsetRequest/OffsetSamples factories retain actual
OffsetResult. prepare_grid(result,resolution=17),request(sample candidates),
sample(borrowed Engine,request) use unchanged generic owned native provenance/row
helpers. Resolution2..257; cap1,048,576 checked before grid allocation and at
stream limit+1. Explicit rejected/invalid rows preserve trim,status-1 and NaNs.
Validate exact querybits/count/order/Lineage/backend/degrees/Frame/native statuses;
close all three actual native children on every failure and permit sameEngine retry.
CPU construction stays CPU. Serialize GPU qualification lanes and final benchmarks.

Independent qualify_result/samples/topology reconstruct original source geometry,
expected graph/domains, source maps/normal feet and volume from original source/d/
Frame, never production offset/trim/material/UVMap2/validator helpers. Cover inward,
outward, oddgrid, distance bounds, allFrame scales, original preservation and strict
identity, exact/subnormal/nextafter material and cap trim cases, all original foot
domain distinctions, seam occurrences and closed-ring ports. Native exact truth
uses actual copied binary64 UV with direct world component position2^-36 and
oriented unit-normal2^-30 limits. Direct adversarial faults target actual controls,
source owner/mapping relations, jointly wrong reciprocal ancestry, metadata/status,
output and recoverable ownership; no broad mutation score is inferred.

## Actual callers, foreign reference and complete costs

Root actual caller constructs source from an already-owned Frame, invokes actual
offset_solid, binds requested signed distance/join/source, reads material/edges/
UV/history/maps and actual source_foot outputs, then actual CPU/P100 sampling,
cleanup and ABI5 typed mate/neighbor readers. Independently qualify afterward.
Retain complete source/result graph/carriers, exact source-foot/map/material/edge/
UV reads, history, actual native rows, ABI5 arrays/readers, owned source/native
hashes and source-derived SVG. Source overlay and result curves must be actual
geometry, with explicit join and CPU construction labels; no result mesher claim.

Recipes inset d=-1/8,origin0/scale1; outset d=+1/8,origin(2,-3,1)/scale1/2;
odd d=17/256,origin(-2,1,1/2)/scale2. All are the same small23-entity/6-chart graph,
grid17=1734candidates. One excluded warmup and three measured fresh processes on
CPU1/CPU24/P1000/P1001 gives48 calls. Prepared-source wall includes constructor,
all semantic consumers/native setup/transfers/normal/scatter/cleanup/topology;
factory wall additionally includes source creation. Independent qualification,
exports/reference/imports/process launch remain outside and separately recorded.
Report medians/ranges/bestCPU and nested/nonadditive phases with shared-fleet limits.

Existing isolated USD BRep offset_brep(source,worlddistance,extended_offset=True,
self_intersection=True) is pure and matches sharp joins. Preserve original full
body/edge scalar probes before/after, verify exact same-world source/distance and
finite returned volume/bounds/manifold/closest points. Use63 witnesses: five per
three actual edges plus eight admitted interior native samples per six charts.
Unchanged declared criteria volume relative1e-8,bounds1e-8*scale,boundary returned-
distance/residual1e-7*scale; write all outcomes before nonzero failed comparison.
Reference finite witnesses do not qualify foreign material/history/full coverage.
Prior revolution issue43 stays open; required community alpha suites remain
incomplete/non-green. Finish and merge this breadth unit before any tuning.
