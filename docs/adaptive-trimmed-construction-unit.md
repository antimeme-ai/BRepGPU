# Cached selected trimmed-face construction

Status: finite DESIGN for review, 2026-10-16. No product implementation is
selected by this document until design and subsequent implementation plan review.
One construction owner connects original shared chains, source leaves, packets,
band components and cache-driven anchors. Numerical/runtime companions are
`papers/native-adaptive-trimmed-facet-bounds.md` and the forthcoming runtime unit.
The study is `papers/native-adaptive-boundary-regions.md`.

## Grounding, purpose and finite scope

NVIDIA USD/BRep c9c5979f04cea8c38213004d4f04df0dcfb40d05:
SmTess::AddQuadBoundaries (SmTess.cpp5334–5430,5610–5840) leaves the region
between original trims and interior packets; TraceLaminaLoop4641–4745 reconciles
rim samples; BuildTree13215–13370 and ImplantUVSegment14293–14445 associate
trims with cache leaves; TriangulateSingleFace8540–8700 triangulates remaining
polygons. OCCT b8f597c677811d1f9f4d8a97f5ae2825c0353a42:
BRepMesh_Triangulator86–270 protects wire links, while FaceChecker35–93,224–294
uses segment boxes. Balázs/Guthe/Klein2004 §§5.2,6,7 and Ma/Hewitt2002
§§4.1–4.4 establish useful cached/boundary/hanging mechanisms, not our original
source theorem. Exact reading scopes, URLs and acquired hashes are in the study.

Selected improvements: exact shared side streams; one face-wide trim index;
no private physical trim/grid intersections; directed-band CDT and void flood
instead of distance hole bridges, angular rim guesses or all-hole containment;
actual SAME-L leaf bounds; bulk anchors/refinement; retained original identities.
These are owned mechanisms, not copied reference code or new dependencies.

First complete face profile: one ORIGINAL-UV construction chart, positive-weight
rational source degree<=3 in each direction with original C0 spans, or admitted
regular analytic source tiles. Complete selected cycles may have multiple outer
components and holes. Explicit unwrapped periodic seams retain separate chart
occurrences and shared physical source nodes. Sphere/cone poles, inferred general
atlases, unknown NURBS periodic metadata, nonidentity plane charts and general
curved-authoritative domain maps remain named unsupported profiles, not omitted
source features. Complete face means every selected region and original use in
this admitted profile is present and every emitted actual facet accepts.

## Retained face/domain authority

FaceConstructionKey binds SourceStore lease/generation, definition/face, query,
source-boundary generation, chart revision and numerical profile. Every immutable
work/result retains these, not just array slots. PhysicalNodeKey plus constructed
revision supplies actual source-boundary node identity; equal XYZ never welds.
A construction boundary site is keyed by the original face LOOP junction
occurrence at a joint, or by original coedge occurrence plus logical sample and
branch/lift in its interior. PhysicalNodeKey is a SEPARATE physical-node link;
it never merges u0 and u2pi chart sectors. Only the already-settled adjacent
loop-joint occurrence unifies two use endpoints. Nonincident equal chart
coordinates are a typed graph issue, not a coordinate weld.
Source loops/use order and sense remain original, with separate ±1 normalized
cycle traversal adapters. Original face.orientation is applied only at physical
facet publication, never by reversing source UV or correspondence roles.

A non-null FaceDomainLease has one explicit policy:

* reconstructed_selected: the complete chosen PL loop graph is the declared
  reference; E_domain=0 by that meaning, not absence of a payload;
* authoritative_exact_image: every authoritative use has a producer-owned exact
  whole-use image/order record, reconstructed uses retain their selected policy.

The geometry original-curve primitive supplies exact raw pcurve endpoints A/B,
a covector with distinct endpoint scalars, homogeneous collinearity of all
original positive-weight controls, and a nonnegative scalar derivative numerator
on every original/mapped span. Endpoint progress is strict; stationary t³-style
endpoints are allowed. Kind1 and positive rational originals are first profiles.
Map1/map2 traversal/lift is preserved; unknown map3 is unsupported. Nonzero
symbolic shifts without an exact finite endpoint-equality profile are unsupported
for this AUTHORITATIVE identity policy. Reconstructed shifted seams remain valid.

The trace/domain producer then checks every chosen use site EXACTLY on the SAME
segment, strict scalar order, exact A/B endpoints and exact common original joints.
It records original use/curve/map/lift, A/B, covector/scalars and chosen site range.
Axis-aligned and diagonal lines use the same algebra. Equal-t speeds may differ.
Physical Ce line-image tags, zero/small boundary upper or rounded endpoint
proximity are insufficient. No region consumer repairs chosen UV privately.
The producer must apply this whole-use profile before unnecessary same-t
flattening requests; old parameter-wise bounds remain independently inspectable.
Original V/Ce/S node guards remain unchanged and exclude constructed Q.

Any other authoritative use requires the actual finite B/T source-domain producer
and inverse-support consumer. Boundary authoritative_domain_upper alone cannot
create that map. The first implementation returns named unsupported before
facet acceptance; later B/T support must charge E_domain to SAME-L facets once.

## Regular source tiles and cache selection

Do not relabel trace-purpose caches as regular surface caches. Rational originals
use their natural knot spans. Analytic periodic envelopes are partitioned by a
regular tiled-preparation factory. Split a too-wide periodic axis until each tile
width upper is <= the conservative common pi/2 lower and existing regular
admission passes. Choose nearest-even binary64 midpoint as a CUT, verify it is
strictly inside the middle half of the original interval, and retain its exact
stored value. An arbitrary exact midpoint is not falsely claimed representable.
Failure to construct an admitted cut is typed unresolved/unsupported. Tile
source/path identities and shared boundary values are deterministic; overlapping
or duplicate carrier requests are not smuggled through the current factory.
Source poles/degeneracies still require their declared specialized profile.

One SurfaceCacheSession owns source restrictions, numerical tiers and point
values. It yields closed leaf-box candidates before spending outside-domain
point/bound work. Construction returns a complete exact classification horizon:
CLOSED trim-segment box/contact means band; no contact plus interior half-open
winding1 means inside; winding0 means outside. Tiny holes are found by segment
queries even if all four leaf corners are inside. Ambiguity never means outside.
Nonoutside children refine/classify until an accepted gap-free source cover of
the selected domain is retained. Outside leaves need not be meshed or sampled.

One packed filtered exact segment index, built once per boundary generation,
admits source cycles and serves winding/contact queries. Reuse/extract the
existing domain SegmentIndex mechanics, including exact grouped Y prefixes;
do not build a second independent admission checker. Degenerate/coarse selected
loops return original segment locations. A canonical positive source/trace upper
can request physical interval splits under finite limits; missing evidence is
unresolved. Tiny holes never disappear because epsilon exceeds their radius.

## Packet union and protected rims

Enable accepted wholly interior leaves in the existing build_packet_mesh.
Its exact collinear side event streams preserve arbitrary hanging depth and all
side nodes. Alternating occupied/empty sectors monotonically demote incident
packets. Rims are simple disjoint cycles; closed trim contact exclusion ensures
that no rim touches a physical trim. Packet point pinches are not passed to CDT.

After the first accepted cover and demotion closure, freeze the GEOMETRIC packet
union for this boundary generation. A packet descendant remains packet material;
a boundary/demoted descendant remains band material. This avoids oscillating
component topology or silently resurrecting removed pinches. Cache approximation
continues adapting everywhere needed. Cost: later band refinement does not
reclaim those areas into packets during this generation; measure band fraction.

Packet splitting may introduce rim side samples. Both incident owners consume
one canonical atomic schedule. Rebuild incident band components when that schedule
changes, preserving its source parameters and actual XYZ. A new physical boundary
generation reconstructs the initial partition; retained anchor values are then
reclassified against the new graph. No source feature is moved for packet fit.

## Directed band triangulation and components

Normalize source cycles with selected domain on the left. Reverse each packet
rim so BAND is on the left. Keep every collinear protected vertex. The resulting
closed disjoint cycles bound D minus the packet union. A whole-face band and a
packet union containing holes are ordinary cases; no hole bridges are needed.

Before calling CDT, admit all cycles with the shared segment index. For each
positive cycle, winding of ALL OTHER cycles at its original first site must be0;
for each negative cycle it must be1. Own-cycle contribution is subtracted once.
The site lies on its own boundary but, by no-contact admission, not another.
This handles nested alternating rings and multiple components without a
hole-by-outer scan. Interior anchors require total winding1 and no boundary
contact. Segment BVH candidate work is charged; no universal logarithmic query
bound is claimed. This replaces the study's more elaborate new containment sweep.

Use the existing D&C + protected constraint-cavity recovery on ALL sites. The
new typed directed-region entry takes boundary loops plus separately admitted
interior sites. Every boundary is band-left. Recover all constraints, seed every
RIGHT incident triangle as void, and flood only across unprotected edges. The
existing core.cpp::Constructor::run already performs precisely this selection
for one outer; generalize its inventory rather than run a second triangulator.
Require each constraint to have a retained left triangle and void/exterior right
side, every site to appear, and all output chart orientations positive.

Let N=all sites, B=boundary sites, H=negative cycles, C=positive cycles. Euler and
3T=2E_internal+B imply T=2N−B+2H−2C. The old N+2H−2 assertion assumes B=N,C=1.
Keep the old public wrapper behavior; add the typed region inventory and checked
count arithmetic. No input anchor lies on a protected edge or outside winding1.

Dual BFS of retained triangles labels connected band components. Each oriented
loop belongs to the component of its LEFT incident triangle; all its edges must
agree. Thus initial component discovery needs ONE CDT, no prior containment tree
or second metric-bootstrap triangulation. Canonical initial component order is
minimum directed protected edge key; persistent component IDs survive local
rebuilds within the frozen packet footprint. A later local rebuild has that
component's outer/negative cycles and accumulated anchors, not the whole face.

## Fixed full metric and exact bridge

Before initial CDT choose ONE fixed FACE metric from the canonical first accepted
band leaf, otherwise the canonical first covered leaf. Its source-valid midpoint
need not lie inside the trim. From actual cached bilinear corners form midpoint
chart derivatives A/B, dividing by exact original leaf widths. Set
G=[[a,b],[b,c]] with a=A·A,b=A·B,c=B·B. Require a>0 and detG>0 exactly; otherwise
request this cache's precision/refinement and retry, never silently use UV identity.

Choose dyadic positive53-significand factor
T00=sqrt53(a), T01=round53(b/T00), T10=0, T11=sqrt53(detG/a), using existing owned
normalized exponent-safe rounded_sqrt53 arithmetic. Exact detT>0 is checked.
The actual chosen metric is TᵀT; it approximates physical Gram for scaling/shear
quality, not a geometric-error theorem. Freeze T for all component/anchor/rim
rebuilds in this boundary generation. This avoids an elaborate curvature metric
or mandatory old GeometryStore while preserving full off-diagonal predicates.

Transform every exact chart coordinate by SAME T, subtract a common origin and
clear denominators with one positive D for BOTH axes. Generic rational sites use
checked LCM growth; dyadics use the existing fast common exponent. Count work,
bits and bytes before construction. Original coordinates remain semantic truth.
The existing unit weights then implement this full chosen metric. No rounding
to satisfy the integer predicate API, no separate X/Y normalization.

## Actual bounds and construction progress

FacetKey binds FaceConstructionKey, persistent component or packet generator key,
connectivity revision and canonical local facet ordinal. RegionFacet contains
three site keys, exact chart roles and SAME actual binary64 XYZ. Numerical
preparation queries selected leaf boxes and exactly clips each facet, retaining
original-facet barycentrics. Virtual crossings never become mesh points. Complete
positive-piece area equals original facet area; source gaps cannot become PASS.

Use numerical design's e_leaf+B−SAME L plus explicit domain term; contained packet
facets use the same kernel. Ambiguity retries common precision and original-source
same-L tightening before construction decisions. Physical zero-area is separate.
A numerical floor requests shared promotion, not endless geometric splitting.
An original analytic plane uses the numerical direct affine source residual at
the actual three vertices before any leaf clipping; source/domain/envelope
admission still applies. Selected leaves form a disjoint NONOUTSIDE partial
cover, while runtime retains outside branch classification internally. Complete prepared-root closure plus the identical retained domain/classifier
lease establishes the constructive nonoutside partition; intersected pieces
retain SAME-L barycentrics. No per-facet rational covered-area accumulator is
needed to reprove that admitted partition.

After a complete current construction-failure horizon, select by actual generator.
A failed BAND triangle installs a new CENTRAL anchor in the actual parent facet,
independent of its numerical clipping partition. Round the parent UV centroid
nearest-even on grids 2^-k, k=1..anchor_bits; choose the first point whose three
exact positive barycentric numerators are each at least one sixth of the parent
oriented double area. A strictly interior point exists with this margin because
the centroid has barycentrics (1/3,1/3,1/3). The finite cap still gives unresolved.
For a direct star split the three child areas are lambda_i times parent area:
lambda_i in [1/6,2/3], so no child exceeds 2/3 parent area. The later component
CDT may reconnect neighboring cells; this is NOT a global CDT area/diameter
contraction theorem. Retaining the central point prevents the old facet from
reappearing and avoids first-overlap-piece anchors converging toward one corner.

Locate the anchor's source leaf using the retained positive-overlap piece leaf
indices, testing exact closed leaf-box containment in their canonical order.
A point on a leaf interface chooses the first containing leaf. The original-plane
shortcut has no piece list and uses the existing metered first-containing-leaf
scan. Split that chosen leaf once, sample through the SAME PointPool, then rebuild
the affected component with all retained anchors. Do not split every overlap leaf
merely because its old triangle crosses it. Numerical tightening/floor policies
remain the numerical owner's mechanism; no evaluator or new tree is introduced.
Existing active sites cannot lie strictly inside a valid old facet; cached
numerical values may already exist.

A failed PACKET triangle instead chooses a prepared-root-relative dyadic target
strictly inside that triangle. For k=1..64, round both normalized centroid
coordinates to the nearest k-bit dyadic (ties-even), in exact arithmetic, and
choose the FIRST pair proving all three strict original-triangle orientations.
Require each reduced denominator to fit its configured axis-depth cap. The
centroid is inside a positive triangle; if no capped target exists, unresolved.
The target carries prepared RootSlot, exact normalized numerators/depths and the
captured old FacetKey. Refine the SAME selected cache until this point becomes a
NEW ACTIVE leaf corner/common-schedule vertex, then rebuild its packet/rims and
incident band components. It may already have an inactive cached numerical value;
new means construction incidence. Check active inclusion before accepting the
rebuild. An arbitrary one-axis split is insufficient: a skinny triangle may fit
unchanged in one child. No arbitrary private packet anchor or bound-failure
packet demotion changes the frozen footprint. Grid-depth exhaustion is unresolved.

Multiple targets are exact-deduplicated. For each point, locate current selected
leaf branches containing it and bisect coordinates while a non-endpoint target
coordinate remains interior. Split the axis with the greater remaining dyadic
depth, U tie. Continue through the canonical lower-key child whose CLOSED box contains
the target; at an exact split-line tie one such child is sufficient. Stop when
target is its corner, retaining all companion children. The existing side-event
sweep exposes that corner as a hanging sample to every incident coarse side;
forcing both sides to split would add unnecessary balancing work. Union the full
multi-target split DAG before reserving controls/nodes/points/frontier, and retain
captured cover/profile revisions. The runtime plan must enumerate required new
corner keys before mutation. Active corner+common-schedule conformity forbids
reemitting the old triangle. No exact target is rounded into a different path.

Reserve the COMPLETE horizon's cache children, anchor values and connectivity
capacity before installing. Rebuild each affected region ONCE, with all retained
anchors; rebuild every changed packet and shared rim schedule in that same horizon. The old failed triangle cannot reappear while that anchor remains.
This is progress, not a universal termination/angle theorem. Finite depth, exact
bits, work, memory and numerical limits report unresolved/resource failure.
A changed rim schedule rebuilds incident components; a physical source change
uses the complete definition horizon, discards stale topology and reclassifies
anchors while retaining immutable source/numerical caches. No old LEPP fallback.

## Finite immutable API and implementation ownership

Shared `adaptive_trim_types.hpp` defines FaceConstructionKey, ConstructionSiteKey,
FacetKey, RegionFacet and retained result records; no geometry→constructor code
cycle. Construction owns `adaptive_regions.hpp/.cpp` and typed CDT-region bridge;
runtime owns SurfaceCacheSession/FaceTrimSession execution; numerics owns actual
facet clipping/piece preparation/common bounds. Existing packet owner is reused.

RegionInput retains SourceStore, regular SourceCache, SharedCurveSnapshot,
PatchSelection, PacketMesh, FaceDomainLease, source adapters and anchor history.
ConstructionSite has exact UV refs, actualXYZ, stable source sample OR face point
identity/revision, and original occurrence/source ancestry. A single immutable
FaceConstructionResult retains one ExactTable, sites, packet/band facets,
components/loops, protected source/rim edges, anchor history and all source leases.
Original physical orientation remains separate from positive chart-role order.

Surface requests name LeafKeys or exact anchor parameters; results retain source,
query/profile/construction revisions. Bound work names FacetKey and whole facet
content. RebuildRequest names affected component IDs, failed current FacetKeys,
cache splits and anchor locators. Boundary requests carry ONLY original BoundaryKeys.
A stale generation never settles a current horizon; there is no evaluator callback,
shared mutable Budget, per-edit whole-mesh scan or partial successful publication.

## Direct oracle and finite delivery

Extend the SAME independent exact chain/embedding reader to actual full face
outputs. Check positive chart areas, exact selected-domain minus packet chain,
all original/rim atomic edge incidence, no lost collinear sites/holes, undeclared
crossings or hanging nodes, every interior anchor and complete source uses.
Nested alternating rings and multiple disconnected components exercise the new
Euler count and right-flood selection. Whole-facet original closed-form error
checks remain that reader's numerical claim; CPU/P100 parity is metamorphic only.

Before watch: tiny one-leaf hole, many aligned holes, nested packet islands,
multiple source components, arbitrary hanging depth/C0 spans, reversed loops,
nonlinear-speed diagonal authoritative identity, periodic cylindrical seam with
real source tiles, long cross-leaf facet, actual failing facet→new anchor→local
rebuild, and missing curved-auth map refusal. Failure/resource cases target
stale snapshots, source/point revisions and full-horizon capacity. Existing
sanitizers/concurrency checks attack memory/races separately; no second certifier.

Measure source/cache work, trim candidates, packet/band fractions, C/H/B/I/T,
recovered constraints, exact bit growth, clipping pieces, anchor horizons,
region rebuild counts, actual CPU/GPU work/transfers and complete cold/warm cost.
No performance target is a forced triangle count; source IDs or watch geometry
never select the algorithm, tolerance or profile.
