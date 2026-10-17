# Original-source bounds for actual trimmed-region facets

2026-10-16. Read-only numerical dependency proposal for the cached adaptive
mesher. No product code, geometry run, build or benchmark is part of this study.
This extends the implemented regular patch and shared-curve services into the
construction owner's connected boundary regions; it does not introduce another
mesher, evaluator or output certifier. The construction proposal is
[native-adaptive-boundary-regions.md](native-adaptive-boundary-regions.md).

## Primary sources and actual implementations read

* Filip, Magedson and Markot, *Surface algorithms using bounds on derivatives*,
  CAGD3 (1986), 295–311, DOI10.1016/0167-8396(86)90005-1. Reread §2,
  Theorems2/4, §3 interpolation applications and the warning following Theorem5:
  independent planar approximants may crack, and reconnecting them does not
  inherit their old error bound. The vector curve theorem supplies the 1/8
  interpolation constant; the stated surface theorem is for right triangles.
  Our arbitrary-triangle bound below is separately derived, not attributed to
  an unstated general theorem. Intact PDF and SHA are in
  whole-mesher-geometry-contract-sources.tsv.
* Balázs, Guthe and Klein, *Efficient Trimmed NURBS Tessellation*, WSCG12 (2004),
  27–33. Reread §§4–7: span-local trim composition, binary surface subdivision,
  conservative bilinear-plus-quarter-twist bound versus an explicitly
  approximate alternative, and handling a hole contained in one mesh cell.
  The approximate rendering mode, near-equal degree reduction, arbitrary fixed
  budget split and trim-loop repair are not adopted. The bilinear-plus-twist
  structure is useful, but our positive rational residual keeps original W
  explicitly rather than treating rational control coordinates as polynomial.
* Ma and Hewitt, *Adaptive Tessellation for Trimmed NURBS Surface*, Eurographics
  Short Presentations (2002), §§3.6/4.1–4.4. Reread the local/global derivative
  comparison, hanging-interface cracks and closing inner/outer boundaries.
  Its patch/trim decomposition supports the architecture; its intersection
  samples/scanline details are not permissions to create private source-edge
  points. Intact paper, URL and SHA are in native-adaptive-subdivision/.
* Pinned NVIDIA USD/BRep c9c5979f04cea8c38213004d4f04df0dcfb40d05:
  `source/SMLib/src/SmSurfaceCache.cpp`, SubdivisionTest around3490–3655,
  including comments around3550–3568: non-B-spline source tests may sample the
  original surface rather than use a complete control-net bound. Around3615,
  a diagonal check is explicitly needed after directional chord checks pass.
  `SmTess.cpp::Phase2CreateFacePolygons`,12023–12125, reuses its cached tree,
  inserts trim classifications, emits inside quads and triangulates remaining
  polygons. This is not an actual-SAME-L certificate for our arbitrary band
  facets. We harvest localization/reuse, not its sampled or minimum-size success.
  Restoration and source hashes remain in the existing quarantine manifest.

Owned source reread: adaptive_numeric.hpp::bound_actual_facet_rn64;
adaptive_geometry.cpp::bound_actual_facet_precise; adaptive_cache.cpp::build/fold
facet stages; cell_bounds.cpp::prepare_actual_cell and its exact rectangle
clipper/barycentric interpolation; plane_cell_bounds.cpp; current
SourceCache/PatchSelection and adaptive_curves/adaptive_traces result contracts.
The old cell clipper has the useful exact SAME-L mechanism but depends on eager
GeometryStore/BoundRootBank. Reuse its mathematics in the new source/cache owner;
do not invoke that old preparation as a prerequisite.

## Exact claim and admitted source scope

An actual selected-chart facet K has three exact chart coordinates and THREE
stored binary64 XYZ values. They define one affine map L on all of K. A regular
original-UV chart uses original S on the selected domain. A named nonidentity
source-domain map h instead makes the reference source point S(h^-1(x)). For
all x in K require

    ||S(h^-1(x)) - L(x)|| <= epsilon_definition.

This is a positional source realization bound, not global injectivity, feature
separation, watertight incidence or physical nonintersection. Exact nonzero
physical facets, protected source-key incidence and definition contact handling
remain separate constructor obligations. Source V/Ce↔S(chosenUV) nodal delta
stays separate; it is not an additional positional budget. Source tolerance is
metadata.

First finite facet dispatch: original-UV regular charts with positive-weight
rational p,q<=3 originals and admitted analytic regular source rectangles.
Original C0 boundaries partition bounds; no C2 theorem crosses them. Every
positive-area part of K must be covered by retained original-root/cache leaves.
A partial cache overlap is not a partial PASS. Full analytic-period faces may
need several admitted regular analytic source rectangles; a trace-purpose cache
must not be relabeled as a regular patch cache. Singular quotient cells and
physical-plane inverse-chart cells require their declared specialized dispatch,
not ordinary UV triangles or a new inferred atlas.

## Domain transport is a real input

Current authoritative_domain_upper bounds a BOUNDARY pcurve-to-selected-trace
error transported through the carrier Jacobian. It is not an interior map and
cannot be added as a maximum edge error to certify a whole original face.

Use the construction owner's selected finite policies:

* Reconstructed-selected: the chosen PL domain is the named reference domain;
  E_domain=0. Preserve raw guides, source maps and original node guards.
* Checked authoritative identity: the complete original trim image, including
  endpoints and source traversal/lift, is proved to be exactly the selected
  polygonal image. A generic small chord error is not this identity.
* Curved authoritative/mixed: require actual finite B joint fans and tapered T
  strip maps, original source descriptors, inverse-support index and generation
  lease. For P subset K query inverse T then inverse B, compose displacement,
  and bound ||S(h^-1(x))-S(x)|| on the relevant convex endpoint hull through
  indexed original source derivatives. Reference/final points must stay in the
  original natural domain; ambient support polygons alone need not lie there.

This reuses the accepted map theory with NEW common curve/source services, not
old eager GeometryStore. Without an implemented admitted map, return named
unsupported rather than manufacture identity. For each positive overlap P the
numerical task receives an original physical E_domain(P), or a complete K upper
if only that conservative indexed query exists. Fold maxima of piece TOTALS;
do not pay domain displacement twice or pay it from nodal delta.

## Selected fast path and exact virtual partition

1. Build/retain a packed leaf-box index for the selected surface generation.
   Closed bounding boxes enumerate all possible triangle/leaf pairs. Cached
   floating boxes only reject when an outward comparison proves disjointness;
   exact chart coordinates decide touch/overlap ambiguity. This is not all
   triangles times all leaves.
2. If all three exact vertices belong to one leaf, use the current contained
   actual-facet kernel directly. Ordinary interior packets already do this.
3. A possible cheap band-facet prefilter can test the SAME L on a whole leaf:
   B-L is bilinear there, so max of four corner residual norms bounds the whole
   leaf. This can accept flat/coherent cases without exact clipping. A failed
   whole-leaf test proves nothing about the triangle; thin band facets may
   routinely need the next step. Do not let this prefilter choose topology.
4. For remaining candidate pairs clip K by the four exact leaf halfplanes.
   Retain ORIGINAL-triangle barycentrics at every crossing; a triangle/rectangle
   intersection has at most7 vertices. Remove exact consecutive duplicates,
   discard zero area only as bound pieces, and fan each positive convex polygon
   canonically into at most5 triangles. No virtual point becomes a physical or
   source-edge sample. Do not change/re-evaluate any emitted Q.
5. At a virtual point p with barycentrics beta, use
   L(p)=sum(beta_i*Q_i), exactly before outward transfer. Normalize p to that
   SAME leaf using its exact original box. Ratios may be nondyadic; they are
   bound data, not new topology sites. Exact endpoint0/1 shortcuts avoid falsely
   refusing a real source endpoint after outward division.
6. Positive pieces partition K. Check exact oriented-area equality once in the
   preparation fold (and source coverage admission), not producer booleans. A
   missing original span, overlapping selected leaf interiors, generation
   mismatch or resource cap returns typed failure with no accepted facet.

The clipping adapter is native CPU ownership/topology work initially; the
bounded coefficient work is batched CPU/P100. Avoid implementing a floating GPU
clipper and a different exact CPU geometry at once. Filtered classification and
an interior single-leaf path keep exact clipping local to real boundary work.
Its actual candidate/clip/piece counts must be measured before enlarging scope.

## One common low-degree leaf-to-facet bound

For leaf rectangle R retain actual four-corner bilinear B and e_R>=sup_R||S-B||.
For every positive overlap triangle P, actual-facet bound is

    e_R + upper_P||B-L|| + E_domain(P).

Corner numerical departures are already inside e_R. B and L use actual stored
XYZ; do not add the curve source-to-Q error or trace upper again here.

Write normalized B=c+x*u+y*v+D*u*v. For P vertices p_i=(u_i,v_i), define
r_i=B(p_i)-L(p_i). The six quadratic triangular Bernstein coefficients are

    C_ii = r_i,
    C_ij = c + x*(u_i+u_j)/2 + y*(v_i+v_j)/2
                 + D*(u_i*v_j+u_j*v_i)/2 - (L_i+L_j)/2.

Their maximum norm is a whole-piece upper. The existing common HD kernel
already implements this SAME-L coefficient construction and a second bound:

    upper||B-L|| <= ||D||/4 + max_i||r_i||.

For arbitrary contained overlap pieces the stronger equally cheap version is

    ||D|| * range_U(P)*range_V(P)/4 + max_i||r_i||.

Derivation: B(p)-L(p)=sum lambda_i*r_i +
D*(E[U]*E[V]-E[UV]); bounded scalar covariance is at most the product of the
ranges divided by4. Take MIN of coefficient hull and covariance upper. The
existing1/4 remains safe; range tightening is a proposed finite improvement for
small clipped pieces, not an unexamined literature theorem. Exact squared
sum-of-two-norm comparison resolves equality without rounding a sqrt above tau.
CPU and P100 must use the same explicit RN/padding profile and operation DAG.

Do not use an ordinary rectangle diagonal's quarter-twist error as the entire
bound for a hanging or arbitrary band triangle: the actual nodal residuals and
SAME-L map are necessary. Do not stop after testing a surrogate plane and then
silently replace it by connected actual triangles.

## Strong same-facet tightening before construction changes

A conservative upper above tau is not a witnessed geometric violation.
Use this finite staged dispatcher on the SAME L:

* Retry ambiguous coefficient/normalization arithmetic at the policy's demanded
  higher tier; cache BASE and precise tiers separately so warm replay agrees.
* For positive rational originals, bypass e_R+B-L if it is too loose: restrict
  ORIGINAL homogeneous source net to the clipped triangle, form N-WL in degree
  d+1 (d=p+q<=6), and divide coefficient norm upper by positive Wmin. Tensor
  blossom partition weights are binom(p,i)*binom(q,k-i)/binom(p+q,k).
  Actual affine L is restricted by original barycentrics, never replaced by S
  samples. Use the same anchored residual and exact squared inequality as the
  current numerical services. CPU/P100 do restriction/coefficient tests; selected
  original exact extraction/restriction is the rare host fallback. No per-leaf
  eager exact net or old BoundRootBank is needed.
* For original analytic S, retain root-local directional constants/ranges and a
  triangle-specific alternative. With coordinate ranges hu,hv on a C2 convex
  support rectangle, direct interpolation is bounded by
  (Muu*hu^2+2*Muv*hu*hv+Mvv*hv^2)/8 plus max virtual-node ||S(p_i)-L_i||.
  Proof is Taylor about the barycentric mean: linear terms cancel, and coordinate
  variances/covariance are bounded by their ranges. These are bound-only original
  point enclosures, not newly emitted or shared source samples. Coarse/global
  analytic constants begin the decision; refine local harmonic ranges on demand.
* If still undecided, refine implicated nodes in the SAME source cache and
  retry the unchanged facet, or return a numerical/source/feature request. Do
  not create an unrelated proof tree whose cache/lifetime can diverge. A cache
  restriction alone cannot count as constructing a better unchanged facet.

If construction is requested after these finite attempts, the construction owner
installs a NEW cache-derived point strictly inside each failed actual triangle
and rebuilds its connected region once per complete horizon. A validated source
point residual lower>tau is a genuine violation witness; absence of a witness
means the decision remains an upper-bound/refinement policy, not a claim of
infeasible source. Finite budget exhaustion is unresolved. Numerical point floors
request the canonical point authority's promotion; domain limits request their
real source edge/map. No minimum-size forced PASS.

Stronger alternatives considered: exact scalar quadratic optimization tightens
bilinear covariance further, and bounded analytic Chebyshev patches can unify
GPU degree buckets. They add solver/profile/remainder work; choose neither as a
prerequisite. The selected range-aware covariance and direct same-L rational
residual already address the actual failure classes while preserving the source
parameterization. Tangent-half-angle conics would require an explicit nonlinear
UV correspondence and are not silently substituted for angular originals.

## Retain the strong physical-plane specialization

The old owned `bound_plane_cell` illustrates an important separate finite map:
with a retained admitted projection/inverse profile, bound the WHOLE original
carrier height above its reference plane plus actual affine plane-lift residual.
For exact chart triangle vertices q_i and actual Q_i,

    height_upper + max_i ||plane_lift(q_i)-Q_i|| + E_domain_plane <= tau.

The affine residual reaches its norm upper at the three vertices. This avoids
parameterization-driven tessellation of a geometrically planar nonlinear NURBS
carrier and does not require splitting the facet at every irrelevant UV leaf.
It must remain a first-class typed path, not silently become an ordinary-UV
triangle bound. Reusing the theorem does not require old GeometryStore.

The accepted finite admission is constructive: original finite UV rectangle,
preconditioned projected-Jacobian symmetric positive definiteness on its convex
support, four original projected natural-boundary curves proved monotone straight
segments forming convex Q, and complete original height bound. These establish
an injective projection onto Q; selected domain/facets stay within Q. The working
polygon is selected in plane XY FIRST, with original UV recovered through the
retained inverse only as an explicit locator/enclosure. An old UV polygon cannot
be relabeled as XY. Boundary, chosen-domain and generation records must reflect
this selected chart, while original source nodal guards remain unchanged.

For an authoritative chart-domain map, charge the reference PLANE-LIFT displacement
inside E_domain_plane; the source height is charged globally ONCE. Do not add two
source heights through a generic S(q)-S(h^-1(q)) triangle inequality. The current
shared boundary checkpoint publishes original-UV chosen traces, so changing its
chart/correspondence is an explicit producer dependency, not a free bound-side
optimization. First original-UV region delivery can type-refuse an unimplemented
plane profile; it must not claim the plane-chart two-triangle benefit until the
actual chosen chart/trace producer exists.

## Minimal service/interface dependency

Finite request retains SourceCache and selected-leaf lease, original carrier,
chart/map kind, source/face/boundary/query/profile generations, immutable exact
triangle/chart coordinates, actual XYZ words, positional allowance and domain
map lease. Parent construction owns the real facet key and connected region.

Geometry prepares transient positive `FacetPiece` rows: parent facet key, selected
leaf key/revision, exact normalized UV6, SAME-L XYZ9, exact remaining allowance tau-E_domain (not tau-E_domain-e_R)
and optional source-root/direct-residual descriptor. The cached route adds e_R;
the tighter direct-source route can bypass that conservative e_R entirely. Exact values live in one
sealed owner table; no Budget-affine Rational escapes. Common numerical inputs
remain POD/slab ranges; GPU work never needs a source callback or a host polygon
pointer. Completion identity binds all generations plus the original actual
facet. Fold only a complete current piece set. The output is accepted actual
facets or typed precision/cache/source/domain/construction requests; no second
public certificate artifact is introduced.

First cohesive delivery must connect actual band CDT output to this dispatcher,
including triangles crossing several leaves/spans and a real same-region
refinement/anchor rebuild. A clipping/evaluator demo is not that delivery.
Reconstructed-selected and exact authoritative identity are finite initial
policies; curved authoritative publication waits for the actual B/T producer.
Physical-plane and singular map cases remain explicit specialized dependencies.

## Direct oracle and performance fault classes

Extend the SAME original-source reader and exact region coverage oracle; no
second certification campaign. Before product code select:

* Nonzero bilinear saddle with e_R=0: usual half-rectangle, hanging-side triangles,
  arbitrary displaced L, skinny clipped overlaps, exact quarter-twist threshold.
* Positive rational weight and POWER originals, nonuniform knots and C0 slope
  changes: long facets crossing multiple leaf/root boxes, virtual coordinates
  such as1/3, and deliberately displaced Q. Independent Fraction polynomial
  residuals use original coefficients, not production restricted nets.
* Nonorthogonal analytic cylinder/cone/sphere/torus regular patches and a flat
  nonlinear parameterization. Known complete formulas attack actual facet
  interiors; finite sample probes are falsifiers, not whole-cell proof.
* Missing/overlapping cache coverage, stale map/query generation, partial result,
  incompatible chart dispatch, and missing curved-authority map must refuse.
* Boundary original nodes reused by adjacent faces must keep EXACT key/XYZ.
  Virtual clip vertices never enter their inventory. Domain displacement and
  actual-cell error must sum within the SAME positional allowance.

Record candidate leaf visits, exact clips, positive pieces, coefficient/direct
source tasks, precise fallbacks, numerical promotions and actual cache/anchor
requests. Thin regions, many holes, long slivers and coarse-cache/fine-boundary
mismatch can expose huge overlap or whole-face band costs. Report those actual
counts and censored outcomes; no quadratic all-leaf traversal, repeated original
extraction, per-point rational trig or geometry-only refinement of unchanged L
may hide behind a correctness PASS. CPU/P100 semantic equality checks common
placement and task decisions separately from the independent source oracle.
