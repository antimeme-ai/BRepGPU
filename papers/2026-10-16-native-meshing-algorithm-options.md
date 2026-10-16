# Native face domains: algorithm and representation options

Research for the operator-authorized meshing surgery and C++ transition. This
lane changed no product code, installed no dependency, and ran no CAD or fleet
benchmark. Recommendations below are engineering hypotheses to discriminate
with actual work counts and wall times, not forecasts of speedup.

## Recommendation

Start with an immutable native face-domain owner, **exact coordinate ranks for
broadphase, packed segment/point BVHs, and an exact integer predicate view**.
Retain the existing ear cursor and original source/occurrence identities. This
removes disjoint candidate generation before spending effort making predicates
faster. Keep a small-input direct path: a tree is not free.

The existing diagnosis caught 8,300 sampled nodes on one four-coedge spring
face. Its boundary scan can enumerate 34.4 million pairs; ear clipping can
visit 68.9 million slots for successful ears alone. Those are code-derived work
counts, not measured new results. The principal opportunity is **less work**,
then cheaper work, then concurrency. A C++ translation of the same searches
would preserve the scalability defect.

The retained studies read first were the import/mesh study, connectivity reading,
mesher diagnosis, C++ numerical seams and transition proposal. Primary sources
read for this report are listed below. No reference implementation is selected
as a production dependency.

## Ranked choices and experiments

| Rank | Mechanism | Expected useful regime | Failure/performance risk | Discriminating experiment |
| --- | --- | --- | --- | --- |
| 1 | Exact ranks + packed BVH candidate pruning | Sparse boundary boxes; local ear boxes; arbitrary coordinate scales | Overlapping boxes remain quadratic; build/order costs can dominate small faces | Same captured face: normalization/build/query/predicate walls and counts, complete domain oracle, original first-crossing tuple |
| 2 | Positive per-axis integer view, owned minimal arbitrary integers | Repeated predicates on fixed rational UVs | LCM/limb growth, allocation and multiplication cost | Numerator/denominator/normalized-coordinate bit histograms; total import + predicates, not predicate-only timing |
| 3 | Active subtree counts and cached unchanged-ear status | Late ear stage full of dead occurrences or repeated unchanged triples | Additional updates; invalidation mistakes around duplicate bridge occurrences | Count stale/dead candidates and repeated triples first; compare unchanged triangle sequence |
| 4 | Indirect plane predicates or interval-filtered rational predicates | Native exact fallback remains measurable after pruning | Invalid filter if construction error omitted; higher predicate degree | Compare exact signs/orders to independent Fraction model on literal and captured oblique planes; filter-hit and exact-fallback counts |
| 5 | Maintained reflex/ear lists | Large simple polygons with substantial repeated ear tests | Weakly-simple bridged rings and collinear samples need a precise extension | Simple-ring and many-hole distributions separately; output-domain/source oracle, changed diagonals allowed explicitly |
| 6 | Direct constrained triangulation with compact triangle neighbors | Large domains where initial ears and Lawson still dominate | New constraint recovery, duplicate chart occurrence and deterministic degeneracy machinery | Whole connectivity wall + triangle count + refinement cost, not unconstrained point triangulation throughput |
| 7 | Parallel finalized faces/definitions | Many independent faces after shared schedules stabilize | Memory amplification, stragglers, output identity races | One-core vs bounded worker scaling with complete load/merge/output and RSS, identical source/quality |

Ranks are an implementation order for the present failure, not a permanent
ranking across CAD workloads. One captured spring must not become the only
performance workload: include a reference-successful part, the many-hole date
disc, and small ordinary faces.

## Exact ranks make broadphase integer-only

For fixed coordinates, exact-sort unique X values and unique Y values once.
Assign their dense ranks to each occurrence. Equal exact coordinates get equal
ranks; equal ranks never merge occurrence identities. For any two input
coordinates, `x <= y` iff `rank(x) <= rank(y)`. Therefore closed segment AABB
overlap and point-in-ear-AABB queries use exact integer comparisons without
rounding bounds or performing rational comparisons in each query.

This is our proposed specialization, not a claimed literature implementation.
It preserves order only: ranks are **not** coordinates for orientation, distance,
area, metric or incircle calculations. All ear corners and bridge endpoints
already belong to the fixed point set. Midpoints used by winding do not acquire
an interpolated rank; use their exact integer/rational expression instead.

Build a compact median-partitioned binary tree over closed rank boxes, with
array child slots and leaf ranges. Avoid pointer allocation per node. An initial
balanced split by rank-box distribution needs no floating SAH or sophisticated
builder. Do not assert logarithmic query bounds: a tree can visit every leaf
when boxes overlap. Record node and candidate visits to expose that case.

For boundary simplicity, process original segment `i` in original order. Query
its closed box, discard `j <= i` and the existing same-loop adjacency exclusions,
sort surviving `j`, and invoke the unchanged exact touching predicate. Stop at
the first true pair. This preserves the externally meaningful lexicographic
first crossing used to choose a circle joint without materializing every
candidate pair for the whole face.

For ears, query a separate immutable point index. Keep occurrence alive bits,
corner-slot exclusions, coordinate-equality exclusions and inclusive exact
containment. The current inner loop exposes only a blocker boolean; it does not
return its slot. Thus ear candidate order need not be sorted merely to preserve
the chosen ear and triangles. Preserve any claimed diagnostics deliberately.
There is one valuable bounded fast path: after simplicity has passed, a no-hole
ring with every cyclic turn strictly positive is strictly convex. Every cursor
ear is empty; retain the same linked removal/final order and skip point-index
construction and blocker queries. A large fan can otherwise have bounding boxes
containing many points even though no point lies in its triangle. Collinear
boundary samples and bridged holes remain on the generic path. All nonnegative
turns are insufficient for this shortcut: a triangular outline with an extra
point on the proposed ear diagonal provides a boundary blocker.
If dead entries dominate, maintain live subtree counts along the removed leaf's
parent chain; bounds remain immutable while zero-live subtrees become skippable.
For optional ear memoization, unchanged triangle geometry does not make a
negative answer permanent: a nonneighbor blocker can be removed. Cache its
occurrence as a witness and reuse the rejection only while that occurrence is
alive and the ear triple unchanged. An unchanged positive answer remains valid
as the active point set shrinks. Reset either cache when the triple changes.

CGAL's box-intersection manual explicitly distinguishes closed boxes, including
zero-width boxes and boundary contact, from half-open boxes. Those distinctions
match our touching predicate. Its streamed segment-tree alternative has an
output-sensitive bound, but its own timings exclude callback work and its cutoff
depends on distribution and hardware. Study that algorithm if the simple BVH
actually loses on overlap-heavy workloads; do not transfer its bound to our BVH.
[CGAL 6.2.1 manual](https://doc.cgal.org/6.2.1/Box_intersection_d/index.html).

### Why not a sweep or uniform hash first?

A sweep over X intervals is compact, but many long nearly-parallel intervals can
leave a large active set although few 2D boxes overlap. A proper geometric
segment sweep also adds exact event ordering, coincident/touching/vertical
segments, and first-crossing-order recovery. That is a larger correctness
surface for the immediate goal. A uniform grid risks clustered points, long
segments spanning many cells and scale-sensitive bucket choice. A BVH requires
neither arbitrary geometric snapping nor a cell size.

Morton order over **exact ranks** is a credible compact point-query alternative.
Quantization may yield extra candidates; it must never become the exact
predicate. Mapbox earcut uses Morton-assisted ear tests and pooled node storage,
but also removes collinear/coincident nodes and attempts local repairs. Those
semantic choices conflict with preserving every source sample/occurrence and
rejecting unsupported crossed boundaries. Learn the indexing/layout idea;
do not transplant the repair policy or its output as a correctness oracle.
Read source at commit
[`177bd66b5338ab66f4e9ce8f3cca418b9463d809`](https://github.com/mapbox/earcut.hpp/blob/177bd66b5338ab66f4e9ce8f3cca418b9463d809/include/mapbox/earcut.hpp),
especially `isEarHashed`, `ObjectPool`, `filterPoints`, and `cureLocalIntersections`.

## Exact arithmetic without a general rational library

For fixed face points `(n_x/d_x,n_y/d_y)`, choose positive common denominators
`D_x,D_y` and store integer `(X,Y)=(n_x D_x/d_x,n_y D_y/d_y)`.
Coordinate comparisons are unchanged and orientation is multiplied by the
positive `D_x D_y`. Existing Lawson code already uses this transformation.
Use the original exact UVs for output/source/error accounting; the integer view
is an acceleration representation, not replacement source geometry.

This permits a deliberately small owned native arithmetic layer: signed limb
comparison, addition, subtraction and multiplication, with checked storage and
size arithmetic. Predicate evaluation needs neither rational reduction nor
general integer division. Producer-side normalization can initially use the
existing Fraction machinery, but its cost, bytes and ownership remain part of
the complete operation. A later native plane producer can exploit its shared
denominator rather than rediscovering an LCM from every point.

Fixed 64/128-bit arithmetic is a valid fast path only with checked overflow and
an arbitrary-length fallback. The fallback cannot become a silent input-domain
limit. Likewise, a huge normalized denominator can make an all-integer view
unattractive for heterogeneous rational points. Measure the actual bit widths
and retain a design escape to per-point homogeneous values if needed. General
homogeneous rationals avoid a global LCM but increase cross-products/predicate
degree. Fixed watch-plane coordinates derived from one inverse are a much more
structured family than arbitrary unrelated rational constructions.

For that specific producer there is a useful stronger observation. Scale every
finite binary64 frame/XYZ value by one common power of two to integers. Write
`M=A/S` and `P-O=b/S`; then UV is `adj(A)*b/det(A)`. Every reduced coordinate
denominator divides the same `abs(det(A))`, so adding more sampled points on
that unchanged plane cannot introduce unrelated odd denominator factors. This
does not bound the generic API's rational inputs, nor imply 128 bits suffice.
It suggests measuring inverse construction separately from normalization:
`_plane_uv` currently recomputes exact frame covectors per point even though
the frame is unchanged. A cached exact inverse or direct cofactor-numerator
producer is a later complete slice, outside the first domain-engine edit.

Anisotropic denominator scaling preserves orientation but not Euclidean circles.
The existing Lawson metric weights compensate explicitly. Do not feed integer
XY into an ordinary unweighted incircle routine or distance ranking.

### Filters need construction error, not just determinant error

The ordinary binary64 orientation error bound applies to its **saved binary64
arguments**. Rounding exact rational UVs to double first changes those arguments.
Calling an exact dyadic predicate afterward does not recover the original sign.
If adding a filter to integer/rational points, enclose the original exact
coordinates (e.g. explicit outward intervals) and propagate those intervals
through the determinant; accept only an interval strictly on one side of zero,
otherwise use exact integers. Include huge exponents/subnormals and overflow
fallback in its explicit contract.

Shewchuk's adaptive method is the primary model for saved floating inputs and
successive sign refinement. Its assumptions do not establish correctness for
rounded intermediate constructions. [Author discussion](https://www.cs.cmu.edu/~quake/robust.html)
and [Triangle section 6](https://www.cs.cmu.edu/~quake/tripaper/triangle6.html).

### Stronger later specialization: predicates through the original plane

The actual plane inverse is `M^-1(P-O)` for saved `M=[X Y Z]`; it is not merely
a dot product with an assumed orthonormal frame. For original sampled XYZ
points P,Q,R, exact linear algebra gives

`orient_uv(P,Q,R) = Z · ((Q-P) × (R-P)) / det(M)`.

Also, `u(Q)-u(P) = (Y×Z)·(Q-P)/det(M)` and similarly for V with `Z×X`.
These identities remain valid for oblique/indirect frames and off-plane points,
provided `det(M) != 0`. They follow from the cross product of the first two
rows of `M^-1`, which equals `Z/det(M)`. They are our derivation for this actual
producer, not a benchmarked replacement.

All polynomial arguments are original saved dyadic values. An owned filtered
indirect predicate could avoid explicitly constructing rational UVs for signs
and ordering. Include determinant sign; a direct XYZ projected orient2d is not
equivalent. Midpoints can be represented as symbolic positive-denominator sums.
UVs produced by a different recovery/construction require their own expression
or the generic integer path. The generic sampled-domain API should not infer
XYZ provenance from coordinates that merely look planar.

Attene's indirect-predicate work describes substituting construction expressions
into a predicate, filtering the resulting numerator/denominator, and falling
back to exact polynomial evaluation. Its cached floating/interval expressions
are especially relevant to repeated use of one face frame. It also reports that
caching exact expansions can cost more than recomputing them. That motivates a
measured specialization rather than universal expression graphs or permanent
per-point expansion caches. [arXiv 2105.09772v2, sections 4–5](https://arxiv.org/html/2105.09772v2).

## Change triangulation only when the remaining cost warrants it

A deferred specialization may be stronger than merely pruning dead leaves:
**simple, hole-free weakly convex rings**, with all exact cyclic turns
nonnegative, still contain many protected collinear samples. For a strict corner
ear, another boundary point can lie in its closed triangle only on its proposed
diagonal. Convexity then forces the rest of the ring onto that supporting line:
the current ring has exactly three strict corners, both immediate neighbors of
the ear tip are strict, and more than three occurrences remain. That is the
triangle-with-flat-opposite-edge blocker identified above. Otherwise a strict
corner has no blocker. Maintain exact strict-corner flags/count after each
removal and preserve the current cursor, skipping this exceptional blocked ear.

This is a proposed proof direction for a separate bounded unit, not implemented
or performance-qualified here. It could eliminate point queries entirely for
exact rectangular charts while preserving every source sample; the first unit's
strict-only shortcut deliberately does less. Exact tiny zigzags invalidate this
admission, even if a chart looks rectangular on screen. Measure actual turn
signs before selecting it, and never snap coordinates to manufacture convexity.
Bridge slits/holes remain outside this argument. A full review must check the
supporting-line argument and total cursor work before advertising linear time.

Eberly's simple-polygon method maintains reflex and ear lists and updates the
neighbors of a removed ear, achieving quadratic rather than straightforward
cubic work. It uses array-backed links. Our slit representation after hole
bridging has duplicate occurrences and collinear samples; applying a reflex-only
blocker rule needs its own precise extension, especially for inclusive boundary
containment. The current all-active-point BVH rule is a smaller semantic change.
[Eberly, sections 2–5, revised 2024-08-03](https://www.geometrictools.com/Documentation/TriangulationByEarClipping.pdf).

Hole visibility can reuse segment boxes for existing boundaries and append the
new slit edges after each bridge. Keep original candidate order until deliberately
changing selection. The current candidate distance is rounded `sqrt` of a
Fraction-valued sum; exact squared-distance ordering can change ties. Neither
ordering alone proves visibility. Preserve segment endpoint exemptions and the
original-domain midpoint winding check. A native implementation must not
quietly advertise identical triangles while changing bridge tie semantics.

If initial connectivity plus Lawson still dominates after pruning, study an
owned constrained triangulator using compact triangle/neighbor arrays and
explicit constrained-edge/source-occurrence metadata. Triangle's same-author,
same-data-structure comparison favored divide-and-conquer over incremental and
sweep methods, and triangle records improved its locality over quad-edge records.
Those historical results suggest hypotheses, not modern CAD speed ratios.
Its code deduplicates input points; our seam/slit occurrences cannot be erased
by copying that policy. [Triangle section 2](https://www.cs.cmu.edu/~quake/tripaper/triangle2.html).

## Ownership, layout and parallelism

Keep hot rank boxes, occurrence slots, alive/link arrays and triangle indices
separate from cold source maps and arbitrary-length limb storage. A compact
node AoS can be sensible for traversal; SoA is useful only if actual queries
benefit. Do not turn a layout preference into a full refactor. Per-worker arena
scratch and reusable candidate vectors avoid per-predicate allocations and
bound lifetime to the face owner. Checked dense slots are distinct from original
source IDs, physical keys and occurrence IDs.

Shared circle schedules must stabilize globally before independent face work.
Then schedule larger estimated faces first against immutable source/schedules,
use worker-owned scratch and bounded memory, and merge in canonical source
order. Limit work by resident bytes as well as worker count; one pathological
face can dominate both runtime and allocation. A GPU port of irregular exact
ear/predicate control is not the first answer: the existing GPU geometry
consumer already needs stable native buffers, and CPU candidate elimination is
the measured defect. GPU broadphase can be evaluated later with total transfer,
fallback and publication costs visible.

## After connectivity: is refinement doing useful geometric work?

Another measured experiment should distinguish slow construction from excessive
output. Current acceptance uses Hessian/chord error, while split selection uses
first-derivative physical edge length. These are different objectives. For a
cylinder, the interpolation bound depends on U width and has zero V curvature;
a very long V extent can nevertheless dominate the edge-length priority.
Predicting child error bounds or using a curvature-weighted edge score could
prefer splits that reduce the actual failing term. This is a counterexample
direction, not a demonstrated OM10 cost share: shared boundary sampling and the
preceding Lawson pass can already eliminate that situation on ordinary strips.

Measure accepted/rejected triangle counts, created nodes, queue/stale storage,
failing bound terms and maximum-child-bound reduction per selected split before
choosing an upgrade. Compare unchanged source-derived boundary samples and final
world-space quality. Do not make a faster algorithm look better by relaxing its
error bound. A later local Bézier/subdivision bound may reduce conservatism,
but it must bound the actual carrier-to-triangle error rather than merely
report smaller sampled residuals. The first fixed-domain native unit deliberately
does not implement any of these refinement changes.

## Performance experiments that can reject these recommendations

Capture the actual fixed boundary input once; retain private source IDs/bytes.
For preservation changes, compare exact first-crossing tuple and triangle
sequence to the current implementation on inputs small enough to finish. Use
the independent source-domain/positive-boundary-chain checks for the large
actual output; agreement with old code alone cannot establish valid geometry.

Measure release builds on the fleet and include input decode/normalization,
rank sorting, index build, queries, exact predicates, hole bridging, connectivity
improvement and result packing. Count visited nodes, returned box candidates,
actual orientation calls, exact fallbacks, rejected ears and maximum limb width.
Counters should explain work, not spawn a new permanent qualification system.

Run a bounded size ladder over long strips, concave combs, dense clusters,
near-collinear rational inputs and many holes, then the captured spring/date
disc/reference-successful part. An overlap-heavy case is necessary: an index
must expose its worst-case behavior. Preserve the unchanged tolerance, source
residual and sample caps. Report a timeout as censored runtime, never as a
completed baseline. Compare one CPU core first, then bounded workers separately.
Fresh whole-watch qualification comes after the isolated changed mechanism
has demonstrated correctness and useful measured cost reduction.

## Acquisition and restoration

Existing Eberly and Shewchuk originals/hashes are in the import/mesh study.
The newly acquired original author paper is ignored
`papers/pdfs/attene-indirect-predicates-2105.09772v2.pdf`, 556,249 bytes,
SHA-256 `8504bb1e1dc029afbec1ae54374008a0b3e47cd3b888c6a3408a6528b721942e`.
Restore from `https://arxiv.org/pdf/2105.09772v2`, preserving original bytes;
compare that hash before relying on the retained revision. Author Marco Attene;
arXiv v2 dated 2025-01-23, CC BY-NC-ND 4.0 as declared by that revision.
The publication is *Computer-Aided Design*126 (2020),102856,
DOI `10.1016/j.cad.2020.102856`; this reading uses the declared arXiv v2 text.
Other linked primary texts were read online. No extracted third-party code or
library is added to the product by this study.
