# Analytic intersection representation and runtime placement

2026-10-14. Research only. Recommend CPU-owned complete branch construction for
the existing axis-aligned plane charts and z-axis cylinder quarters, followed
by a two-sided sample consumer using existing native jets. No general SSI,
GPU intersection construction or speedup claim; no dependencies selected.

## Source study and consequences

Pinned OCCT `IntAna_QuadQuadGeo::Perform(plane,plane)` and `(plane,cylinder)`
separate empty/coincident/line/circle/ellipse results. Its general orientation
and tolerance/near-parallel heuristics are broader than our factory family;
do not copy tolerance branches into exact submitted-input classification.
`GeomInt_IntSS::Line`, `LineOnS1`, `LineOnS2` and the independent approximation
flags expose three representations, not one polyline with guessed UVs.
[OCCT pinned source](https://github.com/Open-Cascade-SAS/OCCT/tree/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/src/ModelingAlgorithms/TKGeomAlgo/GeomInt).

Pinned SISL s1859 first produces guide topology; s1310 constructs pgeom and
ppar1/ppar2, fitting the parameter curves using the same spatial curve parameter
array. Its documented singular/degenerate warning can accompany a partial
trace. This supports explicit completeness and representation flags rather
than inferring success from nonempty geometry.
[SISL s1310](https://github.com/SINTEF-Geometry/SISL/blob/e0a82b1a851e932657f3e1af25033fbbcd33e907/src/s1310.c).

openNURBS' pinned line/plane routine may write a fallback parameter when
returning false; its simple-intersector header explicitly narrows transverse
cases. Its inspected degenerate line/box branch also has a condition/comment
disagreement. These routines are design references, not complete SSI oracles.
[Pinned simple intersections](https://github.com/mcneel/opennurbs/blob/eb92af3ba1806b0a34a99aba0d3bda83e3d46083/opennurbs_intersect.cpp).

Shen/Cheng/Jia's projectable-surface study separates parameter-curve topology
from spatial lifting, including singular, irregular and self-intersection
events that a numerical graph may miss. Trocado/Gonzalez-Vega similarly analyze
cutcurve singularities and lifting for general quadrics. Their higher-degree
algorithms are not selected here; the first paper's projectability and the
second's presented monic-quadric assumptions are not a blanket theorem for our
bounded charts. Read relevant full-text sections and examples, not abstracts
alone. [Shen et al. 2012](https://arxiv.org/abs/1203.0442),
[Trocado/Gonzalez-Vega 2019](https://arxiv.org/abs/1903.06983).

Existing archives remain intact. Exact revisions, read scopes and acquired PDF
hashes are in [source catalog](intersection-runtime-sources.tsv); bibliography
is [intersection-runtime.bib](intersection-runtime.bib). No reference executed.

## A defensible first geometry scope

Accept exact factory snapshots/selected original charts under their current
positive uniform Frame; recognize support geometry from declared roles plus
actual-control binding. Do not infer an analytic type from approximate samples.
Domains are both closed rectangular carrier parameter rectangles, including
trimmed-out portions. Disk/hole membership and editable topology are later work.
Unsupported cylinder/cylinder or general spline pairs remain explicit outcomes.

For unequal coordinate-axis plane normals, the support intersection is a line;
restrict it to both rectangles using exact rational interval constraints. This
yields empty, isolated point or segment. Parallel distinct planes are empty.
Coplanar rectangles need a separate coincident-region relationship: their
two-axis interval product distinguishes empty, point, segment and positive-area
overlap. Do not emit a curve collection labeled complete for a 2D overlap.

For a cylinder quarter, write world support as

`C(u,v)=origin + scale*(R_quarter(u)/4, v-1/2)`,
`R_0(u)=((1-u²)/(1+u²), 2u/(1+u²))`, `u,v in [0,1]`.

The other quarters use exact signed coordinate rotations. A horizontal plane
fixes v rationally and leaves a rational circular arc; plane rectangle UV
constraints reduce to quadratic numerator inequalities with positive denominator.
A vertical coordinate plane produces a degree<=2 equation in u and leaves a
generator in v. Test all roots in [0,1], then restrict height and the other
plane coordinate. Exact discriminant/sign tests distinguish no root, repeated
tangent root and two secant roots without epsilon reclassification.

Construct coefficients from exact binary inputs as Fractions. A quadratic
root is a symbolic algebraic value with an isolating rational interval, not just
a rounded float. Detect identically-zero constraints, reduce degree, preserve
multiplicity and use exact polynomial common-root tests before refining nearly
equal endpoint intervals. A bounded isolation budget may return unresolved;
it may not silently return empty or claim completeness. The degree-two domain
decomposition, including all boundary roots and point cells, is the concrete
completeness mechanism for this family. General resultant/marching machinery
would enlarge the first delivery substantially.

## Spatial and both UV representations

Each curve component should own its source pair, branch/multiplicity, common
parameter domain, spatial expression C(t), and both normalized chart UV maps
P1(t),P2(t), with exact algebraic/rational endpoint records. Lines are affine;
horizontal sections are rational quadratic arcs; generators have a constant
algebraic u and affine v. Same t must identify the same point in all three.
Store source UV separately: cylinder global coordinate is quarter+local-u.
Coincident regions and isolated points are distinct records, not degenerate
curves with fabricated tangents.

Split at every required chart boundary and retain seam/quarter occurrences.
Equal world points at different chart UVs are not duplicate source identities.
Represent proven junction relationships explicitly; do not merge by rounded
XYZ or tolerance buckets. Component order can follow input flat pair and sorted
root/domain order. Orientation of a chosen curve parameter is separate from
face orientation; normals can guide later coedge decisions but must not define
the only tangent at tangency, where their cross product vanishes.

The identities S1(P1(t))=C(t)=S2(P2(t)) should be established for the entire
declared component using independent exact coefficient/radical relations.
This is different from sampling a curve and measuring small residuals. The
original actual controls and frames must be bound to those analytic expressions.

## Useful existing GPU consumer, without a new curve ABI

Gather both source UVs for M requested component samples into one existing
PatchSet/query batch with 2M rows. Keep (component,t,side,source-patch) mappings,
check query/source bytes and statuses, convert jets to the common world frame,
then scatter. Order0 checks positions; order1 additionally checks
`C'=Su*u'+Sv*v'` on both sides. Curves may be constructed and sampled on CPU;
CUDA only evaluates the supplied carrier queries in this first consumer.

Spatial/UV curve evaluation can also be encoded as degree(p,0) patches when
their normalized control/weight envelope is admitted. This is optional reuse,
not arbitrary curve support: clipped analytic curves may still have control
values outside the current cube, so require a named normalization adapter or
CPU expression evaluation rather than silent clipping. An additive curve kernel
could later reduce duplicate outputs, but requires new lifetime/ABI work and
cannot supply exact branch completeness by itself.

Rounded algebraic UV constants generally do not lie exactly on the plane.
Keep that representation error separate from native evaluation error. Check
independent actual-input surface truth and both implicit residuals with stated
units: plane residual is length, cylinder equation residual length². A small
implicit residual alone does not bound position near a tangent; compare against
isolated exact branch intervals and exact/rational sample identities too.
Snap to 0/1 only when exact endpoint identity is proven. Do not mistake finite
native output or CPU parity for intersection correctness.

Existing order0 CUDA payload is 20 input+28 jet/status output bytes per row;
order1 output is 76 bytes. With two sides those become 40 input and 56/152 output
bytes per component sample, plus retained host provenance/descriptors and host
mapping/scatter. Measure CPU construction/root isolation, gather/parameter
evaluation, patch/query setup, native call, download/conversion/residual work,
scatter/export and actual caller wall. Initially qualify all branches/events
independently; performance calls retain the stated cheaper checks. No hardware
cost or break-even point was measured in this study.

## Concrete first delivery and limitations

Deliver deterministic immutable supported-pair results with all points/arcs/
segments or coincidence relationships, both UV maps, exact endpoint identities,
and explicit unsupported/unresolved outcomes. Include an actual caller feeding
pair survivors into construction and producing geometry/source records plus a
two-sided jet-check sample export. Tests must attack tangent/near-tangent,
empty/point/overlap, chart boundary/seam, changed frames, reversed pair ordering,
root equality and bounded partial discovery. Partial broad-phase input cannot
establish all scene intersections. No contact/trimmed-face/Boolean or certified
mesh claim follows.

Return-pass options are recorded separately in
[round2 notes](2026-10-14-intersection-round2-notes.md). Session/pooling and
optimization implementation remain deferred under [RETURN_PASS](../docs/RETURN_PASS.md).
