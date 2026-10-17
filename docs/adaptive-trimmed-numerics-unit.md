# Actual trimmed-facet numerical design

Status: DESIGN for review, not an implementation plan or product code. This is
the geometry/numerical dependency of one cached trimmed-face constructor.
Runtime owns the source cache and construction owns real facets/regions. It
extends the completed regular patch/shared-boundary services; it does not call
old FaceEngine, GeometryStore or the old certificate pipeline.

## Sources and selected strength

The supporting read-only study is
[papers/native-adaptive-trimmed-facet-bounds.md](../papers/native-adaptive-trimmed-facet-bounds.md).
Primary sources reread there are Filip/Magedson/Markot1986 §2/Theorems2/4 and the
warning following Theorem5; Balázs/Guthe/Klein2004 §§4–7 (conservative bilinear
plus quarter-twist versus approximate mode); Ma/Hewitt2002 §§3.6/4.1–4.4 (local
flatness, hanging interfaces and boundary completion). Their original PDFs,
URLs and hashes remain in the existing bibliography/acquisition manifests.

Pinned NVIDIA USD/BRep c9c5979f04cea8c38213004d4f04df0dcfb40d05 actual functions:
SmSurfaceCache.cpp::SubdivisionTest around3490–3655, with original-surface sampled
non-B-spline tests and diagonal warning; SmTess.cpp::Phase2CreateFacePolygons
12023–12125, retaining interior cache cells plus remaining polygons. These
underpin cache locality; neither an approximate test nor an old surrogate's
error is automatically the error of our newly connected triangles.

Owned mechanisms read: adaptive_numeric.hpp::bound_actual_facet_rn64;
adaptive_geometry.cpp::bound_actual_facet_precise; adaptive_cache.cpp facet
preparation/fold; cell_bounds.cpp::prepare_actual_cell exact virtual clip and
SAME-L restriction; plane_cell_bounds.cpp; current SourceCache/PatchSelection,
shared occurrence traces and source-domain reports. This design selects:

1. Direct original affine-plane bound, avoiding irrelevant clipping entirely.
2. Cached source-to-actual-bilinear plus a strong actual-facet residual.
3. Indexed exact virtual overlap partition only for real cross-leaf cases.
4. Direct original rational N-WL or original analytic triangle tightening on
   the SAME emitted facet before requesting construction changes.
5. Exact arithmetic only for demanded uncertainty/equality; never per-cell
   rational trigonometry by default.

## Actual claim and first complete scope

For an actual positive chosen-chart triangle K, fixed three actual XYZ words
define affine L. Identity-domain source is S(x); an admitted source-domain map h
makes it S(h^-1(x)). Accept only after a complete current fold establishes

    sup_K ||S(h^-1(x))-L(x)|| <= epsilon_definition.

A virtual clip/restriction does not resample or move L. Original V/Ce↔S(chosenUV)
node delta and original endpoint guards stay separate. No free trace/chord
allowance is added to epsilon. Original source tolerance remains metadata.

First cohesive scope: original-UV regular charts, positive rational p,q<=3 and
admitted analytic plane/cylinder/cone/sphere/torus source regions. Domain policies
are reconstructed-selected and checked authoritative WHOLE-use exact identity.
Curved authoritative/mixed domains require the actual finite B/T producer and
consumer, named unsupported until present. Poles/other singular quotient cells
and physical-plane inverse charts use their specialized map dispatch; ordinary
UV triangles do not silently consume those records.

Local source immersion, nonzero actual XYZ facets, exact chart coverage and
protected incidence are distinct obligations. This positional bound does not
prove global source/mesh embedding or definition-level nonintersection.

## Shared identities and leases

Reuse EXISTING PatchSelection, SelectedLeaf, PointRecord, LeafKey and SelectionKey.
Do not create a competing surface-leaf geometry model. The runtime's proposed
SurfaceCacheSession supplies a retained bound view of that exact selection and
its owner epoch. Its device control bindings remain opaque; a foreign owner or
mutable control pointer is never an input to geometry work.

Construction's common adaptive_trim_types.hpp owns FaceConstructionKey,
FacetKey and RegionFacet. FacetKey includes face/source/query/boundary/chart/
profile identity, component or packet generator identity, connectivity revision
and facet ordinal. An allocation slot is not a facet identity. RegionFacet
retains actual site/physical identities, exact chosen UV and fixed actual XYZ.
Geometry includes only these common values, not constructor implementation.

A sealed TrimFacetWork retains:

* The exact construction result/values and its real FacetKeys.
* The same source/SourceCache and owner-minted PatchSelection bound view.
* Non-null chosen FaceDomainLease, including policy/source/face/boundary/chart
  generations, complete original-use dispositions and any actual map payload.
* Exact positional allowances, request ordinal and numerical-profile generation.

A common work header qualifies the whole table/batch. Per-piece rows carry only
parent facet index, selected leaf index, piece ordinal and local exact-entry
indices. They do not repeat full source owner/table keys for every coefficient.
Cross-owner completion names the retained work key, operation, chunk and stage.
Old/partial/duplicate completions cannot settle current facets.

## Domain and source coverage admission

Reconstructed-selected has E_domain=0 by explicit source policy. Authoritative
identity must prove the ORIGINAL pcurve's complete ordered image equals the
selected chain, including exact endpoints, original t→s pieces and explicit
period/lift. The physical Ce monotone-line tag is not a raw-pcurve face-domain
identity; a small or zero reported error is not its payload.

The finite WHOLE-use straight-image profile uses the geometry service's exact
rational/line original values, checked homogeneous collinearity and monotone
scalar image, plus construction's selected-chain/complete original-use match.
Unknown analytic identities or a source map that reverses/backtracks outside
that profile refuse. A chosen dyadic endpoint is never called equal to a
nondyadic/transcendental original point merely because enclosures overlap.

Current authoritative_domain_upper is a BOUNDARY estimate. It cannot stand in
for an interior h. Future B/T domain work must retain actual joint fans and
ordered strip supports, query inverse T then inverse B on the SAME facet/piece,
and transport their displacement with original carrier derivative bounds on
admitted convex endpoint hulls. Missing payload refuses before numeric work.

Before publishing a bound view, runtime retains one complete nonoverlapping
internal branch/classification partition per prepared source root. Each path/box
derives from its root; each split uses both children of ONE axis, including
children classified outside. PatchSelection contains only its NONOUTSIDE leaves,
not all prepared-root area. The minted view binds those leaves to that admitted
partial cover. Geometry admits the complete requested prepared-root closure for the face
carrier once, including outside-pruned roots, and checks each actual vertex
inside that rectangular source profile. Construction partitions the chosen
material domain by directed CDT and packets; conservative outside pruning
removes no material. Exact clipping therefore partitions each actual facet by
construction, without a second fraction-area coverage certificate.
Source-root C0 interfaces partition bounds.

Full-period analytic faces use one explicit tiled-source preparation extension,
not a trace-purpose lease relabeled as regular. Current SourceCache allows one
request/carrier and binary64 root endpoints, so this is an actual prerequisite:
prepare_trim_source_cache retains one original analytic evaluator envelope and
emits disjoint admitted prepared tiles with source/tile identities and canonical
shared-endpoint ownership. Binary64 cuts are chosen deterministically and checked
strictly inside the middle half; they are not claimed exact mathematical
midpoints unless representable. No representable admitted cut gives typed finite
failure. Construction proposed maxwidth=conservative pi/2 as a deterministic
analytic source-profile choice; this is NOT required by the current error
inequality or implied by the older <2pi guard. Root must select this one profile
value consistently in design integration. Rational natural C0 spans stay intact.

## Numerical dispatch

### Direct original affine source

For an original analytic plane S=O+uX+vY, S-L is affine. Its norm is bounded by
max of the three ORIGINAL vertex residual norms. Test this before any leaf index
or clip. The same source frame arithmetic is used on CPU/P100, with common RN
and outward padding; a rare exact original-plane Rational comparison resolves
uncertainty/equality. This neither reparameterizes a warped rational plane nor
relabels old UV as physical XY. It is checked source algebra, independent of IDs.
Original envelope and selected-domain admission and actual-facet positivity still
apply. This path does not require selected-leaf clipping/area work: construction's
complete admitted selected domain supplies that distinct premise.

### Cached bilinear actual-facet bound

Every retained leaf has actual corner map B and source e_R>=sup_R||S-B||. For a
contained triangle or virtual overlap P, test

    e_R + upper_P||B-L|| <= tau - E_domain(P).

Original numerical corner departure is already paid in e_R. B/L use actual
stored values; do not add physical edge/trace source error again.

Use the current six degree2 triangular Bernstein coefficients of B-L, including
actual virtual L values. Also use the stronger covariance alternative

    ||D||*range_U(P)*range_V(P)/4 + max_i||B(p_i)-L(p_i)||,

where D=B11-B10-B01+B00 in normalized leaf coordinates. B-L equals its affine
nodal residual plus -D*Cov(U,V), and |Cov|<=range_U*range_V/4. Take MIN of the two
upper bounds. The current universal1/4 is a valid fallback. This range-aware
version is particularly useful for skinny clipped pieces. Exact squared
sum-of-two-norm comparison handles equality without a rounded sqrt overshoot.

### Crossing leaves and original spans

The work builder indexes frozen leaf boxes and enumerates closed candidates; outward
floating filters reject only proved disjoint pairs. No all-facets/all-leaves
loop is selected. A fully contained facet uses the existing contained kernel.
For other pairs native exact clipping intersects K with the four leaf halfplanes.
A convex result has at most7 vertices and at most5 positive fan triangles.
Canonicalize/remove exact consecutive duplicates; zero area is discarded only
from bound pieces, never from original source or output geometry.

Every virtual vertex retains ORIGINAL triangle barycentrics beta. Exact
L(p)=sum beta_i*Q_i before outward transfer. Normalize p to the SAME exact leaf
box; non-dyadic ratios are bound coordinates, not mesh sites. Endpoint0/1
shortcuts prevent admission errors from padded division. No virtual crossing is
installed in a source master chain or PointPool. The leaf index is a search aid. Once the complete prepared face-root closure
and captured construction/view are admitted, the conservative partition and
owned convex clipping construct all overlaps. Positive pieces and nonempty
coverage remain required; no per-facet rational area sum or replacement runtime
coverage checker is added. The SAME independent actual-work oriented-chain
oracle directly attacks dropped/duplicated clips. Generic subset-root views
are refused by this complete-face numerical operation.

The whole-leaf four-corner B-L test was considered as an extra cross-leaf
prefilter. The first unit does NOT add it as another batch: it can be loose for
thin regions, and affine source already has a stronger direct path. Reconsider
only if actual candidate/clip counts justify it.

### Same-facet source tightening

If cached e_R+B-L cannot accept, first try direct ORIGINAL source, keeping L:

* Rational original: restrict original positive homogeneous tensor controls to
  the SAME overlap triangle. Total degree d=p+q<=6. Tensor blossom partition
  factors are multinom(p,beta)*multinom(q,alpha-beta)/multinom(d,alpha).
  Form degree d+1 residual N-WL, with positive Wmin and centered source anchor.
  Compare coefficient squared norm against (tau-E_domain)^2*Wmin^2. This can
  bypass loose cached e_R+B-L completely. CPU/P100 do the ordinary interval
  restriction/residual. Demanded original exact extraction/restriction uses the
  SAME geometry primitives lazily, without old GeometryStore or perleaf eager
  exact nets. Owner-resident child controls may be consumed only behind that
  owning cache service; no host download prerequisite is introduced.
* Analytic original: direct triangle interpolation bound is
  (Muu*hu²+2*Muv*hu*hv+Mvv*hv²)/8 plus maximum ORIGINAL virtual-vertex departure
  from SAME L. Taylor expansion at the barycentric mean cancels the linear
  term; coordinate variance/Cauchy bounds give the formula on a convex C2
  support rectangle. It is our explicit general-triangle derivation, not an
  unstated generalization of Filip's stated right-triangle theorem. Shared
  original directional constants and original RN point formulas are reused;
  local harmonic range precision is demanded only when needed. These samples
  are bound-only source enclosures, not new emitted Q.

Do not let interpolation interval dependency grow unnecessarily: convex
Bernstein/de Casteljau combinations and exact endpoint shortcuts are selected.
The recent original degree32 curve failure is the direct reason to preserve
this source-stable numerical form; patch/profile operation changes must be
explicit and identical CPU/P100, not an accidental helper substitution.

## Result and progression semantics

The device returns ordinary numeric status/upper values; only safe upper<=tauDown
passes there. Host folds against the original exact allowance. A bounded
near-threshold/width-uncertainty route invokes the same-task precise comparison.
This heuristic selects work, never proves acceptance. If precise comparison
proves error<=exact tau, retain exact tau as a valid upper instead of re-rounding
it above itself. BASE/precise cache tiers and point revisions remain separate.

After finite cached/direct-source/precision attempts, return one complete stable
set of undecided facet/piece keys. A failed conservative upper is not a source
infeasibility witness. A source-point enclosure with norm lower>tau can supply a
real violation witness; otherwise subdivision/anchor selection remains an
explicit construction policy. Construction consumes the horizon once, refines
implicated cache leaves and installs NEW strict-interior anchors in failed BAND
facets before rebuilding their affected connected regions. Failed PACKET facets
instead select a canonical prepared-root-relative dyadic point strictly inside
the actual triangle within the axis-depth64 cap. Force the SAME cache until the
target becomes an ACTIVE common-schedule grid corner, then rebuild packets and
incident rim components. Existing numerical allocation is reusable; new active
incidence is the obligation. Conforming target incidence prevents the old facet
from recurring unchanged. No admitted target/path gives typed unresolved; bound
failure does not silently demote packets. Numerical failure rows retain their
generator and do not invent arbitrary packet anchors. No unchanged-L cache-only
loop is called mesh progress or reinstates whole-face LEPP.

Numeric floors request the proper authority. Surrogate surface corner precision
uses cache PointKeys; actual boundary Q uses its physical source-edge/vertex
identity. A face cannot privately replace either Q. Original fixed source
vertices cannot be promoted toward S. Map/source limitations identify their real
feature; profile/resource/depth exhaustion is typed unresolved, never forced
minimum-size PASS. No silent fallback to another mesher or unavailable B/T map.

## CPU/P100 and ownership boundary

Geometry owns proposed adaptive_trimmed_geometry.hpp/.cpp, common
adaptive_trimmed_numeric.hpp and CUDA17-compatible adaptive_trimmed_types.h.
Host exact clipping/freezing is one native ownership operation. Common POD bound
work is batched CPU workers or P100 through the existing registered device owner;
there is no geometry callback or another allocation/ticket/drain subsystem.

The following is the concrete numerical seam for the subsequent plan. Namespace
is bgmesh::adaptive; RootDescriptor/OriginalRootSlot/Interval/NumericStatus remain
the existing CUDA17 POD types. Runtime's retained SourceCache qualifies original
root slots. Base piece tasks reuse ActualFacetInput/ActualFacetBounds with the
range-aware alternative in their common function; no second leaf/facet model.

```
struct SourceTriangleInput {
  OriginalRootSlot original;
  Interval normalized_uv[6]; // 3 vertices normalized to ORIGINAL root box
  Interval affine_xyz[9];   // SAME emitted L, exactly restricted before packing
  double allowance_down;   // exact remaining allowance rounded downward
};
struct SourceTriangleBounds {
  NumericStatus status;
  double position_upper;
};

bound_source_triangle_rn64(original_descriptor, original_control_view, input)
  -> SourceTriangleBounds
bound_original_plane_rn64(original_descriptor, input) -> SourceTriangleBounds
```

These are BOUND-ONLY operations. They cannot enter source sample factories or
pretend a derived trace is an original source family. Normalized parameters are
formed exactly from the retained ORIGINAL root box, not a prepared tile or an
unrelated selected leaf. Analytic source formulas recover original parameters
from that original box; rational source controls are the ORIGINAL root controls.
Exact positivity, source membership and remaining allowance stay in retained
native work. Overlapping binary64 parameter enclosures give needs_wider, never
an invalid-source verdict for a valid exact triangle.

SourceTriangleBounds reports only the bound it computes. Coefficient interval
width is arithmetic uncertainty, not a witnessed geometric error or an actual
point-departure floor. Native fold may target precision before geometry; an
actual facet-corner promotion must name its separate construction authority.
Bounds use the canonical current arithmetic tier, irrespective of cached tighter
tiers; the source/session operation header binds stage/profile and exact inputs.

Host preparation and demanded precise comparison use this conceptual interface:

```
prepare_trim_facet_work(JobContext&, SurfaceBoundView,
                       shared<const FaceConstructionResult>, FaceDomainLease,
                       ExactFacetAllowances) -> TrimFacetWork
compare_source_triangle_precise(JobContext&, TrimFacetWork, PieceSlot)
  -> {SourceTriangleBounds, certified}
```

TrimFacetWork retains the common keyed header, source/selection/construction/
domain leases, exact facet allowances and exact local coordinate tables. Flat
piece rows name parent facet index, selected leaf index and original root slot,
plus exact uv6 and SAME-L9 local entry indices; GPU uses batch-local ordinals.
Whole-plane tasks can name the parent facet directly with no leaf/piece rows.
The plan must select literal native wrapper signatures without changing these
ownership/input meanings. Certified precise success may retain the exact
remaining allowance as its upper. A conservative failure cannot set certified.

Rational p/q degree buckets have at most28 restricted homogeneous triangle
coefficients and36 residual coefficients; lane+32 handles the final four. One
warp cooperatively stages the source slab and coefficients; no private large
net per thread. Analytic tasks use shared original constants. Runtime/root owns
the existing DeviceOwner/module operation extension; geometry owns these common
POD meanings and numerical functions, not a second CUDA lifetime implementation.

Native exact pieces use one sealed ExactTable and flat entry indices qualified
by their retained work. Transient clipping Rational values belong only to their
JobContext Budget. Transfer retains all source/selection/construction/domain
leases; no mutable Budget escapes. Runtime mutation/reclamation waits for the
complete current bound horizon. Restricted device handles are validated by owner
and generation before dispatch; scalar row indices alone grant no authority.

All capacities/work are bounded before publication: candidate visits, clip
vertices, positive pieces, exact limbs, batch records, source restrictions,
precision tasks and output requests. Overflow or allocation failure publishes no
partial accepted facet set. Cancellation and failed device drain follow the
existing retained owner semantics. Counters describe actual operations/transfers,
not advertised configured capacities.

## Direct oracle and cohesive delivery requirement

The next implementation PLAN must specify real direct RED and actual operation
integration, not merely compile these declarations. The cohesive result is real
band CDT facets through this dispatcher, crossing multiple source/cache boxes,
with a real strict-interior anchor/rebuild horizon when needed. Missing domain
maps/refused source charts are visible outcomes, not removed inputs.

Extend SAME independent original-source reader plus exact region coverage oracle:
nonzero saddle e_R=0, hanging/skinny arbitrary displaced-L pieces, quarter-twist
threshold; original POWER/rational positive-weight spans and C0 slopes with
nondyadic virtual crossings; nonorthogonal analytic regular families; affine
plane direct path and separately unsupported physical-plane map; stale/partial
coverage and domain-policy attacks. Original Fraction polynomial restrictions
must be independent of production restricted nets. Analytic closed formulas give
whole-facet bounds; finite probes only falsify. CPU/P100 semantic equality is
additional evidence, not the source oracle.

Measure candidate visits, clips/pieces, direct source tasks, actual numerical
promotions and precision, cache/anchor requests, reused work and transfers on the
same selected fixed families. Long slivers/thin bands/many holes may expose high
piece count or mostly-band construction. Retain failures/censors; do not hide
quadratic traversal, repeated extraction or old exact trig under correctness.

### Reviewed implementation closure: constructive coverage and failure placement

The complete prepared-root closure is the CURRENT query root set, including
outside-pruned roots, not merely roots present among retained leaves. A complete
face request must include every prepared panel for its original carrier.
Candidate classification carries the SAME FaceDomainClassifier lease through
all refinement and frozen-footprint overlays; a generic arbitrary-mask view
cannot be used for complete-face numerical work. Construction and numerical
work retain that exact domain/view/selection identity. Each carrier cache is
compiled from one rectangular request, so exact vertex admission also contains
the complete positive affine facet in its source rectangle. This establishes
clipping coverage constructively; the external actual-work chain oracle attacks
implementation omissions. A per-facet fraction-area sum is not required.

A failed BAND triangle's geometry progress is independent of the overlap
partition: construction rounds the actual parent centroid to a dyadic whose
three barycentrics are each at least1/6, locates one containing captured leaf,
samples through the existing source PointPool and retains that anchor in its
component rebuild. First-overlap-piece centroids can approach a corner as the
numeric cache shrinks and are not selected. Direct-star area<=2/3 is a local
statement, not a global CDT contraction theorem. Packet active-corner targets
retain their separate declared procedure.

Direct original-root-contained SAME-L source bounds before leaf clipping remain
a strong scheduling alternative: the existing interval CPU/P100 kernel can
accept without virtual fraction geometry, while original-span partitions are
mandatory only across source interfaces. This alternative is deferred during
the bounded actual progress correction; it is not secretly implemented or
claimed in current performance results.
