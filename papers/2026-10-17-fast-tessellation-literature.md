# Fast B-rep tessellation: literature synthesis

2026-10-17, branch `research/fast-mesher-study`. Literature lane for the new owned
meshing path that must beat NVIDIA's SMLib-derived tessellator on the whole OM10
(107 definitions, 9,162 faces) at 0.005 mm chord. No product code was written,
built or run. Sources, read depth and hashes are in
[2026-10-17-fast-tessellation-sources.tsv](2026-10-17-fast-tessellation-sources.tsv);
bibliography in [2026-10-17-fast-tessellation.bib](2026-10-17-fast-tessellation.bib).
This builds on, and does not repeat:
[whole-mesher algorithms](2026-10-16-whole-mesher-algorithms.md),
[native meshing options](2026-10-16-native-meshing-algorithm-options.md),
[GPU CDT frontier](2026-10-16-gpu-cdt-frontier.md),
[adaptive subdivision](native-adaptive-subdivision-algorithm.md) and
[trimmed facet bounds](native-adaptive-trimmed-facet-bounds.md).

Operator corrections applied throughout:

- Certification is excised from BRepGPU, not made optional. Exact rational
  acceptance, interval enclosures, per-facet proof obligations and exact
  self-intersection checks leave the runtime. The mesher picks sample density
  correctly from the geometry itself. Independent distance measurement remains
  a test-suite oracle, not a runtime stage.
- CPU SIMD and the CPU/GPU shared batch layout are in scope (§6.2–6.3).
- Domain decomposition moved to its own lane; §6.4 is only a pointer.

## 0. Facts that frame every choice

**The watch is overwhelmingly analytic.** I counted surface entity instances in
the retained `om10-00001_20220701_va_01_3.stp`; their sum equals the 9,162
`ADVANCED_FACE` count. These are entity counts, not a per-face join:

| Surface | Count | Share |
|---|---:|---:|
| `CYLINDRICAL_SURFACE` | 4,431 | 48.4% |
| `PLANE` | 3,688 | 40.3% |
| `B_SPLINE_SURFACE_WITH_KNOTS` | 635 | 6.9% |
| `CONICAL_SURFACE` | 341 | 3.7% |
| `TOROIDAL_SURFACE` | 49 | 0.5% |
| `SPHERICAL_SURFACE` | 18 | 0.2% |

No rational B-spline surface exists. Curves: 14,423 `LINE`, 8,901 `CIRCLE`,
212 `ELLIPSE`, 2,335 `B_SPLINE_CURVE_WITH_KNOTS` and 73 rational B-spline curves.
Pcurves are included in those curve counts.

Consequence: 92% of faces have closed-form sagitta, and 89% are planes or
cylinders. Planes need no interior points. Cylinders need samples only around the
axis, never along it (§1.1). Rational weight handling matters for correctness
breadth, but OM10 does not exercise it on surfaces.

**Where our 847.7 s goes.** Read from
`results/om10-watch-2026-10-17/cpu12-phases.tsv`. Definitions run one after
another; their `complete_component` rows sum to `query0.complete_meshing`.

| Phase | Seconds |
|---|---:|
| `faces` | 541.1 |
| `contacts` (exact facet-contact checks, now excised) | 165.0 |
| `boundary` | 139.9 |
| Everything else | under 3 |

Definition 1713 alone takes 139.3 s. NVIDIA's preparation plus SDK batch runs in
6.1–7.6 s on 12 worker processes (handoff table). The literature below shows
surface evaluation is not where seconds go. CudaTess tessellated 92,256
rational bicubic patches in about 20 ms on a 2008 G80. Embree's SIMD path
evaluates more than 1,000 small patch grids per millisecond per Xeon core. Our
cost is algorithmic: refinement, certification and contact work.

**What NVIDIA actually tests.** I read
`quarantine/sources/usd-brep/source/SMLib/src/SmSurfaceCache.cpp`:

- `SubdivisionTest` (around line 3480) accepts a node when the height of its
  control-net pseudo-box is within the chord tolerance. That is a convex-hull
  slab test: sufficient for B-spline nodes.
- For non-B-spline surfaces it samples the surface. That is an estimate, not a
  bound.
- `SubdivideByBlock` (line 4027) is the default `SM_TA_FASTER_TESSELLATION`
  path. It builds a tensor grid from split points that `FindTessellationSplits`
  chose along sampled iso-curves, capped at 100,000 facets.

So NVIDIA's speed comes from one grid per low-turning block, with split
positions taken from iso-curves. Its sufficiency on analytic faces rests on
sampling.

## 1. Sample density from the geometry: chord-error bounds

The governing fact, reproduced in §4 from Waldron and Shewchuk: the
linear-interpolation error inside any triangle T is at most
½·r_min(T)²·M. Here r_min is the radius of the smallest disk containing T (the
circumradius for acute T, half the longest edge otherwise) and M bounds the
second directional derivative. Waldron's Theorem 4.1 is sharp, with equality
for |x|²; I checked the statement in the retained PDF. For a vector map S, apply
it to e·S for every unit e. Then M = sup over unit directions d of ‖D²_dd S‖, which is at most
λmax([[D_uu, D_uv], [D_uv, D_vv]]).

### 1.1 Analytic surfaces: closed forms

These rules are owned derivations from convexity and Waldron. They are not
quoted theorems. Each is sufficient and needs no runtime check.

**Circle or cylinder.** Every point of a triangle with vertices on a cylinder of
radius R projects, along the axis, into the convex hull of an arc of angular
extent Δθ ≤ π. Its distance to the surface is then at most R(1 − cos(Δθ/2)),
whatever the triangle's extent along the axis.

So for a trimmed cylinder, any parameter-space triangulation whose triangles
each span at most Δθ_max = 2·acos(1 − ε/R) is within ε. A full circle needs
n = ⌈π / acos(1 − ε/R)⌉ segments. At ε = 0.005 mm:

| R (mm) | 0.05 | 0.1 | 0.25 | 0.5 | 1 | 2 | 5 | 10 | 20 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| n | 7 | 10 | 16 | 23 | 32 | 45 | 71 | 100 | 141 |

An untrimmed cylinder face is a strip of 2n triangles of any length. A trimmed
one becomes a 2D triangulation with θ-slab constraint lines spaced Δθ_max
apart, plus its boundary.

**Cone.** Every point of a triangle with vertices on a cone lies in the convex
hull of the generator sector it spans. The far facet of that hull is the plane
through the apex and the two extreme generators. So the deviation is at most
ρ_max(1 − cos(Δθ/2)), with ρ_max the largest vertex radius. Use the same slab
rule with the largest radius in the slab, and a single apex vertex as a pole.

**Sphere.** Three vertices on a sphere of radius R lie on a plane circle of
radius ρ equal to the triangle's 3D circumradius. The triangle sits inside that
cap, so its deviation is at most R − sqrt(R² − ρ²). Sufficient rule:
ρ² ≤ 2Rε − ε².

**Torus** (tube radius r, centre radius R_c, coordinates (u, φ)). Second
derivatives satisfy ‖S_uu‖ ≤ R_c + r, ‖S_φφ‖ = r and ‖S_uφ‖ ≤ r. Size the
triangles by the anisotropic Waldron form (§1.3). Only 49 OM10 faces.

**Plane.** Zero interior deviation. Only boundary chords matter, and those are
bounded on the edge curve (§1.4).

### 1.2 Polynomial and rational B-splines: a-priori sizing per Bézier span

- **Polynomial patch of degree (p, q) on a span.** Sufficient bounds are
  D_uu = p(p−1)·max‖Δ²_u P‖, D_uv = pq·max‖Δ_uΔ_v P‖ and
  D_vv = q(q−1)·max‖Δ²_v P‖ (span-local parameters). For a grid split into right
  triangles, Filip–Magedson–Markot gives
  ε ≥ ⅛(D_uu h_u² + 2D_uv h_u h_v + D_vv h_v²).
- **Midpoint subdivision** scales Bernstein second differences by h², so the
  bound tightens as spans split. A grid factor can therefore be predicted in one
  pass: τ = ⌈sqrt(B/ε)⌉ (Yeo–Bin–Peters 2012; Schwarz–Stamminger 2009). Or
  subdivide a few levels to tighten, then grid each leaf.
- **Rational patch (homogeneous control points).** Zheng–Sederberg 2000,
  Theorem 4, is the strongest a-priori rule read:
  D·δ² ≤ 8ε·min w, with D built from |Δ²(wP)| + (r − ε)|Δ²w|. Zheng and
  Sederberg report steps 6.8–72.6× larger than Cheng's 1992 rule on their test
  curves, and about the step obtained from the true sup of the rational second
  derivative. The weakness is that r = max‖P‖ makes it origin dependent.
  Recenter every patch on its bbox centre before computing D.
- **Rules to avoid:**
  - Cheng 1992 and the Piegl–Richard rational tolerance: unit- and
    origin-dependent; about 100× tolerance loss at 100 mm coordinates.
  - Zhang–Ma's adjacent-weight-ratio derivative bound: Shi 2026 gives a
    counterexample at degree 7.
- **Rational weights** cost only through weight *variation*, since uniform
  scaling cancels.
  - Use min W of the restricted denominator per span, not the global minimum.
  - A Möbius reparametrization can equalize weights (Zhou–Wang 2012, abstract
    only), but trims must be remapped consistently.
- **What not to keep on the hot path.** The earlier owned residual
  R = N∘A − (W∘A)L per actual facet is translation-invariant and tighter than ZS.
  It is, however, a per-facet proof obligation of exactly the kind now excised.
  Keep its lesson: work in homogeneous recentred form and charge only weight
  variation. Choose density a priori from span bounds instead of certifying
  emitted facets.

### 1.3 Arbitrary parameter triangles

Waldron with anisotropic scaling: map u ↦ u·sqrt(D_uu + D_uv) and
v ↦ v·sqrt(D_vv + D_uv), using 2ab ≤ a² + b². Any triangle with
½·r_min'² ≤ ε in the scaled domain is then within ε. This is valid for any
triangle shape, and it is what a metric CDT or front should target.

Sheng–Hirsch's isotropic edge rule, (2/9)λ²(D_uu + 2D_uv + D_vv) ≤ ε, is
4/3–8/3× looser in error than λ²·λmax/6 for the same edge cap. That means
1.3–2.7× more triangles.

The 6ε versus 8ε point (Shewchuk 2002): the error inside an equilateral triangle
is κa²/6, not the κa²/8 measured at an edge midpoint. Midpoint and centroid
sampling therefore *understate* true deviation by up to 4/3. That covers OCCT's
deflection controller, Rhino's "edge to surface" setting and SMLib's sampled
analytic tests. Size by r_min, not by edge length.

### 1.4 Trims and boundary chords

- A mesh boundary edge is a chord of the 3D B-rep edge curve, and that curve
  lies on both incident faces.
- Size edge samples on the 3D curve itself:
  - line: none;
  - circle: closed-form sagitta;
  - ellipse: the a/b² curvature cap;
  - polynomial B-spline: ⅛Δt²·p(p−1)·max‖Δ²P‖ per span;
  - rational: Zheng–Sederberg, recentred.
- Then take the maximum of that count and each incident face's requirement along
  the edge, so boundary triangles do not exceed the face rule. Example: a circle
  bounding a cylinder needs the cylinder's Δθ_max.
- Inside a face, a triangle cut by a trim needs a split budget
  ε = ε_surface + L_S·w_uv (Guthe–Balázs–Klein 2005 use the same Lipschitz form).
  Here w_uv is the parameter-space chord error of the trim segment and L_S
  bounds ‖DS‖. With edge-first vertices (§3), boundary vertices lie on the true
  curve, so only interior triangles need ε_surface.

### 1.5 Normals and shading

No cheap sufficient normal-deviation bound exists in the literature read.
Guthe 2006 is a Taylor estimate. Khoury–Shewchuk 2019 need local feature size.

What the literature establishes is structural:

- Position error scales with r_min².
- Normal error scales with the 3D circumradius, so it blows up only for obtuse
  triangles ("caps"), not for thin ones ("needles") (Shewchuk 2002;
  Khoury–Shewchuk 2019).

For shading:

- Emit exact surface normals per vertex per face occurrence. Analytic normals are
  closed form; B-splines use S_u × S_v. Split them at G1 creases.
- Reject caps with 3D angles above about 150°.

A Bernstein normal-cone half-angle is an owned candidate gate; it is not drawn
from the papers.

## 2. GPU tessellation of trimmed NURBS and B-reps

No published GPU method emits a persistent, watertight, trimmed mesh with a
deviation bound. Every GPU paper that handles trims trims per pixel or texel,
then hides cracks in screen space:

- Guthe 2005: trim textures and "fat borders".
- Schollmeyer–Fröhlich 2009 and 2019: per-fragment trimming. The 2019 authors
  state tiny cracks "cannot be avoided".
- Claux 2014: G-buffer crack fill.
- ETER 2023 and WATER 2025: image-space filling. WATER §6.3 defers trimming
  cracks.

None of these mechanisms has a counterpart in a stored mesh.

What does transfer, from untrimmed GPU tessellation:

- **A-priori object-space factors.**
  - CudaTess (Schwarz–Stamminger 2009) computes Zheng–Sederberg factors per
    rational bicubic patch, takes the max on shared edges, sizes outputs with an
    exclusive scan and writes once. 92,256 patches → 345,751 triangles at
    48–63 Hz on a GeForce 8800 GTS.
  - Yeo–Bin–Peters 2012 do the same from slefe boxes.
- **Edge-local factors and crack-free splitting.** DiagSplit (Fisher et al.
  2009) computes edge factors only from data on that edge and splits so the two
  subedge factors sum to the parent's. Subpatches are independent, with no
  T-junction and no stitching pass. Its own factor function T is sampled, not a
  bound; replace T with §1 rules.
- **Uniform interior plus transition stitching.** Rockwood 1989 coving, Moreton
  2001, D3D11 edge dicing, CudaTess.
- **Bitwise-consistent shared evaluation.** Moreton's canonical traversal order
  and Nießner 2012's reversal-invariant basis sums. Needed only when shared
  vertices are recomputed instead of shared by ID.
- **Arbitrary-degree GPU evaluation.** Krishnamurthy et al. 2009. Its curvature
  constant K comes from central differences, so it is not conservative.

Topology-first B-rep meshing (Zhou, Zint, …, Panozzo, Schneider; arXiv
2604.02141, April 2026; read method and results) is the bleeding edge on
robustness, not speed:

- It ignores pcurves and traces 3D edge polylines across untrimmed patch meshes
  by Dijkstra.
- It guarantees exact B-rep topology on about 10,000 ABC and Fusion 360 models.
- It is single-threaded Python/C++ with an 8-hour cap; 3% of models hit the
  limits.
- Distance is *estimated* by quadrature.
- Errors exceed 3% of the bbox diagonal on 7% of models.
- It reports 14% of Fusion trims self-intersecting.

The relevant lesson for us: when a pcurve is unusable, the 3D edge curve can be
the authority. That is the same case where NVIDIA's distance query refuses OM10.

Learned or neural CAD tessellation: nothing found with any deviation or topology
guarantee.

## 3. Watertight across shared edges

Sederberg et al. 2008 prove gaps between trimmed faces are inherent in the
model. A bicubic trim maps to a degree ≤ 18 curve, while a generic intersection
curve has degree 324 and genus 433. Their T-spline conversion changes the
geometry, and the paper itself recommends coordinated tessellation instead.

Kumar–Manocha (UNC TR94-008, read; journal TVCG 1996) prove per-patch boundary
coving cracks when the two sides parameterize the shared boundary differently.
Their fix assigns each shared boundary to one owning patch, and both sides use
its representation.

Every working persistent-mesh system converges on the same mechanism:

- discretize each physical edge once;
- give each sample a global vertex identity;
- record each sample's (u, v) on every coedge pcurve, including both uses of a
  seam;
- have every incident face consume those vertices as fixed constraints.

Systems that do this: Kumar ownership; OCCT `BRepMesh_EdgeDiscret`, already read
in [the meshing runtime study](2026-10-15-meshing-runtime-study.md); SMLib curve
cache; Dassault US9875577B2's shared parametric boundary vertices; and
Parasolid's `match = topological` (facets meet exactly and share topology).

Rhino documents the alternative honestly: with "jagged seams" on, per-surface
meshes "do not necessarily meet to form a watertight mesh."

With ID sharing, watertightness does not depend on floating-point
reproducibility, because each position is computed once. Bitwise-consistent
evaluation (Moreton, Nießner) becomes a fallback for recomputed data, not the
mechanism.

Seams, poles and T-junctions:

- **Seam.** One edge with two coedge uses on one face. It gets one vertex chain
  with two (u, v) occurrences, as in OCCT's `ModelPostProcessor`.
- **Pole** (cone apex, sphere pole, degenerate edge). One vertex with a fan.
  Parasolid exposes a `degen` option; no GPU paper handles poles beyond
  Moreton's degenerate patches.
- **T-junctions inside a face.** DiagSplit factor partitioning, Nießner's
  transition patch cases, or SMLib's common side schedules for interior packets.
  Never let two sides of one interface choose their own counts.

## 4. Parameter-space triangulation

- **Error versus shape.** Shewchuk 2002 (full read of the interpolation and
  anisotropy sections) and Waldron:
  - value error = c·r_mc², and the Delaunay triangulation minimizes the maximum
    r_mc (Rajan 1991, via Shewchuk);
  - gradient and normal error blow up only as the largest angle approaches 180°;
  - so for a chord tolerance, avoid caps, not needles.
- **Optimal count.** Chen–Sun–Xu 2007 and Mirebeau 2010:
  N(ε) ≈ (C/ε)∫ sqrt|det H| dA, with H = second fundamental form, so
  sqrt|det H| = sqrt|K|.
  - From §1 with equilateral metric triangles, C = 2/(3√3) ≈ 0.385 for elliptic
    regions; a unit sphere at 0.005 mm needs about 967 triangles.
  - Isotropic sizing instead costs (C/ε)∫κ_max dA.
  - On developables (K = 0: cylinders, cones, cylindrical fillet bands) the
    optimal count does not depend on face length.

  This is the strongest triangle-count lever on OM10, and it is the closed-form
  slab rule of §1.1.
- **Metric construction.** Use M = |II|/(6ε) in the first-fundamental-form
  frame, intersected with the boundary-edge metric and a size cap. This is the
  Tristano/Borouchaki lineage, read only through Miranda–Martha 2002. The only
  guaranteed anisotropic 2D quality result is Labelle–Shewchuk 2003 (20° in the
  metric, sequential).
- **GPU triangulators:**
  - gDel2D/gDel3D (Cao et al. 2014) and gCDT 2026 target one large point set.
  - Potter 2011's CUDA metric adaptation spends about 50% of its time in
    transfers.
  - I found no peer-reviewed GPU anisotropic CDT from 2015–2026.

  OM10 faces are small. Parallelism comes from batching faces (§6), not from
  parallelizing inside one triangulation.

## 5. What downstream manufacturing tools require

Hard requirements: these defects break or silently change slicer and CAM output.

- **Cracks.** PrusaSlicer closes loop gaps heuristically up to 2 mm and
  morphologically closes with `slice_closing_radius` 0.049 mm. Cura removes
  layer parts with big holes.
- **T-junctions and unwelded vertices.** These defeat connectivity chaining.
  Hiller–Lipson 2009 describe STL "leaks" that need a welding step.
- **Non-manifold edges.** 3MF Core 1.4.0 MUST: each edge shared by exactly two
  triangles.
- **Inconsistent or inward orientation.** Under positive or winding fill rules
  this subtracts material. 3MF MUST: consistent outward orientation, distinct
  vertex indices.

Tolerated or SHOULD-level:

- **Self-intersection and overlap.** 3MF consumers MUST accept them and fill by
  the positive rule; producers SHOULD NOT emit them. Cura unions by default.
- **Zero-area and needle triangles.** PrusaSlicer keeps them for connectivity.
  They do give garbage STL facet normals, which slicer overhang detection reads,
  so caps and degenerate facets are still worth avoiding.

Resolution context:

- Slicer output resolution: PrusaSlicer `gcode_resolution` 0.0125 mm; Cura
  `meshfix_maximum_deviation` 0.025 mm.
- Cura drops loops under 1 mm circumference. That matters for watch pinions and
  jewel holes as a slicer limitation, not ours.
- Stratasys recommends a 0.0254 mm-class chord.
- CAM finishing tolerances in vendor knowledge-base pages are 0.01–0.03 mm.
  The lane did not retain those URLs, so treat this as anecdotal.

Our 0.005 mm is therefore 2.5–5× finer than the slicers' own resolution and uses
17–50% of a CAM finishing budget. The bar is real but not exotic: watertight,
manifold, oriented, indexed and welded by construction, with deviation that
truly stays under the request.

## 6. High-throughput organization

### 6.1 GPU work organization

- **Patterns read:**
  - Breadth-first bound/split passes with scan compaction (Patney–Owens 2008).
  - Exclusive-scan output allocation, write once (CudaTess).
  - One kernel over the whole batch of small problems (Haidar et al. 2015,
    MAGMA batched).
  - Merge-path partitioning: every thread gets equal work whatever the size
    distribution (Merrill–Garland 2016).
- **Persistent workers, used only when measured idle time demands them:**
  - Aila–Laine 2009: work distribution, not arithmetic, set the 1.5–2.5× gap.
  - Tzeng–Patney–Owens 2010: warp-sized workers, donation ≈ stealing.
  - Steinberger 2014 Whippletree.
  - Gupta–Stuart–Owens 2012: often slower.
- **Avoid CUDA dynamic parallelism:** an average 1.21× net slowdown (Wang and
  Yalamanchili 2014).

For OM10, the current serial loop over definitions is itself the first defect: one
launch per stage should span all 9,162 faces. A per-face work estimate comes from
§1 before triangulating: the face area weighted by sqrt|K|, plus strip and
boundary counts. Sort faces into size classes (a warp per small face, a block per
large one), allocate by scan, and balance by merge-path over cumulative estimates.

### 6.2 CPU SIMD and one batch layout for lanes and warps

The literature is thin but consistent.

- **ispc (Pharr–Mark 2012).** One SPMD program instance per SIMD lane; a gang
  matches a CUDA warp; masked control flow; coherent gathers. Each instance keeps
  its CUDA-thread semantics on CPU lanes.
- **Embree's tessellation cache (Benthin et al. 2015).**
  - It evaluates patch grids in SoA arrays with SIMD.
  - It stitches edges by Moreton's method and sizes sub-grids to the SIMD width
    (3×3 for 4- or 8-wide, 5×3 for 16-wide).
  - It reports 1,351 patches/ms per Xeon core for 3×3 grids, and 10–47
    patches/ms for 33×33 and 65×65 grids, in FP32.

The implied layout is owned, not adopted:

- Evaluation jobs are flat SoA arrays (face or span id, u, v) sorted by
  surface-type and degree class.
- Control data are SoA per class (x[], y[], z[], w[]) padded to the class
  degree.
- The same arrays feed CPU lanes and GPU warps. Lane widths for FP64:
  - AVX2: 4
  - AVX-512: 8
  - NEON: 2 per 128-bit register
  - GPU warp: 32
- Flatness and sizing tests are branch-free max-norm reductions over Bernstein
  second differences. Lanes are spans, not points.

No SIMD constrained-triangulation literature of note was found. For small faces,
the useful SIMD work is evaluation, sizing and candidate pruning. The
triangulation's sequential control flow stays scalar per face, with faces
spread across cores.

### 6.3 FMA, fp-contract and reproducibility

- **Defaults:**
  - CUDA contracts multiply+add into FMA by default. `__dmul_rn`/`__dadd_rn`
    intrinsics are never merged, and a compiler flag disables merging (NVIDIA
    floating-point white paper, read in full).
  - GCC's documented default is `-ffp-contract=fast` when the target has FMA.
  - Clang's default is contested in LLVM's own threads: "on" since 14, but
    "behaves like off" on x86 C.
  - Metal Shading Language 4.1 (2026-06-04) defaults to `fast` math mode
    (reassociation, cross-statement contraction). `safe` sets contract to on.
    `-ffp-contract=off` and `#pragma METAL fp contract` exist.
- **Metal has no double type** ("Metal does not support the double … data
  types"). An Apple GPU path must size and evaluate in FP32, or double-float,
  with recentred coordinates.
- **Libraries differ:**
  - CPU and GPU libm are independent and not correctly rounded; NVIDIA shows
    glibc's `cos` differing between -m32 and -m64.
  - Parenthesization and thread count change reduction results.
- **Accuracy:** FMA helps. Kahan's 2×2 determinant with FMA is within 2u relative
  error (Jeannerod–Louvet–Muller 2013, abstract), useful for cross products,
  normals and orientation filters.

Implications:

1. Watertightness must come from ID sharing (§3), never from cross-device bitwise
   agreement.
2. Where CPU, GPU and Metal results must match bit for bit (regression,
   determinism, debugging):
   - write explicit `fma()` or explicitly uncontracted operations;
   - compile with contraction off;
   - fix the reduction order;
   - own every transcendental used for sample placement. Cylinder angles can use
     an owned rotation recurrence or rational parametrization instead of libm
     `sin`/`cos`.
3. Keep sizing bounds free of FP-contract-sensitive cancellation: recentre and
   difference first.

### 6.4 Domain decomposition (pointer only)

This topic moved to a dedicated research lane. Already acquired here and listed in
the sources TSV: Linardakis–Chrisochoides 2006 (interfaces refined first so
independent subdomain meshes union conformingly) and Chrisochoides' parallel
mesh generation survey. §3's edge-first ID sharing is the same a-priori pattern.

## 7. Ranked candidate directions

Ranked by expected effect on OM10 wall time and fidelity, given the census and
phase costs above.

1. **Edge-first discretization with global vertex IDs.** This is the mandatory
   skeleton (§3).
   - One pass over all physical edges. Per-edge count = max(the curve's own
     closed-form or Bernstein rule, each incident face's rule along the edge).
   - 14,423 lines need no interior samples; 8,901 circles are closed form.
   - Pcurve (u, v) is recorded per coedge occurrence, seams included.
   - Faces consume these vertices as constraints.
   - Watertight, manifold, T-junction-free and welded by construction. This
     replaces any runtime contact or weld stage.
2. **Surface-type generators with closed-form density for analytic faces (92%
   of OM10, §1.1).**
   - Planes: boundary-only CDT.
   - Cylinders and cones: θ-slab constraints plus CDT, or plain strips when
     untrimmed.
   - Spheres: circumradius rule. Tori: anisotropic Waldron.
   - No refinement loop and no acceptance test. Work is proportional to output
     size, independent of face length.
   - This is where NVIDIA's sampled analytic tests are weakest and where our
     541 s of face work should collapse.
3. **B-spline faces (6.9%): span-wise a-priori sizing with built-in subdivision.**
   - Bernstein second-difference bounds, recentred Zheng–Sederberg for rational
     spans, anisotropic Waldron.
   - DiagSplit edge-local factors on leaf edges.
   - Uniform leaf interiors with transition stitching, one pass of factor
     selection (CudaTess/Yeo), and scan allocation.
4. **Trimmed cells inside B-spline faces.**
   - Classify leaves by winding: Bao–Shen–Wang 2025 derivative-ellipse pruning,
     covering space for periodic trims, or Schollmeyer bi-monotone segments.
   - Interior leaves emit grid quads.
   - Crossed leaves triangulate locally between leaf corners and edge-phase
     vertices, never inventing boundary points (Kumar's external-point rule;
     SMLib boundary regions).
5. **One batched GPU/CPU-SIMD organization over all faces (§6.1–6.3).**
   - Work-estimate pre-pass, size classes, scan allocation, merge-path balance.
   - SoA job arrays shared by AVX2/AVX-512/NEON lanes and CUDA warps.
   - Explicit FMA/contract policy; FP32-safe formulation for the Metal path.
6. **Metric parameter-space CDT** (M = |II|/(6ε)). An alternative generator for
   B-spline faces whose curvature directions rotate against the grid or whose
   trims dominate. Lower priority on OM10.
7. **Shading quality.** Exact per-vertex normals per face occurrence, split at
   G1 creases; cap rejection above about 150°. Separate from deviation.
8. **Incremental remeshing.**
   - OCCT semantics, read in `BRepMesh_ModelPreProcessor`: reuse a face whose
     stored deflection is within 1.1× (and 0.9× with quality decrease);
     otherwise outdate the face and its edge polygons.
   - Edge ownership lets an edited face re-stitch to untouched neighbours with
     no global pass.
   - No other literature on localized retessellation after CAD edits was found.
9. **Pcurve-independent fallback.** Topology-first's 3D tracing, for faces whose
   pcurves are unusable. A robustness lead only: hours per model as published.

**Rejected:**

- **Screen-space crack filling** (Guthe 2005, Schollmeyer, Claux, ETER, WATER):
  rendering only.
- **T-spline conversion** (Sederberg 2008): changes geometry.
- **Sampled acceptance as sufficiency:** midpoint, centroid, quadrature and
  iso-curve sampling understate (§1.3).
- **Single-big-triangulation GPU DT/CDT** (gDel2D, gCDT) for our many small
  faces.
- **CUDA dynamic parallelism.**
- **Origin-dependent rational tolerances** (Cheng, Piegl–Richard) and the
  adjacent-ratio bound for degree > 3.
- **Slefe tables for chord sizing.** Tighter enclosures, but per-coordinate
  boxes, and the tables are no longer distributed. Use them only for culling if
  ever.
- **Weld-after meshing for B-rep edges** (§3).

## 8. Questions for the operator

1. **Triangulation predicates.** CDT orientation and incircle decisions on FP64
   input coordinates need robust predicates. Shewchuk's adaptive predicates are
   correct arithmetic on the input doubles, not a proof pass; interval filters
   are listed among the excised machinery. Should robust predicates on the
   triangulation's own inputs stay? This lane assumes yes, as correct geometry
   implementation, but did not decide it.
2. **Self-intersection.** Self-intersection avoidance has no cheap a-priori rule
   in the literature read. Khoury–Shewchuk tie embedding to local feature size,
   and Li et al. 2026 (JCDE) is unread (publisher 403). With exact contact
   checks excised, which construction-level rule should we commit to? One
   candidate: refuse or refine where ε approaches half of a thin-wall or
   knife-edge feature size.

## Acquisition notes

- 52 new open PDFs are in ignored `papers/pdfs/`; names, bytes and SHA-256 are in
  the sources TSV.
- I acquired, verified and read the topology-first, Bao–Shen–Wang, CudaTess,
  Waldron, NVIDIA floating-point, Benthin, ispc, MSL 4.1 and Linardakis–Chrisochoides
  PDFs myself (the last only for the §6.4 pointer).
- Three research sub-lanes acquired and read the remaining sources, each
  reporting its read depth. I spot-checked them: the Waldron theorem statement,
  CudaTess's Zheng–Sederberg factors and tables, Kumar's ownership rule and
  DiagSplit's edge-local rule.
- Paywalled or bot-walled and not acquired (left out of SHOPPING_LIST.md because
  this lane touches no other files): Sheng–Hirsch 1992; Cheng 1992; Nairn et al.
  1999; Reif 2000; Lutterkort–Peters 2001; Zhang–Ma 2006; Concheiro 2014;
  Eisenacher 2009; Rockwood 1989; Cripps–Parwana 2011; Kahlesz 2002;
  Tristano 1998; Borouchaki 2000; D'Azevedo–Simpson; Rippa 1992; ETER 2023 full
  text; Xiao et al. 2021; Li et al. 2026 JCDE; Jeannerod et al. 2013 full text;
  Krishnamurthy 2007/2009 CAD papers.
- One sub-lane removed `*.html` scratch files in a shared scratchpad folder
  (outside the repository). No repository file was affected.
