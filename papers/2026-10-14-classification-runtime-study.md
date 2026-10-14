# Exact material and finite-face classification: arithmetic and placement

Research, 2026-10-14. Read current Blackbird/repository instructions, roadmap,
body factory/trim code, analytic intersection arithmetic, existing outward
interval primitives, pinned OCCT classifier sources and primary robust-arithmetic
literature. No reference code was executed/copied and no dependency was adopted.
The recommended first beachhead is exact CPU material point membership plus
complete finite rectangular-face contact. Root's proposed design selects that
scope; optional native/GPU filtering belongs to the return pass.

## Literature and actual reference reading

Shewchuk develops adaptive expansion arithmetic and predicate filters under
explicit floating-point assumptions. The paper's arithmetic section excludes
overflow/underflow from its general model and gives a restricted exponent range
for its implemented predicates. It does not justify claiming arbitrary finite
binary64 radial signs are exact merely because we reused a TwoProduct pattern.
Our simple rational input algebra already has an exact CPU representation;
adaptive expansions are a future placement option requiring their own range
contract and measured benefit.
[Author's paper](https://people.eecs.berkeley.edu/~jrs/papers/robustr.pdf).

Yap separates geometric exactness from numerical display precision: the branching
decisions need correct signs, while implicitly represented coordinates may be
approximated for display. His filter discussion explicitly retries with a reliable
method when the approximate sign is not certified. This motivates exact labels
with retained exact submitted points, and any future filter must preserve an
uncertain result rather than map it to boundary or exterior.
[Primary chapter, sections on EGC and filters](https://cs.nyu.edu/exact/doc/handbook03.pdf).

CGAL's official Filtered_kernel documentation distinguishes exact predicates from
the underlying kernel's potentially inexact constructions. That distinction is
directly relevant: classifying a rounded intersection endpoint exactly does not
classify the original endpoint. Preserve exact point input and both surface
representations rather than hiding a float conversion between them.
[Official reference](https://doc.cgal.org/latest/Kernel_23/classCGAL_1_1Filtered__kernel.html).

CUDA 12.8.1 documents directed-rounded double add/subtract/multiply intrinsics,
including that elementary operations are not merged into multiply-add. They can
support an owned outward sign filter on sm_60. They do not themselves supply exact
equality decisions, arbitrary integer arithmetic, or a complete classification
API. The repository's discovery code already owns analogous primitives; future
reuse should study their contract instead of importing a library.
[CUDA 12.8.1 double intrinsics](https://docs.nvidia.com/cuda/archive/12.8.1/cuda-math-api/cuda_math_api/group__CUDA__MATH__INTRINSIC__DOUBLE.html).

Actual pinned OCCT reads were `BRepClass_FaceClassifier.cxx`,
`BRepClass3d_SolidClassifier.cxx`, and selected boundary/ray handling in
`BRepClass3d_SClassifier.cxx`. Face classification in 3D first finds a closest
surface parameter with caller tolerances, then performs UV classification. Solid
classification treats tolerance neighborhoods of vertices/edges as ON, retries
faulty rays and can compare squared distances with Tol squared. Those are mature
general B-rep patterns with a tolerance contract, not our exact analytic material
oracle. We should learn its explicit uncertain/retry and finite-face treatment,
without importing its numerical tolerance as exact equality.
[Pinned face classifier](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/src/ModelingAlgorithms/TKTopAlgo/BRepClass/BRepClass_FaceClassifier.cxx),
[pinned solid classifier](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/src/ModelingAlgorithms/TKTopAlgo/BRepClass3d/BRepClass3d_SClassifier.cxx).

## Exact object and arithmetic

The existing factory has local box half widths 3/4 in x/y and 1/2 in z,
cylinder radius 1/4 and z range [-1/2,1/2], and a plate equal to the box with the
open cylindrical void removed. Frame scale is one of the power-of-two values
1/4 through 4; origin is finite binary64 within [-16,16]. Mathematical world
geometry is exact `origin + scale * local_geometry`, matching the intersection
layer's exact conversion before frame arithmetic. The ordinary evaluated world
point used for rendering is an approximation to that geometry.

For each point convert each submitted int, Fraction or finite float to an exact
rational before arithmetic, then compute `(point-origin)/scale` exactly. Bool,
nonfinite coordinates and unsupported types reject the request. No general
decimal/string conversion or implicit conversion of algebraic Scalar endpoints.
Frame binary64 values must likewise become exact rationals before subtracting or
multiplying them. Fraction already exists in the standard-library research stack;
using it does not adopt a new package or decide the eventual shipping language.

Useful literal adversaries:

- Cylinder origin x=2^-1074, query x=1/4,y=z=0. Exact local x is 1/4-2^-1074,
  hence interior. Ordinary subtraction rounds to 1/4 and incorrectly yields
  boundary. The exact world wall point 1/4+2^-1074 can be supplied as Fraction.
- Default cylinder query x=1/4,y=2^-540,z=0. Exact radial excess is 2^-1080>0,
  hence exterior; ordinary squaring can underflow and wrongly yield boundary.
  The same point is interior to the plate's material.
- Cap/hole-cylinder carrier equality at a point outside all finite trims is
  exterior, not a boundary merely because one algebraic equality vanishes.
- Signed zero represents the same rational zero. Nonfinite values are errors,
  never a material exterior result.

Arbitrary int/Fraction inputs can have large bit size. Query-count complexity
does not bound their arithmetic or allocation cost. Catching MemoryError to
return a guessed label is prohibited; propagate failure without result publication.
Native fixed 64/128-bit integers cannot replace the declared input algebra.
Even float-only coordinates aligned to 2^-1074 can require thousands of bits for
squared comparisons; no unproved fixed-limb cap should enter the first design.

## Material decision and face provenance

Box feasibility is six closed slab inequalities. Cylinder adds the two z slabs
and `x*x+y*y <= 1/16`. Plate uses all box slabs and
`x*x+y*y >= 1/16`. Strict feasibility of all constraints means interior; any
violated constraint means exterior; otherwise equalities identify boundary.
Collect boundary face IDs only after feasibility is established.

Face IDs come from actual snapshot face roles, not a new classifier numbering.
At a box corner retain all incident faces. At cylinder cap/rim retain cap and
wall. A plate hole-rim has cap and inward-facing hole wall; void points are
exterior. The periodic wall's artificial atlas seam is not an additional
material boundary face. Directed coedge uses remain the topology reader's
separate occurrence semantics. Material classification must not infer new edge
or coedge identity from coordinates or merge distinct source entities.

## Complete bounded planar contact

Use original rectangular planar faces only. Their single affine control chart
and exact frame determine a fixed axis and two closed world intervals. A pair's
intersection is the Cartesian product of three interval intersections. Empty
width gives dimension -1; otherwise the number of strictly positive widths is
dimension 0/1/2. Returned vertices represent the complete convex locus, with
exact world coordinates and both original UV maps. No sampled intersection or
carrier bounding-box approximation is involved for this declared family.

Equal coplanar complete domains are coincident; unequal area overlap is overlap;
coplanar point/segment incidence is touch. A perpendicular-carrier segment is
crossing only when it intersects both relative interiors. Testing its midpoint
suffices for these convex axis-aligned rectangles: its varying coordinate is
strictly interior when length is positive, while each fixed transverse coordinate
is interior or boundary along the entire segment. A singleton is touch.
These are finite-face relations, not tests of solid penetration.

Original oriented U cross V and face orientation determine same/opposite/
orthogonal normal relation. Store source FaceRefs and original chart identity;
segment endpoints can be lexicographic, area corners follow positive left-chart
parameter order rather than inventing Boolean coedge orientation. Disk, hole-cap
and cylinder faces must reject explicitly. A hole-cap's carrier rectangle is not
its material domain; adding its bounding rectangle would silently change contact.

## Ownership, caller and next use

Prepared immutable snapshot/FaceRef input remains retained by factory-only
immutable result values. Point batches preserve query order, own exact point
triples and return no partially successful batch on an invalid row or iterator
failure. No new native handle registry, JSON persistence or borrowed pointer is
needed. The public classifier can expose exact rational witness decisions to
later arrangements, while topology construction/history remains a separate
operation. Exact Scalar/algebraic endpoints are explicitly outside this first
point-input family; do not round them to silently satisfy the API.

Use an actual consumer reading labels and every supporting original face, and a
contact consumer reading all world/bothUV vertices plus relation/normal metadata.
Export rational TSV and geometry-derived diagrams. Measure conversion, exact
predicate/construction, result publication and full actual consumption; setup and
independent oracle cost are separate. Pure CPU exact placement means no CPU24,
GPU, upload, speedup or residency claim.

Meaningful cost families include small literal primitives, larger deterministic
point clouds across material/void/outside regions, boundary-heavy caps/rims/corners,
one-ULP radial neighbors, subnormal-origin and underflow witnesses, and framed
rectangle pairs covering empty/point/segment/area/coincident outcomes. Keep
integer/rational bit-size distributions visible. Uniform interior point clouds
alone hide difficult boundary decisions. CPU parity or a pretty slice cannot be
the independent geometry oracle.

## Return-pass placement advice

A future owned C++/CUDA interval filter may emit certified interior/exterior/
boundary only when all required signs/equalities are proved. Intervals crossing
zero, overflow/NaN intermediates and unresolved equality request exact CPU
fallback. A known violated constraint may prove exterior early; an uncertain
constraint cannot prove boundary. Conservative squared intervals need a lower
bound of zero when an operand interval straddles zero. Evaluate original binary
inputs, never rounded local coordinates.

Keep input/result ordering, retain fallback indices, and scatter exact resolutions
to original query identity. Report filter throughput, uncertain counts, exact
fallback work, transfers, cleanup and complete consumer latency. Boundary-heavy
work can make fallback dominate. No persistent sessions, filter implementation,
integer backend, vectorized Python rewrite or language migration is part of this
beachhead. The operator placed language/runtime integration investigation after
the breadth sweep; this study records choices without opening that work lane.

## Sources and restoration

[BibTeX](classification-runtime.bib) and [PDF acquisition catalog](classification-runtime-sources.tsv)
record the two acquired primary papers and original SHA256 values. PDFs are
ignored; restore with `curl -fL SOURCE_URL -o papers/pdfs/FILENAME` and verify
the catalog hash. Existing intact OCCT archives/restoration manifests remain
unchanged. Browser access to the author's robust-arithmetic index failed, but
the author PDF was acquired and read; no unseen index contents were assumed.
