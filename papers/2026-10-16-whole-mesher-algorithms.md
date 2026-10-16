# Whole trimmed BRep tessellation: algorithm selection research

2026-10-16. Research for one complete mesher, from original shared edges to a
watertight, source-labelled triangle mesh. No product implementation, build,
modeling, test or timing run was performed in this research unit. This is input
to the root design, not an independently selected implementation plan.

## Recommendation and the decision still required

Use **geometry-first point placement, explicit shared-feature ownership, local
approximation certificates and local connectivity updates** as the framework.
Keep the surface approximation hierarchy separate from the output mesh. A UV
rectangular hierarchy is an excellent bounds/index structure, but it should not
force the output into an axis-aligned grid. The two serious whole algorithms are:

1. Adaptive restricted analytic/Bézier cells, trim embedding, interior templates
   and locally triangulated transition/boundary regions. NVIDIA and Balázs et al.
   show how this avoids global triangulation and repeated geometric work.
2. Geometry-aware anisotropic front/refinement with constrained boundaries and
   local cell certificates. It can align with rotated curvature directions and
   avoid parameter-induced oversampling, but needs explicit feature protection,
   local metric control and a real termination argument for the admitted domain.

Do not choose the first just because its bound is easy, or the second because
the project already has a CDT. Both need the *same* shared-edge, curved-domain,
error, seam/pole, source-identity and publication contracts. Compare them on the
independent counterfamilies below before committing the whole design. An
anisotropic proposal plus local residual certificates is a coherent combination:
the proposal controls where work is attempted; the certificate controls whether
the resulting actual triangle can be emitted.

## What was actually read

Acquired originals and hashes are in
[whole-mesher/acquisition.tsv](whole-mesher/acquisition.tsv). Reference archives,
revisions and restoration are in
[whole-mesher/reference-acquisition.tsv](whole-mesher/reference-acquisition.tsv)
and [whole-mesher/RESTORATION.md](whole-mesher/RESTORATION.md).
[whole-mesher/source-readings.tsv](whole-mesher/source-readings.tsv) hashes the
selected implementation files. Bibliography:
[whole-mesher.bib](whole-mesher.bib).

- Piegl–Richard 1995, full 11-page paper, including error derivation, boundary
  sampling, triangulation and limitations.
- Boissonnat–Shi–Tournois–Yvinec 2015, full author manuscript HAL
  `hal-01146307v3`, submitted 2016-11-02, including implementation departures
  from theoretical conditions and appendix. The acquired file has the HAL cover
  and 23 numbered manuscript pages; do not describe it as the typeset 11 pages.
- Engwirda 2016, full 15-page arXiv `1606.01289v2`, including sharp-feature collars
  and the explicitly sketched termination argument.
- WATER 2025, full nine-page author paper. Supplementary proofs were not acquired
  or read. Selected actual public task/mesh/fragment shader code was read.
- Balázs–Guthe–Klein 2004, Sections 3–8, from the complete original acquired by
  the numerical-contract research owner. See that lane's acquisition manifest.
- NVIDIA USD/BRep `c9c5979f04cea8c38213004d4f04df0dcfb40d05`: actual SMLib
  curve cache, surface cache, tessellation and API source sections cited below.
- OCCT `b8f597c677811d1f9f4d8a97f5ae2825c0353a42`: actual edge/face
  discretization, algorithm dispatch, NURBS node generation and deflection control.
- JIGSAW `d7859d7e1b71e4ee1a33663a617ada9ffb4777b8`: selected refinement queue,
  local update and frontal-predicate code, not its entire implementation.
- Aubry et al. 2015: publisher abstract/intro only; full text not acquired after
  publisher, author/title and open-access-location searches. Its 3D anisotropic,
  tiered, parameter-independence argument remains a serious contender, not a
  theorem whose details have been read. Piegl–Tiller 1998 is also preview-only.

## Concrete lessons from NVIDIA and OCCT

**We have substantive NVIDIA kernel source.** This is not merely USD wrappers
around an inaccessible SMLib implementation. `source/SMLib/src/SmTess.cpp` and
the numerical/control-net/cache sources carry Apache-2.0 headers. Our reference
adapter links the official prebuilt SMLib and SM_API libraries, however; the
precise release binary build configuration was not established by source reading.

NVIDIA's main whole-face architecture is local surface adaptation followed by
trim embedding, not a general CDT over the whole face. `SmTess.cpp:3074` builds
and retains a face/surface cache. `:13215` reuses an existing tree, subdivides
B-spline surfaces directly and uses approximate Bézier starter patches for other
surface types. `:12023` embeds UV trims, labels subdivision nodes, adds quads for
inside nodes, then triangulates remaining polygons. `:9074` builds a vertex
spatial tree for repeated corner queries. `:3558` prunes segment checks against
disjoint UV tree nodes. These are avoided-work and locality mechanisms worth
learning independently of language choice.

The numerical policy differs from a hard full-cell approximation contract.
`SmSurfaceCache.cpp:3577` first tests pseudobox height, then directional control-net
errors determine a split. `:3706` enables an iso-curve block path on sufficiently
low-turning B-spline subdomains. `:4053` chooses U/V partitions from iso-curves;
`SmSurface.cpp:11623` explicitly requires the subdomain to have no unresolved
interior surprises. There are also minimum relative-UV stopping rules
(`SmSurfaceCache.cpp:3927`) and split culling above 100,000 tensor partitions
(`:4121`). Their presence is not evidence that the measured mesh is wrong; it
means identical requested chord tolerance does not establish an identical bound.

For curves, `SmTess.cpp:1347` first tries curve-specific bisection with segment
growth; `:1373` falls back to a span/control-net cache. Generic evaluator stepping
uses position/tangent/curvature estimates and Hermite tests (`:1151`, `:1208`).
The cache allocates original Bézier spans contiguously
(`SmCurveCache.cpp:1210`), caches accepted segment properties, splits only failing
spans and builds an ordered binary tree (`:1511`, `:1739`, `:1817`, `:1854`).
Avoid treating generic Hermite estimation as the only or universal curve path.

OCCT likewise owns one spatial-edge sequence and maps it to all pcurves
(`BRepMesh_EdgeDiscret.cxx:225`, `:283`). Its surface-type factory selects
different geometric generators (`BRepMesh_MeshAlgoFactory.cxx:60`). NURBS
generation uses continuity intervals and iso-line insertion/removal to obtain
interior seeds (`BRepMesh_NURBSRangeSplitter.cxx:315`, `:454`), followed by
Delaunay insertion and sampled deflection checks. Its deflection controller
checks triangle centers and internal-edge midpoints, skips boundary links, and
limits passes to eleven (`BRepMesh_DelaunayDeflectionControlMeshAlgo.hxx:85`,
`:222`, `:296`). Learn the stage boundaries and locality, without promoting
sampled acceptance into a universal cell bound.

Parallel work is another independent mechanism. NVIDIA source has conditional
TBB parallel face-cache construction (`SmTess.cpp:11256`) and pcurve construction
(`:11436`); its final face loop at `:11966` is serial in the source read. OCCT
uses face-parallel dispatch (`BRepMesh_FaceDiscret.cxx:82`). The actual reference
caller uses multiple native worker processes and creates one context/plugin per
worker (`native/om10_nvidia/main.cpp:103`, `examples/render_om10_nvidia.py:143`).
PXR/OMP/BLAS environment limits do not prove a TBB thread count. None of this
attributes a measured percentage of the current owned runtime to any stage.

## What the strongest alternative papers actually assume

**Boissonnat et al.** maintain stretched 3D Delaunay surface stars per site and
refine inconsistent stars, using randomized picking regions. The theorem assumes
a compact closed surface, continuous SPD metric and a sizing field controlled
by metric distortion and local feature size. Sharp features are future work.
Experiments approximate distortion and do not enforce all proof conditions.
The important performance mechanism is conflict-zone filtering with AABBs and
a kd-tree (§5.3), rather than retaining every global tetrahedron. This is a
strong anisotropic candidate, not an immediately applicable trimmed-BRep theorem.

**Engwirda.** Restricted frontal-Delaunay refinement coordinates curves before
surfaces, redirects encroaching insertions and checks local topological disks.
Size-optimal off-centres yield to circumcentres in limiting cases; sharp curve
corners receive protective collars. The paper's actual geometry is polyline/
polyhedral remeshing; general analytic intersection predicates and sharp surface
feature protection are unfinished. §4.5 gives a termination sketch, requiring
positive sizing and non-acute assumptions for the cited bound. Source queues
store generation/pass identifiers and update local cavities rather than rescanning
all elements (`rdel_refine_face_3.inc:108`, `:162`).

**WATER.** Regular interior meshlets and transition strips reconcile independent
edge/interior rates; transition-bank height participates in the error inequality
(§4.2). Specialized boundary evaluation and task grouping reduce repeated work.
But §6.3 explicitly defers trimming-induced cracks. Its actual public code uses
`GL_NV_mesh_shader`, float controls and fragment trimming/discard; no complete
source-edge-conforming trimmed mesh is emitted. It is neither a P100-ready
implementation nor a whole-BRep oracle. Borrow its work decomposition and
coupled boundary/interior reasoning, not an unsupported watertightness claim.

**Piegl–Richard.** Global second-derivative bounds produce safe parametric edge
sizes but their experiments report substantial excess tessellation. Their trim
sampler can discard a feature smaller than its step. Both are reasons to avoid
this as the whole default, despite the useful error derivation.

**Balázs et al.** independently approximate composed spatial trim curves and
carrier cells, use adaptive binary/kd subdivision, then a directed half-edge trim
graph. Conservative bilinear residual plus a quarter-twist bound is directly
relevant. Their sampled/SVD reconstruction of composed curves and intersecting-
trim repair require different numerical/source semantics from ours. Their paper
also distinguishes a faster approximate error mode from the conservative mode.

## A local bound we can own without choosing the output topology first

This subsection is an owned derivation, independently checked by the numerical
research owner. It is not attributed to the papers as a new quoted theorem.

Let a positive rational carrier be `S=N/W`. Restrict the original homogeneous
polynomial to an emitted UV triangle through its barycentric affine map `A`.
Let `L(lambda)=sum(lambda_i Q_i)` use the **actual emitted XYZ corners**. In a
common triangular Bernstein basis, compute

`R = N∘A − (W∘A)L`, `w_low=min(weight coefficients)>0`.

Convex combination and division give

`sup_triangle ||S∘A−L|| <= max_k ||R_k|| / w_low`.

Tensor degree `(p,q)` becomes total degree at most `p+q` under affine restriction;
the residual degree is at most `p+q+1` (seven for bi-cubics). The product needs
proper Bernstein convolution/degree elevation. Controls, source-to-span
extraction, restrictions, numerator subtraction, denominator and norm must be
exact or enclosed. Subtract a nearby physical anchor before forming coefficients
to reduce large-origin cancellation. Direct barycentric restriction avoids
solving a nearly singular UV inverse for thin triangles.

Because `L` uses actual corners, their canonical edge displacement and XYZ
rounding are already included. If `L` instead uses exact surface-evaluated
corners, add the maximum actual nodal displacement once. Never double count.
Bounds on extraction/reconstruction and any earlier domain replacement remain
separate; the formula does not erase them.

For a rectangular leaf, a cheaper proposal is a bounded rational residual to its
bilinear corner map `B`, plus `||P00−P10−P01+P11||/4` for the two diagonal
triangles. This is useful when regular cells fit the geometry. It is not grounds
to force every output triangle into a rectangular grid.

**Parameter dependence must be tested.** On the unit square,
`S(u,v)=(u+u(1−u)/2,v,0)` is regular and maps onto the exact planar square.
Two spatial triangles represent that square exactly, yet the same-UV affine
residual reaches `1/8`. Reducing this bound to a tiny requested tolerance forces
unnecessary subdivisions. A physically planar shortcut needs proved coverage
and orientation of a physical-plane chart, or a certified reparameterization;
coplanar controls alone do not establish injectivity or recover source UV.
Analogous tangential distortion can waste work on nonplanar carriers as well.

An independent axis-alignment counterfamily is
`S(u,v)=(u,v,kappa*(u−v)^2)` on the unit square, a polynomial surface with a
straight ruling along `(1,1,0)`. An axis-aligned UV grid must shrink both U and V
steps to order `sqrt(epsilon/kappa)`, requiring order `kappa/epsilon` rectangles.
Diagonal strips aligned with the ruling need only order
`sqrt(kappa/epsilon)` strips: each crosses the curved direction by that small
step but may remain long along the exact straight ruling. Triangulating clipped
strips preserves this order. These are geometric scaling arguments, not measured
timings; original-boundary segmentation and endpoint features still count.
Adaptive rectangles cannot eliminate this orientation penalty merely by choosing
the better U/V split. A triangular residual certificate can accept the long
ruling-aligned elements without giving up its same-parameter guarantee here.

## Shared boundary and feature protection are first-class algorithms

One original physical edge owns one schedule and XYZ authority, with distinct
per-coedge UV/period/side records. Source discrepancy is an input quantity, not
something refinement automatically removes. Incident faces may propose splits,
but a private face must not create a new physical boundary node by itself.

There are two viable coordination policies to decide explicitly:

- Settle a sufficient shared schedule and protect it; mesh boundary strips with
  compatible interior cells. New interior points never split a protected edge.
  A failed boundary-cell bound must produce a global edge request or an explicit
  unresolved result, not an endless sequence of interior splits.
- Use a coupled edge/cell queue with monotonically increasing schedule
  generations. Merge requests by original edge identity, update all occurrences,
  and invalidate only incident boundary cells. A termination argument must bound
  interactions; merely calling the schedule a fixed point does not prove closure.

Grid clipping intersects *sampled UV chords*, generally not the source curve.
Inserting those points on the curve changes their UV positions and may change
other intersections. A global chord-fraction vertex authority can preserve the
same polygon while adding shared vertices, but that is a different explicit
generator/lineage contract from requiring every node to lie on the original
spatial curve. Do not hide that distinction behind a coordinate weld.

Mandatory feature ownership includes original vertices and edges, C0 carrier
knots, protected small holes/branches, chart cuts and singular loci. Nearby
tangent branches need order/separation treatment beyond nonadjacent AABBs;
arbitrarily small input angles invalidate a universal minimum-angle request.
Use geometry-specific collars or an explicitly relaxed quality requirement near
protected features. Seams preserve both UV occurrences and one physical chain;
poles preserve the physical quotient identity without deleting their UV domain.

Positive triangle chains prove coverage of the **polygonal** chart. A complete
curved-face claim additionally needs an onto correspondence from the original
trimmed domain to that chart, with a controlled geometric displacement. Local
surface bounds plus pointwise edge agreement do not establish it. Neither do
they prove absence of 3D self-intersection. The independent numerical-contract
study owns these qualifications and the recent critical-region paper's limits.

## Termination, complexity and falsifiable performance hypotheses

Require finite degree, positive weight lower bound, finite compact source spans,
resolved chart topology and positive feasible tolerance after unavoidable source
discrepancy. On regular pieces, shrinking restrictions tightens local bounds;
regularity and separation can give a positive sufficient local size. Singular,
tangent or inconsistent input does not inherit that statement automatically.
Distinguish witnessed infeasibility, an inconclusive enclosure and resource
exhaustion. A work budget is an operational bound, not a convergence theorem.

Hypotheses for the complete design, to measure later:

- Carrier-local caches and a span/trim hierarchy replace repeated full-span scans
  with queries proportional to actual intersections, plus tree traversal.
- Direction-aware point placement reduces output size on rotated anisotropy and
  avoids global derivative maxima deciding every cell's size.
- Cached restricted control nets and batch residual evaluation make work roughly
  proportional to visited local cells at fixed degree. Exact predicates filter
  ambiguous topology; exact arithmetic need not run for every ordinary bound.
- Local front/cavity updates with versioned queues avoid whole-face restart after
  every insertion. Cache invalidation is part of this algorithm, not later polish.
- Coarse independent faces/spans/edge jobs expose CPU parallelism; packed bound
  batches expose P100 work. Include ordering, host conversion, upload/download,
  exact fallback, refinement and complete publication in measurements.

No worst-case near-linear claim follows merely from these mechanisms. General
intersection arrangements can have quadratic output, many-hole admission may
be quadratic, and a changing metric can create substantial insertion/repair work.

Preselect workload families: geometrically identical planar surfaces under
nonlinear/anisotropic UV maps; rotated thin ridges; smooth varying-curvature
positive-rational patches; high weight ratios; large translations/tiny spans;
C0 creases; narrow channels and many tiny holes; acute/tangent trim branches;
cylindrical seams, spherical poles and same-face repeated edges; independently
assembled adjacent faces with source discrepancies near the tolerance. Retain
OM10 as one further workload, not the source of algorithm dispatch or capacities.

The next design review should settle: geometric versus same-UV error semantics;
the physical/chart feature authority; boundary schedule/cell closure; the primary
placement/connectivity contender; exact or enclosed source extraction; and the
termination claim actually supported for each admitted singularity class.
