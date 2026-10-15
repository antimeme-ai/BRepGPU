# One complete source-edge fillet: semantics study

2026-10-15. Selected whole breadth unit: fillet one actual complete vertical edge
of the owned canonical box, building a separate immutable closed solid. Keep
the six original plane carriers and their original Frames/parameters; change
their effective trim domains and add one exact circular band. The fixed small
graph exercises genuine trimming, contact, endpoint closure and ancestry.
It does not establish general or interacting fillets, GPU construction, a result
mesher, or an alpha qualification waiver. Offset follows this unit.

## What the references actually teach

The pinned OCCT plane/plane construction computes a cylindrical blend from
oriented supports, then records both contact lines and their plane/cylinder
PCurves and transition senses. That supports treating the two complete contact
rails as first-class operation outputs, rather than inferring tangency from a
visually circular carrier. Its public builder separates Generated, Modified and
IsDeleted using actual splitting/build data. Its box single-edge GTest checks
completion and BRep validity; it supplies neither our exact history nor an oracle
for our source parameter maps. See the [pinned analytic implementation](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/src/ModelingAlgorithms/TKFillet/ChFiKPart/ChFiKPart_ComputeData_FilPlnPln.cxx).

Truck's `simple_fillet` trims the actual adjacent faces and builds an approximated
blend surface. Its separate side-face work is necessary to close the endpoints;
returning the blend and two supports alone is not a closed solid. Edge identity,
orientation and changed boundary words matter independently of the approximation.
The present unit admits the exact analytic case and closes its two caps directly.
See the [pinned fillet module](https://github.com/ricosjp/truck/blob/88ed005249e5e3a6b07f62425399435905cd3ab6/truck-shapeops/src/fillet/mod.rs).

NVIDIA's selective `fillet_edges` operates on actual owned edge handles and mutates
the body. It differs from all-edge `circular_fillet`; numeric cross-section flags
also differ from the exposed enum values. Circular does not imply G2. An optional
comparison must select the unique edge on a disposable copy and preserve the
true original body. Stitch/orient and global merge in the source do not promise
failure atomicity or preserve our entity counts/history. See the [pinned wrapper](https://github.com/NVIDIA-Omniverse/usd-brep/blob/c9c5979f04cea8c38213004d4f04df0dcfb40d05/source/SmPyLib/src/SmPyFillets.cpp).

Kós, Martin and Várady describe the rolling-ball center spine as an intersection
of offset primary surfaces and identify explicit analytic cases including two
planes. Their recovery paper explicitly leaves consistent topology and vertex
blends outside its scope. We use this geometric distinction, not an assumed
endpoint-closure algorithm. The institutional full text was read through the web;
the original PDF download returned403 and remains on the acquisition list.
[Author manuscript and publication record](https://orca.cardiff.ac.uk/id/eprint/13565/).

## Admitted geometry and exact band

Require the exact actual canonical box snapshot, complete typed source geometry,
incidence and original carrier/Frame bits. Its half-width is a=3/4 and Z interval
[-1/2,1/2]. Admit source vertical edgeIDs9..12 and radius on the1/256 local grid
in[1/16,1/2], with the existing bounded positive power-of-two Frame. Reject bool,
foreign keys, wrong revisions, nonvertical selections and noncanonical bodies.
Never manufacture a replacement box as a substitute for validating the input.

For corner k=edgeID-9, let q=(k+2)%4, c=a-r and Rq a positive quarter rotation.
Use center Rq(c,c) and

    C(u)=(1-u²)/(1+u²), S(u)=2u/(1+u²)
    B(u,v)=(Rq(c,c)+r Rq(C(u),S(u)), -1/2+v).

The degree(2,1) homogeneous angular controls are(1,0,1),(1,1,1),(0,2,2).
Rotate their XY numerators, add center times W, and tensor with the two Z levels.
Weights1,1,2 and all dyadic numerators are exactly binary64; no fitting or sqrt2
weight rounding is needed. The actual native weight-relative envelope admits
these controls. Actual native/world first jets still require independent grading.
The exact outward unit normal is Rq(C,S,0), so the two side-rail normals agree
identically with their original planes. Both cap joins remain intentionally sharp.
Homogeneous rational construction follows the acquired Sauer/Shene sources in
the source catalog; exact coefficients do not imply exact floating evaluation.

In signed corner coordinates X=sx*x,Y=sy*y, the section is the original square
restricted by `X<=c OR Y<=c OR (X-c)²+(Y-c)²<=r²`. Do not intersect the entire box
with the disk. Band support additionally requires X>=c,Y>=c and circle equality:
unused portions of the full cylinder cannot be reported as boundary supports.
Return every incident bounded face, including plane+band at a tangent rail and
cap+plane+band at its endpoint. Artificial X=c/Y=c lines are not boundaries.
Volume is `(9/4-r²)+pi*r²/4`, multiplied by original scale³; bounds remain the box.

## Trims, traversal and complete topology

Original horizontal edge length is3/2. A shortened edge therefore maps intrinsic
t to the original subrange with delta=2r/3, often a nonbinary Fraction. Preserve
the original intrinsic direction and compose original coedge UV with that map;
apply the coedge traversal sign separately. Preserve retained original vertex,
Curve and UVMap binary64 payloads; new/modified coefficients are exact Fractions.
All original plane nets/Frames remain unchanged despite changed effective trims.

Use an explicit quarter-arc curve descriptor, never reinterpret the existing
canonical full circle. Its intrinsic parameter is the rational quarter coordinate,
not angle or arc length. Cap UV is a rational quarter arc of radius2r/3 centered
at((cx+a)/(2a),(cy+a)/(2a)) in the actual original cap chart.

The band word is lower arc+, outgoing rail+, upper arc-, incoming rail-.
The cap arc uses are top+,bottom-. Replace the deleted spine use on each side
with its own contact rail occurrence and trim the adjacent cap-edge occurrences.
The complete graph has V10/E15/CO30/L7/F7/CH7=76 entities,15 ordinary opposed
edge-use pairs, connected face adjacency, three-port cyclic vertex links and
Euler2. No same-face seam occurs. It is a closed genus-zero rounded prism.

## Complete history and maps with owners

The source62 dispositions are33 retained,24 modified,5 deleted; output origins
are33 retained,24 modified,19 generated. Per-kind source R/M/D:
V6/0/2, E7/4/1, CO14/8/2, and L/F/CH each2/4/0. Four source charts are MODIFIED
because their effective domains change, despite unchanged plane coefficients.
Births are4 tangent vertices,2 arcs,2 rails,8 coedges and one band L/F/CH.

Every birth has selected spine edge S as a parent. Tangent vertices also name
the deleted endpoint V and their own side F; arcs and arc occurrences also name
the endpoint V and actual cap F; rail edges name their own side F; rail uses add
the precise deleted source coedge on that side. New band L/F/CH name S and both
ordered support faces. Original images name their own original entity. Context
owners do not become image parents merely because they supply UV coordinates.

Operation-owned Origin needs explicit `source_t_owner`, `source_t_relation`,
`source_t`, `source_uv_owner`, `source_uv`, alongside typed target/relation/parents/
role/orientation. A source_t owner is an original edge/coedge. Relation coincident
means actual spatial source-curve correspondence; spine means axial correspondence
only. New band interior maps t=v to S but has no source plane UV. Rails map t=u
to S and UV to their own support F. Arcs/tangent vertices map constant endpoint
t to S and UV to their actual cap/support F. No map is silently applied to both
support planes and no affine curved displacement is invented.

Map2 has exact affine two-row form or rational arc(Uc,Vc,R,q). Use target vertex
arguments(0,0), edge/coedge(u=t,v=0), and unchanged carrier(u,v) for F/L/CH.
Absolute orientation is+1 for V/E, actual traversal sign for CO, owning face sense
for L/F/CH; it is not a second sign applied to parameter maps. New band L/F/CH+1.
Two ContactRail outputs bind target rail, deleted source occurrence, support F,
blend F, both UV maps, selected-spine t and exact normal, making tangency a real
complete reader rather than a few fitted samples.

## Qualification and scope

Construct privately and validate source binding, complete trims/words/links,
whole rational spatial/bothUV identities, both contact normals and full forward/
reverse ancestry before immutable publication. Late failure leaves original and
prior results unchanged; gaps in allocated namespace IDs are harmless. Independent
truth derives the admitted family from original source fields and selection,
not construction helpers. Exercise all four edges, radius endpoints/odd steps,
nonbinary joins/nextafter, support multiplicity, foreign/aliased source IDs and
late rollback. Native sampling uses actual copied binary64 UV and full ABI5 CSR
readers. Optional foreign outcomes remain separate and preserve declared limits
and nonzero failure exit; they do not certify owned history or alpha readiness.

The three benchmark recipes have the same fixed graph. Measure full construction,
readers/history/contact, sampling and topology setup/cleanup; keep qualification,
exports and reference costs separate. No large-model scaling or universal GPU
gain follows. Broader radii/edges, corner patches, arbitrary source geometry,
meshing and integration/Python choices remain later units.
