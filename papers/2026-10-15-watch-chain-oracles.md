# Watch chain: independent geometry, history and mesh obligations

2026-10-15. Study for the first actual profile → extrusion → axial bore → mesh
composition. No product implementation or dependency is selected here. The
watch-inspired assembly is a consumer of bounded solids, not a functioning
horological movement or general gear/sweep model.

## Source study and applicable literature

Read `extrusion.py`, `extrusion_oracles.py`, `extrusion_meshing.py`, its oracle
and direct tests, plus the existing Boolean and cut-mesh interfaces/oracle. An
`ExtrusionResult` owns its actual `PlanarProfile`, `ExtrudedSolid`, and six-kind
history. Its cap patch spans local XY[-1,1]², with intrinsic UV[0,1]²; the polygon
is a trim inside that carrier. Its side patches follow actual directed polygon
edges and extrusion height. A bore must retain those actual patches and Frame,
not reconstruct a canonical box or reframe UVs around the polygon bounding box.

[Shewchuk's primary predicate work](https://www.cs.cmu.edu/~quake/robust.html)
explains why geometric decisions require the sign of the original determinant.
Here the admitted lattice source and exact rational cutter parameters let owned
Fraction arithmetic evaluate those signs directly. No adaptive predicate library
is needed for this bounded contract.

[CGAL's primary mesh-processing documentation](https://doc.cgal.org/6.1/Polygon_mesh_processing/index.html)
distinguishes exact constructions in consecutive operations from inexact output
coordinates. That distinction applies directly: source composition stays exact,
and the actual rounded output must independently retain its embedding. Studying
the reference does not adopt CGAL or its data model.

The existing exact positive triangle-chain mechanism directly establishes whole
face coverage. Extend that mechanism to the admitted convex outer loop and one
strictly contained circular chord loop; do not add a second coverage certificate,
occupancy grid, visual oracle, volume-only acceptance or mutation campaign.

## Exact admission and material

For each directed CCW profile edge a→b, write d=b−a, q=c−a for cutter center c,
and s=det(d,q). The full closed circle of radius r is strictly interior exactly
when s>0 and s²>r²·dot(d,d) for every edge. Equality is tangency and rejects;
testing only the center or cardinal circle points is insufficient. No square
root, float unit normal or epsilon is needed. The cutter must span both actual
extrusion cap planes with strict excess according to the selected design.

Independent material answers use all original polygon support signs, actual
lower/upper heights, and radial squared distance minus r². A point is exterior
if outside the source prism or strictly inside the hole. Otherwise its support
set contains every zero polygon/cap constraint and the wall when radial equality
holds. Cap-hole rim points have both cap and wall support; source polygon corners
keep both side supports. Tiny exact and adjacent-float queries attack accidental
tolerance classification. The oracle derives geometry from source vertices and
cutter parameters; product classify/intersect helpers cannot supply the answer.

## Identity, maps and complete ancestry

Keep typed `(body_id, revision, kind, entity_id)` identity. IDs, slot indices,
physical vertices, chart occurrences and traversal directions are distinct.
Boolean result IDs may be dense but must live in a fresh namespace. Original
patch controls/Frame bits survive, while carrier source ownership remains
explicit. Two seam occurrences on the same wall are different occurrences even
when they map to the same physical seam. Boolean IDs or floats must not alias
native integer IDs.

The direct operation history must disposition all six kinds of actual extrusion
and cutter input, and originate every output entity. The composed ancestry must
reach the actual original profile and cutter; it must neither stop at the
intermediate extrusion nor silently replace it. Preserve ordered typed paths:
same profile entity reached through two intermediate occurrences is still two
paths when their transformations or orientation differ.

For retained source cap boundaries, source parameter and cap UV maps are identity
in original carrier coordinates; cap translation back to the profile is exactly
±height/2 with original lower-cap orientation. Side ancestry retains source edge
parameter and axial map z=−height/2+height·v, rather than pretending a side has a
2D profile face parent. New rim entities couple their actual source cap UV with
the cutter curve parameter; wall ancestry retains actual cutter UV and the
restricted V band. Composing signed traversal requires multiplication only where
the design says the orientation has that meaning; reversing a boundary word
must not replace its intrinsic source parameter map.

Qualification independently enumerates expected roles and parents from the
original n-vertex profile and selected cutter. Merely checking reciprocal forward
and reverse tables permits consistently wrong parent IDs and is insufficient.
Query map interiors as well as endpoints with exact non-dyadic parameters. Check
whole affine/rational identities by their coefficients where the admitted map
degree is known; otherwise use the retained explicit composition contract.

## Whole mesh and actual rounded bound

Reconstruct each expected cap chain from its original directed outer loop and
oppositely directed hole rim; every outer oblique side keeps its original full
rectangle. For each wall angular chord strip, reconstruct the complete four-edge
rectangle between cap rings. At each group, positive signed nondegenerate
triangles must sum to exactly the original boundary chain. This constrains the
whole domain, including holes, and catches equal-count missing/duplicate pieces.
Use original incident patch/UV to reproduce every corner, including both seam
lifts and restricted cutter V. No product mesher, fan, zipper/ear clipping or
inverse-UV helper supplies expected triangles.

Check physical edge incidence: exactly two opposite distinct triangle uses,
single cyclic links at every physical vertex, one connected shell, Euler0 for
one through-hole. These attack distinct identity/manifold faults, not coverage
again. Check exact and actual binary64 positions separately: top/bottom
correspondence, nonzero cap height, globally convex outer polygon, simple ordered
hole ring, strict hole-inside-all-rounded-outer-supports, and positive triangles
on their actual planes/chord strips. Oblique rounded edges cannot be assumed
axis-aligned.

The whole-triangle bound is source curve approximation plus the maximum actual
vertex displacement after binary64 export. Exact barycentric interpolation bounds
triangle displacement by the maximum vertex displacement. Circle arc/chord
approximation is bounded in world units; the full cap-hole triangle lies outside
the guaranteed inner disk. A sampled radial point error or a bound in unscaled
local units is not sufficient. Reject rounded collapse or changed embedding even
if a scalar displacement fits the requested tolerance. Preserve exact threshold
semantics; reject unavailable subdivision budget without partially publishing.

## Bounded test plan after design and plan review

Use triangle, asymmetric oblique chamfer and high-vertex convex source profiles;
noncentered circles where admitted; Frame translations/scales including a
subnormal component; source height extremes; coarse and refined tolerances.
Circle admission cases include center-inside-but-circle-outside, exact tangent,
one exact lattice/rational step inside/outside, tangent to an oblique edge, and
an exterior center whose squared distance alone would falsely pass.

Direct output alterations attack wrong original profile parent, consistently
wrong reciprocal history parent, lost source-cap UV scaling, altered side axial
map, wrong cutter V restriction, merged seam occurrence, missing/duplicate cap
triangle, filled hole, one reversed triangle, split physical seam, wrong source
parameter, stale/foreign chart, and understated actual float displacement.
Assert the intended geometric/history diagnostic so an incidental schema error
cannot impersonate sensitivity. These are tests of distinct admitted claims,
not an attempt to certify the kernel or the suite.

Failure cases verify original profile, extrusion, cutter and any prior successful
mesh remain unchanged; retrying a valid request succeeds. Factory-only ownership
and recursively immutable records remain. Canonical body/mesher guards stay
narrow. Run all geometry and tests on the fleet only. Run the selected direct and
affected compatibility tests, qualify the actual watch caller's delivered solids
once, review findings, ship, and stop.

External alpha qualification remains incomplete. This unit makes no upstream
suite, general Boolean, involute gear/contact, arbitrary trim or GPU construction
claim. The watch assembly must disclose decorative components, unsupported motion
and any intentional exploded/intersecting display placement separately from
the modeled exact solids.
