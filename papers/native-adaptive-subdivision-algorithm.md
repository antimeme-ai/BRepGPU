# Owned cached adaptive subdivision: algorithm and trim construction

Status: concrete research proposal for the operator-selected replacement mesher.
No product code was written in this unit. Cached subdivisions own approximation
and refinement; the old whole-face CDT/circumcenter/LEPP/optimizer loop is not the
default hidden behind a cache. The report distinguishes selected mechanisms from
the implementation-plan obligations still requiring review.

## What the pinned source actually does

Reference: NVIDIA USD/BRep commit
`c9c5979f04cea8c38213004d4f04df0dcfb40d05`, retained under
`quarantine/sources/usd-brep`; restoration and intact archive hash are in
`QUARANTINE.md`. Existing file hashes are in
`papers/whole-mesher/source-readings.tsv`. The official measured reference links
the distributed binary; these readings do not establish its build flags.

* `source/SMLib/src/SmTess.cpp:3074–3117` creates a per-face surface cache.
  `:13215–13355` reuses its subdivision tree and embeds selected UV segments in
  intersected leaves. `:14293–14445` marks touched leaves as boundary leaves.
* `SmTess.cpp:5334–5430` emits interior cache polygons. Its diagram/comment at
  `:5610–5648` explicitly leaves an annular region between interior packets and
  original trim chains; `:5650` onward traces packet rims. It does not require
  every physical trim segment to be cut at every surface-leaf crossing.
* `SmTess.cpp:4850–5233` propagates corner ownership and constructs child
  interfaces by splitting existing polygon sides. `:8557` triangulates remaining
  regions, with a vertex spatial tree at `:9074`; this is distinct from using
  whole-face Delaunay refinement as the approximation engine.
* `SmCurveCache.cpp:1210`, `:1511–1570`, `:1710–1858` stores span blocks, accepts
  leaves, splits failed spans and builds an ordered binary cache.
* `SmSurfaceCache.cpp:3577–3708` selects subdivision from cached geometry tests;
  `:4053–4125` has an iso-partition fast path. Minimum-UV stopping at `:3927`
  and partition culling at `:4121` are reference choices, not our error contract.

These mechanisms motivate persistent restrictions, ordered curve intervals,
interior packets and boundary regions. They do not select pointer-based topology,
tolerance snapping/clamping, approximate analytic starter patches or forced
acceptance at minimum size.

Balázs, Guthe and Klein, *Efficient trimmed NURBS tessellation* (WSCG2004),
§§5–8, was reread from the acquired full text
`papers/whole-mesher-2026-10-16/balazs-guthe-klein-2004.pdf`. It compares binary
kd subdivision with quadtree subdivision, uses a conservative bilinear residual
plus twist bound, and exploits repeated basis evaluations. Its trim graph needs
special treatment for holes wholly inside a cell. Its source conversion and
repair procedures are not inherited as original-source authority. Bibliography:
`papers/whole-mesher.bib`; public author link:
<https://cg.cs.uni-bonn.de/backend/v1/files/publications/balazs-2004-efficient.pdf>.

Ma/Hewitt, *Adaptive Tessellation for Trimmed NURBS Surface* (Eurographics2002),
full8-page publisher PDF acquired/read in this unit, §§4.1–4.5: exposes the same
need for conforming hanging interfaces, removes outside/hole patches and closes
boundary regions with triangles. Its illustrated midpoint triangulation uses
extra triangles, and its scanline procedure discusses tangent parity by
repeating tangent points. We select exact half-open classification and the
no-center side zipper instead. Its pseudocode revisits trim loops per patch;
our indexed classification avoids treating that scan as the default. These are
mechanisms to improve, not a whole-source watertight/error theorem to inherit.
Intact PDF/hash and BibTeX are under `papers/native-adaptive-subdivision/`.
Kumar/Manocha1995's author abstract supports coherent cells/trim tracing as a
research lead; its full linked PostScript returned404, so it is not counted as
a full-paper read or an inherited algorithm guarantee.

## Selected complete construction

1. Compile immutable original source/root geometry once. Preserve the existing
   definition-local physical source-edge owner, explicit source vertices,
   declared quotient aliases and original coedge inventory. Curve-cache leaves
   settle one actual XYZ chain for every edge and all incident uses.
2. Produce selected per-occurrence chart traces and full original-domain maps
   directly from these caches. Keep authoritative pcurve domains and reconstructed
   reference policies distinct. Do not run the previous expensive full analytic
   preparation unchanged in front of the new cache.
3. Adapt packed binary surface-root trees using cached original-to-bilinear
   bounds. Classify leaves against the settled selected polygon. A leaf whose
   closed rectangle touches any trim segment is a boundary leaf. A non-touched
   leaf is inside/outside by exact half-open winding at an interior point.
4. Emit accepted strictly interior leaf packets with common side schedules.
   Extract their external rims. Preserve every original source trim segment
   unchanged. Triangulate the connected remaining boundary regions between the
   source chains and these rims, with cache-derived interior sites as needed.
5. Bound every actual facet using the cache and its SAME affine actual XYZ map.
   Failed packets refine their cache leaves. Failed boundary facets refine their
   cache-backed construction, not LEPP. Source/domain-map limitations request
   the original feature owner. Repeat complete source/cut horizons when needed.
6. Run filtered actual-facet contacts on the resulting current facets. A marked
   facet routes to its packet/boundary-region/cap owner for cache refinement or
   explicit unresolved output. Publish only a complete successful semantic mesh.

The existing source compiler, exact values, source-edge forest, domain maps,
contact classifier and NativeMesh codec remain reusable machinery. The previous
FaceEngine's refinement authority and global fixed-site optimizer do not.

## Trees, splits and numerical cache

Select a binary kd hierarchy rather than mandatory quadtree splitting. Cylinders
and extrusions can refine one parameter direction without multiplying samples in
the other. Four-way/Morton leaves offer simple GPU templates but can spend extra
work on such surfaces. A Morton order may be a storage/BVH hint; it is not the
node's semantic identity.

Roots retain original source span bounds; child coordinates are exact dyadic
fractions of those bounds. Root endpoints need not themselves be dyadic. A node
key contains source/root identity and its complete split-axis/child path: rounded
U-then-V and V-then-U control nets are not interchangeable merely because their
boxes coincide. Cached original data and propagated departure intervals are
separate from query-specific selected leaves and trim classes.

Initial directional policy: trial midpoint restrictions in U and V; compare the
canonical worst-child approximation upper bounds and select the lower value,
then the lower predicted output/work count, then fixed U tie. Retain the selected
children. Concrete fairness proposal: every fourth failed-lineage split instead
halves the longest normalized original-root ACTIVE axis (tie U). An axis is
inactive only after a checked exact source property proves it contributes no
approximation error, such as `S(u,v)=a*u+F(v)` with no mixed term; generic roots
activate both. An infinitely failing lineage then shrinks every active width.
The fairness tax is measured, and finite precision/resource floors still return
unresolved. Root integration must explicitly select this policy or a different
reviewed finite rule before implementation; no minimum-size success is allowed.
This is not an optimal-anisotropy theorem. Source-specific exact planes and
checked physical-plane charts bypass unnecessary parameterization subdivision.

One PointPool owns common chart points. Its key is the exact original carrier
parameter plus declared evaluation profile; the original root and fixed tensor
evaluation order choose its bounded actual XYZ, never the first visiting leaf.
Sharing the evaluation does not weld distinct face/physical node identities.
Original physical-edge samples come only from their curve-cache owner.
An admitted physical-plane chart uses its explicit chart-map key and exact XY
instead: actual points come from that retained affine lift, while original UV
is a separately bounded recovery locator. It must not masquerade as ordinary
original-UV evaluation or inherit a first-face approximation.

For a leaf normalized to `[0,1]^2`, let B interpolate its four retained actual
corner XYZ values and let `D=B11-B10-B01+B00`. Cache a whole-leaf bound
`||S-B|| <= e`. The positive-rational case uses the coefficient residual `N-WB`
and positive W lower bound; analytic cases use original-frame directional bounds.
For any positive triangle wholly inside that leaf,

    ||S-L|| <= e + ||D||/4 + max_i ||B(p_i)-Q_i||.

Indeed `B(sum lambda_i p_i)-sum lambda_i B(p_i)` equals D times a weighted
covariance, bounded by1/4 on the unit square. `Q_i` are the SAME emitted actual
corners. Initial four-corner packets have zero nodal term. Hanging samples have
their B departure computed/cached per incident leaf. This reuses one bound for
many output triangles; no new high-degree residual is rebuilt per triangle.

For a boundary facet crossing leaves, clip its chart triangle against the
overlapping leaf rectangles, fan each positive clipped polygon, and bound
`e+||B-L||` on each piece. At virtual clip vertices evaluate B and the SAME L;
do not evaluate S to replace L. Original-domain displacement is added once.
Virtual clip vertices are numerical partition points, not mesh sites.
The quadratic Bernstein residual of B-L gives a tighter small constant-size
fallback than the shared twist bound. A failed loose upper is not a witnessed
geometric error: tighten the SAME actual facet before adding geometry, and
rebuild an original-root numerical enclosure if accumulated rounding dominates.
CPU/P100 use one defined bounded construction/decision profile and ambiguity
route; common floating intervals need not be exact-real source coefficients.

## Arbitrary hanging interfaces without forced balance

Build packed side records `(axis, exact_constant, begin, end, leaf, side)`.
Sort/group collinear records and sweep their exact endpoints. Each positive
atomic internal interval has two incident leaf sides; its ordered event chain is
consumed identically by both. Root/span interfaces participate in the same
schedule. A face point receives one PointPool value and one face-local node key.
There are no independent side copies to tolerance-stitch later.

Select arbitrary-depth side schedules initially. Mandatory2:1 balancing can
propagate extra splits far from curvature/trim demand; its simpler fixed GPU
templates are a useful measured alternative, not an inherited requirement. If
one coarse side faces many fine sides, those events already imply corresponding
output work. Record event count, largest side fan and packet size.

Interior packet triangulation is linear and adds no center evaluation. Choose
the BL–TR rectangle diagonal (a deterministic alternative diagonal policy can
be reviewed). In the BL–BR–TR half, keep BR for last. Maintain a current endpoint
on the bottom chain starting at BL, and one on the right chain starting at TR.
Consume one next non-BR sample from either chain; emit the positive triangle
against the opposite current endpoint. Choose the shorter new actual-XYZ
diagonal, with exact squared comparison and stable tie, when both candidates
exist. Once those samples are exhausted, emit `(current_bottom,BR,current_right)`.
Do the symmetric construction in the other half, retaining TL for last.
Each step removes an ear of a convex triangle with points on its two sides.
It never removes their common corner prematurely. All collinear side samples
survive, all output triangles are positive, and k boundary samples emit k−2
triangles. No CDT is needed for these packets.

## Exact trims, holes and boundary regions

Build one filtered exact segment index per settled chart/trace generation.
Leaf/trim intersections are between the selected PL trace and an exact rectangle;
exact rational orientation/intersection resolves ambiguous filters. Original
curved-domain meaning remains in retained maps and its error term. This is not
a claim to solve arbitrary algebraic original pcurve/isoline intersections.
Tiny holes wholly inside one leaf are discovered by segment-box queries, not
corner samples. Every original loop/use survives regardless of epsilon.

Do NOT turn leafline/trim intersections into private physical boundary vertices.
Mapping their chord fraction to source t and recovering a new UV generally moves
the point away from the supposed grid crossing. Keep the original source chain;
only canonical source-owner subdivisions may add its nodes.

Packet unions can touch only at a grid vertex, leaving weak rim cycles. Before
using a simple-disjoint-loop triangulator, inspect the exact cyclic packet/band
sectors at side-event vertices. An alternating pattern demotes the incident
packet leaves into the band. Process newly affected sectors until regular.
Demotion is monotone and finite: each packet is removed at most once. Source
features do not move. Worst-case loss of most/all interior packets is recorded,
not hidden by dropping holes or feeding weak loops to an incompatible CDT.

Extract disjoint positive outer/negative hole cycles of the resulting band.
Use the owned direct CDT only within each connected region, protecting all
original source and packet-rim intervals. Interior construction sites come from
the cache/region, not circumcenters or an independent surface-refinement mesh.
A thin/dense-trim face can leave a whole-face-sized band; that is an honest
degenerate workload of the selected algorithm.

The existing typed direct constructor currently integerizes dyadic sites.
Absolute non-dyadic root endpoints or clipping-derived centroids must not be
silently sent through that adapter. Plan either exact per-component denominator
clearing (with its explicit BigInt work/memory cost and preserved metric) or an
owned rational predicate adapter. On the current binary64-root profile, interior
anchors can remain arbitrary-precision dyadics. This is an explicit bridge task.

## Boundary-region refinement that changes the emitted facet

Cache restriction alone cannot improve an unchanged bad XYZ triangle. The
following transaction is required whenever a boundary facet fails:

1. Consume failures in canonical region/facet order after the complete bound
   horizon. Separate original-domain/physical-source limitations from surface
   approximation work. A domain-map failure requests its named source feature;
   an inadequately sampled source side requests its actual master interval.
2. For a surface-approximation failure, subdivide the implicated overlapping
   cache leaves and install new cache-derived interior anchors before rebuilding
   that region. At least one new anchor must be strictly inside the failed
   triangle; otherwise this transaction is not an approximation refinement.
3. Obtain such an anchor from a positive-area exact intersection of a child leaf
   and the failed triangle. Choose a positive triangular piece and its rational
   centroid as a locator, then choose a deterministic exact dyadic point strictly
   inside that piece by successively finer dyadic rounding and exact orientation
   checks. Do not assume binary64 has an interior representable point. Refuse
   under cumulative limits if the required precision/capacity is unavailable.
4. Evaluate the chosen point through PointPool, retain it as a construction site
   of that leaf/region generation, and rebuild only affected region connectivity
   from the protected source/rim cycles and accumulated sites. A source interval
   is never privately split by this operation. Rebound current actual facets.

Collect ALL current region failures at the same deterministic bound horizon;
install their canonical deduplicated anchor/split requests in bulk, then rebuild
each affected region once. Rebuilding a component once per failed triangle would
reintroduce quadratic work and is not this proposal. A new source-chain/domain
generation reclassifies retained construction sites: reuse their geometry values
but discard sites outside the new selected domain. Old connectivity does not
survive merely because a PointPool value remains cached.

New anchors strictly inside the old triangle make it impossible for unchanged
connectivity to survive. This is a concrete progress step, not a universal shape
quality or termination theorem. Arbitrarily small source angles, incompatible
geometry, finite arithmetic floors and pathological skinny regions can still
end unresolved/resource-exhausted. A finite midpoint fairness rule for repeatedly
failed cache lineages, protection/domain routing and exact anchor tie rules must
be specified in the implementation plan. No fallback restores old LEPP.

Source master changes use the complete all-face horizon, rebuilding incident
face classification/regions against the new immutable chain while retaining
source restrictions/point caches whose keys remain valid. Private cap-cut
changes rebuild only that face under the same full horizon; they never enter
the original-edge forest. Query/generation identities reject stale work.

## Seams, poles, output and measured risks

Explicit seam occurrences keep distinct chart sites but use the same declared
physical source-edge nodes. No equal-XYZ seam inference is permitted. Original
knot-span roots share exact side schedules; derivatives never silently cross
a C0 knot. The existing checked PlaneXY profile remains a source-specific strong
path for nonlinear parameterizations of planar geometry.

Finite sphere/cone quotient caps retain their original declared aliases,
symbolic source coverage and shared ordinary/fan cut. Ordinary leaf packets stop
at that selected cut; proxy coordinates are never used as S coverage. Angular
or radial failure updates the cap's cached partition/source masters under the
existing cut protocol. Full-quad positional/corner/band terms and role/facet
permutation remain explicit. The current one-cap/four-use profile is not silently
generalized to arbitrary singular atlases.

NativeMesh still carries original source, traces/domain maps, physical keys,
placements/materials and actual facets. Ordinary facets use actual UV/PlaneXY
maps; poles use the finite symbolic map. Add an explicit adaptive-subdivision
lineage operation/root-path association rather than labeling these sites as
circumcenter or LEPP births. Cache storage offsets do not become source IDs.

Measure root/control/sample reuse, leaf and interface counts, band fraction,
trim candidates, local CDT work, copied/rebuilt regions, ambiguity/exact work,
GPU restriction/evaluation/split/compact work, transfers and complete operation.
Compare cold and declared warm runs separately. Include rotated anisotropic
ridges: an axis-aligned kd cache may need more cells than an aligned anisotropic
triangle mesh. That is a measured tradeoff of the selected architecture, not a
veto. Independent cylinders/extrusions, distorted planes, mixed/C0/rational
patches, many holes, tiny holes, long skinny trims, shared edges, seams and finite
poles precede watch tuning. No new measured speedup is claimed by this report.

## Integrated design/plan review checkpoint

Read the actual initial `docs/ADAPTIVE_MESHER_DESIGN.md` and
`docs/ADAPTIVE_MESHER_PLAN.md`. The cache-owned default, whole conceptual owners,
real untrimmed patch→conforming triangles first checkpoint, subsequent complete
trimmed-source integration and honest cold/warm/NVIDIA comparisons fit this
proposal. The previous constructor remains an explicit development control.

The dependency plan is accepted in scope; it is not yet an executable unit plan
while its main design deliberately leaves split/point/layout policies open.
Before product code, integrate the exact side zipper/rim rule, selected numerical
profile and deterministic split fairness, region-failure transaction, exact
coordinate bridge and new subdivision lineage meaning. These are bounded
constructive decisions listed above, not requests for another research cohort or
for reinstating the superseded CDT-primary design.
