# Complete signed cylinder offset: geometry and history study

2026-10-15. Recommend one actual canonical cylinder, a nonzero signed local
distance d on the1/256 grid in[-1/8,+1/8], and explicit **intersection joins**.
Build a separate complete immutable solid from the actual original source tables,
carriers and Frame. This establishes signed analytic carrier offsets, complete
sharp closure and original normal-foot maps, including honest carrier extensions.
It requires no dependency, native build, general SSI or offset approximation.

## Definitions and reference findings

Rossignac/Requicha distinguish solid dilation/erosion from normal offsets of
smooth faces. Normal-offset face pieces alone need not form the boundary of a
distance-offset solid; edge/vertex contributions and trimming matter. Their
definitions and Boolean relationships explain why inward erosion of this convex
intersection can be constructed by shrinking its factors, while outward dilation
cannot generally be obtained by intersecting enlarged factors. Original author
PDF acquired unchanged; actual introduction, definitions, boundary/normal-offset
discussion and representation sections read, not a whole-paper claim.
[Primary author PDF](https://faculty.cc.gatech.edu/~jarek/papers/Offsets.pdf).

Patrikalakis/Maekawa/Cho define signed displacement along the unit normal and
separate irregular/cuspidal points from self-intersections. Normal direction can
reverse beyond curvature critical values. We exclude such transitions by the
strict positive radius/height margins below, rather than assuming an arbitrary
inward distance is harmless. The acquired author HTML pages retain the actual
equation alternative text as well as prose.
[Differential geometry](https://web.mit.edu/hyperbook/Patrikalakis-Maekawa-Cho/node224.html),
[singularities](https://web.mit.edu/hyperbook/Patrikalakis-Maekawa-Cho/node225.html).

OCCT8.0.1 explicitly separates Arc joins (edge pipes/vertex spheres) from
Intersection joins (enlarge adjacent parallel carriers and intersect). The public
API warns that self-intersection handling/all-parallel intersection options have
implementation limits; they are not universal repair promises. Its actual face
offset code accounts for source orientation and removes rectangular surface trims
before extension. Publication and Generated/Modified/IsDeleted are algorithm-
dependent. The selected bounded family can construct all needed intersections
analytically, without inheriting reference tolerances or history labels.
[Pinned API contract](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/src/ModelingAlgorithms/TKOffset/BRepOffsetAPI/BRepOffsetAPI_MakeOffsetShape.hxx).

Truck's actual Offset/NormalField decorators add a vector field to an existing
curve/surface and evaluate derivatives; surface NormalField uses the normalized
cross-product normal. Its tests cover normal fields and parameter search. This
is geometry evaluation, not a complete trimmed solid, corner-join/history or
singularity-elimination implementation. Do not claim those tests certify a solid.
[Pinned surface decorator](https://github.com/ricosjp/truck/blob/88ed005249e5e3a6b07f62425399435905cd3ab6/truck-geometry/src/decorators/offset/surface.rs).

NVIDIA's actual offset_brep wrapper declares a new result with unchanged input;
extended_offset=True explicitly extends/intersects convex edges for sharp joins.
It is distinct from shell_brep's hollow wall and from planar curve offsetting.
The underlying API call delegates to its offset executive. Runtime availability,
source preservation and actual join behavior need direct reference qualification;
the studied source pin is not automatically the installed runtime version.
[Pinned wrapper](https://github.com/NVIDIA-Omniverse/usd-brep/blob/c9c5979f04cea8c38213004d4f04df0dcfb40d05/source/SmPyLib/src/SmPyOffset.cpp).

## Selected complete family and the Euclidean distinction

Actual source r=1/4,h=1/2 in its existing positive power-of-two Frame, with local
closed material x²+y²<=r² and|z|<=h. Let R=r+d,H=h+d. The result is

    x²+y² <= R² and -H <= z <= H.

For admitted d, R>=1/8,H>=3/8, so there is no axis collapse, zero-height face,
inverted wall or topology transition. Reject zero to give every source entity one
modified image without ambiguous identity/no-op history. Reject unsupported
source objects, foreign/aliased identities, off-grid/nonfinite/bool distances,
singular or wider families. Preserve actual source and Frame; do not substitute
make_body('cylinder') or apply a global uniform scale as the operation.

For d<0, this cylinder is exactly Euclidean erosion by a ball of radius-d: every
interior ball must clear both the radial and axial source boundaries. For d>0,
this is explicitly a sharp intersection-joined carrier offset, **not Euclidean
dilation**. A new rim point is displaced radially and axially by d from the old
rim, so its distance from the old solid is sqrt(2)*d, greater than d. Real outward
distance dilation needs rounded toroidal rim bands; a box would need edge bands
and spherical corner pieces. Those are separate future families, not missing
pieces to conceal in the current API or graphic.

A box intersection-join offset is another valid small family but exercises only
planes and ordinary edge pairs. The cylinder adds exact rational carriers, one
same-face seam and source carrier extension with only23 entities. A full rounded
cylinder could use rational tensor-product torus quarters, but it introduces
new physical rings/faces and a larger history unit; defer it during this sweep.

## Exact carriers, closed graph and readers

Cap plane squares span[-R,R]² at z±H, each with the complete disk trim. Four wall
charts use angular homogeneous triples(A,B,W)=(1,0,1),(1,1,1),(0,2,2), quarter
rotation q and controls(R*Aq,R*Bq,±H*W,W), angular-major/Z-fast. Cap degree1×1,
wall degree2×1; all coefficients are exactly binary64 dyadics. Existing native
admission is abs(X/Y/Znumerator)<=W, not abs(allcontrols)<=1: H*W may reach1.25.
Keep original rawUV quarters(q,q+1,0,1), original Frame bits and fresh Lineage.
Actual CPU/P100 jets still require grading at their actual copied binary64 UV.

Retain semantic source IDs/order in a fresh namespace: two anchors(R,0,±H), two
full circles(R,±H), one seam line(R,0,-H→+H), six coedges, three loops/faces and six
charts. Copy actual source incidence/traversal ownership after strict admission;
change the complete physical geometry and exact UV data. Top cap sense+, bottom-
and wall+. Wall word lower circle+,seam at rawU4+,upper circle-,seam at rawU0-.
The two physical rim edges are ordinary pairs; the seam has two distinct opposed
occurrences on the same face. Three-port anchor links and Euler2 close genus0.
Charts are atlas pieces, not extra physical edges.

Exact material uses original Frame inversion, the closed radial quadratic and
axial interval, returning every bounded supporting face (top1,bottom2,wall3).
Cap trim is the normalized disk; wall trim uses actual ends and physical seam,
with internal chart joins remaining interior. Exact full-circle/line and intrinsic
coedge-UV readers, ABI5 sources with snapshots empty, and complete source maps
must be useful downstream consumers. Volume is2*pi*R²*H*scale³; bounds are original
origin±scale*(R,R,H). This is a separate owned result, not canonical guard widening.

## Complete source history and map semantics

The source/output graph has V2/E3/CO6/L3/F3/CH6=23. All23 sources are modified,
each with exactly one same-kind output image; all23 origins name their own original
entity. No births/deletions are manufactured. Changed radius, height, cap extents
and effective boundary domains are explicit even where incidence or dimensionless
UV descriptors retain their shape. Source remains unchanged and immutable.

Keep two distinct parameter claims. Original/output E/CO can share intrinsic
boundary parameter t, with a named original E/CO owner and relation
`boundary_parameter`. This is not spatial coincidence or a constant-normal
translation of an original boundary curve. Ring/anchor displacement has radial
and axial components; the seam's axial displacement varies with t. Face normal
feet are different and need their own actual original F/CH owners and relation
`normal_foot`. A contextual CO sourceUV owner is its original incident face,
not a second image-parent.

Recommend operation-owned UVMap2(kind,data,domain): exact affine6 rows, or full
circle-foot ratio rho=R/r with output((1+rho*C(4u))/2,(1+rho*S(4u))/2). Store exact
input domains because wall face/loop uses rawtau∈[0,4], while chart/coedge inputs
are normalized/intrinsic[0,1]. Never feed an extended source foot into the native
evaluator whose domain remains bounded. Complete maps:

| Target/context | Exact original raw sourceUV normal-foot map |
| --- | --- |
| Cap F/L/CH | (-2d+(1+4d)u,-2d+(1+4d)v) |
| Wall F/L | (rawtau,-d+(1+2d)v) |
| Wall CH quarter q | (q+u,-d+(1+2d)v) |
| Cap ring CO | full-circle-foot rho=1+4d at intrinsic t |
| Wall lower/upper ring CO | (4t,-d)/(4t,1+d) |
| Wall seam CO rawU0/4 | (0 or4,-d+(1+2d)t) |

Origin should carry target/relation/parents/role, source_t_owner/relation/map,
source_uv_owner/relation/map and absolute orientation. V/E orientation+1; CO its
actual traversal; L/F/CH owning face sense. Do not apply this sign twice to maps.
Exact owners/domains/types and full reciprocal maps must reject internally
consistent but wrong source ancestry.

## An actual complete normal-foot reader

Propose source_foot(chart_slot,u,v) only for an admitted exact target query. Return
factory-immutable source face/chart keys, actual target chart/query, original raw
source UV, exact world target/source points, outward unit normal, signed world
distance d*scale, in_source_square and exact source_trim. Whole identities are

    target_point = source_point + d*scale*normal
    |normal|²=1; source normal = target normal.

Evaluate the original analytic carrier extension exactly from its owned records;
this never claims the foot belongs to the bounded original face. For outward
offsets, a cap foot may be inside its original square yet outside its disk, and
a wall foot may lie outside original V[0,1]. Distinguish these cases: source_trim
INVALID outside the chart rectangle, REJECTED inside a cap square but outside its
disk, otherwise exact original INTERIOR/BOUNDARY. Do not reuse the old canonical
classifier's float coercion as exact foot truth. Inward admitted target feet lie
inside the original bounded faces. At output rims the two incident normal feet
are distinct; no unique old rim foot is implied. This makes analytic extension,
distance and original trim provenance observable and independently testable.

## Qualification and scope

Validate full actual source geometry/incidence/native/packed bits first. Assemble
privately and validate complete curves, both incident UVs, source maps/normal-foot
identities, closed words/ports and all23 source/image roles before atomic immutable
publication. Independent truth comes from original source fields and signed
distance, not construction/material/map helpers. Exercise both signs/extremes/
odd grid, all Frames, rational circle phase/seam joins, source square-vs-disk
distinctions, inward containment, sharp-vs-rounded positive counterexample,
nonfinite/foreign/alias admission, malformed reciprocal history and late retry.
Native consumers grade actual float UV, full normals/provenance and ordinary/
same-face ABI5 readers. No broad mutation score or external-alpha waiver follows.

Use a few fixed-size signed distance/Frame recipes. Measure full construction,
source-foot/history/material/curve reads and actual sampling/topology setup and
cleanup; qualification/exports/foreign reference remain separate. The installed
pure offset_brep extended-offset path may provide same-world geometry comparison
if its actual source-preservation and sharp-join outcomes qualify. Preserve all
declared failures/limits. This does not establish arbitrary solid erosion,
distance dilation, self-intersection repair, GPU construction or general meshing.
