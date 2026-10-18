# Shading fidelity delivered to the consumer

Research lane M4-G13 of the fast-mesher campaign, 2026-10-17. Briefs:
[M4-common](../docs/fast-mesher-campaign/briefs/M4-common.md) and
[M4-G13](../docs/fast-mesher-campaign/briefs/M4-G13.md). Stub:
[lifecycle map](2026-10-17-geometry-lifecycle-map.md) §4.1 G13 (stages S12, S13;
contradictions X5, X15). Bead `brepgpu-12s.14`.

This paper builds on the lifecycle map's S12/S13 rows, the
[fast-tessellation literature](2026-10-17-fast-tessellation-literature.md) §1.5,
the [healing paper](2026-10-17-tolerant-geometry-healing.md) §7.2 continuity
rule, the [M2 paper](2026-10-17-neighbour-reconciliation.md) §3.4 ("what an eye
checks") and the [NVIDIA SmTess study](2026-10-17-nvidia-smtess-study.md). It
does not redo them. The only computation was a small pure-Python probe on the
fleet host (§5). Nothing was built, meshed or rendered. Sources and read depth
are in [2026-10-17-shading-fidelity-sources.tsv](2026-10-17-shading-fidelity-sources.tsv).

Tags: **[src]** a source establishes it; **[code]** read at file:line at a
pinned revision; **[own]** this lane's derivation; **[probe]** measured by the
lane's probe (§5); **[call]** a choice this lane makes, open to override.

## 0. The three rules

**Rule A: cap and fold rule (replaces design §4.7 "caps above about 150°").**
For each emitted triangle T, compute its facet normal n_T (oriented by face
sense) and θ(T) = max over its three corners of ∠(n_T, n_i), where n_i is the
exact surface normal at that corner. Let R and r be T's 3D circumradius and
min-containment radius, and κ(T) the curvature bound the density field already
carries for T's region.

- **A0 fold.** θ(T) < 90°. Otherwise refine or refuse, as design §5.3's
  orientation rule already requires.
- **A1 cap.** If R > c·r and θ(T) > max(√(2ε·κ(T)), 10⁻⁹ rad), repair the cap
  by interior means: move or delete an interior apex, or insert an interior
  Steiner point. Never split a shared chain segment. Default c = 2, which is a
  largest angle of 150°. An arena flag offers c ∈ {1.155, 1.414, 2, off}. A cap
  that cannot be repaired is recorded `valid_goal_unmet`. It is never a refusal,
  because the criterion is shading only.
- **A2 display bound.** Optional and off by default: θ(T) ≤ θ_display (an
  absolute angle). It is offered for callers whose consumer shades with facet
  normals.

Basis:

- Facet-normal error grows with R at fixed curvature [src Khoury–Shewchuk 2019
  eq. 3–4], while chord error grows with r [src same, eq. 1–2]. R/r = 1/sin α_max
  for obtuse triangles.
- On a sphere, the facet error of a chord-limited triangle is exactly
  √(2ε/ρ)·R/r [probe].
- Acute chord-limited triangles already reach up to 2.06× that reference on
  saddles [probe]. So a c = 2 cap is no worse than what the chord rule already
  admits elsewhere.
- The θ gate leaves harmless caps alone: caps on planes (θ = 0), caps along
  cylinder rulings and small caps near dense trims.
- Choosing 2 rather than another factor is a call.

**Rule B: G1 rule (replaces design §4.7 "G1 edges may share a normal" and §5.2
item 6).**

- **Normals.** Every corner of every face occurrence carries that face's exact
  normal at its own UV. Normals are never averaged across faces, at any
  dihedral angle. At singular parameters (poles, apex, collapsed edges), each
  corner carries the one-sided limit normal along its own triangle. The rendered
  normal jump at every edge sample then equals the true jump θ. This makes the
  tangent/crease band irrelevant to shading and handles vanishing creases
  per sample.
- **Classification** feeds records, data quality and feature lines, not normals.
  It is measured at every resolved edge sample, because STEP encodes no edge
  regularity. Defaults:
  - **smooth:** max θ ≤ 0.01° (JAMA G-SH-NT);
  - **crease:** min θ ≥ 0.2° (JAMA crease intent);
  - **vanishing:** θ runs between those two along the edge (JAMA's own
    exception);
  - **defect band:** anything else, reported as a G-SH-NT data-quality finding;
  - **sharp for feature lines:** θ ≥ 1° (CAx-IF GVP §6.2) or ≥ 3 face uses.

  The thresholds are profile data (D-profile).

**Rule C: per-format normal policy (D-formats).**

- **glTF.**
  - Always write NORMAL: FLOAT VEC3, one exact normal per corner per face
    occurrence, normalized in binary64 and then rounded.
  - No byte-quantized normals by default; SHORT-normalized normals only as an
    opt-in.
  - No TANGENT.
  - Reason: a glTF without normals MUST be flat-shaded by the client [src glTF
    2.0 §3.7.2.1].
- **STL.** The facet normal field holds the unit winding normal of the
  float32-rounded triangle. Consumers recompute or replace it anyway [code
  admesh, Uranium].
- **3MF.** Carries no normals. Consumers flat-shade [code Cura, PrusaSlicer].
- **Callers who want stunning visuals** take glTF. For flat-shading consumers,
  A2 is the only lever.

## 1. Question 1: chord error, triangle angle, normal error and visible artefacts

### 1.1 What each error controls

| Error | Governs | Visible as | Consumers affected |
|---|---|---|---|
| Position (chord) ε | r² (min-containment radius squared) [src K–S eq. 1–2; Waldron via Shewchuk 2002 Table 2] | silhouette and contour polygonisation; cracks; sub-pixel position error with correct normals is invisible [src M2 §3.4: Yeo 2012, Claux 2014] | all |
| Facet normal θ(T) | R (circumradius) [src K–S eq. 3–4; Shewchuk 2002 Table 2 gradient bounds] | faceting under flat shading; shadow-terminator step; corrupted normals rebuilt from facets | STL/3MF consumers (flat), glTF clients when NORMAL is absent, ray tracers (terminator), mesh libraries that rebuild normals |
| Interpolated shading normal | second order in r on non-"shading-exact" carriers [own, probe] | highlight and reflection-line displacement and kinks | glTF clients with NORMAL |
| True crease θ at an edge | the geometry itself | a positional break in highlight lines [src Beier–Chen Property 2] | all shaded consumers |

### 1.2 Facet normals

- **Khoury–Shewchuk 2019 [src].** For a triangle with vertices on Σ,
  circumradius R and plane angle φ at vertex v:
  ∠(n_T, n_v) ≤ arcsin((R/ebs(v))·max(cot(φ/2), 1)) (eq. 3). At the
  largest-angle vertex this gives arcsin(√3·R/ebs(v)) (eq. 4).
  - The worst case of eq. 4 is the equilateral triangle, not the cap (§2).
  - Normal error is "linearly proportional to the triangle's circumradius";
    "for triangles with a fixed longest edge length, the worst normal errors are
    suffered by triangles with angles close to 180°" (§1).
  - Interpolation error, by contrast, is "largely insensitive to the shape",
    proportional to r² (eq. 1).
  - The extended lemma bounds the facet normal against the normal at every
    point's nearest surface point (§6).
- **Shewchuk 2002 [src].**
  - The gradient error of linear interpolation lies between c_t·R_circ (lower
    bound) and 3c_t·R_circ (weak upper bound) (Table 2).
  - "angles near 180° are harmful, whereas angles near zero are, by themselves,
    benign".
  - A paraboloid rendered with large-angle triangles "looks like a washboard"
    (Fig. 2).
  - 2D Delaunay minimises the maximum circumradius. So the band CDT in UV
    already minimises R, insofar as the chart is near-isometric.
- **Probe [probe, §5].** These are chord-limited triangles (max chord deviation
  = ε) on seven exact carriers, at ε/ρ_min = 10⁻³ and 10⁻⁴ (5 µm on ρ = 5 mm
  and 50 mm), largest angle 60–165°, six in-plane rotations. Facet error is
  shown at ε/ρ = 10⁻³ (×0.316 at 10⁻⁴):

  | carrier | 60° | 90° | 120° | 150° | 165° |
  |---|---|---|---|---|---|
  | sphere | 2.58° | 2.56° | 2.96° | 5.11° | 9.81° |
  | cylinder | 2.67° | 3.00° | 3.72° | 5.76° | 9.96° |
  | cone 30° | 2.68° | 3.03° | 3.74° | 5.77° | 9.94° |
  | torus outer | 2.57° | 2.74° | 3.45° | 5.56° | 9.92° |
  | torus inner (saddle) | 3.66° | 3.94° | 5.31° | 7.08° | 10.04° |
  | saddle height field | 3.91° | 5.27° | 5.54° | 7.00° | 10.26° |
  | varying-curvature height field | 2.60° | 2.73° | 3.38° | 5.50° | 9.92° |

  Findings from the probe:
  - The reference θ_ref = √(2ε/ρ) is exactly the sphere's acute value (ratio
    1.00–1.01).
  - On the sphere, θ/θ_ref equals R/r: 1.15, 1.41, 2.00 and 3.83–3.86 at 120°,
    135°, 150° and 165°.
  - Across all carriers, acute triangles reach 1.00–2.06× θ_ref. Saddles allow
    larger triangles at a given ε, because opposite curvatures cancel in the
    chord.
  - The interior facet error (against the normal at each grid point's nearest
    surface point) never exceeded the vertex error.
  - Eq. 4, with ebs replaced by the local curvature radius, held in all 504
    rows, at 1.19–28× the measured value.
  - A cylinder needle along the ruling (20× the chord length) has θ =
    Δθ_max/2 = θ_ref (2.56° at 10⁻³), while eq. 4 gives 90°. The bound is
    isotropic and very loose on anisotropic carriers. A rule must therefore
    measure θ(T) directly rather than bound it from R (Rule A).
- **Scale of the problem [own].** At chord limit, θ ≈ √(2ε/ρ):
  - ε = 5 µm: 5.7° at ρ = 1 mm, 1.8° at 10 mm, 0.57° at 100 mm.
  - Every chord-limited mesh therefore looks faceted under flat shading at watch
    scale. This includes NVIDIA's, which uses chord 0.005 and angle 0
    [src SmTess study].
  - Capping θ at an absolute θ_f multiplies the triangle count by about
    (θ_ref/θ_f)² = 2εκ/θ_f². For θ_f = 1° that is ×33 at ρ = 1 mm and ×3.3 at
    ρ = 10 mm.
  - This is why A2 is off by default.

### 1.3 Interpolated shading normals

- **Shading-exact carriers [own].** Let n(p) = (p − A(p))/ρ, with A affine:
  A(p) = centre for a sphere, the axis projection for a cylinder, constant n for
  a plane.
  - For x = Σ bᵢpᵢ on the flat triangle, Σ bᵢnᵢ = (x − A(x))/ρ, because A is
    affine and Σ bᵢ = 1.
  - Normalised, this is exactly the normal at x's nearest surface point x̃.
  - So Phong-interpolated exact vertex normals reproduce the true normal field
    exactly on planes, cylinders and spheres, whatever the triangle shape.
  - The probe confirms it: ≤ 4·10⁻¹⁶° on sphere and cylinder in every row
    [probe].
  - Cones and tori break the affine form: the cone normal is tilted, and the
    torus core point is non-linear.
- **Other carriers [probe].** These are the largest interpolated-normal errors.
  The figure in parentheses is the position-equivalent error δ/κ in units of ε
  (§1.4):

  | carrier (ε/ρ = 10⁻³) | 60° | 120° | 150° |
  |---|---|---|---|
  | cone 30° | 0.052° (0.9ε) | 0.13° (2.2ε) | 0.29° (5.0ε) |
  | torus outer | 3.4·10⁻⁴° (0.006ε) | 7.3·10⁻⁴° | 3.4·10⁻⁴° |
  | torus inner | 0.0023° (0.04ε) | 0.0052° | 0.0084° (0.15ε) |
  | saddle height field | 0.0015° (0.03ε) | 0.0030° | 0.0025° |
  | varying-curvature height field | 0.034° (0.6ε) | 0.034° | 0.034° |

  - At ε/ρ = 10⁻⁴ every value scales by about 0.1.
  - Interpolated-normal error is two to five orders of magnitude below facet
    error.
  - It is shape-sensitive only on the cone, where caps raise it to 5ε. That is
    a second, independent reason for A1 on non-shading-exact carriers.
  - The varying-curvature value sits at the midpoint of the edge along the
    curvature gradient and does not depend on shape [probe; checked with a
    location dump].

### 1.4 Reflection lines and highlights

- **Guthe–Balázs–Klein 2006 [src].**
  - Visual error is the geometric error combined orthogonally with ε_norm, the
    distance to the nearest surface point that has the interpolated normal:
    ε² = ε_geom² + ε_norm² (§4).
  - Geometric-only GPU tessellation of trimmed NURBS averaged 0.93° normal
    deviation per pixel; their normal-preserving method averaged 0.75° (Table 3).
  - Under geometric-only tessellation "the real discontinuity … is
    indistinguishable from tessellation related" ones in reflection lines
    (Fig. 10).
  - "Even a slight normal deviation of a few degree can lead to visual artifacts
    that are indistinguishable from real surface discontinuities" (§6.3).
  - Most of their normal error came from bicubic degree reduction. A mesher that
    evaluates the exact normal at every corner (design §4.7, NVIDIA SmTess
    `EvaluateNormal(uv)` [src SmTess study]) does not have that error.
- **Beier–Chen 1994 [src].** A tangent discontinuity becomes a positional break
  in highlight lines; a curvature discontinuity becomes a tangent break
  (Property 2).
  - Phong shading makes the shading normal C⁰ but not C¹ across triangle edges.
  - So on non-shading-exact carriers, highlight lines kink at mesh edges, as on a
    G1 surface with curvature jumps [own, from Property 2].
- **Displacement [own].** A highlight line is where the extended normal meets a
  line light at distance d.
  - A normal error δ transverse to the light moves the highlight on the surface
    by Δs ≈ d·δ/(1 + d·κ), with κ the transverse normal curvature (small
    angles).
  - For d ≫ 1/κ this is δ/κ, which is Guthe's ε_norm.
  - For a plane it is d·δ, amplified by the light distance; but plane normals
    are exact.
  - Reflection lines see the view direction too and double the angular
    sensitivity.
  - On shading-exact carriers, reflection and highlight lines are the true ones,
    each point displaced radially by at most ε. Tessellation leaves no
    shading signature there except silhouettes.
- **Quantitatively [own, probe].** With exact vertex normals, Guthe's visual
  error at chord limit is:
  - √(1 + 0.6²)·ε ≈ 1.17ε on the varying-curvature field;
  - about 1.35ε on acute cone triangles;
  - about 5.1ε on 150° cone caps.

  Flat shading at the same triangles gives ε_norm = θ/κ ≈ 45ε at ε/ρ = 10⁻³.

### 1.5 Shadow terminator

- **Mechanism [src].**
  - Ray-traced shadows from flat facets cut off light where the geometric
    normal turns from the light, while the shading normal still faces it.
    Coarse meshes with vertex normals therefore show a stepped terminator
    [src Hanika 2021 §4.1, Fig. 4-3].
  - "With shading normals … bent towards the light, brightness does not reach
    zero at the shadow terminator, resulting in a hard cutoff" [src Chiang–Li–
    Burley 2019 Fig. 2].
  - In Deshmukh–Green's framework, light is culled at x + y ≥ π/2, where x is the
    angle between shading and geometric normal and y the angle between light and
    shading normal [src Deshmukh–Green 2020 §2].
- **Step size [own from src].** At the cut, Lambert brightness relative to the
  shading normal is cos y = sin x.
  - The jump is therefore sin x of peak diffuse brightness.
  - The interpolated normal is a normalised convex combination of the corner
    normals, and the cone of directions within angle δ of n_T is convex. So
    x ≤ θ(T) everywhere in T [own].
  - At chord limit, sin θ(T) is 4.5% (equilateral) to 10% (150°) of peak at
    ε/ρ = 10⁻³, and 1.4–2.8% at 10⁻⁴ [probe].
- **Remedies are consumer-side.**
  - Shift the shading point onto a Phong-tessellation-like surface [src Hanika
    §4.3, Listing 4-1].
  - Add a shadowing factor [src Chiang 2019 eq. 1–2; Deshmukh–Green 2020 eq. 3].
  - "The third approach is the obvious choice: … tessellating more finely"
    [src Hanika §4.2].
  - A producer controls only θ(T): A1 bounds it relative to θ_ref, and A2
    absolutely.

### 1.6 What the literature does not say

- No source read gives a perceptual threshold in degrees for a visible crease,
  shading jump or terminator step, or a pixel threshold for a visible kink
  [src M2 §3.4 found the same].
- Guthe uses a 0.5-pixel screen-space error as an engineering target, not a
  perceptual measurement [src Guthe §6].
- The measurement that would settle it is in §5.3.

## 2. Question 2: the cap rule and its basis

Rule A (§0) is the answer. Here is why each part takes the form it does.

- **Why θ(T) and not the angle alone.**
  - The bound is isotropic. The cylinder ruling needle and the plane show that
    caps can be exact or nearly so [probe; §1.2].
  - The draft's pure 150° rule would add Steiner points to planar caps, which
    planar constrained triangulations produce in quantity, for no shading gain:
    the facet normal of a planar triangle is exact.
  - θ(T) costs one cross product and three dot products against corner normals
    that are already computed.
- **Why gated by θ_ref = √(2εκ).** That value is the facet error a
  non-obtuse chord-limited triangle has on a sphere of the same curvature
  [probe]. Below it a cap costs nothing visible beyond what the chord rule
  already implies.
- **Why c = 2 [call, with basis].**
  - At c = 2 a cap has at most twice the reference error on spheres, and
    2.25–2.76× on cones and saddles [probe].
  - That sits inside the 1.0–2.06× that acute triangles already reach on saddles.
  - The draft's 150° is kept, so there is no triangle-count regression against
    it.
  - Tighter c (1.155 ↔ 120°, 1.414 ↔ 135°) are arena flags. Their cost is
    unmeasured because no mesher of the new design exists.
- **Cone exception.** Caps raise interpolated-normal error on cones to 5ε at
  150° [probe]. A1 already refines those, because their θ/θ_ref is 2.25.
- **Repair, not refusal.** Band refinement must not insert points on the shared
  chain (design §5.3). A cap whose long edge is a chain segment is repaired
  through its apex: delete or move the interior apex, or insert an interior
  point that splits the cap's opposite angle. Otherwise it is recorded
  `valid_goal_unmet`.
- **A0** is the design's fold check. It is stated against corner normals,
  because that is the same quantity.

## 3. Question 3: G1 classification without encoded regularity

### 3.1 What the file carries

- **STEP encodes no regularity between faces [code].**
  - `edge_curve` has `edge_geometry` and `same_sense` only
    (`StepShape_EdgeCurve.hxx:35-47`, OCCT 8.0.1 b8f597c).
  - Continuity codes exist only inside composite curves and rectangular
    composite surfaces (`StepGeom_TransitionCode.hxx:20-25`; used by
    `StepGeom_CompositeCurveSegment`, `StepGeom_SurfacePatch`).
- **The "encoded regularity" OCCT has after reading is computed by OCCT
  [code].**
  - `XSControl_TransferReader::ShapeResult` runs `ShapeFix::EncodeRegularity`
    with `read.encoderegularity.angle` (`XSControl_TransferReader.cxx:446-452`).
  - The default is 0.01 rad, i.e. 0.573° (`Interface_StaticStandards.cxx:51`;
    documented in `DESTEP_ConfigurationNode.cxx:279-288`).
  - `BRepLib::ContinuityOfFaces` samples 21 parameters and classifies G1 when the
    cross-edge tangent derivatives are parallel within that angle
    (`BRepLib.cxx:2197-2420`).
  - OCCT's G1 tolerance (0.573°) is almost three times JAMA's crease threshold
    (0.2°).
  - Our adapter does not read `BRep_Tool::Continuity` at all
    (`native/step_source/main.cpp`, no match).
  - "Follow the source's encoded regularity" (design §5.2 item 6; healing paper
    §7.2) therefore has nothing to follow for STEP.
- **Correction to the healing paper §7.2.** STEP `edge_curve.same_sense` is
  curve orientation, not regularity data.

### 3.2 Thresholds the standards give

- **JAMA V5.1 §3.2.28 G-SH-NT [src].**
  - Δθ is the largest angle between the normals of adjacent faces along their
    shared edge.
  - Recommended: Δθ ≤ 0.01° where the faces are intended tangent, and Δθ ≥ 0.2°
    where a crease is intended, "except the range where a crease gradually
    changes to tangency" (但し、折れから接に除変している範囲は除く).
  - Stated reasons: design quality, offsets that separate, and CAM line
    machining gouging or leaving material.
  - Companies relax it: Isuzu 0.5°, Nissan 3° [src JAMA 2025, via the healing
    paper §3.1].
- **CAx-IF GVP v4.6 §6.2 [src].** An edge is sharp if it is non-manifold, or
  manifold with any non-tangent normal angle in its active region. "a 1.0
  degree limit seems to be practical"; too low a threshold classifies edges
  sharp "which the target system would consider smooth".

### 3.3 Decision

- **Shading needs no classification.** Rule B emits exact per-face normals, so
  the rendered jump equals the true θ at every sample [own].
  - A 0.05° defect-band edge renders as the true 0.05° step.
  - A vanishing crease renders correctly along its whole length.
  - A per-edge binary choice can do neither.
  - Averaging normals across a band edge would instead create a
    tessellation-dependent error of up to θ/2, tapering over one triangle row.
    That is exactly Guthe's "tessellation related" discontinuity.
  - NVIDIA does the same: "each face keeps its own normal at shared vertices"
    [src SmTess study; code `usd-brep tools/smtess_usd/src/smtess_usd_main.cpp:85-133`].
  - OCCT's mesh export takes per-face normals from the triangulation or the
    surface (`RWMesh_FaceIterator.cxx:56-77`).
  - OCCT's cross-face averaging, `BRepLib::EnsureNormalConsistency` (0.001 rad,
    per edge node, `BRepLib.cxx:2597-2720`), is called only from a Draw test
    command (`MeshTest.cxx:1693`).
- **Classification is measured** at every resolved edge sample, using the exact
  normals of the two faces at the shared position (the owner position for R4).
  The design already records θ there (§5.2 item 2). Classes are as listed in
  §0 Rule B. The JAMA defaults seed the data-quality report (D-profile), and the
  CAx-IF 1° threshold marks feature lines.
- **The band (0.01°–0.2°)** is a data-quality finding, never a shading choice.
  Whether to also hide such defects visually is an operator taste question
  (§8).
- **Welding.** glTF face ids are a per-vertex attribute, so a vertex cannot
  carry two faces' ids. Vertices are therefore duplicated per face occurrence
  in any case. EXT_mesh_manifold's `mergeIndices` restores connectivity
  [src EXT_mesh_manifold README]. A welded, id-free variant could share one
  vertex at smooth edges with the R4 owner's normal (change ≤ 0.01°). It is
  off and not needed for alpha [call].
- **Singular parameters.** The owned display code already evaluates u-dependent
  limit normals at sphere and cone poles (`native/om10/math.hpp:130-150`).
  - Freeform collapsed edges still fail there (`unit(normal)` returns false).
  - M5 must specify their limit normal [own: along a collapsed v-edge where
    S_u = 0, the limit direction is S_v × S_uv]. Until then they are an explicit
    refusal, not a zero normal.
  - Compare OCCT, which writes a zero normal when it cannot find one
    (`BRepLib.cxx:2634-2640`).

## 4. Question 4: per-format normal policy and normal-less consumers

### 4.1 What consumers do [code at pinned revisions]

| Consumer | Input | What happens to normals |
|---|---|---|
| glTF clients | glTF 2.0 | "When normals are not specified, client implementations MUST calculate flat normals" (spec §3.7.2.1). NORMAL is "Normalized XYZ", float VEC3. three.js sets `flatShading` when the attribute is absent (`GLTFLoader.js:3550-3609`). Filament gltfio generates flat normals (`AssetLoader.cpp`, `ResourceLoader.cpp` comments; code search only, not read in full). |
| PrusaSlicer | STL, 3MF | admesh replaces any stored facet normal that differs from the winding normal by more than 0.001 per component (`admesh/normals.cpp:69-110, 222-226`). The render geometry gives every triangle its face normal (`GeometryBuilder.cpp:68-86`): flat. |
| Cura / Uranium | STL, 3MF | Both readers call `calculateNormals(fast=True)` (Uranium `STLReader.py:36`; Cura `ThreeMFReader.py:149`). That computes cross products per triangle (`MeshData.py:599-625`): flat. |
| fTetWild | mesh files | Reads points and faces only (`MeshIO.cpp:219-277`). Normals are irrelevant. |
| Mesh libraries rebuilding normals | any | Crease-angle averaging: libigl `per_corner_normals` threshold (`per_corner_normals.h:16-24`); Manifold `CalculateNormals(minSharpAngle = 52.5)` (`manifold.h:257`). These smooth real creases below their threshold and cannot represent a vanishing crease. |

The D-consumers set (PrusaSlicer, Cura, a glTF renderer, fTetWild) therefore
splits cleanly:

- Slicers flat-shade whatever they receive. Their previews show facet error,
  about √(2ε/ρ), as for every mesher.
- fTetWild ignores normals.
- Only the glTF renderer can show the shading this design computes.

### 4.2 Policy

- **glTF (alpha).**
  - NORMAL on every triangle primitive, never omitted.
  - FLOAT VEC3, one exact normal per corner per face occurrence: computed in
    binary64, normalised, rounded. Float32 rounding gives about 10⁻⁷ rad.
  - Validator unit-length tolerance is 0.00674 (`glTF-Validator
    lib/src/errors.dart`, via code search).
  - No KHR_mesh_quantization normals by default [own]. BYTE-normalized normals
    (allowed by the extension's table) err by up to about √2/254 rad ≈ 0.32°,
    comparable to the shading errors this design removes. SHORT-normalized
    normals (≈ 0.0012°) are an opt-in.
  - No TANGENT, because there are no normal textures.
  - Mirrored placements rely on the determinant winding rule (spec §3.7.4).
    Placement policy belongs to G9/G3.
- **STL (alpha).**
  - Write the unit winding normal of the float32-rounded triangle. It is what
    both slicers recompute or keep.
  - Writing an exact surface normal there would be replaced by admesh or ignored
    by Cura.
  - A degenerate rounded triangle is G3's problem.
- **3MF (alpha).** No normals exist in the core spec. Nothing to write.
- **Optional A2.** When a caller declares a flat-shading consumer and wants it
  to look smooth, A2 sizes triangles by θ_display, at the cost in §1.2.
- **Later formats.**
  - USD: normals as a faceVarying primvar with `subdivisionScheme = none`
    [src lifecycle map S13].
  - OBJ: `vn` per corner over welded positions.
  - PLY: per vertex, with vertices duplicated per face.
  - AP242 tessellated: per-point normals through `pnindex` (point, normal)
    couples [src CAx-IF 3D tessellated RP v1.1 §5]. Its `angular_deviation`
    accuracy parameter is "not tested in the MBx-IF yet" [src same §4].

## 5. Question 5: measurement

### 5.1 Probe

- **What it is.** `lanes/M4-G13/shading_probe.py` under the fleet BRepGPU root.
  Pure Python with only the `math` module (the fleet Python has no numpy) and no
  repository code. SHA-256
  `e28699b000534e14456e72ddce8cd3768436929793a4c62eb54b9833e8a1a4a7`. Output
  `probe.tsv`, 507 lines, SHA-256
  `8e4c0d692abf64d133eb5fae4578a6c65aed5069e23395f08efa7b25d234265f`.
- **Carriers**, each with κ_max = 1:
  - sphere, cylinder, cone of half-angle 30° (circumferential κ = 1 at the base
    point);
  - torus R = 3, r = 1 at the outer and inner equator (the inner is a saddle);
  - height fields ½(x² − y²) and ½(x² + 0.3y²) + 0.1x³.

  Nearest points are closed-form, or Newton for the height fields.
- **Triangles.**
  - Isosceles with largest angle α ∈ {60, 90, 120, 135, 150, 165}°.
  - Six in-plane rotations, placed in the tangent plane and lifted to the
    surface by nearest-point projection.
  - Scaled by 70-step bisection until the maximum chord deviation on a
    66-point barycentric grid equals ε.
  - Measured on a 153-point grid, together with a cylinder ruling needle.
- **Outputs per triangle:**
  - R/r;
  - facet-to-vertex and facet-to-nearest-point normal angles;
  - the K–S eq. 4 value;
  - θ_ref;
  - normalised-barycentric normal versus exact normal at the nearest point;
  - sin θ.
- **Oracle.**
  - The closed-form normal of each carrier, a model oracle.
  - The probe also tests eq. 4 as a falsifiable claim. It held in all rows.
  - It tests the shading-exact derivation of §1.3, which held to FP round-off.
- **Run context.**
  - One cold process pinned to CPU 2 (socket 0); 31.16 s wall, 11.5 MB RSS.
  - Load average 11.92/9.79/6.20 with other M4 lanes running; governor
    `schedutil`; sampled 3.00–3.15 GHz; SMT on; AutoNUMA 1.
  - Its timing carries no claim.

### 5.2 Renders not run, and why

- The repository renderer (`native/om10`) is an orthographic rasterizer:
  - Lambert and Blinn shading with interpolated corner normals
    (`math.hpp:278-298`; `main.cpp:357-361`);
  - no environment, reflection-line or shadow-ray shading.
- A reflection-line render needs a new shader and meshes of authored families,
  but the meshers that produced display meshes are retired by operator decision.
- That is not cheap, and the quantitative relations the brief asks for came
  out of the closed-form probe instead.

### 5.3 The measurement that settles the open thresholds

What remains open is perceptual: whether c = 2 caps, sin θ terminator steps
and defect-band creases are visible. These renders would settle it:

- **Families, once the new mesher exists:**
  - sphere, cylinder, cone, torus, a saddle and a varying-curvature B-spline;
  - a plane-to-cylinder blend;
  - crease-band edges at θ = 0.005°, 0.05°, 0.2° and 1°, plus a vanishing crease.
- **Settings:** ε/ρ ∈ {10⁻³, 10⁻⁴}; arena c ∈ {1.155, 1.414, 2, off}.
- **Renders:**
  - zebra (line-array environment) and a shadow-ray terminator;
  - each against a reference with exact per-pixel normals (ray-cast to the
    carrier).
- **Metric:** Guthe's per-pixel normal-deviation image and the fraction of
  foreground pixels above 0.5 px of equivalent error [src Guthe §6.2].
- **Plus** an operator viewing of the paired images for the visibility call.

## 6. Hypotheses and falsifiers

| # | Hypothesis | Falsifier | Mechanism |
|---|---|---|---|
| H1 | Exact corner normals give interpolated-normal error 0 on planes, cylinders and spheres | Any oracle sample above 10⁻¹² rad on those carriers (probe max 7·10⁻¹⁸ rad) | Whole-output normal oracle: interpolated corner normals against the exact normal at the nearest carrier point |
| H2 | Facet error θ(T) ≤ arcsin(√3·R·κ(T)) whenever R·κ ≤ 1/√3 and T is small against proximity | One emitted triangle above it means a geometry, κ-bound or oracle defect | Same oracle, per triangle |
| H3 | A1 at c = 2 raises triangle count by ≤ 2% on the corpus families against A1 off, and caps max θ/θ_ref at ≤ 2.1 outside planes [call: 2%] | Arena count increase above 2%, or any triangle above 2.1 θ_ref | Arena flag ablation |
| H4 | Written glTF normals reproduce the source dihedral at every edge sample | \|∠(n_glTF,f1, n_glTF,f2) − θ_source\| > 10⁻⁵ rad at any sample | Written-bytes oracle after float32 re-read |
| H5 | Every written glTF triangle primitive has NORMAL, unit within 10⁻⁶, within 10⁻⁶ rad of the exact corner normal | Any primitive without NORMAL, or any corner outside | Written-bytes oracle |
| H6 | Facet error at chord limit is √(2ε/ρ)·[1.0, 2.1] for acute triangles on all carrier kinds | A corpus carrier kind measured outside that band | Arena record per carrier kind |

## 7. Consequences for M5 and corrections

- **Design §4.7.**
  - Replace "G1 edges may share a normal; creases never do" with Rule B: never
    share across faces.
  - Replace "caps above about 150° refined away" with Rule A.
  - Add singular-parameter limit normals.
- **Design §5.2 item 6.** Replace "follow encoded regularity" with measured
  classes. The 1° crease used there becomes JAMA 0.2° for data quality and
  CAx-IF 1° for feature lines.
- **Design §5.3 orientation.** Use A0, which works on the corner normals that
  are already evaluated.
- **Design §5.5 export.**
  - glTF joins STL and 3MF per D-formats, with NORMAL mandatory.
  - The design's "per-corner normals" reach consumers only through glTF (X5
    confirmed).
- **Fast-tessellation §1.5.**
  - "Reject caps with 3D angles above about 150°" holds only on curved,
    non-anisotropic regions.
  - "No cheap sufficient normal-deviation bound exists": θ(T) needs no bound,
    because it is measured exactly and cheaply per triangle.
- **Healing paper §7.2.** STEP has no regularity data (§3.1).
- **Lifecycle map X15** is resolved by Rule A's basis.

## 8. Return items

**Defects in our code.**
- None found.
- One open capability, not a defect: freeform collapsed-edge limit normals fail
  explicitly (`native/om10/math.hpp:126,151`, `unit(normal, normal)` false) rather than
  emitting a limit normal (§3.3).

**Operator questions.**
1. Is the D-consumers "one glTF renderer" a rasterizer (three.js / Filament
   class) or a path tracer?
   - With a path tracer, the facet-normal terminator step (sin θ, 4.5–10% of
     peak at ε/ρ = 10⁻³) is visible in principle, and A2 or a tighter c may
     deserve a default.
   - With a rasterizer, Rule A as stated suffices.
   - Rules A–C are defined either way.
2. Defect-band edges (0.01°–0.2°) render faithfully under Rule B, so a sloppy
   tangent join shows its true small step in reflections. Should a display
   option ever hide such defects? Rule B says no [call]. It is the operator's
   eye that decides.

**Shopping items.** See the next section.

## For SHOPPING_LIST.md

- Woo, Pearce, Ouellette 1996, "It's really not a rendering bug, you see",
  IEEE CG&A 16(5) (terminator origin cited by Hanika). Paywalled.
- van Overveld, Wyvill 1997, "Phong normal interpolation revisited", ACM TOG
  16(4). Paywalled.
- Conty Estevez, Lecocq, Stein 2019, "A microfacet-based shadowing function to
  solve the bump terminator problem", Ray Tracing Gems ch. 12. Open access, not
  fetched: its subject is bump maps, and Deshmukh–Green restate its formula.
- No source read gives a perceptual threshold for visible crease, kink or
  terminator steps. This is a measurement (§5.3), not a purchase.
