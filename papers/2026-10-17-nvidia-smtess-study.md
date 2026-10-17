# NVIDIA SmTess tessellator, end to end

Study of the pinned NVIDIA USD BRep source, `c9c5979f04cea8c38213004d4f04df0dcfb40d05`,
at `quarantine_proj/oss-cad-2026-10-13/usd-brep`. Study only; never a dependency.
Read-only lane: nothing was built, run or benchmarked. This builds on
[whole-mesher algorithms](2026-10-16-whole-mesher-algorithms.md),
[curve runtime](native-adaptive-curves-runtime.md),
[subdivision algorithm](native-adaptive-subdivision-algorithm.md),
[boundary regions](native-adaptive-boundary-regions.md) and
[the sphere chord finding](../docs/TESSELLATION_FINDING.md).

Paths below are relative to `usd-brep/source/` unless they say otherwise. Tags:
**[code]** = read in code; **[computed]** = arithmetic on code formulas or retained
data; **[inferred]** = reasoning not directly established by reading.

Caveat that bounds every claim here: BRepGPU links NVIDIA's prebuilt 1.0.0 binary.
This is the source at the pinned commit. Its build flags are read from
`SMLib/inc/SmConfig.h` and the premake files, not from the binary.

## Corrections to earlier notes

1. `SmCurveTessDriver::Initialize`/`TestStep` (`SMLib/src/SmTess.cpp:1141`, `:1179`)
   have no callers. The live Hermite step test is the static `sm_TestStep` in
   `SMLib/src/SmCurve.cpp:20932`, called from `SmCurve::TessellateByBisection`
   (`:21162`). [code, grep]
2. Analytic surfaces are B-spline subclasses. `SmPlane : SmBSplineSurface`
   (`SMLib/inc/SmPlane.h:41`); `SmSphere : SmSurfOfRevolution : SmBSplineSurface`
   (`SMLib/inc/SmSphere.h:91`, `SmSurfOfRevolution.h:73`). Cone, cylinder, torus and
   extrusion follow the same pattern. They therefore take the exact-rational
   `BuildTreeWithSubdivision` path (`SMLib/src/SmTess.cpp:13250`). The "approximate
   Bézier starter" path (`SmSurfaceCache.cpp:4860`) only serves surfaces that are not
   B-splines, e.g. `SmOffsetSurface`. [code]
3. `NL_Tessellate.cpp` (`N_TessTrimmedSrf`) is not on the `SmApiTessellate` path.
   Its callers are `SmFace::ComputeProperties` (`SMLib/src/SmFace.cpp:6450`/`:6613`,
   used for area and mass), `SmFace::OutputPolygons` (`:14310`),
   `SmBSplineSurface::OutputPolygons` (`SmBSplineSurfaceNLib.cpp:7731`) and
   `SmTrimmingTools::CheckTessellation` (`:2540`). The area inaccuracy in
   KNOWN_ISSUES belongs to that path, not to the watch meshes. [code]

## 1. Pipeline

### Entry and parameters

- `SmApiTessellate` (`SM_API/src/SmApiBrep.cpp:679-766`) checks for null input. It
  converts a negative max edge into `0.025·|v|·bbox diagonal` (`:715-721`) and calls
  `ConvertToPolyBrep` with advancing front FALSE and smoothing FALSE (`:725-736`). It
  discards output on error, or on any face failure unless partial output is allowed
  (`:739-751`). [code]
- `ConvertToPolyBrep` (`SMLib/src/SmBrep.cpp:37186-37639`) builds the two drivers.
  [code]
  - `SmCurveTessDriver(0, chord, curveAngle, maxEdge, minParamRatio=0.0001, FALSE)`
    (`:37269`).
  - `SmSurfaceTessDriver(chord, surfAngle, maxEdge, minEdge3D=0, minUVRatio=0.001,
    aspect)` (`:37270`).
- `SmTess` defaults to `SM_TA_FASTER_TESSELLATION` (`SmTess.cpp:813-830`). That
  enables the iso-curve block path described below. [code]
- No 0.5, 2 or sqrt factor is applied to the chord on the way in. Chord is absolute
  world units. The surface angle goes from degrees to radians at `SmTess.cpp:3100`.
  [code]
- The whole B-rep is copied, because tessellation splits edges in place
  (`SmBrep.cpp:37283`).

**BRepGPU's watch settings.** `native/om10_nvidia/main.cpp:157-158` sets only chord
0.005, curve angle 0 and surface angle 0. Max edge and aspect stay 0 (off). Strict,
non-partial import and tessellation (`:150`, `:160`). [code]

### Phase 1: `SmTess::Phase1SetupBrep` (`SmTess.cpp:11118-11866`)

1. **Per-face surface cache.** One `SmTessSrfCache` per face (`:11300-11360`). The
   TBB-parallel version at `:11256` is compiled out because `SM_USE_TBB` is never
   defined. The cache subdivides the *natural* UV domain of the carrier surface, not
   the face's trimmed domain (`SmTrimSrfCache.cpp:986-993`; `SmTess.cpp:13139`).
   [code]
2. **Iso-edge splits.** Edges whose pcurve is an iso-line are split where cache-leaf
   boundaries cross them (`IsoEdgeSubdivide`, `:2696-2752`; `FindIsoLineSplits`,
   `:2761-2867`). [code]
3. **3D curve tessellation of every edge.** `TessellateEdge` (`:3009-3066`) runs
   `TessellateCurve` (`:1320-1438`), which tries `SmCurve::TessellateByBisection`
   first and falls back to `SmCurveCache::Tessellate`. `ClampAndSplitEdge`
   (`:2884-2987`) then splits the B-rep edge topologically: "1 new SmVertex and 1 new
   SmEdge per split". Split parameters closer to an end vertex than its tolerance are
   dropped. [code]
4. **Face-size splitting.** `SplitFaceEdgesToUVSize` (`:3218-3541`) bisects an edge,
   at its 3D curve parameter midpoint, while its UV chord exceeds 1.1× the UV size of
   any cache leaf its UV box touches (`CheckAgainstSurfaceCache`, `:3558-3746`). This
   is UV-size matching, not a deviation test. It is skipped when the 3D length is
   below 20× the B-rep tolerance, which acts as the pole guard (`:3708-3730`). [code]
5. **UV boundary polygon per face.** `MakePolyBrepFromFaceEdges` (`:4317-4435`)
   calls `MakePolygon` (`:1742-2661`), which makes one straight UV PolyEdge per split
   edgeuse.
   - Pole and gap filler edges with no edgeuse are added by
     `sms_FillGapUVWithPolyEdge` (`:1456-1605`). The gap tolerance is 1/1000 of the
     natural UV domain (`:1846-1848`).
   - `CheckPolygonIntersections` (`:3754-4084`) and `CheckPolygonNesting`
     (`:4123-4194`) repair crossing or mis-nested linearised trims. They split shared
     B-rep edges through `TrySplitSegment` (`:12170-12269`) or delete topological
     vertices.
   - Any repair triggers a full rebuild of *every* face, up to 100 passes
     (`:11634-11845`; the comment at `:11826-11831` admits that only the affected
     faces need it).
   - Errors from both repair checks are swallowed: `return SM_ERR` is commented out,
     only a message is logged (`:4388-4404`). [code]

### Phase 2: `Phase2CreateFacePolygons` (`:12044-12131`), one face at a time

1. **Embed the trim.** `BuildTree` re-runs: the tree already exists, so it embeds
   every boundary PolyEdge into the leaves it crosses (`ImplantUVSegment`,
   `:14293-14429`). The leaf box is expanded by `10 × min(tol mapped to UV,
   domain/100)`. Leaves it touches are marked ON_BOUNDARY. [code]
2. **Classify leaves.** `MarkFaceContainment` (`:14437-14621`) labels the other
   leaves INSIDE or OUTSIDE by flood fill, with one point-in-polygon test per
   connected region. [code]
3. **Build inside quads.** `CreatePolygonsFromInsidePartsOfTree` (`:5289-5324`)
   walks the binary tree top-down from one rectangle. `PrepareTreeNodeData`
   (`:4940-5179`) inserts each bisector and splits neighbour sides. Hanging nodes
   become extra polygon vertices, so the result is conforming. Each split point
   snaps to an existing vertex within ¼ of the child length (`:4998-5072`). Subtrees
   with no INSIDE leaf are deleted (`:4959`). [code]
4. **Leave the annulus.** ON_BOUNDARY leaves contribute no surface vertices. The
   region between the trim polyline and the inside quads becomes a few large
   polygons, traced by `TraceLaminaLoop` (`:4641-4843`). The comment and diagram at
   `:5610-5648` describe it as "a large annular area to be broken into triangles at a
   later time". [code]
5. **Triangulate.** `TriangulateFace` (`:10046-10284`) drives `TriangulateSingleFace`
   (`:8557-10037`).
   - Inner loops are bridged to the outer loop at their closest vertices
     (`:8640-8740`).
   - Then ear clipping on the best-scoring convex corner. The score is the corner
     angle (2D only in both active parameter sets, `:9150-9158`), plus a 1000×
     penalty per UV-size split the new diagonal would need, plus BAD for non-convex
     corners or corners with vertices inside.
   - New diagonals longer than 1.1× the leaf UV size are subdivided by surface
     evaluation at UV midpoints (`SubdivideManifoldEdge`, `:10843-11054`). This is
     skipped when the 3D length is below 10× the face tolerance or the adjacent
     aspect ratio is below 0.02 (`:10180-10190`). [code]
6. **Smoothing** (`:5904`) does nothing with the default zero passes. [code]

### Output: `OutputFacePolygons` → `OutputToPolyBrep` (`:7182-8048`, `:6494-6920`)

- **Positions.** Boundary vertices use the *B-rep vertex point* on the 3D edge curve.
  Interior vertices use `surface.EvaluatePoint(uv)` (`:7329-7340`). [code]
- **Normals.** Per face corner, `surface.EvaluateNormal(uv)`, negated for faces
  reversed relative to their shell (`:7343-7350`). They are stored per edge as
  auxiliary data (`:6855`), so each face keeps its own normal at shared vertices.
  [code]
- **Assembly.** All faces go into one `m_p3DPolyBrep`.
  - 3D edges shorter than `tol/100` are merged. Polygons with fewer than 3 distinct
    3D edges are dropped (`:6650-6665`, `:6760-6790`, `:6880-6895`).
  - Lamina and spine edges are only counted (`SmBrep.cpp:37604-37617`), never
    enforced.
  - Note: `SmApiTessellate` succeeds when the output is non-manifold.

## 2. Crack-freedom, seams, poles, degeneracies

- **Mechanism: shared topology.** There is no welding and no post-hoc stitching by
  distance. [code]
  - Every 3D edge split mutates the copied B-rep, so every incident face sees the same
    `SmVertex` sequence (`ClampAndSplitEdge`, `SplitFaceEdgesToUVSize` and
    `TrySplitSegment` all call `MakeVertexSplitEdge`).
  - Output maps each `SmVertex` to exactly one 3D PolyVertex through a back pointer
    (`GetOrCreatePolyVertex`, `:6939-7031`).
  - PolyEdges are glued only when they share both endpoint PolyVertex objects
    (`SmPolyBrep::StitchEdgeTopological`, `SMLib/src/SmPoly.cpp:10408-10470`). It is
    called with non-manifold stitching allowed (`SmPoly.cpp:19665-19678`), so more
    than two uses form a "spine" rather than an error.
- **What a face may not do.** Boundary PolyEdges are never subdivided only in a face's
  UV. Any split of a boundary PolyEdge goes through `TrySplitSegment`, which splits
  the B-rep edge as well (`:12210-12260`). Filler edges, which have no edgeuse, are
  the exception. [code]
- **Consequence for adjacent faces.** The boundary vertex set is identical by
  construction. Each face's triangles join 3D curve points to its own surface
  points. The two faces' triangles therefore meet along identical straight segments,
  but each face's boundary strip may sit up to the edge tolerance off its own
  surface. [inferred]
- **Seams.** A periodic seam is one B-rep edge with two edgeuses in the same face and
  two pcurves. The same `SmVertex` maps to two UV PolyVertices (diagram at
  `:1652-1665`), but there is only one 3D PolyVertex, so the seam closes. Normals are
  evaluated at each side's UV. [code]
- **Poles.**
  - The singular boundary has no edgeuse. `sms_FillGapUVWithPolyEdge` adds filler UV
    edges across the gap (`:1456-1605`; diagram `:1666-1690`).
  - Zero-length 3D edges collapse at output, and collapsed polygons are deleted
    (`:6650-6665`, `:6880-6895`).
  - Boundary splits near poles stop below 20× the B-rep tolerance (`:3708-3730`).
  - In `ImplantUVSegment`, the comment at `:14304-14316` says pole-inflated UV
    tolerances can mark *every* leaf ON_BOUNDARY. The face then gets only boundary
    vertices and "ignor[es] the specified tessellation accuracy parameters". [code]
- **Illegal gaps.** A loop gap that is not at a pole gets a filler edge
  (`:1680-1688`). That edge is lamina in 3D, so the output has a hole. [code]
- **Degenerate edges and loops.** Loops with fewer than 3 edgeuses are skipped
  (`:1846`). Degenerate curves give a single point (`:1388-1394`). [code]

## 3. Chord-height estimation, and the geometry knowledge it lacks

**Framing.** BRepGPU is removing runtime proof machinery entirely (operator,
2026-10-17): no exact rational acceptance, no interval enclosures, no per-facet proof
obligations, no exact self-intersection checks. "The way we certify is by
understanding how to correctly implement geometry in our code."

This section therefore reads NVIDIA's estimator as a *density rule*. For each
mechanism it asks what correct geometric knowledge the sampler must embody so that
the density it chooses actually keeps deviation within tolerance. Each understatement
mechanism below has a matching correct rule in the subsection "What the sampler must
know".

### Curves

The live test is in `SmCurve.cpp:20932-21360`. For a step `[t0,t1]` with
`h = t1−t0`, it builds the Hermite inner control points

```
Q1 = C(t0) + h·C'(t0)/3
Q2 = C(t1) − h·C'(t1)/3          (SmHermiteCurve.cpp:440-452)
```

and computes `e_H = max(dist(Q1, chord), dist(Q2, chord))`. It cuts the step when
`e_H > ε` and grows it when `e_H < ε/2` (`SmCurve.cpp:20961-20975`).

- **Quality of the estimate.** e_H is a property of a *synthetic cubic built from
  endpoint data*. It is not the curve. [code + computed]
  - For an angle-parameterised arc it is about 4/3 of the sagitta, so conservative.
  - For a polynomial cubic it is the convex-hull bound.
  - For rational curves, nonuniform speed, or content above cubic within the step it
    is only an estimate.
- **Sub-samples.** Points at ¼, ½ and ¾ of interior knot spans re-run the same Hermite
  test on `[t0,tj]`. None of them compares C(tj) with the emitted chord
  (`:21343-21360`). [code]
- **Unverified acceptances.** [code]
  - Steps at the minimum size `len/1e5` are accepted untested (`:21264`).
  - After more than 100 tests the current step is accepted (`:21381`).
  - Curves that pass `IsLine` at ε/100 are emitted as one segment (`:21244`).
  - The driver's `minParamRatio = 1e-4` (`SmBrep.cpp:37269`) feeds the
    `SmCurveCache` fallback, which also force-accepts spans below
    `1e-4·interval/2` or below 1e-6 (`SmCurveCache.cpp:1647`, `:1760`).
- **`SmCurveCache` fallback.**
  - Its test is the maximum distance of Euclidean (`Pw/w`) control points from the
    end-to-end line (`SmCurveCache.cpp:887`, `:986-997`). With positive weights that
    is a true hull bound.
  - For curves that are not B-splines it caches Hermite approximants. Their
    approximation error (`0.5·ZoneTol`, checked at 20 samples) is not added to ε
    (`:1157`, `:1310`).
  - There is an evaluation-parameter bug at `:1105`. [code]

### Surfaces (exact rational sub-patches; every analytic carrier)

`SmSurfaceCache::TestAgainstTolerances` (`SmSurfaceCache.cpp:3461-3990`) builds a
pseudobox from the patch's Euclidean control points `P = Pw/w`
(`SmBSplineSurface.cpp:516-540`, `:668-686`, `:803-806`):

```
b1 = unit((P00−P10) + (P01−P11))
b3 = unit(b1 × ((P00−P01) + (P10−P11)))
H  = (max_ij b3·P_ij − min_ij b3·P_ij) + 2·faceTol
accept iff H² ≤ ε²                         (SmSurfaceCache.cpp:3581-3584)
```

- **What H bounds.** With positive weights, both the patch and its corner quad lie in
  the slab of thickness H, so this is a valid normal-direction slab bound. It
  measures the distance from triangle to surface only where the patch is a graph over
  the slab plane. With both angles at 0 nothing enforces that. [code; graph condition
  inferred]
- **Direction choice only.** The directional net heights and the UV-diagonal net
  height are computed only *after* a failure, to pick the split direction
  (`:3600-3655`). The code itself says the true error is roughly the U value plus the
  V value (`:3398`). [code]
- **Inherited flags.** Children inherit the parent's `m_bChordHeightSatisfied` flag
  (`:2615`, `:2627`). [code]

### Iso-curve block fast path (`SubdivideByBlock`, `SmSurfaceCache.cpp:4027-4252`)

- **Entry.** Taken when a B-spline node's net turning angles fall below 30°
  (`:3712-3720`, default at `:5722`). This happens even when the angle tolerance is 0.
- **Stale flag.** `m_bSwitchToFastSubdivision` is per cache, never cleared by the
  test, and only reset after a block is built (`:4621`). A later node can take the
  block path whatever its own angles. [code; effect inferred]
- **Split selection.** `FindTessellationSplits` (`SmSurface.cpp:11644-11960`) runs the
  1D Hermite test on the two *boundary* iso-curves only, falling back to iso-curves at
  0.4/0.6 of the domain. The comment at `:11625` says "any localized bumps in the
  middle will be ignored".
- **No test on the result.** The resulting base cells are never tested
  (`SmSurfaceCache.cpp:4177-4252`).
- **Early returns.**
  - With at most 2 splits in each direction the failing node is accepted as it is
    (`:4128-4131`).
  - Above 100,000 cells every other split is dropped, which doubles cell size
    (`:4124-4149`). [code]

### Minimum-UV stop

When a split side is below `0.001 ×` the *natural* domain (`SmSurfaceCache.cpp:3932`),
the split turns to the other direction if that side is above 1/100 of the domain. If
not, the node is returned as passing with the tolerance unmet (`:3936-3967`). [code]

For OM10's analytic carriers this is harmless: a cylinder's u side at that size is
0.00628 rad, a sagitta of `r·4.9e-6`. It can bite on small faces trimmed from large
B-spline carriers. [computed / inferred]

### Final triangles

**Final triangles are never checked against the surface.** [code]

- After cache construction, the chord tolerance appears in `SmTess.cpp` only in the
  silhouette driver. [grep]
- The ear-clipper's own comment lists "Proposed PolyEdges have chord heights within
  tolerance" among cases that "need to be extended" (`SmTess.cpp:9437-9449`).

That gap is what allows the following:

- **Annulus triangles span tested cells.** They join trim vertices to rim vertices of
  the inside quads across ON_BOUNDARY leaves that contribute no interior samples. Such
  a triangle is not a sub-element of any tested patch, and its diagonal may cross
  several leaves. Sag grows as length², so a triangle twice a leaf's span sags about
  4× more. [code + computed]
- **Ear clips carry diagonals the cell never had.** Ear clips of T-junction quads (5-
  and 6-gons) produce diagonals no cell test covered. [code]
- **Snapping.** Snapping at `:4998-5072` makes emitted quads up to about 1.25× the
  tested cell. [code + computed]
- **Thin skips.** Diagonal subdivision is skipped for thin triangles (aspect below
  0.02) and below 10× the face tolerance (`:10180-10190`). [code]

### Representation error on carriers that are not B-splines

Offset and other non-B-spline carriers are re-approximated per node by a bicubic
Hermite patch (`SmSurfaceCache.cpp:679-892`, `:2972-2987`). The pseudobox is of that
approximant, and the approximant's own error is never measured or charged. [code]

### Sphere witnesses (0.2→0.2203, 0.05→0.0574, 0.01→0.0145)

Decoded from the retained `results/cpu-2026-10-13/tess_sphere_*/mesh-witness.tsv`.
[computed]

- **Vertex placement.** Every vertex lies at dyadic parameters of the rational
  quadratic quarter-arc (`w=√2/2`).
- **Triangle shape.** Each witness triangle's long edge is the diagonal of a cell
  with a T-vertex on one side, which is the ear-clip of a 5-gon. The cells lie in the
  second column from the `u=0` seam and end on the `v=1/2` split.
- **What is ruled out.** Even the cell's own unsplit long side sags beyond ε, so a
  correctly applied pseudobox or meridian Hermite test would have split that cell. A
  cell accepted on the pseudobox path is therefore ruled out.

Leading hypotheses, none confirmed: [inferred]

1. **A notch beside the seam's boundary leaves** in the inside-quad region, filled by
   the deviation-blind ear-clipper.
2. **A leaf accepted by an untested path:** a block base node, the ≤2-split return,
   or the stale fast-path flag.

Settling it needs an instrumented build on the fleet.

### Ranked understatement mechanisms (all live at 0.005 mm, angles 0)

| # | Mechanism | Where | Confidence |
|---|---|---|---|
| 1 | Final triangles never measured; annulus and T-junction ear clips span untested regions | `SmTess.cpp:5610-5648`, `:9386`, `:9437-9449` | high [code] |
| 2 | Block path: 2D cell accepted from two boundary iso-curve 1D tests (error ≈ s_u + s_v ≲ 2ε; interior bumps ignored); base cells untested | `SmSurfaceCache.cpp:4027-4252`; `SmSurface.cpp:11625` | high [code + computed] |
| 3 | Curve Hermite e_H is an estimate from endpoint derivatives; sub-samples never test the emitted chord | `SmCurve.cpp:20932-21360` | medium-high [code] |
| 4 | Stale cache-wide fast-path flag | `SmSurfaceCache.cpp:3720`, `:4621` | medium [code; effect inferred] |
| 5 | Silent stops: min-UV, ≤2-split return, 100k cell culling, 1e-5 curve step, 100-test cap, curve-cache minimum ratio | listed above | high where they fire [code] |
| 6 | Approximation error for non-B-spline carriers not charged | `SmSurfaceCache.cpp:2972-2987` | high for offsets [code] |
| 7 | Quad-corner snapping (≈1.25× cell) | `SmTess.cpp:4998-5072` | medium [code] |
| 8 | Pole-inflated ON_BOUNDARY classification drops interior samples entirely | `SmTess.cpp:14304-14316` | high near poles [code comment] |
| 9 | Slab height ≠ distance when the patch is not a graph over the slab plane; no angle guard at 0° | `SmSurfaceCache.cpp:3583` | low-medium [inferred] |
| 10 | Linear shortcuts (IsLine ε/100, IsLinear ε/10, diagonal check ε/2) | `SmCurve.cpp:21244`; `SmSurface.cpp:11827`; `SmSurfaceCache.cpp:4068` | low [code] |

Rational weights are *not* a source of understatement on the main paths. Both the
pseudobox and the curve cache use Euclidean `Pw/w` points, which is a valid hull with
positive weights. Weights matter only through the Hermite curve estimate (row 3).
[code]

### What the sampler must know (density rules that replace the estimates above)

The common failure behind rows 1-3, 5 and 7 is that NVIDIA decides density on one
object (a tree cell, a boundary iso-curve, or a synthetic Hermite cubic) and then
emits a different one (an ear-clipped triangle spanning cells, a tensor cell, or a
chord). The correct-geometry answer is to know, at every location, the largest
triangle edge the carrier admits, and to size *every* emitted edge by it: interior,
boundary and annulus diagonals alike. NVIDIA instead sizes by UV leaf width × 1.1
(`SmTess.cpp:3218`, `:9413`).

Notation: ε = tolerance; r = radius; κ = curvature; θ = angle subtended by a chord;
L = 3D chord or edge length; R = major radius, ρ = minor radius;
M_uu, M_uv, M_vv = bounds on |S_uu|, |S_uv|, |S_vv|.

1. **Analytic carriers: exact sagitta.** These formulas are geometry, not estimates.
   - **Circle and cylinder.** A chord of length L subtending θ has sag
     `s = r(1−cos(θ/2))`. So `θ_max = 2·acos(1−ε/r)`; for small ε,
     `L_max ≈ √(8rε)`. A cylinder needs that many segments around and none along its
     ruling. Diagonals of a ruled quad sag by the same formula with the diagonal's
     projected angle.
   - **Cone.** Ruled, so no sag along a generator. At each height use the local circle
     radius, which shrinks to zero at the apex; that is the pole rule.
   - **Sphere.** Every great-circle chord sags `r(1−cos(θ/2))`. A triangle's
     worst-case sag is at its circumcentre:
     `s = r − √(r² − R_c²)`, where R_c is the triangle's circumradius. Size triangles
     so that R_c ≤ √(2rε − ε²). Choosing *cell* size instead is what lets an
     ear-clipped diagonal over-sag. The sphere witnesses are exactly an over-long
     diagonal.
   - **Torus.** Use the largest normal curvature at each point. With minor radius ρ and
     major radius R, that is max(1/ρ, |cos φ/(R+ρ cos φ)|); the radius of curvature
     is its inverse.
   - **Plane.** Zero sag, so density comes only from boundary curvature and triangle
     quality.
2. **Rational and polynomial NURBS: derivative bounds.** For a triangle or bilinear
   interpolant over a UV region with diameters (Δu, Δv), the interpolation error is at
   most `(1/8)(M_uu·Δu² + 2·M_uv·Δu·Δv + M_vv·Δv²)`. For triangles over UV this is the
   standard linear-interpolation bound, with the constant set by the circumradius. The
   M bounds come from the second hodographs of the Euclidean patch.
   - For rational patches, bound the derivatives of `S = N/W` using a positive lower
     bound on W.
   - Precompute these once per Bézier sub-patch (cached, cheap). That turns them into
     a per-location maximum edge length, which applies to annulus and boundary
     triangles too.
   - The control-net slab (NVIDIA's pseudobox) remains a useful cheap *cell-split
     proposal*. It does not size the triangles that are actually emitted.
3. **Edge curves.** Sag from the curve's own geometry:
   - analytic arcs: exact `r(1−cos(θ/2))`;
   - NURBS spans: `L²·κ_max/8` with κ_max from the second hodograph, or the Euclidean
     control-polygon distance on exact sub-spans.

   An endpoint-derivative Hermite proxy is not used. The edge sequence must also meet
   the face-side maximum edge length of every incident face (NVIDIA's 1.1× rule, but
   in 3D length terms).
4. **Representation error is geometry too.** A pcurve that is reconstructed or
   approximated, an offset approximant, and the 3D-curve-to-surface gap at boundary
   vertices all displace emitted points. The density rule must reserve that
   displacement out of ε rather than ignore it (row 6).
5. **No silent stops.** A minimum-size, iteration-cap or cell-count limit that is
   reached means the geometry knowledge was wrong or the input is degenerate. It
   becomes an explicit refusal with a reason, not an acceptance (row 5).

Test oracles (dense distance sampling against the exact carrier, the sphere and
cylinder witnesses above, authored families) then establish that these rules are
implemented correctly. They are test-time oracles, not runtime proof.

## 4. Parallelism and cost

### Threads

- The shipped build is single-threaded. `SM_USE_TBB` is never defined, so the TBB
  loops at `SmTess.cpp:11256` (face caches) and `:11436` (UV trim curves) are
  compiled out. [code, grep]
- TBB is linked, but `SmConfig.h` and the premake files define no TBB macro.
  `.agents/operations/concurrency.md:12-36` says the unit of concurrency is the
  process.
- The only unconditional parallelism is `SmApiUsdTessellate.cpp:276-292`, which runs
  `tbb::parallel_for` over BrepArrays, each with a private context, and with partial
  output allowed (`:106`). BRepGPU does not use that path. [code]

### Allocation

- Poly topology is bump-allocated from `SmPolyBrep::m_sMgr` in `calloc` blocks that
  double in size (`SmMemBlockMgr.cpp:30-77`). A PolyVertex is about 280 B
  (`SmPoly.cpp:502`).
- Tree nodes come from blocks of at most 50 (`SmTess.cpp:13111-13115`). Other
  `SmObject`s, including the per-corner auxiliary data, cost one `calloc` each
  (`SmObject.cpp:338-344`).
- `SM_USE_TBB_SCALABLE` is undefined, so allocation is plain malloc
  (`SmMemory.cpp:29-50`). [code]
- Shared vertices are found by an O(1) pointer lookup. There is no hash or spatial
  merge. The vertex spatial tree at `:9074` serves only one polygon's inside-vertex
  tests. [code]
- All face caches stay alive until each face's Phase 3 (`SmBrep.cpp:37507`), so peak
  memory holds every tree of the definition. [inferred]

### How BRepGPU drove it

Source: `native/om10_nvidia/main.cpp:134-178`, `examples/render_om10_nvidia.py:107-152`
and `results/om10-nvidia-complete-2026-10-17/heal{0,1}/`. [code/data]

- **Workers.** 12 single-threaded processes (`PXR_WORK_THREAD_LIMIT=1`,
  `OMP_NUM_THREADS=1`). Definitions are grouped by face count, largest first. Worker 0
  gets only 1808 (1,112 faces); the others get about 731 faces each.
- **Per definition, in sequence:** import → `SmApiTessellate` → f64 corner extract →
  write.
- **Measured costs (heal0):**
  - `tessellate`, summed over all definitions: **22.86 CPU-s**.
  - Busiest worker's tessellation: 2.75 s.
  - Import: 1.45 s with heal0, 15.92 s with heal1.
  - Our own plane-envelope preparation: 17.7 s, about as much as the SDK's meshing.
- **Rates:** 2.49 ms per face and 98.8 µs per triangle, about 10⁴ triangles/s/core.
  [computed]

| Def | Faces | NV tris | NV `tessellate` | Owned `faces` phase | Ratio |
|---|---:|---:|---:|---:|---:|
| 1713 | 438 | 24,078 | 1.995 s | 109.0 s | 55× |
| 1808 | 1,112 | 17,632 | 1.746 s | 20.2 s | 12× |
| 1753 | 557 | 11,776 | 1.177 s | 12.0 s | 10× |
| 1731 | 35 | — | 0.155 s | 32.2 s | 208× |
| 1762 | 94 | — | 0.178 s | 28.2 s | 159× |

Owned totals (`results/om10-watch-2026-10-17/cpu12-phases.tsv`, summed per
definition): faces 541.1, contacts 165.0 and boundary 139.9 out of 847.7 s. 1713 alone
takes 139.3 s. Our worst ratios are on definitions with under 100 faces, which points
at a fixed cost per face or per definition in the owned code, not cost per triangle.
That is the next profiling target. [computed / inferred]

### Cost model

Work per face is linear in (boundary samples + tree nodes + annulus polygon size).
Nothing spans faces except the Phase 1 retry loop. [inferred from the code read]

- **Planar faces.** With angles 0 and no max edge, a plane's pseudobox height is 0 and
  its tree is one node. The face is just its boundary polyline plus ear clipping. This
  explains the median of about 30 triangles per face. [code + inferred]
- **Curved faces.** About one pseudobox per node, i.e. 16 or more control points and a
  dot product each, plus exact rational `SplitAt`. Fast-path blocks replace recursive
  splitting with two 1D curve passes and a tensor grid. [code]
- **Output.** One `EvaluatePoint` and one `EvaluateNormal` per interior vertex.
  Boundary vertices reuse B-rep points. [code]
- **Size of the cost.** About 2.5 ms per face is roughly 10⁷ cycles. That is not cheap;
  it is cheap relative to us because nothing is verified. [inferred]

### Work NVIDIA does not do that we do

[code for criteria; absence by grep and reading]

- No error measurement on emitted triangles, and no whole-cell bound.
- No exact or interval predicates anywhere in `source/`. A grep for orient2d,
  orient3d, Shewchuk and interval arithmetic finds nothing.
- UV crossing and nesting repairs use `SM_EFF_ZERO` floats, and their errors are
  swallowed (`SmTess.cpp:4388-4404`).
- No global self-intersection or contact check. Our contacts phase alone is 165 s.
- Manifoldness is counted but not enforced.
- No distance verification. `SM_ASSERT` and `SM_ASSERT_VALID` are empty in release
  (`SmMessages.h:625`, `:714`; `SmAssertArray.h:133`).
- No per-facet source maps beyond the original-face pointer and per-edge corner
  normal/UV auxiliary data.

### Test oracles

Weak. [code / grep]

- `tests/prog_test/src/smTessTest.cpp` mostly tessellates and writes STL, with
  `SM_ASSERT` checks that compile to nothing in release.
- `TestSmBrep.cpp:1150-1200` checks exact report fields on an injected-failure box.
- `test_omni_solid.py:613-700` checks `is_manifold_solid`, that a finer setting gives
  more faces, determinism, and report dictionaries.
- Nothing checks triangle-to-surface distance, self-intersection or normal error.
  `TestFiles/pt_TestFiles/Tess/tess_20_2_80_0.smb` is never referenced.

## 5. Known failure modes

- **Non-triangle output.** When every corner scores BAD under both parameter sets,
  `TriangulateSingleFace` `break`s out of its loop and returns `SM_SUCCESS` with an
  n-gon left in place (`SmTess.cpp:9583-9668`, `:10034`). [code]
  - This matches the "unsupported nontriangle raw export" seen on the four authored
    shifted/reversed periodic inputs in the handoff. [inferred]
- **Collinear ears and duplicate triangles (1808).** The fallback score set
  `{2D, bAllowColinear=TRUE}` (`:9150-9158`, `:9349-9367`) allows clipping an ear
  whose three UV vertices are collinear. Inside-vertex tests are 2D only.
  - On a curved wall whose top edge is a straight iso-line in UV, three consecutive
    shared circle vertices are collinear in UV but form a real 3D triangle in the
    cap's plane.
  - The planar cap may clip the same three shared vertices as an ear. Shared topology
    then yields an opposite duplicate triangle, and each of its edges is glued four
    times into a spine (`SmPoly.cpp:10408-10470`, non-manifold stitching allowed).
  - This fits the 1808 diagnosis exactly: three four-use edges, opposite duplicate
    triangles on a Plane and its curved extruded wall, 12 spine halfedges reported by
    the SDK. It also fits why denser 25° curve sampling triggers it and 0/0 does not.
    [inferred, medium: mechanism read, trigger not instrumented]
- **Swallowed trim-repair errors.** Crossing linearised UV trims can proceed into
  triangulation and give folded or overlapping triangles (`:4388-4404`). [code]
- **Inside-out triangles next to fillets** (`KNOWN_ISSUES.md:45-49`), avoided by a 15°
  surface angle. All triangulation decisions are 2D in UV (`bAnglesIn3d=FALSE` in both
  sets), so a fold that is valid in UV but reversed in 3D is not detected.
  [code + inferred]
- **Pole classification collapse.** Pole-inflated UV tolerances can mark every leaf
  ON_BOUNDARY, leaving no interior samples and ignoring the tolerance
  (`:14304-14316`). Several pole and seam healer bugs are listed separately
  (`KNOWN_ISSUES.md:22-37`). [code]
- **Foreign UV trims** from OCCT-based exporters can tessellate incorrectly
  (`KNOWN_ISSUES.md:67-69`).
- **Failures not raised.** Non-manifold output, chord violations and the 100-pass
  retry exhaustion (`SmTess.cpp:11810-11822`) are the only signals. `SmApiTessellate`
  fails only on recorded face failures, and BRepGPU's physical-shell checker is what
  caught 1808.
- **Advancing front is not a fallback.** One 1808 call was censored at 180 s with no
  result (`REPORT.md:91-96`).

## 6. What to learn, and what to do differently

### Learn (adopt the ideas, own the code)

1. **One canonical boundary vertex sequence per physical edge, shared by identity.**
   Crack-freedom then becomes a topological invariant, not a welding problem. The
   reference does it by mutating a copied B-rep; we already keep this in source-keyed
   form. Keep it, and keep it O(1) per lookup.
2. **Surface-driven edge refinement.** Boundary samples are refined to match the
   adjacent faces' interior cell sizes (the 1.1× leaf rule). That keeps annulus
   triangles well-shaped and cheap.
3. **Interior as a quadtree-like conforming quad mesh.** Inside leaves become quads,
   with hanging nodes promoted to polygon vertices. Only a thin boundary annulus needs
   general triangulation. That is the main reason planar and simply-curved faces cost
   almost nothing, and it maps naturally to GPU (cells → quads by scan).
4. **Cheap hull-based slab tests on exact rational sub-patches** using Euclidean
   control points.
5. **Per-face transient caches freed immediately.**
6. **Embarrassingly parallel per-definition (and per-face) work** with no cross-face
   synchronisation after boundary sampling. NVIDIA wins on throughput with 12
   unshared processes; we should win with shared-memory tasks, which do not need
   process isolation.

### Do differently, given the fidelity definition

The fidelity definition: deviation actually within tolerance; watertight; manifold;
no self-intersections; consistently oriented; no slivers; small features survive;
sharp edges stay sharp; stunning shading.

1. **Size every emitted edge by the carrier's local admissible length.** Use the
   density rules in section 3: exact sagitta for analytic carriers, second-derivative
   bounds for NURBS, circumradius rules for doubly curved surfaces. That covers
   interior, boundary and annulus/T-junction diagonals alike. NVIDIA's gap is sizing
   cells and then emitting other triangles.
2. **No 1D-proxy density for 2D cells.** The block fast path's speed idea (a tensor
   grid from iso-curve splits) is good. Its grid spacing must come from the 2D rule,
   with both directions and the twist term, not from two boundary iso-curves.
3. **Curve sampling from the curve's actual geometry.** Exact arc formulas, or
   hodograph curvature bounds on exact sub-spans. No endpoint-Hermite proxy.
4. **Correct orientation geometry in triangulation.** UV orientation and in-triangle
   decisions must not return the wrong sign; use robust floating-point orientation
   tests. Add a 3D orientation check of each new triangle against the carrier normal
   at its UV centroid.
   - There must be no "clip anyway", no collinear-ear clipping across a curved
     iso-boundary, and no "leave an n-gon" path. A face that cannot be triangulated
     must fail loudly.
   - This removes, by construction, the 1808-class duplicate triangles and the
     KNOWN_ISSUES inside-out class.
5. **Never swallow trim-linearisation crossing errors.** Refine the shared edge, or
   refuse.
6. **Count edge uses in the hot path.** Count two-use edges and closed-shell Euler over
   shared keys; it is O(n). Global self-intersection and contact checking belong in
   test oracles, not in the per-run pipeline. That is where our 165 s contacts phase
   goes if we want NVIDIA-class speed.
7. **Reserve representation displacement out of ε.** This covers pcurve
   reconstruction, non-B-spline approximants, and the edge-to-surface gap carried by
   shared 3D curve points into each face's boundary strip.
8. **Sliver control and shading.**
   - NVIDIA skips diagonal refinement for thin triangles (aspect below 0.02). We
     should instead refine or flip to meet a minimum-angle target in the annulus.
   - Keep per-face corner normals (sharp edges stay sharp, as NVIDIA does) and
     evaluate them from the exact carrier at each corner's UV.
9. **Incremental remeshing.** NVIDIA's Phase 1 retry rebuilds *all* faces whenever one
   edge splits (`SmTess.cpp:11826-11831`). Keyed shared edges let us invalidate only
   incident faces. That is necessary for the "add a complication" roadmap.
10. **Performance target.** NVIDIA's whole watch tessellation is 22.9 CPU-seconds,
    about 2.5 ms per face, with no verification. To beat 6-8 s wall, our hot path must
    be:
    - O(boundary + cells) per face;
    - density taken from cached per-patch geometry (radii, hodograph bounds), not
      from sampled searches or runtime proofs;
    - parallel across faces after boundary sampling;
    - free of per-definition fixed overheads. The 1731/1762 ratios above 150× point
      there first.
