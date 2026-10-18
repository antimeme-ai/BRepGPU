# Physical embedding near thin and knife-edge features

Research lane M4-G4 of the fast-mesher campaign, 2026-10-17. Brief:
[docs/fast-mesher-campaign/briefs/M4-G4.md](../docs/fast-mesher-campaign/briefs/M4-G4.md)
with [M4-common.md](../docs/fast-mesher-campaign/briefs/M4-common.md). Bead
`brepgpu-12s.5`. Stub: lifecycle map §4.1 G4 (stage S21, contradiction X16).
Sources, read depth and hashes are in
[2026-10-17-thin-feature-embedding-sources.tsv](2026-10-17-thin-feature-embedding-sources.tsv).

What ran: the lane read the doctrine, the journal (G1 decisions), the plan,
the lifecycle map, the working-draft design, the earlier papers named below,
the literature in §2 and reference code in quarantine. The only computation was
a standard-library Python check of this paper's own derivation, run on the
fleet in the lane directory (§3.8). No mesher, Boolean or NVIDIA run, nothing
built, nothing run on the laptop.

Tags: **[src]** a source establishes it; **[code]** read at file:line in a
pinned reference or in this repository; **[own]** this lane's derivation;
**[num]** checked by the lane's probe (§3.8).

## 0. Answer

1. **The draft's 2ε rule is correct where it applies, but it is stated against
   the wrong objects.** "Faces more than 2ε apart cannot intersect" holds for
   the *trimmed faces of the repaired target*, inflated by each face's actual
   localization bound, not for source surfaces inflated by ε. Two corrections:
   the repair moves faces by up to μ (β = ε/2 under R2–R4, up to ¼ local feature
   size under M2's adopted transverse leaps), and band triangles over concave
   trims sit up to 2ε + s_e from their own face (s_e the edge chord sagitta), not ε
   (§3.1). With those constants it is the special case of a clean sufficient
   condition (Lemma A, §3.2).
2. **The query has nothing to say about adjacent faces, and adjacent faces are
   where it fails.** Every pair sharing an edge overlaps in the inflated query, so
   the query flags all of them and tells construction only to "tighten". We give
   the missing rule (Lemma C, §3.4): near a shared edge each face's star triangles
   must tilt from their own tangent plane by less than half the sine of the
   smallest wedge opening, and the remaining triangles must satisfy Lemma A's
   half-clearance. A counterexample family shows the draft as written
   self-intersects: an extruded crescent (convex cylinder against concave
   cylinder) meshed by the draft's cylinder slab rule at the requested ε. With
   slab lines on a fixed angular lattice, 59 of 2,016 configurations whose crescent
   is thicker than ε (up to 3.6ε) cross, and 2,157 of 3,168 sub-ε crescents
   cross, invert or collapse. With slabs spread evenly, 112 of 132 sub-ε
   crescents fail and none of the 84 thick ones do. Under the proposed per-face
   rule all 5,400 configurations are clean, at most seven added vertices per
   face cross-section **[num]**.
3. **Tightening one face alone can cause the knife-edge failure.** A concave face
   whose chords rotate out of the wedge protects its convex neighbour. Refining
   the concave face shrinks that protection. The rule therefore budgets each face
   against the wedge on its own, without credit for the neighbour (§3.4, §4.2).
4. **Two hazards are not face pairs at all, so a face-pair query cannot see them:**
   one face that comes back near itself (hems, multi-turn helical and spiral
   faces, B-spline hairpins), and trim loops of one face that run close or meet
   at a small angle in UV. The second is the classic "UV wire self-intersection".
   OCCT detects it but deliberately ignores crossings below 5°, which is exactly
   the near-tangent case [code `BRepMesh_FaceChecker.cxx:26,155`] (§2.4, §4.3).
5. **A thin wall needs ε_loc below half the clearance, not refusal.** The draft
   lets construction refuse "when the feature is below what the request can
   represent". Under D-features a wall or gap of width w > ε is a feature that
   must not be dropped, so it is meshed with local deviation below w/2. That is
   free on planes, where triangles do not deviate. Refusal is kept for
   clearance at or below a floor and for source contact (§4.7). Below width ε the
   unsigned rule still applies. Where each carrier's curvature sign is known,
   the signed rule shows only w < ε crosses, and the probe confirms it on thin
   tubes **[num]**.
6. **The decision M5 needs.** Alpha does not need a check of emitted triangle
   pairs in the hot path for soundness. It needs **clearance budgets in
   construction**: per-face sizing limits from source clearance (non-adjacent
   and same-face), per-triangle tilt limits at shared edges and vertices, and
   per-chord UV limits for trim loops. All three come from source geometry or from
   the triangle being built, in the same way as the Waldron size test. The
   face-level ε-inflated query stays, as the cheap filter that finds where
   clearance must be computed. Where a clearance lower bound cannot be
   established within its budget, the result is `refused` with a record. The
   alternative, a targeted triangle-pair test in that region only, is an
   operator question (§8, Q1). The offline self-intersection oracle stays.
7. **Cost.** The edge rule is a few flops per edge sample and per star triangle,
   and every input it uses is already measured by §5.2 step 2 (dihedral angle
   at each sample) or computed for density (curvature bounds). It runs as SoA
   batches on AVX and GPU without change. Face-pair candidates reuse the existing
   AABB pair-discovery path (CPU/CUDA). Clearance lower bounds are closed forms
   for analytic pairs, and a span-pair worklist on the CPU for freeform pairs.
   Candidate counts on the corpus are unknown. They are the measurement that
   settles the cost claim (§5, H4).

## 1. What the draft says and what is at stake

Design §5.6 (working draft), accepted by the operator as the default:

> Each face's triangles lie within ε of its own surface. Two faces whose source
> surfaces are more than 2ε apart therefore cannot produce intersecting
> triangles. The risks are source features thinner than 2ε: thin walls, narrow
> gaps, and knife-edge dihedral angles at shared edges. Before meshing, a
> face-level query over ε-inflated face bounds finds candidate pairs; for those,
> the local tolerance tightens or the faces are refused when the feature is below
> what the request can represent. This is a planning input to density, not a
> check of the output. The full self-intersection check stays an offline oracle.

Fidelity contract §4.8 requires that no pair of emitted triangles intersects
except along shared mesh edges and vertices. §4.9 exempts source
self-intersections, which are reproduced and reported. The old mesher's 165 s
hot-path contact check found 0 forbidden pairs over 2,218,277 candidates in the
watch's 107 definitions [owned-mesher-audit §0 item 4, §3]. That is one workload
and one mesher. The new mesher sizes differently (slab lattices, a dyadic
quadtree, bands), so that zero carries no evidence about the new path.

The whole-mesher geometry contract study (§5) put the obligation in
construction: "even complete cells with positional error ≤ ε can cross when the
original feature clearance is smaller than twice ε" and "do not skip an entire
pair simply because it shares a vertex". It assumed robust contact
classification of emitted triangles, which the operator has since excised from
the hot path. Fast-tessellation §8 question 2 recorded that no cheap a-priori
rule was found in the literature read, and that Li et al. 2026 was unread.

## 2. What the literature and mature systems establish (Q1)

A literature sub-lane acquired and read the sources in §2.1–2.3. This lane
checked the load-bearing statements against them (KS 2019 read directly).
Statements are quoted from the sources. Interpretation is marked.

### 2.1 Smooth closed surfaces: reach, sampling, projection

- **Local feature size and r-samples** [src Amenta–Bern 1999 §4, Defs 1–3].
  - LFS(p) is the distance to the medial axis.
  - Lemma 7: "If a ball B intersects surface F in more than one connected
    component, then B contains a point of the medial axis of F".
  - So a wall of thickness t has LFS ≤ t/2 **[inf]**.
- **Triangle normals** [src Amenta–Bern Lemma 6; ACDL 2000 Th 9; Khoury–Shewchuk
  2019 Lemmas 2–3].
  - The angle between a triangle with vertices on the surface and the surface
    normal is bounded by its circumradius over the empty-ball size or LFS.
  - KS Lemma 2: sin ∠(aff τ, T_v Σ) ≤ (R/ebs(v))·max(cot(φ/2), 1), for any vertex
    v with plane angle φ.
  - KS stress that normal error varies with the circumradius, which "can be much
    larger than τ if τ has a large angle", while interpolation error varies with
    the smallest enclosing ball [src KS §1, Lemma 1].
  - Each of these proofs uses only that the triangle's other vertices lie outside
    the two empty balls tangent at v **[src KS Lemma 2 proof; inf for ACDL Th 9]**.
- **Embedding by projection.**
  - ACDL Th 15 (SoCG 2000): "the mapping μ defines a homeomorphism from the
    triangulation N to the surface S for ε ≤ 0.06". It needs an explicit
    assumption that "two adjacent triangles meet at their common vertex at an
    angle of greater than π/2", which "excludes manifolds which contain sharp
    folds and, for instance, flat tunnels".
  - The journal version's hypotheses H0–H4 are reported by Boissonnat–Oudot §4.2
    (Th 4.5): a closed 2-manifold complex with vertices on S, adjacent normals
    within π/2, small Delaunay balls, and a normal bound at the largest-angle
    vertex.
  - Morvan–Thibert 2004 take injectivity of the projection as a *hypothesis*
    ("closely near") and bound normals and area.
  - Isotopy follows from embedding plus closeness [src Amenta–Peters–Russell
    2003 Th 9; Chazal–Cohen-Steiner 2003 Prop 1].
  - **[inf]** The projection argument is an embedding certificate built from local
    facts: if π∘f is injective on the abstract complex K, then f is injective.
    Lemma B (§3.3) uses exactly this.
- **What it does not cover.** Every one of these results assumes a smooth surface
  without boundary. At a sharp B-rep edge the medial axis reaches the edge, so
  LFS and reach are zero there. The projection theorem cannot be applied to a
  B-rep's boundary as a whole **[inf]**.

### 2.2 Piecewise-smooth complexes: sharp edges and small angles

- **Cheng–Dey–Ramos (DCG 2010, extended version).**
  - Elements are compact pieces of C³ manifolds. The feature size f = min(m, g),
    where g(x) is the smallest radius at which a ball around x meets two elements,
    one not containing x.
  - Protecting balls along edges satisfy radius ∈ (λω/21·f, 5λg). The refinement
    never inserts points inside them.
  - Th 5.2: the output "is homeomorphic to |D|", "without any constraint on
    input angles". Th 6.1 adds normal error within 26ω and a dihedral angle above
    π − 52ω at edges with an unweighted endpoint.
- **Dey–Levine (2009) and Cheng–Dey–Levine (IMR 2007).** These are practical
  versions with a guessed scale λ. Homeomorphism holds "if λ ≤ λc" with λc not
  explicit.
- **CGAL Mesh_3.** It implements protecting balls (no three balls intersect; balls
  on different 1-features do not intersect). This guarantees that consecutive
  feature segments stay in the mesh and that refinement terminates "whatever may
  be the dihedral angles formed by input surface patches incident to a given
  1-feature" [code CGAL 28811b67 `Mesh_3/doc/Mesh_3/Mesh_3.txt:185-218`].
- **[inf]** In these methods the surface mesh is a subcomplex of a 3D (weighted)
  Delaunay triangulation, so it is embedded automatically. Small dihedral angles
  cost sampling density, since g ≈ d·sin θ inside a thin wedge. They cost no extra
  argument. That route to embedding does not transfer to per-face parametric
  meshing at NVIDIA-class speed, but it confirms the scaling our Lemma C finds:
  the required resolution near an edge goes with the wedge angle.

### 2.3 CAD tessellation without self-intersection

**Li, Luo, Feng, Zhang, Gai, Lei, JCDE 13(1):239–258, 2026** (read from the full
publisher HTML through a summarising fetch tool; no PDF; pseudocode figures and
appendix proofs seen only in summary).

- **Method.**
  1. Shared edges are sampled and mapped so that the "corresponding 3D points
     coincide exactly" (§4.1).
  2. A BVH of control-point boxes in a local frame subdivides patch pairs "only
     when volumes intersect" (§4.2).
  3. Critical-region edges become CDT constraints (§4.3).
- **Guarantee.** Lemma 2: for "two C²-continuous non-intersecting NURBS surface
  patches", triangles that each lie within one critical region "are guaranteed
  to be disjoint". The proof is a convex-hull argument (Appendix B). Termination
  is argued for "initially disjoint surfaces with a positive minimal geometric
  distance".
- **Results.** Zero self-intersections on six test models, against Gmsh 8,485 and
  15,410, OCC 36 and 36, and Netgen 4 plus one crash (Table 4). 6.3–45.8 s
  single-threaded for 128–649 surfaces, with the critical-region stage dominating
  (Table 3).
- **Limits it admits.** Valid B-rep input only, no overlapping surfaces, no
  T-junctions (§6, §4.1). Dihedral angle and chord tolerance are not discussed.
  It does not say what happens when subdivision reaches its minimum size without
  separating the hulls.
- **[inf]** Its guarantee covers disjoint patch pairs, the analogue of our
  Lemma A. Faces sharing an edge violate its premise: their hulls overlap near the
  edge at every subdivision level. So the knife-edge case is outside it, and it
  has no same-patch theorem. Its usable core is the convex-hull fact that the
  E2 lower bound in §4 also uses.
- **Cost.** The reported times are far from the speed target. The construction
  rule here therefore runs subdivision only on candidate zones and uses closed
  forms for analytic pairs.

Other CAD-specific work found was about watertightness, not self-intersection
(Wang et al., IEEE TIM 2022, abstract only). Rounding to floating point after
meshing is G3's subject [src Valque–Lazard 2025, held].

### 2.4 Mature meshers (reference code)

- **OCCT 8.0.1** (b8f597c6). BRepMesh checks only parameter-space wire
  self-intersection.
  - `BRepMesh_FaceChecker` intersects the discretised wire segments of one face in
    UV [code `BRepMesh_FaceChecker.cxx:224-290`]. It ignores crossings whose
    segments meet at under 5° (`MaxTangentAngle = 5°`, `:26`, `:155`), and
    self-loops smaller in area than π(2·deflection)² (`:104-106`, `:256`, where
    "Tolerance is set to twice value of face deflection in order to fit
    regressions").
  - `BRepMesh_ModelHealer::amplifyEdges` divides the deflection of the offending
    edges by 3 and re-meshes the incident faces, for at most 5 rounds
    [code `BRepMesh_ModelHealer.cxx:57, 173-215`].
  - No 3D check between faces was found in TKMesh.
  - **[inf]** Near-tangent UV crossings, the U1 family, are exactly what the 5°
    exemption skips.
- **Gmsh** (b4e84a43). When boundary recovery fails because "the 1D mesh
  self-intersects", it splits those edges and re-meshes, recursively
  [code `src/mesh/meshGFace.cpp:1152, 1165-1240, 2061-2084`]. No 3D inter-face
  check was found.
- **Netgen** (31e85f81).
  - Close-edge sizing: edges are sampled into 100 sections, the nearest
    non-touching section is found, and local h is set to mindist/closeedgefac,
    floored at 1e-3 of the bounding-box diagonal
    [code `libsrc/meshing/basegeom.cpp:413-480`]. The default is
    `closeedgefac = 2` [code `libsrc/occ/python_occ.cpp:40`].
  - `SplitBentEdges` inserts a curvature midpoint only if the new triangles do not
    "poke through other surface elements", using a triangle-triangle test near the
    insertion [code `basegeom.cpp:1359-1465`]. That is a targeted in-construction
    contact test, confined to its own insertions.
- **NVIDIA USD/BRep** (c9c5979f). No tessellation-time self-intersection
  handling was found. A comment in `SmTess.cpp:9446` assumes the UV polygon does
  not self-intersect. The `bDoSelfInt` flags belong to modelling operations
  (shell and sweep), not tessellation (`SM_API/inc/SmApiBrep.h:352, 494`).
- **This repository.** The old mesher's hot-path check classified every
  candidate emitted pair with interval filters and exact fallback
  (`native/mesher/contact.cpp:395-480`). It is retained as the offline oracle
  (§9).

### 2.5 What the literature does not say

No source read gives a condition for the embedding of a mesh built per face to a
chord tolerance near shared edges at small dihedral angles. That is the case the
draft's query cannot decide. The smooth-surface results stop at the creases.
The piecewise-smooth results get embedding from a 3D Delaunay complex. Li et al.
cover only disjoint patches. Lemma C below is this lane's derivation. Its
straight-edge cross-section is checked numerically (§3.8). The curved-edge and
doubly-curved cases are a proof sketch, to be settled by the authored families
K4–K5 under the offline oracle (§6).

## 3. A sufficient condition (Q2)

Everything in this section is **[own]** unless a source is cited. The
statements are written so that a builder can turn each hypothesis into a
sizing limit, and an oracle can falsify each conclusion.

### 3.1 Setting and quantities

- One definition D with trimmed faces F_f of the **repaired target** (§4.2 of
  the design: the coherent solid after edge and vertex resolution). Carrier
  S_f maps the UV domain D_f to R³. Self-intersection is judged within one
  definition only. Different occurrences may touch (geometry contract §5).
- The output M = ∪ M_f. M_f is the 3D image of an embedded UV triangulation K_f
  of the chain polygon Π_f. Π_f is the polygon through the UV positions of the
  face's chain vertices. Chains are shared by ID between the faces of an edge
  (§4.3).
- **Footprint and deviation.** A point p of a triangle t ∈ M_f has the
  UV-linear preimage ũ. Its footprint q(p) is S_f(ũ) when ũ ∈ D_f, else the
  nearest point of F_f. Its deviation is δ(p) = |p − q(p)|.
- **Localization ε̂_f.** The density rules bound |T(ũ) − S_f(ũ)| ≤ ε over every
  UV triangle (Waldron, §6.1). Where the trim is locally concave in UV, Π_f's
  chords leave D_f across a sliver between the pcurve and its UV chord. There
  S_f(ũ) lies within ε + s_e of the edge curve, with s_e the edge chord sagitta.
  So δ(p) ≤ 2ε + s_e in boundary slivers and δ(p) ≤ ε elsewhere. Write
  ε̂_f = sup δ over M_f. The contract's "within ε of the target solid" bounds
  distance to the union of faces, which is weaker than localization to one's own
  face. Embedding needs localization.
- **Displacement μ_f.** μ_f is the largest distance between the target face and
  its source face: recorded R2 moves (≤ β = ε/2), R3 and R4 moves, M2's adopted
  transverse leaps (≤ ¼ local feature size) and recorded tangent steps. When
  clearance is computed on source geometry, the target's clearance is at least
  the source's minus μ_f + μ_g.
- **Clearance.** For q ∈ F_f, c_g(q) = dist(q, F_g) for each other face g, and
  c(q) = min over g. Same-face self-proximity uses remote pieces of F_f as "g"
  (§3.3).
- **Wedge at a shared edge e of f and g.** In the plane normal to e at one of its
  points, the two faces leave e as curves. At radius ρ from e they are separated
  by a polar opening Δψ_in(ρ) through the material and Δψ_out(ρ) through the
  void. Δψ_in(0) is the dihedral θ that §5.2 step 2 already measures at every
  edge sample. Δψ_min(e) is the minimum of both openings over ρ ∈ [0, r*_e],
  where r*_e = 2(ε̂ + μ)/sin Δψ_min is the depth beyond which the plain budget
  already satisfies Lemma A. Curvature changes the opening along ρ:
  Δψ(ρ) ≥ θ − ½(κ_f + κ_g)ρ, with κ the bound on each carrier's normal curvature
  across the edge.
- **Star and tilt.** A star triangle of e has a vertex on e's chain. Its tilt at
  chain vertex v is α(t, v) = ∠(aff t, T_v S_f). τ_v is the angle between the
  chain chord at v and the edge tangent. It is zero for straight edges and about
  κ_e ℓ/2 for a chain segment of length ℓ on an edge of curvature κ_e.

### 3.2 Lemma A: half-clearance separation

**Lemma A.** Let f ≠ g. Suppose every p ∈ M_f has δ(p) < ½ dist(q(p), F_g), and
every p′ ∈ M_g has δ(p′) < ½ dist(q(p′), F_f). Then M_f ∩ M_g = ∅.

*Proof.* Suppose p = p′. Then |q − q′| ≤ δ(p) + δ(p′) < ½ dist(q, F_g) +
½ dist(q′, F_f) ≤ ½|q − q′| + ½|q′ − q| = |q − q′|, which is a contradiction. ∎

The same argument shows each mesh point lies strictly in its own face's
Voronoi region relative to the other face. That is the geometric content:
**every mesh point stays on its own side of the medial surface between faces.**

- **A1, the draft's rule.** If dist(F_f, F_g) > ε̂_f + ε̂_g on the target, the
  hypothesis holds. The draft's "2ε" is this with ε̂ = ε and source in place of
  target, so it needs ε̂ and μ (§3.7 items a, b).
- **A2, local tightening.** Where dist < ε̂_f + ε̂_g, giving both faces a local
  deviation budget ε_loc(q) < ½ c(q) over the zone restores the hypothesis.
  Planes deviate by zero and need nothing.
- **A3, signed variant.** Interpolating triangles of a convex carrier lie in its
  convex hull, on the concave side of the face. Where that side faces away from
  g (a convex boss beside a wall, the inner wall of a tube), f's deviation does
  not reduce clearance, and only g's deviation must stay below c. On a thin
  tube the outer face's chords move toward the inner face and the inner face's
  chords move away, so crossing needs w < ε, not w < 2ε. The probe shows exactly
  that threshold **[num]**. This is an optimisation that saves refinement, and it
  is an arena flag.
- **A in UV.** The same lemma, in two dimensions, applies to the chain polylines
  of one face. Each UV chord's deviation from its pcurve must stay below half
  the UV clearance to any other loop, or to a remote stretch of the same loop
  (§4.3).

### 3.3 Lemma B: one face is embedded

**Lemma B.** M_f is embedded if:

- (B1) Π_f is a simple polygon set (no UV loop crossings, §4.3), and K_f
  triangulates it with positive orientation;
- (B2) δ < the local reach of the carrier around each footprint. The reach is
  bounded by 1/κ_max from curvature and by half the distance to remote pieces
  of F_f from self-proximity;
- (B3) every triangle's normal makes an angle below π/2 with the carrier normal
  at its footprints.

*Sketch.* Under B2 the nearest-point projection π onto the carrier is defined
and continuous on a tube containing M_f. B3 makes π restricted to each triangle
injective, so π∘M_f is a local homeomorphism. The straight-line homotopy from
the PL map to S_f stays in the tube, so π∘M_f is homotopic to the embedding
S_f on K_f and injective on ∂K_f. It is therefore injective, and so is M_f,
because a map whose composition with π is injective is injective. For closed smooth
surfaces this projection argument is the literature's (§2.1). The extension to
patches with boundary is this lane's.

B3 is met quantitatively by Khoury–Shewchuk's Triangle Normal Lemma
**[src KS 2019, Lemma 2]**: sin ∠(aff t, T_v Σ) ≤ (R/ebs(v))·max(cot(φ/2), 1),
where R is the circumradius and φ the angle at v. Its proof uses only the two
empty balls tangent at v, so for a face the empty-ball size can be the
carrier's local reach from B2. Skinny triangles with large circumradius tilt
badly even when their deviation is tiny **[src KS 2019 §1; Shewchuk 2002]**.
That is why the tilt rule of Lemma C is per triangle and not per size.

Self-proximity of one face (a hem, a hairpin B-spline, a multi-turn helical or
spiral face) is Lemma A applied between remote pieces of F_f. A face-pair query
cannot see it.

### 3.4 Lemma C: the wedge at a shared edge

**Lemma C.** Let e be shared by f and g, with zone Z_e (points within r*_e of e).
Suppose:

- (C1) **tilt:** every star triangle t of f satisfies
  α(t, v) + τ_v ≤ ½ sin Δψ_min(e) − m at its chain vertices, with margin m > 0,
  and likewise for g;
- (C2) **chain sagitta:** ρ₀ = 2 s_e / (sin Δψ_min − 2 max α) is smaller than the
  first-row depth of both faces (distance from the chain to the nearest non-chain
  vertex) and smaller than the chain segment lengths;
- (C3) **half-clearance:** every non-star triangle of f in Z_e satisfies Lemma A's
  hypothesis against g, and conversely.

Then within Z_e, M_f ∩ M_g is exactly the shared chain.

*Sketch.* In the normal plane at radius ρ the faces are at least ρ·sin Δψ apart
(ρ itself when Δψ > π/2).

1. For ρ ≥ ρ₀, a star-triangle point deviates from its face by at most
   s_e + αρ. By C1 and C2 that is less than ½ρ sin Δψ, so the point lies on its
   face's side of the medial surface. Non-star points do so by C3. Lemma A's
   argument then excludes any coincidence.
2. For ρ < ρ₀ only star triangles exist (C2).
   - Two star triangles sharing a chain segment meet only along it unless their
     dihedral is zero. C1 keeps the discrete dihedral at least
     Δψ(0) − α_f − α_g − 2τ > 0 on both sides, so each fan stays on its own side of
     the plane through the chain chord that bisects the discrete wedge.
   - Star triangles sharing only a chain vertex are separated by that plane except
     at the vertex.
   - Non-incident star triangles are separated along the edge by at least one
     chain segment, which is longer than ρ₀ (C2).

For a straight edge (s_e = τ = 0) the statement reduces exactly to a 2D polyline
problem in the cross-section, which the probe checks **[num]**. For curved edges
the zone argument is a sketch, and the doubly-curved families in §6 are its test.

What C1 means in closed form. Each face's star triangles may tilt at most about
half the wedge opening. A cylinder strip whose chain runs along a ruling tilts by
half the slab angle, acos(1 − ε/R). C1 therefore gives
ε_loc ≤ R(1 − cos(½ sin Δψ_min)) ≈ R·Δψ_min²/8 for the star row. Beyond the star,
C3 lets the budget grow linearly, δ(ρ) < ½ρ sin Δψ, up to ε at ρ = r*_e. Example:
Δψ = 2° and R = 1 mm give a star budget of 0.15 µm, a zone 0.29 mm deep at
ε = 5 µm, and about three graded strips. The probe's added vertices per face
cross-section are 0–7 **[num]**.

Why the budget is per face, with no credit taken. In signed form the first-row
condition is θ − α_f^in − α_g^in > 0, where a chord rotating out of the wedge
counts negative. In the crescent family the convex face f rotates in by δ_f and
the concave face g rotates out by δ_g, so the discrete wedge inverts when
δ_f − δ_g > θ.

- For constant-curvature arcs with chords spread evenly, the closing of the
  crescent forbids this. f's first chord would have to be longer than the arc
  allows. 0 of 84 thick configurations with even spreading fail **[own; num]**.
- It happens when g's first chord is shorter than g's own chord rule would make
  it. A fixed slab lattice (its phase), a local tightening of g, a maximum-length
  cap and a fine trim band all do this. Refining g by 5× raises failures in the
  probe, for example from 2/24 to 11/24 at ε = 0.025, R = (1, 1.5), θ = 8°
  **[num]**.

A rule that splits one budget across the two faces is therefore unsafe under
independent face sizing. The per-face form of C1 is not. Signed credit, which
lets a concave neighbour widen the budget, is an arena flag. It is not the
default.

Crescents thinner than ε everywhere are a separate failure. The whole sliver
lies within ε of one chord, so both faces can collapse onto the same chord
(coincident triangles) or swap sides (an inverted section) **[num]**. D-features
does not protect sub-ε features from being simplified, but the design does not
defeature, and coincident or inverted output is invalid. C1 refines these
crescents as well.

### 3.5 Lemma D: vertices

At a vertex v, each incident face's star triangles at v lie within their tilt α
of that face's tangent cone at v.

- Faces sharing an edge at v are covered by Lemma C at that edge.
- Faces sharing only v (opposite faces at a pyramid apex, two knife edges
  converging at a needle tip) are separated by a plane through v if the angular
  gap between their tangent cones exceeds α_f + α_g + m.
- Apexes and poles inside a single face are fans of chord planes. A cone apex fan
  is inscribed in the convex cone and is embedded. That case falls under Lemma B
  with the apex fan.

This is a sketch, tested by family V1 in §6.

### 3.6 Theorem

M is embedded except along shared chains if all of the following hold:

- Lemma B for every face;
- Lemma A for every pair of faces that share neither an edge nor a vertex, and
  for every remote pair of pieces of one face;
- Lemma C at every shared edge;
- Lemma D at every vertex;
- the UV form of Lemma A for every face's chains.

Source contacts reported under design §4.9 are exempt. Every hypothesis is a
limit on sizes, tilts or UV chords computed from source geometry or from the
triangle being built. None of them requires testing pairs of emitted triangles.

### 3.7 What this says about the draft: eight gaps

| # | Draft (§5.6) | Correction | Counterexample family |
|---|---|---|---|
| a | "within ε of its own surface" | Localization is to the trimmed face. ε̂ ≤ 2ε + s_e in concave-trim slivers. | U2: band slivers over a concave trim facing a near neighbour |
| b | distances on "source surfaces" | Distances on the repaired target, or inflate the source by μ (β, or ¼ lfs under M2 leaps) | R1: faces 2ε + η apart where an incident edge's R3/M2 resolution moves one face toward the other |
| c | adjacent pairs: "tighten" | No criterion exists. Lemma C (per-face tilt, half-clearance in the zone) | K1: extruded crescent, convex against concave cylinder **[num]** |
| d | one tightening per pair | Budgets per face. Tightening one face can create the failure | K1 with only the concave face tightened **[num]** |
| e | box overlap identifies the risk | Overlap is not clearance. A curved thin wall's boxes overlap completely while its clearance is w, so tightening needs a clearance lower bound | T1: coaxial-cylinder thin tube **[num]** |
| f | face pairs | One face near itself is not a pair | S1: B-spline hairpin, multi-turn helical face |
| g | face pairs | UV trim loops of one face that come close or meet at a small angle | U1: hole near a convex outer arc; loops meeting at 0.5–10° |
| h | refuse "below what the request can represent" | D-features: walls and gaps wider than ε are kept and meshed with ε_loc < w/2. Refusal only at a floor or for source contact | T3: parallel planes at w ∈ (ε, 2ε), which need no change at all |

Export rounding to float32 (G3) shrinks every clearance by up to the rounding
displacement of two vertices. If export does not snap-round, the budgets must
reserve it. That decision belongs to G3.

### 3.8 Numerical check of the derivation

Probe `g4_embedding_probe.py` (standard-library Python, ~280 lines, SHA-256
`928d1938…13236c`; output SHA-256 `f0e9eb1a…401919a`) and
`g4_rule_cost.py`, run in the fleet lane directory. It is a derivation check,
not a timing, so no timing context applies. The host's load average was about 7
to 11 from other lanes during the runs.

- **Construction.** Extruded faces mesh as planar strips between slab lines, so
  strips of two faces intersect exactly when their cross-section chords do.
  Chords use the draft's cylinder rule Δφ = 2·acos(1 − ε/R), placed either
  spread evenly over the arc ("even") or on a fixed angular lattice with a random
  phase per face ("lattice"). The draft does not say which (§5.3).
- **Failure test.** Every crossing, touching or coincident-segment decision uses
  exact rational orientation tests (`fractions.Fraction`) on the float64
  vertices. A failure is any forbidden contact, or a cross-section polygon whose
  signed area is zero or of the wrong sign (an inversion).
- **Rule tested.** The proposed per-face rule is applied to each face from source
  data alone. Chords touching a tip: half the central angle
  < 0.9·½ sin θ. Other chords: sagitta < 0.9·½·clearance from the chord's arc
  to the other circle. Chords are bisected until both hold.

Results:

| Family | Placement | Configurations | Draft fails | Rule fails |
|---|---|---|---|---|
| Crescent thicker than ε (t_max up to 230ε) | even | 84 | 0 | 0 |
| Crescent thicker than ε | lattice | 2,016 | 59 (all crossings; failing t_max up to 3.62ε) | 0 |
| Crescent thinner than ε | even | 132 | 112 (coincident or inverted) | 0 |
| Crescent thinner than ε | lattice | 3,168 | 2,157 (1,782 crossings, 1,780 inversions; they overlap) | 0 |
| Thin tube, w ≤ 0.9ε, both faces at ε | both | 156 | 87 crossings | 0 at ε_loc = w/2 |
| Thin tube, w ≥ 0.99ε, both faces at ε | both | 312 | 0 (signed threshold w = ε) | 0 |

Sweep: ε ∈ {0.005, 0.025} mm; (R_A, R_B) ∈ {(1, 1.5), (1, 2), (1, 4), (1, 10),
(0.5, 4), (5, 6)} mm; θ ∈ {0.5, 1, 2, 3, 4, 6, 8, 12, 20}°; concave face at ε or
ε/5; 24 lattice phases. Rule cost: 0–7 extra vertices per face cross-section,
and none beyond θ ≈ 8–20° for the thick cases. Limits of the check: circles
only, straight edges only, one cross-section. Doubly-curved knives, curved edges
and B-splines are §6 families for the offline oracle.

## 4. Failure modes and construction rules (Q3)

| Feature | Failure it produces | Construction rule | Refuse or report when |
|---|---|---|---|
| Thin wall or gap between non-adjacent faces (clearance w < ε̂_f + ε̂_g + μ) | Triangles of the two faces cross (thin tube **[num]**) | A2: ε_loc < ½(c − μ_f − μ_g)·(1 − m) on both faces over the zone. Planes need nothing. Signed A3 as a flag | c at or below the floor (§4.7). Budgeted refinement count exceeded |
| Knife edge, material side (small θ_in) | Star fans invert, or a first row crosses the other face's second row (crescent **[num]**). Sub-ε slivers collapse to coincident triangles | C1 per face, C3 in the graded zone. KS tilt per star triangle for bands. Closed forms for strips | Δψ_min ≤ floor angle with both faces turning into the wedge |
| Knife edge, void side (slot, crack, reflex edge with small θ_out) | Same, mirrored into the void | Same, with Δψ_out | Same |
| Cusp: faces tangent at the edge (θ → 0 on one side) | No finite unsigned budget exists | Signed credit is required. The wedge is safe only if the net signed rotation opens it (plane against concave cylinder at a material cusp: the plane does not tilt and the cylinder turns away) | Otherwise refuse the edge's faces with a record. This is a refusal, not a dropped feature (D-partial-output) |
| Near-tangent trims in UV (a hole near a convex outer arc; loops meeting at a small UV angle) | Crossing UV chains, so Π_f is not simple and the CDT input is invalid | UV Lemma A (chord deviation < ½ UV clearance) and the UV wedge (chord tilt < ½ the UV angle). The fix re-splits a shared edge chain, so it feeds the face-to-edge loop (G2) | UV clearance at the floor |
| Tangent (G1) edges, R4 class | No wedge hazard (Δψ ≈ π on both sides). The risk is lateral chain position, handled because bands are built after resolution | None beyond §5.2 | — |
| One face near itself | Remote triangles of one face cross | Lemma A between remote span pieces of the same face | As for thin walls |
| Vertex where faces share only the vertex; needle tips | Star triangles of non-adjacent faces overlap near v | Lemma D tilt budget against the tangent-cone gap | Cone gap ≤ floor angle |
| Repair displacement | Clearance shrinks after resolution | Compute clearance on the target, or subtract μ | — |
| Float32 export | Rounding moves vertices across small clearances | Reserve 2·u32 in c if export does not snap-round (G3) | — |

The construction rule, as one mechanism (call it the **clearance budget**):

- **E0. Constants.** ε̂_f and μ_f per face, from §5.2 records.
- **E1. Candidates.**
  - (a) Face pairs whose boxes, inflated by ε̂ + μ, overlap and which share
    neither an edge nor a vertex. This is the draft's query, on the existing AABB
    pair-discovery path.
  - (b) Every shared edge.
  - (c) Every vertex with three or more faces, or with faces sharing only it.
  - (d) Per face, span pairs that are not neighbours in UV but whose inflated
    boxes overlap (self-proximity), and chord boxes of different loops, or of
    remote stretches of one loop, in UV.
- **E2. Clearance lower bounds.**
  - Closed forms for analytic pairs: parallel planes, coaxial cylinders and
    cones, sphere–plane, sphere–sphere, cylinder–plane.
  - Otherwise, recursive span-pair box distance using the convex-hull property of
    Bézier spans, until the bound clears ε̂ + μ (the pair drops out), or the
    sub-pieces are small enough to set a local budget, or a work budget runs out.
    Running out is "could not establish clearance", not "clear".
  - For edges, Δψ_min from the measured θ minus the curvature terms over the
    zone.
- **E3. Budgets.** The local deviation limit ε_loc(q), the per-star-triangle tilt
  limit and the per-UV-chord limit join the density field as size limits (§6.1).
  For analytic strips they enter in closed form. For bands they are checked
  per triangle after triangulation, in the same pass as the existing 3D
  orientation check (§5.3), and violators refine.
- **E4. Outcomes.**
  - Clearance at or below the floor: the faces touch or intersect in the source.
    Mesh faithfully, report the pair and exempt it in the oracle (§4.9), with the
    operator question in §8 on gaps below the stated source uncertainty.
  - Budgeted refinement exceeded, or clearance not established: `refused` for
    the affected faces with a record. Never `valid`.

## 5. Cost and batching (Q4)

| Piece | Work | Batches as |
|---|---|---|
| E1(a) face-pair candidates | BVH over faces, O(F log F); existing CPU/CUDA AABB pair discovery | Existing SoA box pages |
| E1(b) edges | None: θ is already measured at every edge sample (§5.2 step 2) | Edge sample SoA |
| C1 for analytic strips | One arccos or its polynomial per edge sample, plus the zone grading | Edge sample SoA: 4-wide AVX doubles, no FMA needed; GPU warps on the same arrays |
| C1 for band triangles | KS bound per star triangle: circumradius from one cross product and three dots, cot(φ/2), and the carrier's reach (closed form, or 1/κ_max from span bounds) | Star-triangle SoA, in the band's existing orientation pass |
| E2 analytic clearances | O(1) per candidate pair | SoA by kind |
| E2 freeform clearances | Irregular worklist of span pairs | CPU. GPU only if the measured counts justify it |
| A2/C3 budgets | Enter the density field as size limits. Cost is the extra triangles | — |

For comparison, the old hot-path contact check cost 64–67 µs per candidate
triangle pair, 2.2 M pairs and 165 s on the watch [owned-mesher-audit §3].
The clearance budget does no work per emitted triangle pair. Its work is per
edge sample, per star triangle and per candidate face pair. Nobody has measured
how many candidate face pairs and span pairs the corpus produces, how much area
needs tightening, or how many triangles the budgets add. Those are the numbers
that decide the cost (H4).

## 6. Hypotheses, falsifiers and families to author

- **H1, soundness.** With the clearance budget on, the offline self-intersection
  oracle (adjacent and coplanar pairs included) reports no forbidden pair on the
  families below or on the corpus, except pairs reported under §4.9 and E4.
  *Falsifier:* any other forbidden pair.
- **H2, the edge rule is necessary.** With E3's tilt rule off (the draft as
  written), family K1 under lattice slabs produces forbidden pairs.
  *Falsifier:* zero failures in the arena with the flag off. That would make the
  rule cost without benefit on real sizing.
- **H3, signed credit is safe.** The signed variants (A3, the signed C1) remove
  refinement with no new failures. *Falsifier:* any failure with signed credit on.
- **H4, cost.** The clearance budget adds under 2% of meshing time and under 5%
  of triangles on the corpus. *Falsifier:* the arena measurement. The thresholds
  are proposals, to be set by the operator or M5.
- **H5, the draft's zero is no evidence.** The watch shows few or no budget
  activations. *Falsifier:* not applicable. Recorded so that no one reads "0
  forbidden pairs on the watch" as soundness.

Families to author (parametric, generated, none keyed to the watch):

| Id | Family | Parameters | Tests |
|---|---|---|---|
| K1 | Extruded crescent knife edge: convex cylinder against concave cylinder | θ ∈ {0.5, 1, 2, 4, 8, 12, 20}°; R_B/R_A ∈ {1.2, 1.5, 4, 10}; ε ∈ request set; slab placement even and lattice; concave face tightened ×1 and ×5 | C1, C3, gap d **[num in 2D]** |
| K2 | Lens knife edge (both faces convex) | as K1 | Negative control for signed credit: convex inscribed sections are safe |
| K3 | Plane against convex cylinder (chisel); plane against concave cylinder at θ → 0 (material cusp) | θ down to 0 | Cusp handling, the signed requirement |
| K4 | Doubly-curved knives: sphere–sphere crescent of revolution; torus–plane knife; B-spline blade with a curvature spike at the edge | as K1 | Lemma C in 3D; tilt decoupled from size |
| K5 | Curved knife edge: two cones or cylinders meeting along a circle or ellipse at a small dihedral (a countersink lip) | edge radius / ε, θ | τ_e and s_e terms (C1, C2) |
| K6 | Reflex slot and crack: void-side wedge | θ_out ∈ {0.5…20}° | Δψ_out |
| T1 | Thin tube: coaxial cylinders | w/ε ∈ {0.25, 0.5, 0.9, 1.1, 1.9, 2.5} | A2, A3 threshold at w = ε **[num]** |
| T2 | Flat slab with a convex boss or fillet curving toward the opposite face | w/ε | Signed credit one-sided |
| T3 | Parallel planes | w/ε ∈ {0.1, 0.5, 1.5} | Negative control: no tightening, no refusal (gap h) |
| T4 | Thin spherical shell, concentric and eccentric | min gap / ε | Zone-local tightening |
| S1 | One-face self-proximity: extruded B-spline hairpin, leg gap w/ε ∈ {0.5, 1, 1.5, 3} with legs curved toward each other; one helical face spanning many turns at small pitch | w/ε, pitch/ε | E1(d), Lemma B self-reach |
| U1 | Planar face: hole near a convex outer arc (bridge w/ε ∈ {0.5, 1, 2}); two loops meeting at a vertex at a UV angle of 0.5, 2, 5, 10° | as listed | UV Lemma A; straddles OCCT's 5° exemption |
| U2 | Concave trim sliver near a non-adjacent neighbour | sliver depth / gap | ε̂ (gap a) |
| V1 | Pyramid apex with 4–8 faces at a small solid angle; two knife edges converging at a needle tip | apex angle | Lemma D |
| R1 | Faces 2ε + η apart where an incident edge is resolved by R3 or an M2 leap toward the other face | η/ε, move | μ inflation (gap b) |
| C1 | Faces of one definition touching or intersecting in the source | contact kind | E4 reporting and oracle exemption (§4.9) |

Every family is checked by the offline self-intersection oracle that includes
adjacent pairs, plus the orientation-consistency check on sections (inversion).

## 7. Proposed §5.6 for M5

> **Embedding by clearance budgets.** Each emitted point stays strictly on its
> own face's side of the medial surface between faces. Construction guarantees it
> with three budgets computed from source geometry and from the triangle being
> built. (1) Deviation below half the clearance to every other face and to
> remote pieces of the same face, inside zones found by an ε̂+μ-inflated face and
> span query; elsewhere the plain ε holds. (2) At every shared edge, each face's
> star triangles tilt from their tangent plane by less than half the sine of the
> smallest wedge opening, and the zone beyond grades from that budget up to ε.
> At vertices, the same against tangent cones. (3) In UV, chain chords stay below
> half the clearance to other loops and below half the angle at loop junctions.
> Clearance is computed on the repaired target. Budgets are per face, so no face's
> refinement can weaken its neighbour. Walls and gaps above ε are meshed, never
> dropped. Refusal happens only at the clearance floor, for source contact
> (reported, §4.9), or when a budget runs out. No emitted triangle pair is tested
> in construction. The full self-intersection check stays an offline oracle.

## 8. Questions for the operator

1. **Clearance that cannot be established cheaply.** A freeform span-pair
   recursion can run out of budget without proving clearance. Should the
   affected faces be `refused` (the default here), or should construction run a
   floating-point triangle-pair test confined to that region? The second is a
   self-intersection test of emitted triangles in construction, which the
   certification excision may forbid. Netgen does this, but only around midpoints
   it inserts (§2.4).
2. **Gaps narrower than the source's stated uncertainty.** A file may carry
   0.01 mm uncertainty (FTC-10), larger than ε = 0.005 mm. Should a 0.003 mm gap
   in such a file be meshed faithfully with ε_loc < w/2 (the default here, which
   follows D-features literally), or be treated as source contact, reported and
   exempted?

## 9. Defects

None found in the code read. `native/mesher/contact.cpp` (retained as the
offline oracle) classifies pairs sharing a vertex or an edge rather than skipping
them. Its shared-edge shortcut returns the edge only when a non-shared vertex is
strictly off the other triangle's plane (`contact.cpp:416-423`). Its
shared-vertex shortcut requires the other two vertices of one triangle strictly on
one side of the other's plane (`contact.cpp:424-439`). Both are correct reductions.
The design-draft corrections in §3.7 are document findings for M5, not code
defects.

## For SHOPPING_LIST.md

| Item | Why | Tried |
|---|---|---|
| Li et al. 2026, "Robust tessellation of CAD models without self-intersections", JCDE 13(1):239–258, doi 10.1093/jcde/qwaf134 (open access, CC BY-NC): **the PDF** | Read only through a summarising HTML fetch. Algorithms 1–2 (image figures), the Appendix A and B proofs, the l and edge-length settings, and the self-intersection counting tool are unverified | `academic.oup.com/jcde/article-pdf/13/1/239/66002200/qwaf134.pdf`, `/advance-article-pdf/…` and doi.org all return a Cloudflare 403. No Wayback copy. Needs a browser download |
| Amenta, Choi, Dey, Leekha, IJCGA 12(1–2):125–141, 2002, doi 10.1142/S0218195902000773 | The journal version holds Th 19 (H0–H4), known here only second-hand through Boissonnat–Oudot §4.2. The SoCG 2000 version was read | Closed per OpenAlex; the author's `.pdf.gz` returns 404 |
| Wang et al., "Robust and fast CAD model tessellation for inspection", IEEE TIM 2022, doi 10.1109/TIM.2022.3156988 | Abstract only. Checking that it has no self-intersection content | Closed access |
