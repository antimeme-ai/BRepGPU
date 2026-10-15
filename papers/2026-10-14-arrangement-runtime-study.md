# Planar arrangements: exact geometry and owned region construction

Research, 2026-10-14. Read current doctrine/instructions, roadmap, delivered exact
sections and material classification, source intersection graph construction,
pinned OCCT wire/face building, and primary CGAL arrangement documentation/source.
No product code, dependency adoption, reference execution or new benchmark.

## Bounded recommendation

Build a complete exact CPU subdivision of one owned rectangular planar face by
rational axial grid cuts and multiple circles. Each circle comes from an existing
complete closed plane/wall IntersectionSet, is strictly inside the source face
and one open grid cell, and has a closed disk strictly disjoint from every other
disk. Explicitly reject clipping, tangent contact, coincidence, nested/overlapping
disks, cut/circle crossings and source holes. This is a useful multi-region and
multi-hole family, not a general curve arrangement or a preview sketch.

Every grid cell yields its rectangle-minus-disks region with a positive outer
loop and clockwise holes. Every disk yields a separate region with a positive
circle outer loop. Grid edges are shared by adjacent cell regions; circle edges
are shared by disk and surrounding cell. Include one exterior region and the
opposite rectangle-boundary uses so every edge has twins. The resulting regions
cover the original closed face, have disjoint interiors and share exact boundary
identity. Selecting the outside-disk region for a one-circle cap is an immediate
input to a later through-hole Boolean; this operation does not assemble that solid.

An arbitrary single clipped circle is not the smaller whole conceptual unit.
Generic transverse clipping introduces new exact algebraic endpoints and boundary
splits; tangency introduces equal tangent directions, degree-four or pinched
boundary walks, zero-dimensional contacts and distinctions between shared
vertices and touching loops. The current SSI emits exact contacts, but converting
them into complete ordered regions is another invariant. Record that extension
for round two instead of allowing an attractive partial graph to stand for it.

## Primary literature and inspected reference code

CGAL's arrangement manual describes separate geometric traits and DCEL topology,
paired halfedges with a face on the left, linked next/previous boundary walks,
positive outer components, negative inner components and an exterior face. Its
general conic traits require algebraic machinery beyond simple rational segment
arrangements. For this admitted family, direct cell/loop construction supplies
the complete result without implementing a sweep-line framework. This is an
owned algorithm selection, not adoption of CGAL or a claim to its generality.
[Official arrangement manual](https://doc.cgal.org/latest/Arrangement_on_surface_2/index.html),
[conic traits](https://doc.cgal.org/latest/Arrangement_on_surface_2/classCGAL_1_1Arr__conic__traits__2.html).

The official Arrangement_with_history interface retains relationships between
input curves and induced edges. This motivates retaining actual section/component/
piece/event and original boundary-use sources on owned records, rather than
discarding them after constructing coordinates. Its history model does not
provide our persistent naming, edit transactions or source-body identity.
[Official history interface](https://doc.cgal.org/latest/Arrangement_on_surface_2/classCGAL_1_1Arrangement__with__history__2.html).

Read current public CGAL Arr_dcel_base.h as an implementation reference for edge
pairing and boundary bookkeeping; its pointer ownership is not our data model.
The browser source is mutable main, so no pinned acquisition or reproduction is
claimed. Existing owned dense records/typed lineage remain the representation.
[Inspected source](https://github.com/CGAL/cgal/blob/main/Arrangement_on_surface_2/include/CGAL/Arr_dcel_base.h).

Read pinned OCCT BOPAlgo_BuilderFace.hxx/cxx and BOPAlgo_WireSplitter.hxx, including
PerformAreas loop grouping and growth/hole handling. It builds draft loops,
separates holes/growths, classifies holes into containing areas and constructs
faces. The useful lesson is that a collection of edges or loops is not yet a
face arrangement. Its general tolerance/classifier machinery is not an exact
oracle for our rational family and no code is copied.
[Pinned face builder](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/src/ModelingAlgorithms/TKBO/BOPAlgo/BOPAlgo_BuilderFace.cxx),
[pinned wire interface](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/src/ModelingAlgorithms/TKBO/BOPAlgo/BOPAlgo_WireSplitter.hxx).

## One physical circle edge, complete source maps

Use one closed circle edge and one anchor vertex, matching the existing body's
closed ring representation. Retain the original complete IntersectionSet,
component ID, ordered four source piece IDs and all source events/maps on the
edge's source description. Select the anchor from a named endpoint incidence;
do not infer it by rounded coordinate equality. The twin circle uses reverse
traversal on the same physical edge; they do not copy or perturb geometry.

The four rational chart pieces remain geometry/atlas segmentation. Their
cardinal joins and wall seam are retained in source provenance, but do not become
four artificial B-rep feature edges or new geometry identities. A consumer can
traverse the original pieces in canonical order for the forward use, reverse
piece order/parameter traversal for the other use, and still evaluate the same
spatial and both original UV maps. The seam retains its distinct source lifts.

My initial quarter-edge suggestion was a valid explicit refinement, but root and
the semantics lane selected the one-edge representation. It is simpler and
better matches the intended first Boolean cap/cylinder ring correspondence.

## Exact input and construction

Read/validate the actual affine rectangular source chart and exact frame, as in
the classification unit. Cuts are rational coordinates on its two parameter
axes, strictly between boundaries, with duplicates rejected and canonical order.
Full SSI circles are horizontal in the current corpus; grid-only rectangles may
support other canonical orientations if the design admits them explicitly.

Convert submitted int/Fraction/finite float values before frame arithmetic.
For each retained full circle validate supported source-face correspondence,
closed nondegenerate component, complete ordered source pieces and exact map/
endpoint connectivity. Use exact center/radius from original source geometry and
verify the section is wholly within the target rectangle and its assigned cell.
Do not replace a clipped section with an assumed complete carrier circle.

Disk disjointness uses squared center distance strictly greater than
`(r1+r2)^2`; equality rejects tangency and the opposite sign rejects overlap or
nesting. Circle footprint bounds strictly inside a cell reject touching a cut or
outer boundary. All tests are rational for the supported source circles; no square
root, polygonal circle approximation, epsilon welding or sampled containment.

Grid vertex identity is its canonical row/column generator pair. Grid segment
identity is its adjacent generator pair. Boundary segments retain original
edge/coedge identity and exact source subrange. Circle identity is its retained
section/component source, not its center or sampled positions. Duplicate or
coincident section geometry rejects rather than silently merging independent
source histories. Dense result-local IDs identify new records, not persistent
body names. Preserve typed body/revision/face/chart identities in retained sources.

Construct loops from these generators: each cell has four ordered grid uses;
each contained circle adds a reversed-use hole; each disk gets the positive use.
The outside rectangle uses form the exterior's inner component. Assign both
twins, incident region, loop, next and previous consistently. Loop orientation is
positive/negative in target UV; original face orientation remains separate for
future solid assembly. A grid crossing has the same single generated vertex in
all incident edges/regions, while independent circle components stay distinct.

For k columns, l rows and m circles the one-anchor representation yields
`V=(k+1)(l+1)+m`, `E=2*k*l+k+l+m`, `F=k*l+m+1` including exterior. There are
m+1 connected components and `V-E+F=m+2`. These counts complement exact embedding
and source-map truth; Euler alone cannot detect a geometrically wrong loop.

## Region witnesses and a usable consumer

Disk centers give exact rational interior witnesses. For a cell's outside-disk
region, choose U strictly between the cell's left boundary and every circle's
left footprint, then choose the V midpoint. Strict containment guarantees a
positive rational interval, so no unbounded witness search or algebraic input is
needed. With no circles, use the cell center. Verify the witness against actual
region inequalities, not only an arbitrary original solid's point label.

Expose all ordered outer/hole loops and shared edge uses to an actual cap-region
consumer. It should follow next/previous, resolve twins/adjacent regions, evaluate
line endpoints and every source circle piece through its exact spatial/plane/
wall maps, and retain original boundary provenance. An optional point-location
reader must define boundary incidence explicitly: closed neighboring regions
share edges/vertices, so returning one arbitrary region at a boundary is wrong.
Witness classification against a tool is not by itself proof that a whole general
face has one material label; here construction proves the supported sign cells.

## Ownership, cost and independent attacks

Retain immutable face/section inputs and publish one complete immutable result.
Malformed sources/cuts, unsupported configurations, iterator failures and
arithmetic/allocation failures raise before any partial result is returned.
No native handles, body mutations, JSON internals or GPU resources are introduced.
Arbitrary rational bit size affects cost and errors propagate without guessed
topology. Bound admitted counts explicitly in the reviewed design if necessary.

Exact pairwise disk admission is quadratic in circle count; state that cost and
include it in complete construction timing rather than hiding it as setup. A
spatial broad phase is a future measured optimization, not an oracle. Record SSI
preparation, arrangement admission/construction/publication, actual consumer and
independent qualification separately. Small one-hole caps, grid-only subdivisions,
several holes per cell and a moderate regular multi-hole grid are useful fixtures.
They do not establish performance on interacting cuts or arbitrary CAD solids.

Independent truth should read original source curves and planar halfspaces,
check strict separation and complete per-cell disk membership, directed loop
orientation and exact whole circle maps, then check twin/next/previous/source
incidence and rational witnesses. Useful faults include dropped hole/disk,
wrong twin/incident region, skipped grid segment, reversed hole traversal,
lost source map/seam lift, geometry-coordinate welding and omitted boundary
subrange. Execute product tests/mutants only on the fleet, after actual red and
accepted whole-unit plan.

GPU arrangement construction, general sweep ordering, tangent/clipped/overlapping
circle topology, cross-pair global event naming, persistent edits and Python's
shipping role remain return-pass work. No native/GPU or integration prototype is
part of this recommendation.

## Reference record

[Bibliography](arrangement-runtime.bib) records inspected primary documentation
and source URLs. This study reused the existing intact OCCT source archive and
its restoration manifest. No new source archive or dependency was imported;
the prior robust-arithmetic PDFs remain available for exact predicate design.
