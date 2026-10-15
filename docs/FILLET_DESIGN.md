# One complete source-box edge fillet

Purpose: establish a genuine fillet solid with changed source trims, endpoint
closure, tangent side contacts and complete source history before the final
offset breadth unit. This design follows the independent fillet semantics,
oracle and runtime studies. No general/interacting fillet, mesher, new library,
native build, integration tuning or shipping-language choice is selected.

## Source and selection

`fillet_edge(source, edge_key, radius) -> FilletResult` requires the exact factory
canonical BodySnapshot with shape='box'. Preserve the actual source and Frame;
validate its full canonical geometry, typed incidence, carrier/Frame/Lineage
bits and source table consistency before assembly. The canonical local box has
XY half-width a=3/4 and Z endpoints±1/2. Accept its existing bounded positive
power-of-two Frame. No Boolean/extrusion/revolution result or subclass is admitted.
Do not construct a replacement source with make_body or select by coincident XYZ.

edge_key is the actual shared EntityKey(source.body_id,source.revision,'edge',id),
id9..12 only. Wrong type, namespace, revision, kind or nonvertical ID is rejected.
Radius accepts exact int/Fraction/finite binary64 excluding bool, exactly on the
1/256 local grid in[1/16,1/2], before any float conversion. No angle, partial edge,
multiple selection, variable radius, concave blend or repair. Every corner and
Frame is supported; local radius is multiplied by Frame.scale in the world.

Let k=edgeID-9. The source CCW XY corners are k0(-a,-a),k1(a,-a),k2(a,a),k3(-a,a).
Put q=(k+2)%4, c=a-radius and let Rq rotate XY by q positive right angles.
The new quarter center is Rq(c,c). Preserve original Z levels. In corner-reflected
coordinates X=sx*x,Y=sy*y the whole section D is the original square satisfying

    X<=c OR Y<=c OR (X-c)^2+(Y-c)^2<=radius^2.

The complete material is D×[-1/2,1/2]. All straight sections stay positive-length
at the radius bounds; the quarter arc meets only the two declared side contacts.
This gives one embedded genus-zero closed solid. Normal continuity applies to
the two side contact rails; cap joins remain sharp. No spherical endpoint blend
or general rolling-ball corner solution is implied.

## Carriers, domains, exact geometry and contact rails

All six original plane control/rawUV/Frame bits remain, with fresh output
Lineage. Two cap domains become whole rounded squares. Two adjacent original
side rectangles shorten in their original UV; their exact endpoints include
2r/3 or1-2r/3 and generally are not binary64. The two nonadjacent sides retain
their complete domains. Do not rebuild shortened plane carriers to hide trims.

The new band has normalized native(u,v) in[0,1]^2:

    C=(1-u^2)/(1+u^2), S=2u/(1+u^2)
    B(u,v)=(Rq(c,c)+r*Rq(C,S), -1/2+v).

One degree(2,1) rational Patch covers the entire quarter. For angular homogeneous
triples(A,B,W)=(1,0,1),(1,1,1),(0,2,2), rotate(A,B) by q, then emit
(cx*W+r*A,cy*W+r*B,z*W,W) at original z=-1/2,+1/2, angular-major/Z-fast order.
All coefficients are exact dyadics, weights1/1/2, abs(numerator)<=3/4*W; no fitting,
sqrt weights or control rounding is used. Raw UV is(0,1,0,1), retained Frame,
new output face/chart7 Lineage. All band face orientations are+1. Rq(C,S,0) is
the exact outward unit normal. The CCW rotation avoids reflection/parity flips.
The chart quadrant records q; it is one finite quarter, not a periodic wall.

The two contacts are u0/incoming and u1/outgoing relative to the original CCW
XY perimeter. Every complete contact rail has exact world point equality between
new band and actual original side carrier at its ORIGINAL UV. Their outward
unit normals agree there, independently of parameter speed. A ContactRail record
binds target edge, source face, source selected-edge coedge, blend face, blend_u,
source_uv Map2, blend_uv Map2, source_t affine triple and exact normal3.
Contact map arguments are(u=t,v=0) for t∈[0,1], intrinsic upward Z. The source
coedge identifies the precise deleted old occurrence on that original side face.
Map/read both occurrences; geometric tangency does not merge physical edges.

Exact result readers: classify(chart_slot,u,v),classify_point(worldpoint),
edge_point(one_based_edge_id,t),coedge_uv(one_based_coedge_id,t),topology_input().
Use exact int/Fraction/finite binary64 excluding bool for geometry, t∈[0,1].
Material inverts original Frame exactly, uses the direct D×Z predicate and returns
interior/boundary/exterior with EVERY bounded supporting output face. Circle
center/artificial X=c or Y=c lines are not boundaries; a point on the unused
part of the full circle cannot name the quarter-band face. At a side rail return
plane+band; at its endpoint cap+plane+band; cap arc interior has cap+band.

Trim slots are zero-based and actual normalized query values are interpreted
before decisions. Nonfinite/out-of-square is INVALID. Caps reject the removed
corner and label actual rounded/straight outline BOUNDARY. Shortened sides reject
outside exact retained U interval and label rectangle boundary. Band fullsquare
labels its entire four-edge boundary. Original untrimmed sides retain fullsquare.
Native Candidates copy floats BEFORE exact labels; no snapping to rational joins.

Retained source vertex positions, line curves and coedge UV data preserve their
original binary64 bits. New/modified positions, curves and UV coefficients use
Fractions; exact readers normalize retained dyadics to Fractions without a
float round trip. Output IDs/slots remain newly owned even when immutable
geometry payloads are shared. Store explicit own
quarter_arc curves with(cx,cy,r,q,z), exact coefficients and integer q. Intrinsic
arc t is the rational quarter coordinate; it is not an old line subrange, radians
or arc length. Existing canonical circle semantics/packed tags stay unchanged.
Cap arc coedge UV uses kind='arc',data=(Uc,Vc,R,q), Uc=(cx+a)/(2a),Vc similarly,
R=r/(2a). Other coedge maps use kind='line' with4 exact endpoint coefficients.
Exact readers evaluate these descriptors, applying traversal sign separately.

## Deterministic output graph and original images

Source62 six-kind entities become76:10 vertices,15 edges,30 coedges,7 loops,
7 faces,7 charts. All original face/loop/chart slots0..5 remain, with new band
slot6. Original cap face senses top+ and bottom- and original side senses remain.
Each modified cap loop replaces its source corner by the new quarter arc, CCW
on top and reversed on bottom. Two side words replace the deleted original edge
use by their contact rail use. All other original coedges retain their word order.
New band loop: lower arc+, outgoing rail+, upper arc-, incoming rail-. All
physical edges have two opposed uses, connected face adjacency, cyclic3-port
vertex links and Euler2. No periodic or same-face pair exists in this family.

Retain source vertices except the two selected endpoints in original order,
then append lower incoming/outgoing and upper incoming/outgoing tangent vertices.
Retain source edges except selected edge in original order (four shortened cap
edges get changed exact line records), then append lower arc,upper arc,incoming
rail,outgoing rail. Retain source coedges except its two selected-edge uses in
original order (eight changed cap-edge uses get restricted source parameters),
then append lower cap arc use,upper cap arc use,incoming side rail use,outgoing
side rail use,and the four band uses in the word above. IDs=slot+1 per kind;
source-to-output slot maps come from original IDs, not geometry welding.
Retain all six source face names; output cap kind='fillet_cap', sides='rectangle',
new face name='fillet',kind='fillet_band'. Old/new chart quadrant values are-1/q.

Exact local volume is (9/4-r^2)+pi*r^2/4, both coefficients multiplied by scale^3
in shared Measure. Tight world bounds remain original box extrema. Counts,
volume and bounds supplement complete original-domain coverage and tangent/
curve/UV identities; they do not replace those checks. TopologyInput uses ABI5
snapshots=() and actual typed result sources, with15 ordinary paired edges.

## Source history and explicit map ownership

Source dispositions:33retained,24modified,5deleted. By kind retained/modified/
deleted are vertices6/0/2,edges7/4/1,coedges14/8/2,and loops/faces/charts2/4/0
each. Changed effective-domain charts are MODIFIED despite unchanged plane nets.
Output origins:33retained+24modified+19generated. Births are4 tangent vertices,
4 edges(2 arcs+2 rails),8 coedges and one band loop/face/chart. Deleted means no
retained/modified boundary image; source edge/endvertices may still generate.

Use operation-owned Origin(target,relation,parents,role,source_t_owner,
source_t_relation,source_t,source_uv_owner,source_uv,orientation). Relation is
retained/modified/generated. Keys are actual shared typed EntityKeys. History
records cover all62 sources exactly once; origins cover all76 outputs exactly
once in six-kind table order. Forward images filter retained/modified origins;
generated links filter generated origins by actual causal parents. No source
of another kind becomes an image merely because it supplies mapping context.

Map2(kind,data) is immutable, with evaluate(u,v) on exact arguments in[0,1]^2.
'affine' data is6 exact Fractions: two affine triples. 'arc' data is(Uc,Vc,R,q),
three Fractions plus integer q0..3, and emits(Uc+R*Cq(u),Vc+R*Sq(u)), ignoring v.
All map coefficients/domains are exact. Vertices use(0,0), edges/coedges use
(u=t,v=0), face/loop/chart use normalized/raw unchanged carrier(u,v).
source_t is an optional exact affine triple with explicit original edge/coedge
owner. Relation='coincident' means retained/restricted source curve parameter;
'spine' means corresponding original selected-edge axial position, never spatial
coincidence of new fillet interior. source_uv_owner names ONE original face/chart,
not every causal parent. Context owners can differ from causal parents; validate
their actual source incidence/domain. Null owner/map pairs mean inapplicable.

| Target role | Relation / causal parents | Parameter and UV map |
| --- | --- | --- |
| Original vertex image | retained; own source vertex | no t/UV map |
| Original edge image | retained/modified; own source edge | coincident source_t=t0+(t1-t0)*u, no UV |
| Original coedge image | retained/modified; own source coedge | coincident source_t as its physical edge; sourceUV owner original incident face, exact original UV composed with restricted t |
| Original loop image | retained/modified; own source loop | sourceUV owner its original face, identity Map2; whole changed word separately retained |
| Original face/chart image | retained/modified; own corresponding source | sourceUV owner that original face/chart, identity Map2; exact effective trim determines disposition |
| Tangent vertex | generated; selected endpoint vertex,selected edge,own support face | spine t0/1; own support face constant UV |
| Cap arc / cap arc use / band arc use | generated; selected edge,selected endpoint vertex,actual cap face | spine t0/1; coincident sourceUV arc Map2 owner cap face |
| Contact rail edge | generated; selected edge,own support face | spine t=u; coincident own support face affine UV |
| Side rail use / band rail use | generated; selected edge,own deleted source coedge,own support face | spine t=u; coincident own support face affine UV |
| Band loop/face/chart | generated; selected edge,incoming original side face,outgoing original side face | spine t=v; no sourceUV for interior |

Original image roles use role='source_vertex'/'source_edge'/'source_coedge'/
'source_loop'/'source_face'/'source_chart'. Generated roles are
'lower_incoming_vertex','lower_outgoing_vertex','upper_incoming_vertex',
'upper_outgoing_vertex','lower_arc','upper_arc','incoming_rail','outgoing_rail',
'lower_cap_arc','upper_cap_arc','incoming_side_rail','outgoing_side_rail',
'band_lower_arc','band_outgoing_rail','band_upper_arc','band_incoming_rail',
'band_loop','band_face','band_chart'. Orientations retain original occurrence/
face senses; cap arc uses bottom-/top+, rail side uses equal their deleted source
use sign, band word signs+,+,-,-. Vertex/edge orientation is+1. Loop/face/chart orientation equals its target
face sense; coedge orientation equals its target traversal sign. These are
absolute senses relative to stored intrinsic carriers/curves, not a second
application to source parameter maps. New band loop/face/chart are+1.

Lookup rejects foreign/nonkeys. Actual role/source IDs and full maps must match
precise geometry/occurrences; mutually reciprocal wrong ancestry is invalid.
Retain source, selected typed edge and exact radius on result/solid. ContactRails
are operation outputs in result.contacts, separate from six-kind entity history.

## Publication, consumers and qualification

Construct privately, validate whole source binding/domain/carrier/trim/curve/UV/
contact/history maps, closed oriented words and complete incidence, then publish
one immutable result. A private final validator supports late exception tests;
no public failure flag. Inputs/prior results survive all failures, retry succeeds,
namespace gaps are acceptable. No native handle needed to construct the solid.

Separate FilletRequest/FilletSamples adapter borrows Engine, reuses actually
compatible owned descriptor/row/child-lifetime helpers without widening other
operation guards, and preserves all output IDs/source parameters. Bound complete
candidates to1,048,576, grid2..257 excluding bool, precheck7-chart grid. Copy actual
float UV before exact trim, gather admitted rows, check descriptors/querybits/
Lineage/status/backend/count/order, convert original per-Patch Frame and orient
normals. Rejected/invalid rows keep status-1/NaNs, no fallback. Close all three
owned native children on late native/host failure and permit healthy Engine retry.

Independent expected truth uses original actual source box geometry/Frame and
selected key/radius, without construction/trim/material/history helpers. Derive
complete source domains and all retained/modified/generated/deleted roles,
whole rational edge/incident-UV identities, both contact curves/normal identities,
all supporting material faces and closed topology. Original rational circle
components give exact native normals; grade at actual copied binary64 UV with
position component<=2^-36 and normal component<=2^-30. CPU parity/render/counts
cannot supply expected geometry. Use native ABI5 actual15pairs/mates/neighbors
under source slot permutations. All4edges, radiusbounds/oddgrid, every Frame,
nonbinary joins/nextafter, cap closures, false full-circle supports and late
source/consumer rollback need direct cases. Required external alpha suites remain
incomplete; local truth and optional reference outcomes cannot waive them.

Actual caller reads material/curves/UV, forward/reverse history, evaluates actual
source/spine/UV maps and both contact rails, samples and reads native adjacency.
Measure prepared-source and source-points/factory-to-consumed walls explicitly,
including construction/publication, all reads, native setup/transfers/checks/
cleanup and topology. Separate source creation, independent qualification,
exports/SVG/reference/process overhead. Three edge/radius/Frame recipes have the
same7-chart graph; report fixed-size geometry cases, not larger models or repeated
patch throughput. CPU1/CPU24/bothP100 fresh calls, no residency tuning.

Optional existing reference: exact same world box, copy original, identify unique
selected vertical edge on COPY from exact source endpoints, mutate only copy via
fillet_edges(radius=r*scale). Retain/probe true original curves/body before/after,
discard stale copy child handles after mutation. Compare finite bounds/manifold,
volume and actual boundary witnesses using unchanged limits1e-8 relative volume,
1e-8*scale bounds,1e-7*scale boundary distance/residual. Preserve complete outcomes
and nonzero exit if any criterion fails; do not repeat the reference clamp as a
promise of certified final accuracy. Existing revolution issue43 stays open.
Offset is next; broader fillets/integration/Python/NCSX timing stay deferred.
