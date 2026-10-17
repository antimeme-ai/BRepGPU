# Connected trimmed boundary regions over cached surface leaves

2026-10-16. Read-only next-dependency proposal, not implemented product support.
The regular patch operation and shared chains/bounded occurrence traces are
implemented separately. This proposal does not promote their boundary bounds to
a full original trimmed-face map. No new dependency or geometry run was used.

## Sources actually read

NVIDIA USD/BRep c9c5979f04cea8c38213004d4f04df0dcfb40d05, intact restoration/hash
in QUARANTINE.md and papers/whole-mesher/source-readings.tsv:

* SmTess.cpp, SmTess::AddQuadBoundaries, 5334–5430 and 5610–5840: original
  outer/hole chains coexist with cached interior polygons. Boundary-intersected
  subdivision nodes are omitted, leaving a connected annular region. Lamina
  rims become additional loops; inner loops are assigned to enclosing faces.
* SmTess::TraceLaminaLoop, 4641–4745, and corner propagation in ProcessTree,
  4860–4910: common corner ownership and neighboring subdivision frequency are
  explicit. The reference traces a rim twice to reconcile its vertices, and
  resolves ambiguous continuations with floating angular comparisons.
* SmTess::TriangulateSingleFace, 8540–8700: joins inner loops using a distance
  criterion, tests convex corners and cache accuracy, then splits recursively.
  These mechanisms do not establish our exact protected-sample/domain contract.
* SmTessSrfCache::BuildTree, 13215–13370, and ImplantUVSegment,
  14293–14445: per-face trim segments are indexed into intersected cache leaves.
  The implementation expands UV boxes by tolerances. We select closed exact
  classification and retain immutable packed indices instead of these pointers.

OCCT 8.0.1 b8f597c677811d1f9f4d8a97f5ae2825c0353a42:
BRepMesh_Triangulator.cxx, Perform/addTriange34/prepareMeshStructure/triangulate,
86–270, projects wire nodes, protects frontier links and calls Delaun.
BRepMesh_FaceChecker.cxx, SegmentsFiller and Perform/perform, 35–93,224–294,
uses wire segment box trees before exact intersection work; its per-wire outer
loop still iterates over every other wire and expands boxes by source tolerance.
We select one face-wide segment index and no tiny-hole/tolerance suppression.

Balázs/Guthe/Klein2004, full acquired text §§5.2,6,7 reread: binary kd subdivision
avoids a cylinder's unnecessary second-axis splits; halfedge trim traversal
requires special handling when a whole hole lies in one mesh face. Their hole
bridges and loop-repair procedure are not our source-preserving algorithm.
Ma/Hewitt2002, full acquired text §§4.1–4.4 reread: hanging interfaces and closing
outer/inner boundaries are explicit stages. Tangent scanline parity and inserting
trim/grid intersections are reference choices, not permissions for private
physical-edge samples. Existing citations/PDF hashes are in
papers/native-adaptive-subdivision/references.bib and acquisition.tsv, and
papers/native-adaptive-curves-construction.bib. Kumar/Manocha's unavailable full
text remains a research lead only; no unexamined guarantee is inherited.

Owned current source read: adaptive_packets.hpp/.cpp and its unit plan;
domain.hpp/.cpp; direct_cdt/core.hpp/.cpp; adaptive_types.hpp;
adaptive_curves_types.hpp; and the accepted adaptive design/plan. This proposal
extends these components, not the superseded whole-face LEPP controller.

## Input and first complete face-domain meaning

One invocation retains original SourceStore/SourceCache, one face/chart and
source/boundary/query/numerical generations, complete SharedCurveSnapshot,
selected disjoint source leaves and chosen-domain policy. Selected chart sites
are exact; actual shared XYZ and logical physical identities come only from the
physical curve authority. Distinct seam occurrences retain distinct chart sites.

Reconstructed-selected domains use their named chosen PL loops as the reference
domain. Their per-use original V/Ce/S obligations remain independently checked.
Authoritative exact geometric-line images may use identity only after the source
profile proves their complete original image equals the selected PL interval.
Other authoritative boundaries require a constructive retained full-domain map;
current authoritative_domain_upper is a BOUNDARY bound and cannot supply it.

The selected map extension is the existing mathematical B/T construction, built
from NEW common curve caches: disjoint ambient joint fans B map original endpoint
A to chosen Q; disjoint monotone tapered-strip maps T flatten transformed original
arcs. Mixed reconstructed references are B^-1(selected PL), not discarded guides.
Every map retains original use/map/lift, source point descriptors, support polygon,
positive sector/fiber order and inverse displacement. Common-service original
curve enclosures/derivatives replace mandatory old GeometryStore. Unknown finite
profiles refuse or request their canonical source interval. The new region path
must not instantiate old FaceEngine or call boundary maxima a face theorem.

For an actual selected triangle K, query inverse T supports, then inverse B
supports, compose component displacements, and bound carrier endpoint differences
on their convex hull within the natural rectangle, across indexed C0 spans.
That E_domain is subtracted ONCE from this SAME triangle's positional allowance.
An identity map has E_domain=0 by declared policy, not by missing map payload.
General curved authoritative complete-face support remains explicitly unavailable
until this actual map producer and consumer are implemented and reviewed.

## Classify leaves; keep source edges intact

1. Admit the complete selected loop graph once per boundary generation. One
   packed closed-box BVH checks nonincident segment contacts and answers leaf
   intersection queries. Exact filtered orientation/closed interval tests resolve
   ambiguous candidates. Winding uses half-open source traversal consistently.
2. A leaf whose CLOSED box touches a chosen trim segment is boundary material.
   A non-touched leaf is wholly inside/outside by one interior winding query.
   Querying segment boxes discovers a tiny closed hole wholly inside a leaf;
   checking four leaf corners would not. Coarse collapsed/crossed traces route
   through the typed source issue/request protocol, not deletion by epsilon.
3. Enable only accepted wholly interior leaves in build_packet_mesh. Existing
   common line events include arbitrary-depth hanging neighbors. Its monotone
   alternating-sector demotion produces regular packet rims. Demoted leaves
   remain band material; no partial rectangle result becomes a complete face.
4. Packet rims never meet physical trims: closed-box trim contact excludes that
   packet. No trim/leaf crossing becomes a mesh vertex. Source boundaries retain
   their original master chain; virtual intersections below are bound points only.

## Connected regions and holes

Reverse each PacketRim to put BAND interior on its left. Combine these oriented
cycles with the original normalized selected loops (outer positive, holes
negative), preserving original source traversal adapters separately. Degree-two
rim continuation uses canonical chart point IDs, not nearest-angle guesses.
Source contact/weak-loop issues remain explicit. Packet-only point pinches have
already been removed by the existing demotion owner.

Build a containment forest of the resulting SIMPLE, disjoint oriented cycles.
Use an offline exact vertical sweep over all cycle segments and leftmost-cycle
queries, rather than every hole against every outer cycle. A packed balanced
active-edge index compares segment ordinates at the sweep coordinate using exact
crossproducts. Group equal X events; endpoint ties use one symbolic right-side
query x+δ and deterministic y+δ², resolving comparisons by the first nonzero
coefficient, never numerical jitter. Vertical segments do not occupy an open-X
slab. A cycle query excludes its own incident edges. The immediately lower other
edge identifies its cycle if the query lies inside that cycle, otherwise that
cycle's previously established parent. The unbounded region is the root.
Noncrossing admission ensures active edge order cannot swap between events.
The unit plan must spell out all grouped-event/start/end/exclusion cases before
implementation; this is an owned construction proposal, not an inherited
O(logN) claim about the existing BVH.

Every positive cycle names a band component outer; its immediate negative children
are its holes. Nested positive cycles belong to separate components. A whole-face
band with no interior packets is valid. A source hole or packet island remains a
hole even when its diameter is below positional epsilon. Batch each connected
component once, with all protected source and rim intervals intact.

## Local connectivity and a real current API gap

Use the owned D&C plus constraint-recovery CDT only within each band component.
Initial sites include protected loop sites plus retained CACHE construction
anchors strictly inside that component. No extra Steiner choice belongs to CDT.
Clear exact coordinate denominators with ONE common positive D for BOTH axes;
retain originals in output. Bound integer bit growth/work/bytes before mutation.

Current direct_cdt::solve already triangulates every predicate site and receives
boundary constraints separately, but core.cpp~978 asserts T=N+2H−2, assuming all
sites are boundary. Interior anchors require T=2N−B+2H−2. The current domain adapter
also requires every site in a supplied cycle. A narrow typed region adapter must
admit unique interior sites, test their strict component membership/indexed
nonincident-edge exclusion, count B separately, and retain all site/output IDs.
This is a reviewed extension of the existing constructor, not a reason to switch
to the old FaceEngine or pretend the current API already accepts anchors.

## Actual facets, cache refinement and progress

For each actual band triangle, query intersecting cache leaves. Exact rational
triangle/rectangle clipping partitions its chart domain. Virtual vertices retain
barycentrics in the ORIGINAL actual triangle, so every piece consumes SAME L.
Evaluate cached B at virtual points, never resample S to replace L. Bound each
piece by e_leaf + upper(norm(B−L)) + E_domain(K), using the common quadratic
residual/covariance kernel and original same-facet tightening on ambiguity.
Actual physical noncollapse remains separate from positive chart orientation.

Consume a COMPLETE stable failure horizon. Domain/source errors request their
named physical authority. For a genuine surface construction failure, refine
implicated cache leaves AND install at least one NEW anchor strictly inside each
failed actual triangle before rebuilding that region. Choose a positive exact
triangle/child-leaf overlap piece; its rational centroid is a locator. Round on
successively finer exact dyadic grids until orientations prove strict interiority.
No binary64 interior representability assumption; no unchanged-L cache-only loop.

Canonicalize/deduplicate ALL requested anchors and splits, evaluate through the
same PointPool, and rebuild each affected connected region ONCE per horizon.
Anchors remain in that region's immutable generation history, so the old failed
triangle cannot reappear without containing an installed site. Boundary changes
reclassify retained anchors before rebuilding; outside anchors are discarded from
construction, not from original-source inventory. Numerical floors promote the
shared cache profile; they do not force endless geometry splits. Fair root-axis
splits and explicit cumulative depth/work/bytes bound the operation. This is
concrete progress, not a universal finite-termination or angle-quality theorem.

## Finite interface and direct oracle obligations

Proposed RegionInput retains source/chart identity, SharedCurveSnapshot,
PatchSelection and PacketMesh, named DomainMap, exact source-cycle adapters,
and sorted retained anchor keys. RegionOutput retains those leases plus packed
component/loop/constraint/site/facet rows, source-or-rim edge provenance and
cache-leaf overlap ranges. A boundary site uses physical sample identity; packet
and anchor sites use source chart/parameter identity plus constructed revision.
Immutable numerical work carries actual XYZ, exact chart roles, SAME-L barycentric
pieces and all generations. No evaluator callback, old topology owner or shared
Budget escapes. Output publication waits all component bounds and contact work.

Extend the EXISTING independent exact chain/embedding reader for actual region
outputs: positive triangle areas, selected-domain minus packet chain equality,
all original physical edges and rim atomic intervals with opposite incidence,
no omitted holes, no undeclared crossings/hanging nodes, interior anchor incidence
and complete original use inventory. Original closed-form curved sources supply
whole-facet error checks in the SAME reader. CPU/P100 byte parity is additional
metamorphic evidence, not the geometry oracle.

First authored families before watch: tiny hole entirely in one leaf; many aligned
holes; packet islands and nested band components; nonuniform span endpoints and
arbitrary hanging depths; diagonal packet pinch demotion; reversed source loops;
separate periodic seam occurrences; one long triangle crossing many cached leaves;
curved actual facet requiring a new strict-interior anchor; exact line-image
nonlinear-speed boundary; and supported authoritative B/T versus explicit missing
map refusal. Count band fraction, side events, containment comparisons, CDT work,
anchor horizons, rebuilds and clipping pieces. Dense/thin trims may consume most
of the face; that is measured work, not a feature-dropping admission trick.

No product code or new benchmark campaign is authorized by this report. The next
unit plan must close grouped containment events, typed CDT interior admission,
actual cache-driven split/anchor API and domain-map producer before its promised
finite complete-face support is claimed.
